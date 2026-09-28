// Spielt nach, was die Original-IDE von Blitz3D mit blitzcc macht: dieselbe
// Prozesserzeugung (DETACHED_PROCESS, stdout und stderr in einer Leitung)
// und dieselbe Auswertung, uebernommen aus blitzide/libs.cpp (initLibs,
// execProc) und blitzide/mainframe.cpp (startProc, MainFrame::compile).
// Damit laesst sich die IDE-Anbindung von blitzcc pruefen, ohne die IDE zu
// bedienen.
//
//   g++ -std=c++17 -static scripts/blitzide_sim.cpp -o blitzide_sim.exe
//   blitzide_sim init  <blitzcc.exe>                 -q und +k beim Start
//   blitzide_sim build <blitzcc.exe> -q [-c] [-o x.exe] <datei.bb> [args]
//                                                     F5, Check, Create Exe
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>
using namespace std;

static string execProc(const string &proc) {
  HANDLE rd, wr;
  SECURITY_ATTRIBUTES sa = {sizeof(sa), 0, true};
  string t;
  if (CreatePipe(&rd, &wr, &sa, 0)) {
    STARTUPINFOA si = {sizeof(si)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = si.hStdError = wr;
    PROCESS_INFORMATION pi = {0};
    if (CreateProcessA(0, (char *)proc.c_str(), 0, 0, true, DETACHED_PROCESS, 0, 0, &si, &pi)) {
      CloseHandle(pi.hProcess); CloseHandle(pi.hThread); CloseHandle(wr);
      char buf[1024];
      for (;;) {
        DWORD sz;
        BOOL n = ReadFile(rd, buf, 1024, &sz, 0);
        if (!n && GetLastError() == ERROR_BROKEN_PIPE) break;
        if (!n) { t = ""; break; }
        if (!sz) break;
        t += string(buf, sz);
      }
    } else { CloseHandle(wr); }
    CloseHandle(rd);
  }
  return t;
}

static HANDLE startProc(const string &proc) {
  HANDLE rd, wr;
  SECURITY_ATTRIBUTES sa = {sizeof(sa), 0, true};
  if (CreatePipe(&rd, &wr, &sa, 0)) {
    STARTUPINFOA si = {sizeof(si)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = si.hStdError = wr;
    PROCESS_INFORMATION pi = {0};
    if (CreateProcessA(0, (char *)proc.c_str(), 0, 0, true, DETACHED_PROCESS, 0, 0, &si, &pi)) {
      CloseHandle(pi.hProcess); CloseHandle(pi.hThread); CloseHandle(wr);
      return rd;
    }
    CloseHandle(rd); CloseHandle(wr);
  }
  return 0;
}

int main(int argc, char **argv) {
  string mode = argv[1], cc = string("\"") + argv[2] + "\"";
  if (mode == "init") {
    string valid = execProc(cc + " -q");
    if (valid.size()) { printf("IDE: Compiler environment error: %s -> IDE beendet sich\n", valid.c_str()); return 1; }
    printf("IDE: -q ok (keine Ausgabe)\n");
    string kws = execProc(cc + " +k");
    if (!kws.size()) { printf("IDE: Error generating keywords\n"); return 1; }
    map<string, string> keyhelps;
    size_t pos = 0, n;
    int count = 0;
    while ((n = kws.find('\n', pos)) != string::npos) {
      string t = kws.substr(pos, n - pos - 1);
      for (size_t q = 0; (q = t.find('\r', q)) != string::npos;) t = t.replace(q, 1, "");
      string help = t;
      size_t i = t.find(' ');
      if (i != string::npos) {
        t = t.substr(0, i);
        if (!t.size()) { printf("IDE: Error in keywords\n"); return 1; }
        if (!isalnum((unsigned char)t[t.size() - 1])) t = t.substr(0, t.size() - 1);
      }
      keyhelps[t] = help; ++count;
      pos = n + 1;
    }
    printf("IDE: %d Schluesselwoerter\n", count);
    for (const char *k : {"If", "Str", "CreatePlane", "EntityX", "PositionEntity", "Cls", "MilliSecs", "Rnd", "LoadFont", "Left", "Print"})
      printf("  %-15s -> %s\n", k, keyhelps.count(k) ? ("\"" + keyhelps[k] + "\"").c_str() : "FEHLT");
    return 0;
  }
  // build: wie MainFrame::compile
  string cmd = cc;
  for (int k = 3; k < argc; ++k) cmd += string(" ") + argv[k];
  _putenv("blitzide=1");
  HANDLE rd = startProc(cmd);
  _putenv("blitzide=");
  if (!rd) { printf("IDE: Error launching compiler\n"); return 1; }
  string line, err;
  int steps = 0;
  for (;;) {
    char buff; DWORD sz;
    BOOL n = ReadFile(rd, &buff, 1, &sz, 0);
    if (n && !sz) break;
    if (!n && GetLastError() == ERROR_BROKEN_PIPE) break;
    if (!n) { err = "Internal Error"; break; }
    if (buff == '\r') continue;
    if (buff != '\n') { line += buff; continue; }
    if (!line.size()) continue;
    if (line[0] == '\"') {
      err = line;
      size_t n = line.find("\"", 1);
      if (n == string::npos) break;
      if (++n == line.size() || line[n] != ':') break;
      string file = line.substr(1, n - 2); line = line.substr(n + 1);
      n = line.find(':'); if (!n || n == string::npos) break;
      int row1 = atoi(line.substr(0, n).c_str()); line = line.substr(n + 1);
      n = line.find(':'); if (!n || n == string::npos) break;
      int col1 = atoi(line.substr(0, n).c_str()); line = line.substr(n + 1);
      n = line.find(':'); if (!n || n == string::npos) break;
      line = line.substr(n + 1);
      n = line.find(':'); if (!n || n == string::npos) break;
      line = line.substr(n + 1);
      printf("IDE: oeffnet \"%s\", Cursor auf Zeile %d, Spalte %d\n", file.c_str(), row1, col1);
      err = line;
      break;
    } else if (line.find("...") != line.size() - 3) {
      err = line; break;
    }
    printf("IDE: Fortschritt \"%s\"\n", line.c_str());
    if (line.find("Executing") == 0) { printf("IDE: Programm laeuft, IDE liest nicht weiter\n"); break; }
    ++steps; line = "";
  }
  CloseHandle(rd);
  if (err.size()) printf("IDE: Meldungsfenster \"%s\"\n", err.c_str());
  else printf("IDE: keine Meldung\n");
  return 0;
}
