#ifndef BB_NET_H
#define BB_NET_H

// Mehrspieler: die 14 Net-Befehle von Blitz3D (StartNetGame ... NetMsgData)
// und die Spielsuche (FindNetGames und Verwandte).
//
// Blitz3D legte sie auf DirectPlay; das gibt es heute nur noch als
// abgeschaltetes Legacy-Feature. Hier steht ein eigenes Protokoll auf UDP,
// Konzept und Messungen am Original in NETWORK.md. Kurz:
//
// - Stern ueber den Host: jeder Client hat genau eine Verbindung, zum Host,
//   der Nachrichten weiterreicht. Faellt der Host weg, wird das aelteste
//   verbliebene Programm Host (Host-Migration), die anderen verbinden sich
//   neu mit ihm.
// - Je Verbindung ein zuverlaessiger Kanal (Sequenznummer, Bestaetigung,
//   Neusendung, Reihenfolge, Fenster von 256 Paketen) und ein
//   unzuverlaessiger. Grosse Nachrichten werden zerlegt.
// - Ein eigener Thread empfaengt, bestaetigt, sendet neu und haelt die
//   Verbindungen am Leben, auch wenn das Programm eine Weile nicht
//   RecvNetMsg aufruft. Alles Gemeinsame steht unter einem Mutex.
// - Spieler-IDs sind (Programmnummer << 16) | Zaehler: eindeutig ohne
//   Rueckfrage beim Host, nie 0 und nie -1.
//
// Einstellungen von aussen (gewinnen vor dem Code): Umgebungsvariablen
// BLTZNXT_NET_JOIN/SERVER/APP/PORT/LAN/UPNP, sonst die Datei <exe>.net neben der
// Exe. Nur fuer Tests: BLTZNXT_NET_BIND (Adresse der Sockets, 127.0.0.1 haelt
// die Firewall-Abfrage fern) und BLTZNXT_NET_DROP (Prozent verworfener
// Pakete).

#include "bb_socket.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <thread>

#ifdef _WIN32
#include <winhttp.h>
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
#else
#include <fcntl.h>
#endif

// ============================================================
// Konstanten und Hilfen
// ============================================================

inline constexpr int    BB_NET_VERSION   = 1;
inline constexpr int    BB_NET_PORT      = 2350;   // aus dem alten DirectPlay-Bereich
inline constexpr size_t BB_NET_FRAG      = 1100;   // Nutzdaten je Paket
inline constexpr size_t BB_NET_WINDOW    = 256;    // unbestaetigte Pakete je Verbindung
inline constexpr int    BB_NET_TIMEOUT   = 5000;   // ohne Lebenszeichen: weg
inline constexpr int    BB_NET_PING      = 250;
inline constexpr int    BB_NET_MIGRATE   = 8000;   // so lange dauert eine Host-Migration hoechstens

// Paketarten
enum : int {
  BB_NK_DISCOVER = 1, BB_NK_OFFER, BB_NK_JOIN, BB_NK_WELCOME, BB_NK_REJECT,
  BB_NK_REL, BB_NK_UNREL, BB_NK_ACK, BB_NK_PING, BB_NK_PONG, BB_NK_BYE,
  BB_NK_RESUME, BB_NK_RESUMED
};
// Sitzungsnachrichten im zuverlaessigen oder unzuverlaessigen Kanal
enum : int { BB_NS_MSG = 1, BB_NS_PADD, BB_NS_PDEL, BB_NS_MADD, BB_NS_MDEL };
// Gruende fuer REJECT
enum : int { BB_NR_PROTOCOL = 1, BB_NR_APP, BB_NR_NAME, BB_NR_CLOSED };

inline int64_t bb_net_now_() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void bb_net_sleep_(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

inline std::string bb_net_lower_(std::string s) {
  for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}
inline std::string bb_net_trim_(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
  return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

struct bb_NetW_ {
  std::string b;
  void u8(int v) { b.push_back(static_cast<char>(v & 255)); }
  void u16(int v) { u8(v); u8(v >> 8); }
  void u32(uint32_t v) { for (int i = 0; i < 4; ++i) u8(static_cast<int>(v >> (8 * i))); }
  void i32(int v) { u32(static_cast<uint32_t>(v)); }
  void str(const std::string& s) { u32(static_cast<uint32_t>(s.size())); b += s; }
};

struct bb_NetR_ {
  const std::string& b;
  size_t p;
  bool ok = true;
  explicit bb_NetR_(const std::string& s, size_t start = 0) : b(s), p(start) {}
  int u8() {
    if (p >= b.size()) { ok = false; return 0; }
    return static_cast<unsigned char>(b[p++]);
  }
  int u16() { int a = u8(); return a | (u8() << 8); }
  uint32_t u32() {
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(u8()) << (8 * i);
    return v;
  }
  int i32() { return static_cast<int>(u32()); }
  std::string str() {
    const uint32_t n = u32();
    if (!ok || n > b.size() - p) { ok = false; return std::string(); }
    std::string s = b.substr(p, n);
    p += n;
    return s;
  }
  std::string rest() {
    std::string s = p < b.size() ? b.substr(p) : std::string();
    p = b.size();
    return s;
  }
};

// Kopf jedes Pakets: "BNX", Protokollversion, Paketart
inline bb_NetW_ bb_net_pkt_(int kind) {
  bb_NetW_ w;
  w.b = "BNX";
  w.u8(BB_NET_VERSION);
  w.u8(kind);
  return w;
}

inline bool bb_net_same_addr_(const sockaddr_in& a, const sockaddr_in& b) {
  return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}
inline sockaddr_in bb_net_addr_(uint32_t ip_host_order, int port) {
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(ip_host_order);
  a.sin_port = htons(static_cast<uint16_t>(port));
  return a;
}
inline std::string bb_net_ip_text_(const sockaddr_in& a) {
  const uint32_t ip = ntohl(a.sin_addr.s_addr);
  return std::to_string(ip >> 24) + "." + std::to_string((ip >> 16) & 255) + "." +
         std::to_string((ip >> 8) & 255) + "." + std::to_string(ip & 255);
}

// ============================================================
// Zustand
// ============================================================

struct bb_NetOut_ { std::string pkt; int64_t sent = 0; int rto = 0; int tries = 0; };
struct bb_NetUnrelAsm_ { std::vector<std::string> parts; int got = 0; int64_t start = 0; };

struct bb_NetPeer_ {
  sockaddr_in addr{};
  bb_socket_t sock = BB_INVALID_SOCKET;
  int machine = 0;
  // zuverlaessig, senden
  uint32_t nextSeq = 1;
  std::map<uint32_t, bb_NetOut_> unacked;
  std::deque<std::string> waiting;        // Teilstuecke (Flags + Daten), warten aufs Fenster
  // zuverlaessig, empfangen
  uint32_t recvNext = 1;
  std::map<uint32_t, std::string> recvBuf;
  std::vector<uint32_t> acks;
  std::string reasm;
  // unzuverlaessig
  uint32_t unrelId = 0;
  std::map<uint32_t, bb_NetUnrelAsm_> unrel;
  int64_t lastRecv = 0, lastPing = 0;
  double srtt = 100;
};

struct bb_NetMachine_ { uint32_t ip = 0; int port = 0; int order = 0; };
struct bb_NetPlayer_  { std::string name; int machine = 0; };
struct bb_NetMsg_     { int type = 0, from = 0, to = 0; std::string data; };
struct bb_NetOffer_   { std::string name; sockaddr_in addr{}; int players = 0; int lan = 0; uint32_t token = 0; };

struct bb_NetState_ {
  std::mutex m;
  bool inited = false;
  std::atomic<bool> quit{false}, threadDone{false};
  bb_socket_t sockC = BB_INVALID_SOCKET;   // Client-Verkehr und Suche, freier Port
  bb_socket_t sockH = BB_INVALID_SOCKET;   // Spielport, solange gehostet wird

  // Einstellungen: von aussen (ext*) vor dem Code (code*)
  std::string extApp, codeApp, extServer, codeServer, extJoin;
  bool extAppSet = false, extServerSet = false;
  int extPort = 0, codePort = 0;
  int lan = 1, drop = 0;
  uint32_t bindIp = 0;                     // INADDR_ANY

  // Sitzung
  std::atomic<bool> started{false};
  bool isHost = false, lost = false;
  int myMachine = 0, hostMachine = 0, nextMachine = 2, nextOrder = 1, localCounter = 0;
  uint32_t token = 0;
  std::string gameName;
  std::map<int, bb_NetPeer_> peers;
  std::map<int, bb_NetMachine_> machines;
  std::map<int, bb_NetPlayer_> players;
  std::deque<bb_NetMsg_> queue;
  bb_NetMsg_ cur;
  std::vector<std::string> pendingOut;     // zuverlaessige Nachrichten waehrend einer Migration

  // Beitritt (Client) und doppelte JOINs (Host)
  bool joining = false;
  sockaddr_in joinAddr{};
  uint32_t joinNonce = 0;
  std::string joinName;
  int joinResult = 0;                      // 0 laeuft, 1 beigetreten, -1 abgelehnt
  std::string joinReason;
  int64_t joinLast = 0;
  std::map<uint32_t, int> joinNonces;

  // Host-Migration
  bool migrating = false;
  int candidate = 0;
  int64_t migrateStart = 0, migrateLast = 0, resumeDeadline = 0;

  // Suche
  bool collecting = false;
  int64_t discLast = 0;
  std::vector<bb_NetOffer_> offers, list;

  // Listen-Server
  std::atomic<int> regGen{0};
  std::string regKey, regUrl;

  // UPnP: Schalter (ext -1 = nicht gesetzt), feste Geraetebeschreibung statt
  // der Suche, Stand und was DeletePortMapping braucht
  int extUpnp = -1, codeUpnp = 1;
  std::string upnpUrl;
  std::atomic<int> upnpGen{0};
  int upnpState = 0;                       // 1 weitergeleitet, 0 nicht, -1 Antwort steht aus
  std::string upnpCtl, upnpSvc;
  int upnpPort = 0;

  std::mt19937 rng{std::random_device{}()};
};

inline bb_NetState_ bb_net_;

// ============================================================
// Einstellungen
// ============================================================

inline std::string bb_net_app_() {
  return bb_net_lower_(bb_net_.extAppSet ? bb_net_.extApp : bb_net_.codeApp);
}
inline int bb_net_port_() {
  return bb_net_.extPort ? bb_net_.extPort : (bb_net_.codePort ? bb_net_.codePort : BB_NET_PORT);
}
inline std::string bb_net_server_() {
  return bb_net_.extServerSet ? bb_net_.extServer : bb_net_.codeServer;
}

inline void bb_net_setting_(const std::string& key, const std::string& val, const char* where) {
  auto& S = bb_net_;
  const std::string k = bb_net_lower_(key);
  if (k == "join") S.extJoin = val;
  else if (k == "server") { S.extServer = val; S.extServerSet = true; }
  else if (k == "app") { S.extApp = val; S.extAppSet = true; }
  else if (k == "port") S.extPort = std::atoi(val.c_str());
  else if (k == "lan") S.lan = std::atoi(val.c_str()) != 0;
  else if (k == "bind") {
    in_addr a{};
    if (inet_pton(AF_INET, val.c_str(), &a) == 1) S.bindIp = ntohl(a.s_addr);
  }
  else if (k == "drop") S.drop = std::max(0, std::min(100, std::atoi(val.c_str())));
  else if (k == "upnp") {
    // 0 aus, 1 an, oder die URL der Geraetebeschreibung (Tests, Router, die die Suche nicht beantworten)
    if (val.compare(0, 7, "http://") == 0) { S.upnpUrl = val; S.extUpnp = 1; }
    else S.extUpnp = std::atoi(val.c_str()) != 0;
  }
  else std::cerr << "Net settings: unknown key '" << key << "' in " << where << "\n";
}

inline void bb_net_read_settings_() {
  std::string exe;
#ifdef _WIN32
  char buff[MAX_PATH + 1];
  const DWORD n = GetModuleFileNameA(nullptr, buff, MAX_PATH);
  exe.assign(buff, n);
#else
  if (bb_argc_ > 0) exe = bb_argv_[0];
#endif
  const size_t dot = exe.find_last_of('.'), sep = exe.find_last_of("\\/");
  const std::string base = (dot != std::string::npos && (sep == std::string::npos || dot > sep))
                               ? exe.substr(0, dot) : exe;
  std::ifstream f(base + ".net");
  std::string line;
  while (f && std::getline(f, line)) {
    const size_t c = line.find(" ;");
    if (c != std::string::npos) line = line.substr(0, c);
    line = bb_net_trim_(line);
    if (line.empty() || line[0] == ';' || line[0] == '#') continue;
    const size_t eq = line.find('=');
    if (eq == std::string::npos) {
      std::cerr << "Net settings: line without '=' in " << base << ".net: " << line << "\n";
      continue;
    }
    bb_net_setting_(bb_net_trim_(line.substr(0, eq)), bb_net_trim_(line.substr(eq + 1)), (base + ".net").c_str());
  }
  // Umgebungsvariablen gewinnen vor der Datei
  static const char* keys[] = {"JOIN", "SERVER", "APP", "PORT", "LAN", "BIND", "DROP", "UPNP"};
  for (const char* k : keys) {
    const std::string name = std::string("BLTZNXT_NET_") + k;
    if (const char* v = std::getenv(name.c_str())) bb_net_setting_(k, v, name.c_str());
  }
}

// ============================================================
// Sockets
// ============================================================

inline bb_socket_t bb_net_open_socket_(int port) {
  bb_socket_t s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == BB_INVALID_SOCKET) return s;
  int one = 1, big = 4 << 20;
  setsockopt(s, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&one), sizeof(one));
  setsockopt(s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&big), sizeof(big));
  setsockopt(s, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&big), sizeof(big));
#ifdef _WIN32
  setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&one), sizeof(one));
#endif
  sockaddr_in a = bb_net_addr_(bb_net_.bindIp, port);
  if (bind(s, reinterpret_cast<sockaddr*>(&a), sizeof(a)) != 0) {
    closesocket(s);
    return BB_INVALID_SOCKET;
  }
#ifdef _WIN32
  u_long nb = 1;
  ioctlsocket(s, FIONBIO, &nb);
  // Ein ICMP "Port unreachable" soll das naechste recvfrom nicht scheitern lassen.
  BOOL off = FALSE;
  DWORD ret = 0;
  WSAIoctl(s, SIO_UDP_CONNRESET, &off, sizeof(off), nullptr, 0, &ret, nullptr, nullptr);
#else
  fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#endif
  return s;
}

inline int bb_net_sock_port_(bb_socket_t s) {
  if (s == BB_INVALID_SOCKET) return 0;
  sockaddr_in a{};
  socklen_t n = sizeof(a);
  if (getsockname(s, reinterpret_cast<sockaddr*>(&a), &n) != 0) return 0;
  return ntohs(a.sin_port);
}

inline void bb_net_sendto_(bb_socket_t s, const sockaddr_in& a, const std::string& d) {
  if (s == BB_INVALID_SOCKET) return;
  sendto(s, d.data(), static_cast<int>(d.size()), 0, reinterpret_cast<const sockaddr*>(&a), sizeof(a));
}

// Testschalter BLTZNXT_NET_DROP: verwirft einen Anteil der Pakete beider Richtungen.
inline bool bb_net_dropped_() {
  return bb_net_.drop > 0 && static_cast<int>(bb_net_.rng() % 100) < bb_net_.drop;
}

inline void bb_net_send_peer_(bb_NetPeer_& p, const std::string& d) {
  if (!bb_net_dropped_()) bb_net_sendto_(p.sock, p.addr, d);
}

// ============================================================
// Kanaele
// ============================================================

inline int bb_net_rto_(const bb_NetPeer_& p) {
  return std::max(30, std::min(1000, static_cast<int>(p.srtt * 2) + 10));
}

inline void bb_net_pump_(bb_NetPeer_& p, int64_t now) {
  while (!p.waiting.empty() && p.unacked.size() < BB_NET_WINDOW) {
    const uint32_t seq = p.nextSeq++;
    bb_NetW_ w = bb_net_pkt_(BB_NK_REL);
    w.u32(seq);
    w.b += p.waiting.front();
    p.waiting.pop_front();
    bb_NetOut_& o = p.unacked[seq];
    o.pkt = std::move(w.b);
    o.sent = now;
    o.rto = bb_net_rto_(p);
    o.tries = 1;
    bb_net_send_peer_(p, o.pkt);
  }
}

inline void bb_net_send_rel_(bb_NetPeer_& p, const std::string& payload) {
  size_t off = 0;
  const size_t n = payload.size();
  do {
    const size_t len = std::min(BB_NET_FRAG, n - off);
    std::string rec(1, static_cast<char>(off + len >= n ? 1 : 0));
    rec.append(payload, off, len);
    p.waiting.push_back(std::move(rec));
    off += len;
  } while (off < n);
  bb_net_pump_(p, bb_net_now_());
}

inline void bb_net_send_unrel_(bb_NetPeer_& p, const std::string& payload) {
  const size_t cnt = std::max<size_t>(1, (payload.size() + BB_NET_FRAG - 1) / BB_NET_FRAG);
  if (cnt > 65535) return;
  const uint32_t id = ++p.unrelId;
  for (size_t i = 0; i < cnt; ++i) {
    bb_NetW_ w = bb_net_pkt_(BB_NK_UNREL);
    w.u32(id);
    w.u16(static_cast<int>(i));
    w.u16(static_cast<int>(cnt));
    w.b.append(payload, i * BB_NET_FRAG, BB_NET_FRAG);
    bb_net_send_peer_(p, w.b);
  }
}

inline void bb_net_send_(bb_NetPeer_& p, const std::string& payload, bool reliable) {
  if (reliable) bb_net_send_rel_(p, payload);
  else bb_net_send_unrel_(p, payload);
}

inline void bb_net_flush_acks_(bb_NetPeer_& p) {
  size_t i = 0;
  while (i < p.acks.size()) {
    const size_t n = std::min<size_t>(280, p.acks.size() - i);
    bb_NetW_ w = bb_net_pkt_(BB_NK_ACK);
    w.u16(static_cast<int>(n));
    for (size_t k = 0; k < n; ++k) w.u32(p.acks[i + k]);
    bb_net_send_peer_(p, w.b);
    i += n;
  }
  p.acks.clear();
}

inline bb_NetPeer_& bb_net_new_peer_(int machine, const sockaddr_in& addr, bb_socket_t sock) {
  bb_NetPeer_& p = bb_net_.peers[machine];
  p = bb_NetPeer_();
  p.machine = machine;
  p.addr = addr;
  p.sock = sock;
  p.lastRecv = p.lastPing = bb_net_now_();
  return p;
}

// ============================================================
// Sitzungsnachrichten
// ============================================================

inline std::string bb_net_msg_payload_(const bb_NetMsg_& m) {
  bb_NetW_ w;
  w.u8(BB_NS_MSG);
  w.i32(m.type);
  w.i32(m.from);
  w.i32(m.to);
  w.str(m.data);
  return w.b;
}
inline std::string bb_net_padd_payload_(int id, const bb_NetPlayer_& pl) {
  bb_NetW_ w;
  w.u8(BB_NS_PADD);
  w.i32(id);
  w.i32(pl.machine);
  w.str(pl.name);
  return w.b;
}
inline std::string bb_net_id_payload_(int kind, int id) {
  bb_NetW_ w;
  w.u8(kind);
  w.i32(id);
  return w.b;
}
inline std::string bb_net_madd_payload_(int id, const bb_NetMachine_& mc) {
  bb_NetW_ w;
  w.u8(BB_NS_MADD);
  w.i32(id);
  w.u32(mc.ip);
  w.u16(mc.port);
  w.i32(mc.order);
  return w.b;
}

inline void bb_net_event_(int type, int from) {
  bb_NetMsg_ m;
  m.type = type;
  m.from = from;
  bb_net_.queue.push_back(std::move(m));
}

// Eine Nachricht so verteilen, wie der Host es tut. origin = Programm, von dem
// sie kommt. An alle: alle ausser dem Absender. An einen Spieler: an dessen
// Programm, auch an das eigene (lokal an lokal).
inline void bb_net_route_(int origin, const bb_NetMsg_& msg, const std::string& payload, bool reliable) {
  auto& S = bb_net_;
  if (msg.to == 0) {
    if (origin != S.myMachine) S.queue.push_back(msg);
    for (auto& [mid, p] : S.peers)
      if (mid != origin) bb_net_send_(p, payload, reliable);
    return;
  }
  const auto it = S.players.find(msg.to);
  if (it == S.players.end()) return;
  const int owner = it->second.machine;
  if (owner == S.myMachine) S.queue.push_back(msg);
  else if (owner != origin) {
    const auto pit = S.peers.find(owner);
    if (pit != S.peers.end()) bb_net_send_(pit->second, payload, reliable);
  }
}

// Ein Programm verlaesst die Sitzung: seine Spieler gehen (101), der Host
// sagt es den anderen.
inline void bb_net_remove_machine_(int machine) {
  auto& S = bb_net_;
  std::vector<int> ids;
  for (auto& [id, pl] : S.players)
    if (pl.machine == machine) ids.push_back(id);
  S.peers.erase(machine);
  for (int id : ids) {
    S.players.erase(id);
    bb_net_event_(101, id);
    if (S.isHost)
      for (auto& [mid, p] : S.peers) bb_net_send_rel_(p, bb_net_id_payload_(BB_NS_PDEL, id));
  }
  S.machines.erase(machine);
  if (S.isHost)
    for (auto& [mid, p] : S.peers) bb_net_send_rel_(p, bb_net_id_payload_(BB_NS_MDEL, machine));
}

inline void bb_net_reg_start_();
inline void bb_net_upnp_start_();

inline void bb_net_become_host_(int64_t now) {
  auto& S = bb_net_;
  S.isHost = true;
  S.hostMachine = S.myMachine;
  S.migrating = false;
  S.pendingOut.clear();
  int maxId = S.myMachine, maxOrder = 0;
  for (auto& [id, mc] : S.machines) { maxId = std::max(maxId, id); maxOrder = std::max(maxOrder, mc.order); }
  S.nextMachine = maxId + 1;
  S.nextOrder = maxOrder + 1;
  S.resumeDeadline = now + BB_NET_MIGRATE;
  if (S.sockH == BB_INVALID_SOCKET) S.sockH = bb_net_open_socket_(bb_net_port_());
  bb_net_event_(102, -1);
  bb_net_reg_start_();
  bb_net_upnp_start_();
}

// Der Client hat den Host verloren (BYE oder Zeitueberschreitung).
inline void bb_net_host_gone_(int64_t now) {
  auto& S = bb_net_;
  const int old = S.hostMachine;
  S.peers.clear();
  std::vector<int> ids;
  for (auto& [id, pl] : S.players)
    if (pl.machine == old) ids.push_back(id);
  for (int id : ids) { S.players.erase(id); bb_net_event_(101, id); }
  S.machines.erase(old);
  int cand = 0, best = INT_MAX;
  for (auto& [id, mc] : S.machines)
    if (mc.order < best) { best = mc.order; cand = id; }
  if (cand == S.myMachine) {
    bb_net_become_host_(now);
  } else if (cand == 0) {
    S.lost = true;
    bb_net_event_(200, 0);
  } else {
    S.migrating = true;
    S.candidate = cand;
    S.migrateStart = now;
    S.migrateLast = 0;
  }
}

// Eine vollstaendige Sitzungsnachricht von Programm `machine`.
inline void bb_net_deliver_(int machine, const std::string& pl, bool reliable) {
  auto& S = bb_net_;
  bb_NetR_ r(pl);
  const int kind = r.u8();
  if (kind == BB_NS_MSG) {
    bb_NetMsg_ m;
    m.type = r.i32();
    m.from = r.i32();
    m.to = r.i32();
    m.data = r.str();
    if (!r.ok) return;
    if (S.isHost) bb_net_route_(machine, m, pl, reliable);
    else S.queue.push_back(std::move(m));
  } else if (kind == BB_NS_PADD) {
    const int id = r.i32();
    bb_NetPlayer_ p;
    p.machine = r.i32();
    p.name = r.str();
    if (!r.ok || S.players.count(id)) return;
    if (S.isHost && p.machine != machine) return;
    S.players[id] = p;
    bb_net_event_(100, id);
    if (S.isHost)
      for (auto& [mid, peer] : S.peers)
        if (mid != machine) bb_net_send_rel_(peer, pl);
  } else if (kind == BB_NS_PDEL) {
    const int id = r.i32();
    const auto it = S.players.find(id);
    if (!r.ok || it == S.players.end()) return;
    if (S.isHost && it->second.machine != machine) return;
    S.players.erase(it);
    bb_net_event_(101, id);
    if (S.isHost)
      for (auto& [mid, peer] : S.peers)
        if (mid != machine) bb_net_send_rel_(peer, pl);
  } else if (kind == BB_NS_MADD && !S.isHost) {
    const int id = r.i32();
    bb_NetMachine_ mc;
    mc.ip = r.u32();
    mc.port = r.u16();
    mc.order = r.i32();
    if (r.ok) S.machines[id] = mc;
  } else if (kind == BB_NS_MDEL && !S.isHost) {
    const int id = r.i32();
    if (r.ok && id != S.myMachine) S.machines.erase(id);
  }
}

// ============================================================
// Pakete empfangen
// ============================================================

inline bb_NetPeer_* bb_net_find_peer_(const sockaddr_in& from) {
  for (auto& [mid, p] : bb_net_.peers)
    if (bb_net_same_addr_(p.addr, from)) return &p;
  return nullptr;
}

inline void bb_net_reject_(bb_socket_t s, const sockaddr_in& to, uint32_t nonce, int code, const std::string& detail) {
  bb_NetW_ w = bb_net_pkt_(BB_NK_REJECT);
  w.u32(nonce);
  w.u8(code);
  w.str(detail);
  bb_net_sendto_(s, to, w.b);
}

inline void bb_net_send_welcome_(bb_socket_t s, const sockaddr_in& to, uint32_t nonce, int machine) {
  auto& S = bb_net_;
  bb_NetW_ w = bb_net_pkt_(BB_NK_WELCOME);
  w.u32(nonce);
  w.i32(machine);
  w.i32(S.myMachine);
  w.u32(S.token);
  w.str(S.gameName);
  w.u16(static_cast<int>(S.machines.size()));
  for (auto& [id, mc] : S.machines) { w.i32(id); w.u32(mc.ip); w.u16(mc.port); w.i32(mc.order); }
  w.u32(static_cast<uint32_t>(S.players.size()));
  for (auto& [id, pl] : S.players) { w.i32(id); w.i32(pl.machine); w.str(pl.name); }
  bb_net_sendto_(s, to, w.b);
}

inline void bb_net_on_join_(bb_socket_t s, const sockaddr_in& from, bb_NetR_& r, int version) {
  auto& S = bb_net_;
  const std::string app = bb_net_lower_(r.str()), name = r.str();
  const uint32_t nonce = r.u32();
  if (!r.ok || !S.started || !S.isHost) return;
  if (version != BB_NET_VERSION) { bb_net_reject_(s, from, nonce, BB_NR_PROTOCOL, std::to_string(BB_NET_VERSION)); return; }
  if (app != bb_net_app_()) { bb_net_reject_(s, from, nonce, BB_NR_APP, bb_net_app_()); return; }
  if (!name.empty() && bb_net_lower_(name) != bb_net_lower_(S.gameName)) {
    bb_net_reject_(s, from, nonce, BB_NR_NAME, S.gameName);
    return;
  }
  if (S.lost) { bb_net_reject_(s, from, nonce, BB_NR_CLOSED, ""); return; }
  const auto known = S.joinNonces.find(nonce);
  if (known != S.joinNonces.end() && S.peers.count(known->second) &&
      bb_net_same_addr_(S.peers[known->second].addr, from)) {
    bb_net_send_welcome_(s, from, nonce, known->second);   // WELCOME ging verloren
    return;
  }
  const int id = S.nextMachine++;
  bb_NetMachine_ mc;
  mc.ip = ntohl(from.sin_addr.s_addr);
  mc.port = ntohs(from.sin_port);
  mc.order = S.nextOrder++;
  for (auto& [mid, p] : S.peers) bb_net_send_rel_(p, bb_net_madd_payload_(id, mc));
  S.machines[id] = mc;
  bb_net_new_peer_(id, from, s);
  S.joinNonces[nonce] = id;
  bb_net_send_welcome_(s, from, nonce, id);
}

inline void bb_net_on_welcome_(const sockaddr_in& from, bb_socket_t s, bb_NetR_& r) {
  auto& S = bb_net_;
  const uint32_t nonce = r.u32();
  if (!S.joining || S.joinResult != 0 || nonce != S.joinNonce) return;
  const int mine = r.i32(), host = r.i32();
  const uint32_t token = r.u32();
  const std::string game = r.str();
  std::map<int, bb_NetMachine_> machines;
  const int nm = r.u16();
  for (int i = 0; i < nm && r.ok; ++i) {
    const int id = r.i32();
    bb_NetMachine_& mc = machines[id];
    mc.ip = r.u32();
    mc.port = r.u16();
    mc.order = r.i32();
  }
  std::map<int, bb_NetPlayer_> players;
  const uint32_t np = r.u32();
  for (uint32_t i = 0; i < np && r.ok; ++i) {
    const int id = r.i32();
    bb_NetPlayer_& pl = players[id];
    pl.machine = r.i32();
    pl.name = r.str();
  }
  if (!r.ok) return;
  S.myMachine = mine;
  S.hostMachine = host;
  S.token = token;
  S.gameName = game;
  S.machines = std::move(machines);
  S.players = std::move(players);
  S.isHost = false;
  S.lost = false;
  S.localCounter = 0;
  S.queue.clear();
  S.cur = bb_NetMsg_();
  bb_net_new_peer_(host, from, s);
  // Wie im Original: gleich nach dem Beitritt je eine 100 fuer jeden, der schon da ist.
  for (auto& [id, pl] : S.players) bb_net_event_(100, id);
  S.started = true;
  S.joinResult = 1;
}

inline void bb_net_on_resume_(bb_socket_t s, const sockaddr_in& from, bb_NetR_& r) {
  auto& S = bb_net_;
  const uint32_t token = r.u32();
  const int machine = r.i32();
  if (!r.ok || !S.started || !S.isHost || token != S.token || machine == S.myMachine ||
      !S.machines.count(machine))
    return;
  bb_NetW_ ok = bb_net_pkt_(BB_NK_RESUMED);
  ok.u32(S.token);
  const auto it = S.peers.find(machine);
  if (it != S.peers.end() && bb_net_same_addr_(it->second.addr, from)) {
    bb_net_sendto_(s, from, ok.b);   // Antwort ging verloren
    return;
  }
  bb_NetMachine_& mc = S.machines[machine];
  mc.ip = ntohl(from.sin_addr.s_addr);
  mc.port = ntohs(from.sin_port);
  bb_NetPeer_& p = bb_net_new_peer_(machine, from, s);
  bb_net_sendto_(s, from, ok.b);
  // Der Zurueckgekehrte bekommt den Stand des neuen Hosts, die anderen seine neue Adresse.
  for (auto& [id, m] : S.machines) bb_net_send_rel_(p, bb_net_madd_payload_(id, m));
  for (auto& [id, pl] : S.players)
    if (pl.machine != machine) bb_net_send_rel_(p, bb_net_padd_payload_(id, pl));
  for (auto& [mid, q] : S.peers)
    if (mid != machine) bb_net_send_rel_(q, bb_net_madd_payload_(machine, mc));
}

inline void bb_net_on_resumed_(const sockaddr_in& from, bb_socket_t s, bb_NetR_& r) {
  auto& S = bb_net_;
  const uint32_t token = r.u32();
  if (!r.ok || !S.migrating || token != S.token) return;
  S.migrating = false;
  S.hostMachine = S.candidate;
  bb_NetPeer_& p = bb_net_new_peer_(S.candidate, from, s);
  for (auto& pl : S.pendingOut) bb_net_send_rel_(p, pl);
  S.pendingOut.clear();
}

inline void bb_net_on_rel_(int machine, bb_NetR_& r) {
  auto& S = bb_net_;
  bb_NetPeer_* p = &S.peers[machine];
  const uint32_t seq = r.u32();
  if (!r.ok) return;
  p->acks.push_back(seq);
  if (seq < p->recvNext || p->recvBuf.count(seq) || seq - p->recvNext > 1000000) return;
  p->recvBuf[seq] = r.rest();
  for (;;) {
    auto it = p->recvBuf.find(p->recvNext);
    if (it == p->recvBuf.end()) break;
    std::string rec = std::move(it->second);
    p->recvBuf.erase(it);
    ++p->recvNext;
    if (rec.empty()) continue;
    p->reasm.append(rec, 1, std::string::npos);
    if (rec[0] & 1) {
      std::string whole;
      whole.swap(p->reasm);
      bb_net_deliver_(machine, whole, true);
      const auto again = S.peers.find(machine);   // die Nachricht kann die Sitzung veraendert haben
      if (again == S.peers.end()) return;
      p = &again->second;
    }
  }
}

inline void bb_net_on_unrel_(int machine, bb_NetR_& r, int64_t now) {
  bb_NetPeer_& p = bb_net_.peers[machine];
  const uint32_t id = r.u32();
  const int idx = r.u16(), cnt = r.u16();
  if (!r.ok || cnt < 1 || idx >= cnt) return;
  std::string chunk = r.rest();
  if (cnt == 1) { bb_net_deliver_(machine, chunk, false); return; }
  bb_NetUnrelAsm_& a = p.unrel[id];
  if (a.parts.empty()) { a.parts.resize(cnt); a.start = now; }
  if (static_cast<int>(a.parts.size()) != cnt || !a.parts[idx].empty()) return;
  a.parts[idx] = std::move(chunk);
  if (++a.got < cnt) return;
  std::string whole;
  for (auto& s : a.parts) whole += s;
  p.unrel.erase(id);
  bb_net_deliver_(machine, whole, false);
}

inline void bb_net_on_packet_(bb_socket_t s, const sockaddr_in& from, const std::string& d, int64_t now) {
  auto& S = bb_net_;
  if (d.size() < 5 || d.compare(0, 3, "BNX") != 0) return;
  const int version = static_cast<unsigned char>(d[3]), kind = static_cast<unsigned char>(d[4]);
  bb_NetR_ r(d, 5);
  if (kind == BB_NK_JOIN) { bb_net_on_join_(s, from, r, version); return; }
  if (kind == BB_NK_REJECT) {
    const uint32_t nonce = r.u32();
    const int code = r.u8();
    const std::string detail = r.str();
    if (!r.ok || !S.joining || S.joinResult != 0 || nonce != S.joinNonce) return;
    auto shown = [](const std::string& app) { return app.empty() ? std::string("(no NetGameApp)") : "'" + app + "'"; };
    if (code == BB_NR_PROTOCOL)
      S.joinReason = "host speaks protocol " + detail + ", this program " + std::to_string(BB_NET_VERSION);
    else if (code == BB_NR_APP)
      S.joinReason = "host runs " + shown(detail) + ", this program is " + shown(bb_net_app_());
    else if (code == BB_NR_NAME)
      S.joinReason = "host's game is '" + detail + "', not '" + S.joinName + "'";
    else
      S.joinReason = "the game is closed";
    S.joinResult = -1;
    return;
  }
  if (version != BB_NET_VERSION) return;
  if (kind == BB_NK_DISCOVER) {
    const std::string app = bb_net_lower_(r.str());
    if (!r.ok || !S.started || !S.isHost || S.lost || app != bb_net_app_()) return;
    bb_NetW_ w = bb_net_pkt_(BB_NK_OFFER);
    w.u32(S.token);
    w.str(S.gameName);
    w.u16(static_cast<int>(S.players.size()));
    bb_net_sendto_(s, from, w.b);
    return;
  }
  if (kind == BB_NK_OFFER) {
    const uint32_t token = r.u32();
    const std::string name = r.str();
    const int players = r.u16();
    if (!r.ok || !S.collecting || (S.started && token == S.token)) return;
    for (auto& o : S.offers)
      if (o.token == token) return;
    bb_NetOffer_ o;
    o.name = name;
    o.addr = from;
    o.players = players;
    o.lan = 1;
    o.token = token;
    S.offers.push_back(o);
    return;
  }
  if (kind == BB_NK_WELCOME) { bb_net_on_welcome_(from, s, r); return; }
  if (kind == BB_NK_RESUME) { bb_net_on_resume_(s, from, r); return; }
  if (kind == BB_NK_RESUMED) { bb_net_on_resumed_(from, s, r); return; }

  bb_NetPeer_* p = bb_net_find_peer_(from);
  if (!p) return;
  if (bb_net_dropped_()) return;
  p->lastRecv = now;
  const int machine = p->machine;
  switch (kind) {
    case BB_NK_REL: bb_net_on_rel_(machine, r); break;
    case BB_NK_UNREL: bb_net_on_unrel_(machine, r, now); break;
    case BB_NK_ACK: {
      const int n = r.u16();
      for (int i = 0; i < n && r.ok; ++i) {
        const auto it = p->unacked.find(r.u32());
        if (it == p->unacked.end()) continue;
        if (it->second.tries == 1)
          p->srtt = p->srtt * 0.875 + 0.125 * static_cast<double>(now - it->second.sent);
        p->unacked.erase(it);
      }
      bb_net_pump_(*p, now);
      break;
    }
    case BB_NK_PING: {
      bb_NetW_ w = bb_net_pkt_(BB_NK_PONG);
      w.u32(r.u32());
      bb_net_send_peer_(*p, w.b);
      break;
    }
    case BB_NK_PONG: {
      const uint32_t t = r.u32();
      const double rtt = static_cast<double>(static_cast<uint32_t>(now) - t);
      if (r.ok && rtt >= 0 && rtt < 10000) p->srtt = p->srtt * 0.875 + 0.125 * rtt;
      break;
    }
    case BB_NK_BYE:
      if (S.isHost) bb_net_remove_machine_(machine);
      else if (machine == S.hostMachine) bb_net_host_gone_(now);
      break;
  }
}

// ============================================================
// Takt: Neusenden, Lebenszeichen, Zeitueberschreitungen, Suche
// ============================================================

inline std::vector<sockaddr_in> bb_net_discover_targets_() {
  std::vector<sockaddr_in> out;
  const int port = bb_net_port_();
  out.push_back(bb_net_addr_(0x7F000001u, port));
  if (bb_net_.bindIp != 0) return out;   // an 127.0.0.1 gebunden: kein Broadcast
  out.push_back(bb_net_addr_(0xFFFFFFFFu, port));
  char name[256];
  if (gethostname(name, sizeof(name)) == 0) {
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    if (getaddrinfo(name, nullptr, &hints, &res) == 0) {
      for (addrinfo* a = res; a; a = a->ai_next) {
        const uint32_t ip = ntohl(reinterpret_cast<sockaddr_in*>(a->ai_addr)->sin_addr.s_addr);
        if ((ip >> 24) != 127) out.push_back(bb_net_addr_(ip | 0xFFu, port));   // /24 angenommen
      }
      freeaddrinfo(res);
    }
  }
  return out;
}

inline void bb_net_tick_(int64_t now) {
  auto& S = bb_net_;
  if (S.collecting && now - S.discLast >= 300) {
    S.discLast = now;
    bb_NetW_ w = bb_net_pkt_(BB_NK_DISCOVER);
    w.str(bb_net_app_());
    for (auto& a : bb_net_discover_targets_()) bb_net_sendto_(S.sockC, a, w.b);
  }
  if (S.joining && S.joinResult == 0 && now - S.joinLast >= 250) {
    S.joinLast = now;
    bb_NetW_ w = bb_net_pkt_(BB_NK_JOIN);
    w.str(bb_net_app_());
    w.str(S.joinName);
    w.u32(S.joinNonce);
    bb_net_sendto_(S.sockC, S.joinAddr, w.b);
  }
  if (!S.started) return;
  if (S.migrating) {
    if (now - S.migrateStart > BB_NET_MIGRATE) {
      // Der Kandidat antwortet nicht: er ist wohl mit dem Host gegangen. Der
      // naechste in der Reihe uebernimmt; ist keiner mehr da, ist die Sitzung verloren.
      const int gone = S.candidate;
      std::vector<int> ids;
      for (auto& [id, pl] : S.players)
        if (pl.machine == gone) ids.push_back(id);
      for (int id : ids) { S.players.erase(id); bb_net_event_(101, id); }
      S.machines.erase(gone);
      int cand = 0, best = INT_MAX;
      for (auto& [id, mc] : S.machines)
        if (mc.order < best) { best = mc.order; cand = id; }
      if (cand == S.myMachine) {
        bb_net_become_host_(now);
      } else if (cand == 0) {
        S.migrating = false;
        S.lost = true;
        S.pendingOut.clear();
        bb_net_event_(200, 0);
      } else {
        S.candidate = cand;
        S.migrateStart = now;
        S.migrateLast = 0;
      }
    } else if (now - S.migrateLast >= 250 && S.machines.count(S.candidate)) {
      S.migrateLast = now;
      const bb_NetMachine_& mc = S.machines[S.candidate];
      bb_NetW_ w = bb_net_pkt_(BB_NK_RESUME);
      w.u32(S.token);
      w.i32(S.myMachine);
      bb_net_sendto_(S.sockC, bb_net_addr_(mc.ip, mc.port), w.b);
    }
  }
  if (S.isHost && S.resumeDeadline && now > S.resumeDeadline) {
    S.resumeDeadline = 0;
    std::vector<int> gone;
    for (auto& [id, mc] : S.machines)
      if (id != S.myMachine && !S.peers.count(id)) gone.push_back(id);
    for (int id : gone) bb_net_remove_machine_(id);
  }
  std::vector<int> timedOut;
  for (auto& [mid, p] : S.peers) {
    if (now - p.lastRecv > BB_NET_TIMEOUT) { timedOut.push_back(mid); continue; }
    if (now - p.lastPing >= BB_NET_PING) {
      p.lastPing = now;
      bb_NetW_ w = bb_net_pkt_(BB_NK_PING);
      w.u32(static_cast<uint32_t>(now));
      bb_net_send_peer_(p, w.b);
    }
    for (auto& [seq, o] : p.unacked) {
      if (now - o.sent < o.rto) continue;
      bb_net_send_peer_(p, o.pkt);
      o.sent = now;
      o.rto = std::min(o.rto * 2, 2000);
      ++o.tries;
    }
    bb_net_pump_(p, now);
    for (auto it = p.unrel.begin(); it != p.unrel.end();)
      it = (now - it->second.start > 2000) ? p.unrel.erase(it) : std::next(it);
  }
  for (int mid : timedOut) {
    if (!S.peers.count(mid)) continue;
    if (S.isHost) bb_net_remove_machine_(mid);
    else if (mid == S.hostMachine) bb_net_host_gone_(now);
  }
}

inline void bb_net_thread_() {
  auto& S = bb_net_;
  std::string buf(65536, '\0');
  while (!S.quit) {
    bb_socket_t socks[2];
    {
      std::lock_guard<std::mutex> g(S.m);
      socks[0] = S.sockC;
      socks[1] = S.sockH;
    }
    fd_set rs;
    FD_ZERO(&rs);
    int maxfd = 0, nsock = 0;
    for (bb_socket_t s : socks) {
      if (s == BB_INVALID_SOCKET) continue;
      FD_SET(s, &rs);
      maxfd = std::max(maxfd, static_cast<int>(s));
      ++nsock;
    }
    if (nsock) {
      timeval tv{0, 5000};
      if (select(maxfd + 1, &rs, nullptr, nullptr, &tv) < 0) { bb_net_sleep_(5); FD_ZERO(&rs); }
    } else {
      bb_net_sleep_(5);
    }
    std::lock_guard<std::mutex> g(S.m);
    const int64_t now = bb_net_now_();
    for (bb_socket_t s : socks) {
      if (s == BB_INVALID_SOCKET || !FD_ISSET(s, &rs)) continue;
      if (s != S.sockC && s != S.sockH) continue;   // inzwischen geschlossen
      for (int i = 0; i < 4000; ++i) {
        sockaddr_in from{};
        socklen_t fl = sizeof(from);
        const int n = recvfrom(s, &buf[0], static_cast<int>(buf.size()), 0,
                               reinterpret_cast<sockaddr*>(&from), &fl);
        if (n <= 0) break;
        bb_net_on_packet_(s, from, std::string(buf.data(), static_cast<size_t>(n)), now);
      }
    }
    bb_net_tick_(now);
    for (auto& [mid, p] : S.peers) bb_net_flush_acks_(p);
  }
  S.threadDone = true;
}

// ============================================================
// Listen-Server (HTTP)
// ============================================================

inline std::string bb_net_urlenc_(const std::string& s) {
  static const char* hex = "0123456789ABCDEF";
  std::string o;
  for (unsigned char c : s) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') o += static_cast<char>(c);
    else { o += '%'; o += hex[c >> 4]; o += hex[c & 15]; }
  }
  return o;
}

// "http://host:port/pfad" zerlegen; false bei einem anderen Schema.
inline bool bb_net_url_split_(const std::string& url, std::string& hostport, std::string& host, std::string& port,
                              std::string& path) {
  if (url.compare(0, 7, "http://") != 0) return false;
  const size_t slash = url.find('/', 7);
  hostport = url.substr(7, slash == std::string::npos ? std::string::npos : slash - 7);
  path = slash == std::string::npos ? "/" : url.substr(slash);
  host = hostport;
  port = "80";
  const size_t colon = hostport.find(':');
  if (colon != std::string::npos) { host = hostport.substr(0, colon); port = hostport.substr(colon + 1); }
  return true;
}

// HTTP/1.0 ueber einen eigenen Socket, nur http://. Unter Windows nimmt
// bb_net_http_ WinHTTP, das auch https kann. Fuer UPnP im LAN immer diese
// Fassung: WinHTTP wuerde einen eingestellten Proxy nehmen.
inline std::string bb_net_http_plain_(bool post, const std::string& url, const std::string& body,
                                      int timeout, int& status,
                                      const char* ctype = "application/x-www-form-urlencoded",
                                      const std::string& extra = std::string()) {
  status = 0;
  std::string hostport, host, port, path;
  if (!bb_net_url_split_(url, hostport, host, port, path)) return std::string();
  addrinfo hints{}, *res = nullptr;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0 || !res) return std::string();
  bb_socket_t s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  std::string out;
  if (s != BB_INVALID_SOCKET) {
#ifdef _WIN32
    DWORD tv = static_cast<DWORD>(timeout);
#else
    timeval tv{timeout / 1000, (timeout % 1000) * 1000};
#endif
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    if (connect(s, res->ai_addr, static_cast<int>(res->ai_addrlen)) == 0) {
      std::string req = std::string(post ? "POST " : "GET ") + path + " HTTP/1.0\r\nHost: " + hostport +
                        "\r\nUser-Agent: BLTZNXT\r\nConnection: close\r\n" + extra;
      if (post)
        req += std::string("Content-Type: ") + ctype + "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n";
      req += "\r\n" + body;
      send(s, req.data(), static_cast<int>(req.size()), 0);
      char b[4096];
      int n;
      while ((n = recv(s, b, sizeof(b), 0)) > 0 && out.size() < (1u << 20)) out.append(b, n);
    }
    closesocket(s);
  }
  freeaddrinfo(res);
  const size_t sp = out.find(' '), hdrEnd = out.find("\r\n\r\n");
  if (sp == std::string::npos || hdrEnd == std::string::npos) return std::string();
  status = std::atoi(out.c_str() + sp + 1);
  std::string data = out.substr(hdrEnd + 4);
  // Manche Router antworten auch auf HTTP/1.0 in Stuecken.
  if (bb_net_lower_(out.substr(0, hdrEnd)).find("transfer-encoding: chunked") != std::string::npos) {
    std::string plain;
    size_t p = 0;
    while (p < data.size()) {
      const size_t eol = data.find("\r\n", p);
      if (eol == std::string::npos) break;
      const size_t n = std::strtoul(data.c_str() + p, nullptr, 16);
      if (n == 0 || eol + 2 + n > data.size()) break;
      plain.append(data, eol + 2, n);
      p = eol + 4 + n;
    }
    data.swap(plain);
  }
  return data;
}

inline std::string bb_net_http_(bool post, const std::string& url, const std::string& body, int timeout,
                                int& status) {
#ifdef _WIN32
  status = 0;
  std::string out;
  const std::wstring wurl(url.begin(), url.end());
  wchar_t host[256], path[2048], extra[2048];
  URL_COMPONENTS uc{};
  uc.dwStructSize = sizeof(uc);
  uc.lpszHostName = host;  uc.dwHostNameLength = 256;
  uc.lpszUrlPath = path;   uc.dwUrlPathLength = 2048;
  uc.lpszExtraInfo = extra; uc.dwExtraInfoLength = 2048;
  if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return out;
  HINTERNET ses = WinHttpOpen(L"BLTZNXT", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                              WINHTTP_NO_PROXY_BYPASS, 0);
  if (!ses) return out;
  WinHttpSetTimeouts(ses, timeout, timeout, timeout, timeout);
  HINTERNET con = WinHttpConnect(ses, host, uc.nPort, 0);
  const std::wstring p = std::wstring(path, uc.dwUrlPathLength) + std::wstring(extra, uc.dwExtraInfoLength);
  HINTERNET req = con ? WinHttpOpenRequest(con, post ? L"POST" : L"GET", p.c_str(), nullptr,
                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
                      : nullptr;
  if (req) {
    const wchar_t* hdr = L"Content-Type: application/x-www-form-urlencoded\r\n";
    const DWORD n = static_cast<DWORD>(body.size());
    BOOL ok = WinHttpSendRequest(req, post ? hdr : WINHTTP_NO_ADDITIONAL_HEADERS, post ? static_cast<DWORD>(-1) : 0,
                                 post ? const_cast<char*>(body.data()) : WINHTTP_NO_REQUEST_DATA,
                                 post ? n : 0, post ? n : 0, 0) &&
              WinHttpReceiveResponse(req, nullptr);
    if (ok) {
      DWORD code = 0, sz = sizeof(code);
      WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                          &code, &sz, WINHTTP_NO_HEADER_INDEX);
      status = static_cast<int>(code);
      DWORD avail = 0;
      while (WinHttpQueryDataAvailable(req, &avail) && avail && out.size() < (1u << 20)) {
        std::string chunk(avail, '\0');
        DWORD got = 0;
        if (!WinHttpReadData(req, &chunk[0], avail, &got) || !got) break;
        out.append(chunk.data(), got);
      }
    }
    WinHttpCloseHandle(req);
  }
  if (con) WinHttpCloseHandle(con);
  WinHttpCloseHandle(ses);
  return out;
#else
  if (url.compare(0, 8, "https://") == 0) {
    std::cerr << "Net list server: https needs Windows in this version: " << url << "\n";
    status = 0;
    return std::string();
  }
  return bb_net_http_plain_(post, url, body, timeout, status);
#endif
}

inline std::string bb_net_server_url_(const std::string& base, const std::string& query) {
  return base + (base.find('?') == std::string::npos ? "?" : "&") + query;
}

// Haelt den Eintrag beim Listen-Server: anmelden, alle 30 s ein Lebenszeichen.
// Endet, sobald regGen weiterzaehlt (StopNetGame, neue Anmeldung, Ende).
inline void bb_net_reg_run_(int gen, std::string url, std::string body) {
  auto& S = bb_net_;
  std::string key;
  int64_t next = 0;
  bool warned = false;
  while (!S.quit && S.regGen == gen) {
    const int64_t now = bb_net_now_();
    if (now >= next) {
      int status = 0, players = 0;
      {
        std::lock_guard<std::mutex> g(S.m);
        players = static_cast<int>(S.players.size());
      }
      std::string resp;
      if (key.empty()) {
        resp = bb_net_trim_(bb_net_http_(true, url, "a=host&" + body + "&players=" + std::to_string(players), 5000, status));
        if (status == 200 && resp.compare(0, 3, "ok ") == 0) {
          key = bb_net_trim_(resp.substr(3));
          std::lock_guard<std::mutex> g(S.m);
          if (S.regGen == gen) { S.regKey = key; S.regUrl = url; }
        } else if (!warned) {
          warned = true;
          std::cerr << "Net list server: registration failed (" << url << ", status " << status << "): "
                    << resp.substr(0, 80) << "\n";
        }
      } else {
        resp = bb_net_trim_(bb_net_http_(true, url, "a=beat&key=" + bb_net_urlenc_(key) + "&players=" +
                                                        std::to_string(players), 5000, status));
        if (resp.compare(0, 3, "err") == 0) key.clear();   // abgelaufen: neu anmelden
      }
      next = bb_net_now_() + (key.empty() ? 10000 : 30000);
    }
    bb_net_sleep_(50);
  }
}

// unter dem Mutex aufrufen
inline void bb_net_reg_start_() {
  auto& S = bb_net_;
  const int gen = ++S.regGen;
  S.regKey.clear();
  const std::string url = bb_net_server_();
  if (url.empty() || !S.isHost) return;
  const int port = S.sockH != BB_INVALID_SOCKET ? bb_net_sock_port_(S.sockH) : bb_net_sock_port_(S.sockC);
  const std::string body = "app=" + bb_net_urlenc_(bb_net_app_()) + "&name=" + bb_net_urlenc_(S.gameName) +
                           "&port=" + std::to_string(port) + "&v=" + std::to_string(BB_NET_VERSION) +
                           "&token=" + std::to_string(S.token);
  std::thread(bb_net_reg_run_, gen, url, body).detach();
}

// Meldet den Eintrag ab; aus dem Hauptthread, ohne Mutex.
inline void bb_net_reg_stop_() {
  std::string key, url;
  {
    std::lock_guard<std::mutex> g(bb_net_.m);
    ++bb_net_.regGen;
    key.swap(bb_net_.regKey);
    url = bb_net_.regUrl;
  }
  if (key.empty()) return;
  int status = 0;
  bb_net_http_(true, url, "a=bye&key=" + bb_net_urlenc_(key), 1500, status);
}

inline std::vector<bb_NetOffer_> bb_net_server_list_(int timeout) {
  std::vector<bb_NetOffer_> out;
  const std::string base = bb_net_server_();
  if (base.empty()) return out;
  int status = 0;
  const std::string url = bb_net_server_url_(base, "a=list&app=" + bb_net_urlenc_(bb_net_app_()) +
                                                       "&v=" + std::to_string(BB_NET_VERSION));
  const std::string resp = bb_net_http_(false, url, "", std::max(timeout, 1000), status);
  if (status != 200) {
    std::cerr << "Net list server: no list from " << base << " (status " << status << ")\n";
    return out;
  }
  size_t pos = 0;
  while (pos < resp.size()) {
    size_t e = resp.find('\n', pos);
    if (e == std::string::npos) e = resp.size();
    std::string line = resp.substr(pos, e - pos);
    pos = e + 1;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::vector<std::string> f;
    size_t a = 0;
    for (;;) {
      const size_t t = line.find('\t', a);
      f.push_back(line.substr(a, t == std::string::npos ? std::string::npos : t - a));
      if (t == std::string::npos) break;
      a = t + 1;
    }
    if (f.size() < 4) continue;
    in_addr ia{};
    if (inet_pton(AF_INET, f[1].c_str(), &ia) != 1) continue;
    bb_NetOffer_ o;
    o.name = f[0];
    o.addr = bb_net_addr_(ntohl(ia.s_addr), std::atoi(f[2].c_str()));
    o.players = std::atoi(f[3].c_str());
    o.token = f.size() > 4 ? static_cast<uint32_t>(std::strtoul(f[4].c_str(), nullptr, 10)) : 0;
    o.lan = 0;
    out.push_back(o);
  }
  return out;
}

// ============================================================
// UPnP: der Router leitet den Spielport weiter
// ============================================================
//
// Beim Hosten bittet ein eigener Thread den Router (Internet Gateway Device)
// um eine Weiterleitung des UDP-Spielports auf diesen Rechner: SSDP-Suche im
// LAN, Geraetebeschreibung holen, AddPortMapping per SOAP. Die Leihfrist ist
// eine Stunde und wird nach der halben Zeit erneuert; Router, die nur
// dauerhafte Weiterleitungen kennen (Fehler 725), bekommen eine dauerhafte.
// StopNetGame gibt die Weiterleitung mit DeletePortMapping zurueck.

inline constexpr int BB_NET_LEASE = 3600;

inline bool bb_net_resolve_(const std::string& text, sockaddr_in& out);

// Inhalt des ersten <tag>...</tag> ab from, auch mit Namensraum (<u:tag>).
inline std::string bb_net_xml_(const std::string& x, const std::string& tag, size_t from = 0) {
  size_t p = from;
  while ((p = x.find(tag + ">", p)) != std::string::npos) {
    if (p > 0 && (x[p - 1] == '<' || x[p - 1] == ':')) {
      size_t lt = p - 1;
      while (lt > 0 && x[lt] != '<') --lt;
      if (x[lt] == '<' && x[lt + 1] != '/') {
        const size_t a = p + tag.size() + 1, e = x.find('<', a);
        return bb_net_trim_(x.substr(a, e == std::string::npos ? std::string::npos : e - a));
      }
    }
    p += tag.size();
  }
  return std::string();
}

inline std::string bb_net_xml_esc_(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else o += c;
  }
  return o;
}

// SSDP-Suche nach dem Router; die LOCATION-Adressen der Antworten.
inline std::vector<std::string> bb_net_ssdp_(int timeout) {
  std::vector<std::string> out;
  bb_socket_t s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == BB_INVALID_SOCKET) return out;
#ifdef _WIN32
  DWORD tv = 100;
#else
  timeval tv{0, 100000};
#endif
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
  const sockaddr_in to = bb_net_addr_(0xEFFFFFFAu, 1900);   // 239.255.255.250
  static const char* types[] = {"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
                                "urn:schemas-upnp-org:device:InternetGatewayDevice:2",
                                "urn:schemas-upnp-org:service:WANIPConnection:1",
                                "urn:schemas-upnp-org:service:WANPPPConnection:1"};
  for (const char* st : types) {
    const std::string q = std::string("M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\n"
                                      "MAN: \"ssdp:discover\"\r\nMX: 2\r\nST: ") + st + "\r\n\r\n";
    bb_net_sendto_(s, to, q);
  }
  const int64_t start = bb_net_now_();
  int64_t end = start + timeout;
  char b[2048];
  while (bb_net_now_() < end) {
    const int n = recv(s, b, sizeof(b) - 1, 0);
    if (n <= 0) continue;
    const std::string r(b, n), low = bb_net_lower_(r);
    const size_t l = low.find("\nlocation:");
    if (l == std::string::npos) continue;
    const size_t e = r.find('\r', l + 10);
    const std::string loc = bb_net_trim_(r.substr(l + 10, e == std::string::npos ? std::string::npos : e - l - 10));
    if (std::find(out.begin(), out.end(), loc) == out.end()) out.push_back(loc);
    end = std::min(end, bb_net_now_() + 300);   // weitere Antworten kommen fast gleichzeitig
  }
  closesocket(s);
  return out;
}

struct bb_NetIgd_ { std::string ctl, svc; };

// Geraetebeschreibung lesen: Steuer-URL des Dienstes WANIPConnection (vor WANPPPConnection).
inline bool bb_net_igd_(const std::string& loc, bb_NetIgd_& out) {
  int status = 0;
  const std::string x = bb_net_http_plain_(false, loc, "", 3000, status);
  if (status != 200) return false;
  std::string hostport, host, port, path;
  std::string base = bb_net_xml_(x, "URLBase");
  if (!bb_net_url_split_(base, hostport, host, port, path) && !bb_net_url_split_(loc, hostport, host, port, path))
    return false;
  size_t p = 0;
  while ((p = x.find("<serviceType>", p)) != std::string::npos) {
    const std::string type = bb_net_xml_(x, "serviceType", p);
    const size_t next = x.find("<serviceType>", p + 13);
    std::string ctl = bb_net_xml_(x.substr(0, next), "controlURL", p);
    p += 13;
    const bool ip = type.find(":WANIPConnection:") != std::string::npos;
    if (ctl.empty() || (!ip && type.find(":WANPPPConnection:") == std::string::npos)) continue;
    if (!ip && !out.ctl.empty()) continue;
    if (ctl.compare(0, 7, "http://") != 0) ctl = "http://" + hostport + (ctl[0] == '/' ? "" : "/") + ctl;
    out.ctl = ctl;
    out.svc = type;
    if (ip) break;
  }
  return !out.ctl.empty();
}

inline std::string bb_net_soap_(const bb_NetIgd_& d, const std::string& action, const std::string& args,
                                int timeout, int& status) {
  const std::string body =
      "<?xml version=\"1.0\"?>\r\n<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
      "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:" + action + " xmlns:u=\"" + d.svc +
      "\">" + args + "</u:" + action + "></s:Body></s:Envelope>\r\n";
  return bb_net_http_plain_(true, d.ctl, body, timeout, status, "text/xml; charset=\"utf-8\"",
                            "SOAPAction: \"" + d.svc + "#" + action + "\"\r\n");
}

// Eigene Adresse, ueber die dieser Rechner den Router erreicht.
inline std::string bb_net_local_ip_for_(const std::string& url) {
  std::string hostport, host, port, path;
  sockaddr_in a{};
  if (!bb_net_url_split_(url, hostport, host, port, path) || !bb_net_resolve_(host + ":" + port, a)) return "";
  bb_socket_t s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == BB_INVALID_SOCKET) return "";
  sockaddr_in me{};
  socklen_t n = sizeof(me);
  const bool ok = connect(s, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0 &&
                  getsockname(s, reinterpret_cast<sockaddr*>(&me), &n) == 0;
  closesocket(s);
  return ok ? bb_net_ip_text_(me) : "";
}

// 0 bei Erfolg, sonst der UPnP-Fehlercode (-1 ohne Antwort).
inline int bb_net_upnp_add_(const bb_NetIgd_& d, int port, const std::string& ip, const std::string& desc, int lease) {
  const std::string p = std::to_string(port);
  int status = 0;
  const std::string r = bb_net_soap_(d, "AddPortMapping",
      "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + p + "</NewExternalPort><NewProtocol>UDP</NewProtocol>"
      "<NewInternalPort>" + p + "</NewInternalPort><NewInternalClient>" + ip + "</NewInternalClient>"
      "<NewEnabled>1</NewEnabled><NewPortMappingDescription>" + bb_net_xml_esc_(desc) +
      "</NewPortMappingDescription><NewLeaseDuration>" + std::to_string(lease) + "</NewLeaseDuration>",
      3000, status);
  if (status == 200) return 0;
  const std::string code = bb_net_xml_(r, "errorCode");
  return code.empty() ? -1 : std::atoi(code.c_str());
}

inline void bb_net_upnp_del_(const bb_NetIgd_& d, int port, int timeout) {
  int status = 0;
  bb_net_soap_(d, "DeletePortMapping",
               "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + std::to_string(port) +
                   "</NewExternalPort><NewProtocol>UDP</NewProtocol>",
               timeout, status);
}

// Private Bereiche und Carrier-NAT (100.64.0.0/10): von dort ist der Router selbst nicht erreichbar.
inline bool bb_net_ip_public_(const std::string& text) {
  in_addr a{};
  if (inet_pton(AF_INET, text.c_str(), &a) != 1) return true;   // unbekannt: nicht warnen
  const uint32_t ip = ntohl(a.s_addr);
  return !((ip >> 24) == 10 || (ip >> 24) == 127 || (ip >> 24) == 0 || (ip >> 20) == 0xAC1 ||
           (ip >> 16) == 0xC0A8 || (ip >> 22) == (0x6440 >> 6) || (ip >> 16) == 0xA9FE);
}

inline void bb_net_upnp_run_(int gen, int port, std::string fixed, std::string desc) {
  auto& S = bb_net_;
  auto alive = [&] { return !S.quit && S.upnpGen == gen; };
  auto fail = [&](const std::string& why) {
    std::lock_guard<std::mutex> g(S.m);
    if (S.upnpGen != gen) return;
    S.upnpState = 0;
    std::cerr << "Net UPnP: " << why << "; to host over the Internet, forward UDP port " << port
              << " to this computer by hand\n";
  };
  std::vector<std::string> locs;
  if (!fixed.empty()) locs.push_back(fixed);
  else locs = bb_net_ssdp_(2500);
  bb_NetIgd_ d;
  bool found = false;
  for (auto& l : locs)
    if (alive() && (found = bb_net_igd_(l, d))) break;
  if (!alive()) return;
  if (!found) {
    fail(locs.empty() ? "no router answered" : "the router offers no port forwarding");
    return;
  }
  const std::string ip = bb_net_local_ip_for_(d.ctl);
  int lease = BB_NET_LEASE;
  int err = bb_net_upnp_add_(d, port, ip, desc, lease);
  if (err == 725) err = bb_net_upnp_add_(d, port, ip, desc, lease = 0);
  if (err != 0) {
    if (err == 718) fail("the router already forwards UDP port " + std::to_string(port) + " to another computer");
    else if (err == 606 || err == 401) fail("the router does not allow programs to change its settings (UPnP)");
    else if (err < 0) fail("the router did not answer");
    else fail("the router refused the forwarding (error " + std::to_string(err) + ")");
    return;
  }
  int status = 0;
  const std::string ext = bb_net_xml_(bb_net_soap_(d, "GetExternalIPAddress", "", 3000, status), "NewExternalIPAddress");
  const bool pub = ext.empty() || bb_net_ip_public_(ext);
  {
    std::lock_guard<std::mutex> g(S.m);
    if (S.upnpGen == gen) {
      S.upnpState = pub ? 1 : 0;
      S.upnpCtl = d.ctl;
      S.upnpSvc = d.svc;
      S.upnpPort = port;
    }
  }
  if (!alive()) { bb_net_upnp_del_(d, port, 1500); return; }   // StopNetGame kam waehrenddessen
  if (!pub)
    std::cerr << "Net UPnP: the router forwards UDP port " << port << ", but its own address " << ext
              << " is not public (carrier NAT); others cannot join over the Internet\n";
  int64_t next = lease ? bb_net_now_() + lease * 500LL : INT64_MAX;
  while (alive()) {
    if (bb_net_now_() >= next) {
      bb_net_upnp_add_(d, port, ip, desc, lease);
      if (!alive()) bb_net_upnp_del_(d, port, 1500);
      next = bb_net_now_() + lease * 500LL;
    }
    bb_net_sleep_(50);
  }
}

inline bool bb_net_upnp_on_() {
  auto& S = bb_net_;
  if (!S.upnpUrl.empty()) return true;
  if ((S.extUpnp >= 0 ? S.extUpnp : S.codeUpnp) == 0) return false;
  return (S.bindIp >> 24) != 127;   // Tests an 127.0.0.1: kein Router im Spiel
}

// unter dem Mutex aufrufen; beim Hosten und nach einer Migration
inline void bb_net_upnp_start_() {
  auto& S = bb_net_;
  const int gen = ++S.upnpGen;
  S.upnpState = 0;
  S.upnpCtl.clear();
  if (!S.isHost || !bb_net_upnp_on_() || S.sockH == BB_INVALID_SOCKET) return;
  S.upnpState = -1;
  std::thread(bb_net_upnp_run_, gen, bb_net_sock_port_(S.sockH), S.upnpUrl, "BLTZNXT " + S.gameName).detach();
}

// Gibt die Weiterleitung zurueck; aus dem Hauptthread, ohne Mutex.
inline void bb_net_upnp_stop_() {
  bb_NetIgd_ d;
  int port = 0;
  {
    std::lock_guard<std::mutex> g(bb_net_.m);
    ++bb_net_.upnpGen;
    bb_net_.upnpState = 0;
    d.ctl.swap(bb_net_.upnpCtl);
    d.svc = bb_net_.upnpSvc;
    port = bb_net_.upnpPort;
  }
  if (!d.ctl.empty()) bb_net_upnp_del_(d, port, 1500);
}

// ============================================================
// Start, Ende
// ============================================================

inline void bb_net_quit_();

inline bool bb_net_init_() {
  auto& S = bb_net_;
  if (S.inited) return S.sockC != BB_INVALID_SOCKET;
  S.inited = true;
  if (!bb_sockets_ready_()) return false;
  bb_net_read_settings_();
  S.sockC = bb_net_open_socket_(0);
  if (S.sockC == BB_INVALID_SOCKET) {
    std::cerr << "Net: cannot open a UDP socket\n";
    return false;
  }
  std::thread(bb_net_thread_).detach();
  std::atexit(bb_net_quit_);
  return true;
}

inline void bb_net_clear_session_() {
  auto& S = bb_net_;
  S.started = false;
  S.isHost = S.lost = S.migrating = S.joining = false;
  S.myMachine = S.hostMachine = S.localCounter = 0;
  S.resumeDeadline = 0;
  S.token = 0;
  S.gameName.clear();
  S.peers.clear();
  S.machines.clear();
  S.players.clear();
  S.queue.clear();
  S.cur = bb_NetMsg_();
  S.pendingOut.clear();
  S.joinNonces.clear();
  if (S.sockH != BB_INVALID_SOCKET) { closesocket(S.sockH); S.sockH = BB_INVALID_SOCKET; }
}

// Verlaesst das Spiel: offene zuverlaessige Nachrichten bis zu flush_ms noch
// zustellen, dann BYE an alle.
inline void bb_net_stop_(int flush_ms) {
  auto& S = bb_net_;
  const int64_t end = bb_net_now_() + flush_ms;
  while (bb_net_now_() < end) {
    {
      std::lock_guard<std::mutex> g(S.m);
      bool idle = true;
      for (auto& [mid, p] : S.peers)
        if (!p.unacked.empty() || !p.waiting.empty()) idle = false;
      if (idle) break;
    }
    bb_net_sleep_(5);
  }
  {
    std::lock_guard<std::mutex> g(S.m);
    const std::string bye = bb_net_pkt_(BB_NK_BYE).b;
    for (int i = 0; i < 3; ++i)
      for (auto& [mid, p] : S.peers) bb_net_sendto_(p.sock, p.addr, bye);
    bb_net_clear_session_();
  }
  bb_net_reg_stop_();
  bb_net_upnp_stop_();
}

inline void bb_net_quit_() {
  auto& S = bb_net_;
  if (!S.inited || S.quit) return;
  if (S.started) bb_net_stop_(300);
  S.quit = true;
  for (int i = 0; i < 100 && !S.threadDone; ++i) bb_net_sleep_(2);
}

inline void bb_net_need_game_() {
  bb_net_init_();
  if (!bb_net_.started) bb_RuntimeError("Multiplayer game not started");
}
inline void bb_net_need_no_game_() {
  bb_net_init_();
  if (bb_net_.started) bb_RuntimeError("Multiplayer game already started");
}

// "host[:port]" aufloesen; false, wenn es den Rechner nicht gibt.
inline bool bb_net_resolve_(const std::string& text, sockaddr_in& out) {
  std::string host = bb_net_trim_(text);
  int port = bb_net_port_();
  const size_t colon = host.rfind(':');
  if (colon != std::string::npos) {
    port = std::atoi(host.c_str() + colon + 1);
    host = host.substr(0, colon);
  }
  addrinfo hints{}, *res = nullptr;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || !res) return false;
  out = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
  out.sin_port = htons(static_cast<uint16_t>(port));
  freeaddrinfo(res);
  return true;
}

// Sucht im LAN und beim Listen-Server; Spiele im LAN zuerst, ohne Doppelte
// und ohne das eigene.
inline std::vector<bb_NetOffer_> bb_net_find_(int timeout) {
  auto& S = bb_net_;
  timeout = std::max(0, timeout);
  const int64_t end = bb_net_now_() + timeout;
  {
    std::lock_guard<std::mutex> g(S.m);
    S.offers.clear();
    S.collecting = S.lan != 0;
    S.discLast = 0;
  }
  std::vector<bb_NetOffer_> fromServer = bb_net_server_list_(timeout);
  while (bb_net_now_() < end) bb_net_sleep_(5);
  std::vector<bb_NetOffer_> out;
  {
    std::lock_guard<std::mutex> g(S.m);
    S.collecting = false;
    out = S.offers;
    for (auto& o : fromServer) {
      if (S.started && o.token && o.token == S.token) continue;
      bool dup = false;
      for (auto& x : out)
        if ((o.token && x.token == o.token) || bb_net_same_addr_(x.addr, o.addr)) dup = true;
      if (!dup) out.push_back(o);
    }
  }
  return out;
}

// Beitritt bei addr; 1 beigetreten, 0 nicht (Grund auf stderr, wenn quiet = false).
inline int bb_net_join_(const sockaddr_in& addr, const std::string& name, bool quiet) {
  auto& S = bb_net_;
  {
    std::lock_guard<std::mutex> g(S.m);
    S.joining = true;
    S.joinAddr = addr;
    S.joinName = name;
    S.joinNonce = S.rng() | 1u;
    S.joinResult = 0;
    S.joinReason.clear();
    S.joinLast = 0;
  }
  const int64_t end = bb_net_now_() + 3000;
  int result = 0;
  std::string reason;
  for (;;) {
    {
      std::lock_guard<std::mutex> g(S.m);
      if (S.joinResult != 0 || bb_net_now_() >= end) {
        result = S.joinResult;
        reason = S.joinReason;
        S.joining = false;
        break;
      }
    }
    bb_net_sleep_(5);
  }
  if (result == 1) return 1;
  if (!quiet) {
    if (result < 0) std::cerr << "JoinNetGame: " << reason << "\n";
    else std::cerr << "JoinNetGame: no answer from " << bb_net_ip_text_(addr) << ":" << ntohs(addr.sin_port) << "\n";
  }
  return 0;
}

inline int bb_net_host_(const std::string& name) {
  auto& S = bb_net_;
  std::lock_guard<std::mutex> g(S.m);
  const int port = bb_net_port_();
  S.sockH = bb_net_open_socket_(port);
  if (S.sockH == BB_INVALID_SOCKET) {
    std::cerr << "HostNetGame: port " << port << " is in use\n";
    return 0;
  }
  S.isHost = true;
  S.lost = false;
  S.myMachine = S.hostMachine = 1;
  S.nextMachine = 2;
  S.nextOrder = 1;
  S.localCounter = 0;
  S.token = S.rng() | 1u;
  S.gameName = name;
  S.machines.clear();
  S.machines[1] = bb_NetMachine_();
  S.players.clear();
  S.peers.clear();
  S.queue.clear();
  S.cur = bb_NetMsg_();
  S.started = true;
  bb_net_reg_start_();
  bb_net_upnp_start_();
  return 2;
}

inline std::string bb_net_default_name_() {
  std::string exe;
#ifdef _WIN32
  char buff[MAX_PATH + 1];
  exe.assign(buff, GetModuleFileNameA(nullptr, buff, MAX_PATH));
#else
  if (bb_argc_ > 0) exe = bb_argv_[0];
#endif
  const size_t sep = exe.find_last_of("\\/");
  if (sep != std::string::npos) exe = exe.substr(sep + 1);
  const size_t dot = exe.find_last_of('.');
  if (dot != std::string::npos) exe = exe.substr(0, dot);
  return exe.empty() ? std::string("game") : exe;
}

// ============================================================
// Die Befehle von Blitz3D
// ============================================================

// Ohne Dialog: kurz suchen, beitreten (1), sonst hosten (2).
inline int bb_StartNetGame() {
  bb_net_need_no_game_();
  if (bb_net_.sockC == BB_INVALID_SOCKET) return 0;
  if (!bb_net_.extJoin.empty()) {
    sockaddr_in a{};
    if (!bb_net_resolve_(bb_net_.extJoin, a)) {
      std::cerr << "StartNetGame: unknown host '" << bb_net_.extJoin << "'\n";
      return 0;
    }
    return bb_net_join_(a, "", false);
  }
  for (int round = 0; round < 2; ++round) {
    if (round == 1) {
      // Zufaellige Pause: starten zwei Rechner gleichzeitig, hostet nur einer.
      int pause;
      {
        std::lock_guard<std::mutex> g(bb_net_.m);
        pause = static_cast<int>(bb_net_.rng() % 501);
      }
      bb_net_sleep_(pause);
    }
    for (auto& o : bb_net_find_(round == 0 ? 1000 : 500))
      if (bb_net_join_(o.addr, "", true)) return 1;
  }
  return bb_net_host_(bb_net_default_name_());
}

inline int bb_HostNetGame(const bbString& game_name) {
  bb_net_need_no_game_();
  if (bb_net_.sockC == BB_INVALID_SOCKET) return 0;
  return bb_net_host_(game_name);
}

// Leere IP oder 0.0.0.0: im LAN (und beim Listen-Server) nach dem Spiel suchen.
// Ein leerer Name passt auf jedes Spiel.
inline int bb_JoinNetGame(const bbString& game_name, const bbString& ip_address) {
  bb_net_need_no_game_();
  if (bb_net_.sockC == BB_INVALID_SOCKET) return 0;
  const std::string ip = bb_net_trim_(ip_address);
  if (ip.empty() || ip == "0.0.0.0") {
    const std::string want = bb_net_lower_(game_name);
    bool any = false;
    for (auto& o : bb_net_find_(1000)) {
      if (!want.empty() && bb_net_lower_(o.name) != want) continue;
      any = true;
      if (bb_net_join_(o.addr, game_name, true)) return 1;
    }
    std::cerr << "JoinNetGame: " << (any ? "could not join" : "found no") << " game '" << game_name << "'\n";
    return 0;
  }
  sockaddr_in a{};
  if (!bb_net_resolve_(ip, a)) {
    std::cerr << "JoinNetGame: unknown host '" << ip << "'\n";
    return 0;
  }
  return bb_net_join_(a, game_name, false);
}

inline void bb_StopNetGame() {
  bb_net_init_();
  if (bb_net_.started) bb_net_stop_(500);
}

inline int bb_CreateNetPlayer(const bbString& name) {
  bb_net_need_game_();
  auto& S = bb_net_;
  std::lock_guard<std::mutex> g(S.m);
  if (S.lost || S.localCounter >= 0xFFFF) return 0;
  const int id = (S.myMachine << 16) | ++S.localCounter;
  bb_NetPlayer_& pl = S.players[id];
  pl.name = name;
  pl.machine = S.myMachine;
  const std::string payload = bb_net_padd_payload_(id, pl);
  if (S.isHost) for (auto& [mid, p] : S.peers) bb_net_send_rel_(p, payload);
  else if (S.migrating) S.pendingOut.push_back(payload);
  else if (S.peers.count(S.hostMachine)) bb_net_send_rel_(S.peers[S.hostMachine], payload);
  return id;
}

inline void bb_DeleteNetPlayer(int player) {
  bb_net_init_();
  auto& S = bb_net_;
  if (!S.started) return;
  std::lock_guard<std::mutex> g(S.m);
  const auto it = S.players.find(player);
  if (it == S.players.end() || it->second.machine != S.myMachine) return;
  S.players.erase(it);
  const std::string payload = bb_net_id_payload_(BB_NS_PDEL, player);
  if (S.isHost) for (auto& [mid, p] : S.peers) bb_net_send_rel_(p, payload);
  else if (S.migrating) S.pendingOut.push_back(payload);
  else if (S.peers.count(S.hostMachine)) bb_net_send_rel_(S.peers[S.hostMachine], payload);
}

inline bbString bb_NetPlayerName(int player) {
  if (player == 0) return "<all>";
  bb_net_init_();
  std::lock_guard<std::mutex> g(bb_net_.m);
  const auto it = bb_net_.players.find(player);
  return it == bb_net_.players.end() ? bbString("<unknown>") : it->second.name;
}

inline int bb_NetPlayerLocal(int player) {
  bb_net_init_();
  std::lock_guard<std::mutex> g(bb_net_.m);
  const auto it = bb_net_.players.find(player);
  return bb_net_.started && it != bb_net_.players.end() && it->second.machine == bb_net_.myMachine;
}

// to_player = 0: an alle anderen Programme. Zuverlaessig ist die Vorgabe.
inline void bb_SendNetMsg(int type, const bbString& msg, int from_player, int to_player = 0, int reliable = 1) {
  bb_net_need_game_();
  auto& S = bb_net_;
  if (type == 0) return;   // 0 hiesse "keine Nachricht"
  std::lock_guard<std::mutex> g(S.m);
  if (S.lost) return;
  bb_NetMsg_ m;
  m.type = type;
  m.from = from_player;
  m.to = to_player;
  m.data = msg;
  if (to_player != 0) {
    const auto it = S.players.find(to_player);
    if (it == S.players.end()) return;
    if (it->second.machine == S.myMachine) { S.queue.push_back(std::move(m)); return; }
  }
  const std::string payload = bb_net_msg_payload_(m);
  if (S.isHost) bb_net_route_(S.myMachine, m, payload, reliable != 0);
  else if (S.migrating) { if (reliable) S.pendingOut.push_back(payload); }
  else if (S.peers.count(S.hostMachine)) bb_net_send_(S.peers[S.hostMachine], payload, reliable != 0);
}

inline int bb_RecvNetMsg() {
  bb_net_need_game_();
  std::lock_guard<std::mutex> g(bb_net_.m);
  if (bb_net_.queue.empty()) return 0;
  bb_net_.cur = std::move(bb_net_.queue.front());
  bb_net_.queue.pop_front();
  return 1;
}

inline int bb_NetMsgType() { std::lock_guard<std::mutex> g(bb_net_.m); return bb_net_.cur.type; }
inline int bb_NetMsgFrom() { std::lock_guard<std::mutex> g(bb_net_.m); return bb_net_.cur.from; }
inline int bb_NetMsgTo()   { std::lock_guard<std::mutex> g(bb_net_.m); return bb_net_.cur.to; }
inline bbString bb_NetMsgData() { std::lock_guard<std::mutex> g(bb_net_.m); return bb_net_.cur.data; }

// ============================================================
// Spielsuche und Einstellungen (BLTZNXT)
// ============================================================

inline int bb_FindNetGames(int timeout = 1000) {
  if (!bb_net_init_()) return 0;
  std::vector<bb_NetOffer_> found = bb_net_find_(timeout);
  std::lock_guard<std::mutex> g(bb_net_.m);
  bb_net_.list = std::move(found);
  return static_cast<int>(bb_net_.list.size());
}

inline const bb_NetOffer_* bb_net_listed_(int index) {
  return (index >= 1 && index <= static_cast<int>(bb_net_.list.size())) ? &bb_net_.list[index - 1] : nullptr;
}

inline bbString bb_NetGameName(int index) {
  std::lock_guard<std::mutex> g(bb_net_.m);
  const bb_NetOffer_* o = bb_net_listed_(index);
  return o ? o->name : bbString();
}

inline bbString bb_NetGameIP(int index) {
  std::lock_guard<std::mutex> g(bb_net_.m);
  const bb_NetOffer_* o = bb_net_listed_(index);
  if (!o) return bbString();
  const int port = ntohs(o->addr.sin_port);
  return bb_net_ip_text_(o->addr) + (port == bb_net_port_() ? std::string() : ":" + std::to_string(port));
}

inline int bb_NetGamePlayers(int index) {
  std::lock_guard<std::mutex> g(bb_net_.m);
  const bb_NetOffer_* o = bb_net_listed_(index);
  return o ? o->players : 0;
}

inline int bb_NetGameLAN(int index) {
  std::lock_guard<std::mutex> g(bb_net_.m);
  const bb_NetOffer_* o = bb_net_listed_(index);
  return o ? o->lan : 0;
}

// 1, solange dieses Programm das Spiel hostet (auch nach einer Migration).
inline int bb_NetGameHost() {
  std::lock_guard<std::mutex> g(bb_net_.m);
  return bb_net_.started && bb_net_.isHost && !bb_net_.lost;
}

inline void bb_NetGameApp(const bbString& id) {
  bb_net_init_();
  std::lock_guard<std::mutex> g(bb_net_.m);
  bb_net_.codeApp = id;
}

inline void bb_NetGamePort(int port) {
  bb_net_init_();
  std::lock_guard<std::mutex> g(bb_net_.m);
  bb_net_.codePort = (port > 0 && port < 65536) ? port : 0;
}

inline void bb_NetGameServer(const bbString& url) {
  bb_net_init_();
  std::lock_guard<std::mutex> g(bb_net_.m);
  bb_net_.codeServer = bb_net_trim_(url);
}

// UPnP beim Hosten ein (Vorgabe) oder aus, etwa fuer reine LAN-Spiele.
inline void bb_NetGameUPnP(int enable) {
  bb_net_init_();
  std::lock_guard<std::mutex> g(bb_net_.m);
  bb_net_.codeUpnp = enable != 0;
}

// 1: der Router leitet den Spielport weiter, 0: nicht, -1: die Antwort steht noch aus.
inline int bb_NetGameForwarded() {
  std::lock_guard<std::mutex> g(bb_net_.m);
  return bb_net_.started && bb_net_.isHost ? bb_net_.upnpState : 0;
}

#endif // BB_NET_H
