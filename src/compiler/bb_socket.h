#ifndef BB_SOCKET_H
#define BB_SOCKET_H

// TCP-Streams und Namensaufloesung.
//
// Nach bbruntime/bbsockets.cpp. Ein TCP-Stream ist dort ein `bbStream` wie
// eine Datei: ReadLine, WriteLine, ReadAvail und Eof arbeiten darauf genauso.
// Bei uns haengen sie deshalb an denselben Handles (bb_file.h) und rufen
// ueber die Haken dort hierher.
//
// Lesen holt so lange nach, bis die verlangte Menge zusammen ist oder die
// Verbindung endet - mit TCPTimeouts laesst sich eine Schranke setzen, ohne
// sie wartet select unbegrenzt (read_timeout 0). Eof meldet 1, wenn die
// Gegenseite geschlossen hat, und -1 nach einem Fehler.
//
// UDP (CreateUDPStream und Verwandte) steht weiter unten; ein UDP-Stream ist
// ebenfalls ein Stream wie eine Datei. Die DirectPlay-Befehle fehlen weiterhin.

#include "bb_file.h"
#include "bb_string.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using bb_socket_t = SOCKET;
inline constexpr bb_socket_t BB_INVALID_SOCKET = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
using bb_socket_t = int;
inline constexpr bb_socket_t BB_INVALID_SOCKET = -1;
#define closesocket close
#define ioctlsocket ioctl
#endif

// ============================================================
// Zustand
// ============================================================

struct bb_TcpStream_ {
  bb_socket_t sock = BB_INVALID_SOCKET;
  int ip = 0, port = 0;
  int e = 0;            // 0 = offen, 1 = Gegenseite zu, -1 = Fehler
  int server = 0;       // Handle des TCPServers, falls angenommen
};

struct bb_TcpServer_ {
  bb_socket_t sock = BB_INVALID_SOCKET;
  int e = 0;
  std::vector<int> accepted;
};

inline std::unordered_map<int, bb_TcpStream_> bb_tcp_streams_;
inline std::unordered_map<int, bb_TcpServer_> bb_tcp_servers_;
inline int bb_tcp_read_timeout_   = 0;   // 0 = unbegrenzt warten
inline int bb_tcp_accept_timeout_ = 0;
inline std::vector<int> bb_host_ips_;

// Ein UDP-Stream hat zwei Puffer: was gelesen wird, ist die zuletzt empfangene
// Nachricht (RecvUDPMsg ersetzt sie, ein Fehlschlag laesst sie liegen), was
// geschrieben wird, die naechste zu sendende (SendUDPMsg leert sie).
struct bb_UdpStream_ {
  bb_socket_t sock = BB_INVALID_SOCKET;
  int port = 0;                   // Port beim Erstellen, 0 = frei; Vorgabe fuer SendUDPMsg
  int msg_ip = 0, msg_port = 0;   // Absender der letzten Nachricht (anfangs der eigene Port)
  std::vector<char> in;
  size_t in_pos = 0;
  std::vector<char> out;
};

inline std::unordered_map<int, bb_UdpStream_> bb_udp_streams_;
inline int bb_udp_timeout_ = 0;   // RecvUDPMsg wartet so lange auf eine Nachricht, 0 = gar nicht

inline bool bb_sockets_ready_() {
#ifdef _WIN32
  static bool ok = [] {
    WSADATA d;
    return WSAStartup(MAKEWORD(2, 2), &d) == 0;
  }();
  return ok;
#else
  return true;
#endif
}

inline bb_TcpStream_* bb_tcp_get_(int handle) {
  auto it = bb_tcp_streams_.find(handle);
  return (it != bb_tcp_streams_.end()) ? &it->second : nullptr;
}

// ============================================================
// Die Haken aus bb_file.h
// ============================================================

inline int bb_tcp_avail_(int handle) {
  bb_TcpStream_* p = bb_tcp_get_(handle);
  if (!p) return -1;
  unsigned long t = 0;
#ifdef _WIN32
  if (ioctlsocket(p->sock, FIONREAD, &t) != 0) { p->e = -1; return 0; }
#else
  if (ioctl(p->sock, FIONREAD, &t) != 0) { p->e = -1; return 0; }
#endif
  return static_cast<int>(t);
}

// Liest bis `size` Bytes, wie TCPStream::read: nachfassen, bis die Menge
// zusammen ist oder die Gegenseite schliesst.
inline int bb_tcp_read_(int handle, void* buf, int size) {
  bb_TcpStream_* p = bb_tcp_get_(handle);
  if (!p) return -1;              // kein Socket - die Datei uebernimmt
  if (p->e) return 0;
  char* b = static_cast<char*>(buf);
  char* l = b + size;
  const long long tout = bb_tcp_read_timeout_
                       ? static_cast<long long>(bb_MilliSecs()) + bb_tcp_read_timeout_
                       : 0;
  while (b < l) {
    long dt = 0;
    if (bb_tcp_read_timeout_) {
      dt = static_cast<long>(tout - bb_MilliSecs());
      if (dt < 0) dt = 0;
    }
    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(p->sock, &fd);
    timeval tv;
    tv.tv_sec  = dt / 1000;
    tv.tv_usec = (dt % 1000) * 1000;
    const int n = select(static_cast<int>(p->sock) + 1, &fd, nullptr, nullptr,
                         bb_tcp_read_timeout_ ? &tv : nullptr);
    if (n != 1) { p->e = -1; break; }
    const int r = static_cast<int>(recv(p->sock, b, static_cast<int>(l - b), 0));
    if (r == 0) { p->e = 1; break; }
    if (r < 0)  { p->e = -1; break; }
    b += r;
  }
  return static_cast<int>(b - static_cast<char*>(buf));
}

inline int bb_tcp_write_(int handle, const void* buf, int size) {
  bb_TcpStream_* p = bb_tcp_get_(handle);
  if (!p) return -1;
  if (p->e) return 0;
  const int n = static_cast<int>(send(p->sock, static_cast<const char*>(buf), size, 0));
  if (n < 0) { p->e = -1; return 0; }
  return n;
}

// TCPStream::eof - 0 offen, 1 zu, -1 Fehler
inline int bb_tcp_eof_(int handle) {
  bb_TcpStream_* p = bb_tcp_get_(handle);
  if (!p) return -1000;           // kein Socket
  if (p->e) return p->e;
  fd_set fd;
  FD_ZERO(&fd);
  FD_SET(p->sock, &fd);
  timeval tv{ 0, 0 };
  switch (select(static_cast<int>(p->sock) + 1, &fd, nullptr, nullptr, &tv)) {
    case 0: break;
    case 1: if (!bb_tcp_avail_(handle)) p->e = 1; break;
    default: p->e = -1;
  }
  return p->e;
}

// ---- UDP-Haken ----

inline bb_UdpStream_* bb_udp_find_(int handle) {
  auto it = bb_udp_streams_.find(handle);
  return (it != bb_udp_streams_.end()) ? &it->second : nullptr;
}

// Wie im Debug-Modus des Originals: "UDP Stream does not exist".
inline bb_UdpStream_& bb_udp_get_(int handle) {
  bb_UdpStream_* p = bb_udp_find_(handle);
  if (!p) bb_RuntimeError("UDP Stream does not exist");
  return *p;
}

inline int bb_udp_avail_(int handle) {
  bb_UdpStream_* p = bb_udp_find_(handle);
  if (!p) return -1;
  return static_cast<int>(p->in.size() - p->in_pos);
}

// Was da ist, bis zu size Bytes; der Rest der Nachricht bleibt. Hinter dem Ende
// der Nachricht liest man nichts (ReadInt liefert 0).
inline int bb_udp_read_(int handle, void* buf, int size) {
  bb_UdpStream_* p = bb_udp_find_(handle);
  if (!p) return -1;
  const int n = std::min<int>(size, static_cast<int>(p->in.size() - p->in_pos));
  if (n > 0) {
    std::memcpy(buf, p->in.data() + p->in_pos, static_cast<size_t>(n));
    p->in_pos += static_cast<size_t>(n);
    return n;
  }
  return 0;
}

inline int bb_udp_write_(int handle, const void* buf, int size) {
  bb_UdpStream_* p = bb_udp_find_(handle);
  if (!p) return -1;
  const char* c = static_cast<const char*>(buf);
  p->out.insert(p->out.end(), c, c + size);
  return size;
}

// Eof: 1, wenn von der Nachricht nichts mehr da ist (auch ohne jede Nachricht)
inline int bb_udp_eof_(int handle) {
  bb_UdpStream_* p = bb_udp_find_(handle);
  if (!p) return -1000;
  return p->in_pos >= p->in.size() ? 1 : 0;
}

// TCP zuerst, dann UDP; ein Handle, das beides nicht kennt, ist eine Datei.
inline int bb_sock_read_(int h, void* buf, int n) {
  const int r = bb_tcp_read_(h, buf, n);
  return r >= 0 ? r : bb_udp_read_(h, buf, n);
}
inline int bb_sock_write_(int h, const void* buf, int n) {
  const int r = bb_tcp_write_(h, buf, n);
  return r >= 0 ? r : bb_udp_write_(h, buf, n);
}
inline int bb_sock_avail_(int h) {
  const int r = bb_tcp_avail_(h);
  return r >= 0 ? r : bb_udp_avail_(h);
}
inline int bb_sock_eof_(int h) {
  const int r = bb_tcp_eof_(h);
  return r != -1000 ? r : bb_udp_eof_(h);
}
inline bool bb_sock_exists_(int h) {
  return bb_tcp_get_(h) != nullptr || bb_udp_find_(h) != nullptr;
}

inline const bool bb_sock_hooks_reg_ = [] {
  bb_sock_read_hook_   = bb_sock_read_;
  bb_sock_write_hook_  = bb_sock_write_;
  bb_sock_avail_hook_  = bb_sock_avail_;
  bb_sock_eof_hook_    = bb_sock_eof_;
  bb_sock_exists_hook_ = bb_sock_exists_;
  return true;
}();

// ============================================================
// Namen und Adressen
// ============================================================

// DottedIP schreibt die vier Bytes von oben nach unten (bbDottedIP).
inline bbString bb_DottedIP(int ip) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
                (ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);
  return bbString(buf);
}

inline int bb_CountHostIPs(const bbString& host_name) {
  bb_host_ips_.clear();
  if (!bb_sockets_ready_()) return 0;
  addrinfo hints{};
  hints.ai_family = AF_INET;
  addrinfo* res = nullptr;
  if (getaddrinfo(host_name.c_str(), nullptr, &hints, &res) != 0 || !res) return 0;
  for (addrinfo* p = res; p; p = p->ai_next) {
    if (p->ai_family != AF_INET) continue;
    const auto* a = reinterpret_cast<const sockaddr_in*>(p->ai_addr);
    bb_host_ips_.push_back(static_cast<int>(ntohl(a->sin_addr.s_addr)));
  }
  freeaddrinfo(res);
  return static_cast<int>(bb_host_ips_.size());
}

// 1-basiert, wie bbHostIP.
inline int bb_HostIP(int host_index) {
  if (host_index < 1 || host_index > static_cast<int>(bb_host_ips_.size())) return 0;
  return bb_host_ips_[host_index - 1];
}

// ============================================================
// TCP
// ============================================================

static inline int bb_find_host_ip_(const bbString& name) {
  const unsigned long direct = inet_addr(name.c_str());
  if (direct != INADDR_NONE) return static_cast<int>(direct);   // Netzreihenfolge
  addrinfo hints{};
  hints.ai_family = AF_INET;
  addrinfo* res = nullptr;
  if (getaddrinfo(name.c_str(), nullptr, &hints, &res) != 0 || !res) return -1;
  int ip = 0;
  for (addrinfo* p = res; p; p = p->ai_next) {
    if (p->ai_family != AF_INET) continue;
    ip = static_cast<int>(reinterpret_cast<const sockaddr_in*>(p->ai_addr)->sin_addr.s_addr);
    break;
  }
  freeaddrinfo(res);
  return ip;
}

static inline int bb_tcp_register_(bb_socket_t s, int server_handle) {
  bb_TcpStream_ st;
  st.sock   = s;
  st.server = server_handle;
  sockaddr_in addr{};
#ifdef _WIN32
  int len = sizeof(addr);
#else
  socklen_t len = sizeof(addr);
#endif
  if (getpeername(s, reinterpret_cast<sockaddr*>(&addr), &len) == 0) {
    st.ip   = static_cast<int>(ntohl(addr.sin_addr.s_addr));
    st.port = ntohs(addr.sin_port);
  }
  const int h = bb_file_next_id_++;
  bb_tcp_streams_[h] = st;
  return h;
}

inline int bb_OpenTCPStream(const bbString& server, int server_port, int local_port = 0) {
  if (!bb_sockets_ready_()) return 0;
  const int ip = bb_find_host_ip_(server);
  if (ip == -1) return 0;
  bb_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == BB_INVALID_SOCKET) return 0;
  if (local_port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<unsigned short>(local_port));
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
      closesocket(s);
      return 0;
    }
  }
  sockaddr_in addr{};
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(static_cast<unsigned short>(server_port));
  addr.sin_addr.s_addr = static_cast<unsigned long>(ip);
  if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    closesocket(s);
    return 0;
  }
  return bb_tcp_register_(s, 0);
}

inline void bb_CloseTCPStream(int tcp_stream) {
  auto it = bb_tcp_streams_.find(tcp_stream);
  if (it == bb_tcp_streams_.end()) return;
  if (it->second.server) {
    auto sv = bb_tcp_servers_.find(it->second.server);
    if (sv != bb_tcp_servers_.end()) {
      auto& a = sv->second.accepted;
      a.erase(std::remove(a.begin(), a.end(), tcp_stream), a.end());
    }
  }
  closesocket(it->second.sock);
  bb_tcp_streams_.erase(it);
}

inline int bb_CreateTCPServer(int port) {
  if (!bb_sockets_ready_()) return 0;
  bb_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == BB_INVALID_SOCKET) return 0;
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(static_cast<unsigned short>(port));
  if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
      listen(s, SOMAXCONN) != 0) {
    closesocket(s);
    return 0;
  }
  bb_TcpServer_ sv;
  sv.sock = s;
  const int h = bb_file_next_id_++;
  bb_tcp_servers_[h] = sv;
  return h;
}

inline void bb_CloseTCPServer(int tcp_server) {
  auto it = bb_tcp_servers_.find(tcp_server);
  if (it == bb_tcp_servers_.end()) return;
  // Der Server nimmt seine angenommenen Streams mit (TCPServer::~TCPServer).
  const std::vector<int> kids = it->second.accepted;
  for (int k : kids) bb_CloseTCPStream(k);
  closesocket(it->second.sock);
  bb_tcp_servers_.erase(it);
}

inline int bb_AcceptTCPStream(int tcp_server) {
  auto it = bb_tcp_servers_.find(tcp_server);
  if (it == bb_tcp_servers_.end() || it->second.e) return 0;
  fd_set fd;
  FD_ZERO(&fd);
  FD_SET(it->second.sock, &fd);
  timeval tv;
  tv.tv_sec  = bb_tcp_accept_timeout_ / 1000;
  tv.tv_usec = (bb_tcp_accept_timeout_ % 1000) * 1000;
  const int n = select(static_cast<int>(it->second.sock) + 1, &fd, nullptr, nullptr, &tv);
  if (n == 0) return 0;
  if (n != 1) { it->second.e = -1; return 0; }
  bb_socket_t t = accept(it->second.sock, nullptr, nullptr);
  if (t == BB_INVALID_SOCKET) { it->second.e = -1; return 0; }
  const int h = bb_tcp_register_(t, tcp_server);
  bb_tcp_servers_[tcp_server].accepted.push_back(h);
  return h;
}

inline int bb_TCPStreamIP(int tcp_stream) {
  bb_TcpStream_* p = bb_tcp_get_(tcp_stream);
  return p ? p->ip : 0;
}

inline int bb_TCPStreamPort(int tcp_stream) {
  bb_TcpStream_* p = bb_tcp_get_(tcp_stream);
  return p ? p->port : 0;
}

inline void bb_TCPTimeouts(int read_millis, int accept_millis) {
  bb_tcp_read_timeout_   = read_millis;
  bb_tcp_accept_timeout_ = accept_millis;
}

// ============================================================
// UDP
// ============================================================
//
// Nach bbsockets.cpp, am Original gemessen (tests/test_udp_streams.bb). Ein
// Stream ist an einen Port gebunden (0 = ein freier) und schickt und empfaengt
// ganze Nachrichten: alles, was man schreibt, geht mit SendUDPMsg als ein
// Datagramm hinaus; RecvUDPMsg holt das naechste herein.
//
// Was das Original so macht und hier uebernommen ist: Ohne Zielport nimmt
// SendUDPMsg den Port, mit dem der Stream erstellt wurde - bei einem freien
// Port ist das 0, und die Nachricht kommt nirgends an. Der Schreibpuffer ist
// nach dem Senden leer, und auch eine leere Nachricht wird gesendet und
// empfangen. Eine Nachricht hat bis 65507 Bytes Platz; mehr schickt das Netz
// nicht. UDPStreamIP liefert immer 0.
//
// Abweichung: Die Sockets duerfen an Broadcast-Adressen senden (SO_BROADCAST;
// das Original schickt dorthin nichts), damit ein Spiel im LAN nach Spielen
// rufen kann. Und unter Windows melden wir keine "Verbindung zurueckgesetzt"
// als Lesefehler, wenn eine Nachricht an einen geschlossenen Port ging
// (SIO_UDP_CONNRESET aus).

inline int bb_CreateUDPStream(int port = 0) {
  if (!bb_sockets_ready_()) return 0;
  bb_socket_t s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s == BB_INVALID_SOCKET) return 0;
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(static_cast<unsigned short>(port));
  if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    closesocket(s);
    return 0;
  }
  int yes = 1;
  setsockopt(s, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));
#ifdef _WIN32
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
  DWORD bytes = 0;
  BOOL off = FALSE;
  WSAIoctl(s, SIO_UDP_CONNRESET, &off, sizeof(off), nullptr, 0, &bytes, nullptr, nullptr);
#endif
  bb_UdpStream_ st;
  st.sock     = s;
  st.port     = port;
  st.msg_port = port;
  const int h = bb_file_next_id_++;
  bb_udp_streams_[h] = std::move(st);
  return h;
}

inline void bb_CloseUDPStream(int udp_stream) {
  bb_UdpStream_& p = bb_udp_get_(udp_stream);
  closesocket(p.sock);
  bb_udp_streams_.erase(udp_stream);
}

// Holt die naechste Nachricht in den Lesepuffer und liefert die IP des
// Absenders; ohne Nachricht (nach UDPTimeouts Millisekunden) 0, und der
// Lesepuffer bleibt, wie er war.
inline int bb_RecvUDPMsg(int udp_stream) {
  bb_UdpStream_& p = bb_udp_get_(udp_stream);
  fd_set fd;
  FD_ZERO(&fd);
  FD_SET(p.sock, &fd);
  timeval tv;
  tv.tv_sec  = bb_udp_timeout_ / 1000;
  tv.tv_usec = (bb_udp_timeout_ % 1000) * 1000;
  if (select(static_cast<int>(p.sock) + 1, &fd, nullptr, nullptr, &tv) != 1) return 0;
  static std::vector<char> buf(65536);
  sockaddr_in from{};
#ifdef _WIN32
  int len = sizeof(from);
#else
  socklen_t len = sizeof(from);
#endif
  const int n = static_cast<int>(recvfrom(p.sock, buf.data(), static_cast<int>(buf.size()), 0,
                                          reinterpret_cast<sockaddr*>(&from), &len));
  if (n < 0) return 0;
  p.in.assign(buf.begin(), buf.begin() + n);
  p.in_pos   = 0;
  p.msg_ip   = static_cast<int>(ntohl(from.sin_addr.s_addr));
  p.msg_port = ntohs(from.sin_port);
  return p.msg_ip;
}

// Schickt, was in den Stream geschrieben wurde, als eine Nachricht an
// dest_ip:dest_port (Vorgabe: der Port, mit dem der Stream erstellt wurde).
// Ob sie ankommt, erfaehrt man nicht.
inline void bb_SendUDPMsg(int udp_stream, int dest_ip, int dest_port = 0) {
  bb_UdpStream_& p = bb_udp_get_(udp_stream);
  sockaddr_in to{};
  to.sin_family      = AF_INET;
  to.sin_port        = htons(static_cast<unsigned short>(dest_port ? dest_port : p.port));
  to.sin_addr.s_addr = htonl(static_cast<unsigned long>(dest_ip));
  sendto(p.sock, p.out.data(), static_cast<int>(p.out.size()), 0,
         reinterpret_cast<sockaddr*>(&to), sizeof(to));
  p.out.clear();
}

inline int bb_UDPStreamIP(int udp_stream) {
  bb_udp_get_(udp_stream);
  return 0;
}

inline int bb_UDPStreamPort(int udp_stream) {
  bb_UdpStream_& p = bb_udp_get_(udp_stream);
  sockaddr_in a{};
#ifdef _WIN32
  int len = sizeof(a);
#else
  socklen_t len = sizeof(a);
#endif
  if (getsockname(p.sock, reinterpret_cast<sockaddr*>(&a), &len) != 0) return 0;
  return ntohs(a.sin_port);
}

inline int bb_UDPMsgIP(int udp_stream)   { return bb_udp_get_(udp_stream).msg_ip; }
inline int bb_UDPMsgPort(int udp_stream) { return bb_udp_get_(udp_stream).msg_port; }

inline void bb_UDPTimeouts(int recv_timeout) { bb_udp_timeout_ = recv_timeout; }

// ============================================================
// CopyStream
// ============================================================
//
// Liest aus src, bis sie leer ist, und schreibt nach dest. Das Original
// prueft bei jedem Durchgang auch Eof(dest) und kopiert deshalb in eine Datei
// nie etwas (der Zielstrom gilt nach dem Lesen davor als am Ende, der Puffer
// laesst sich nicht mehr beschreiben); in TCP- und UDP-Streams kopiert es, wenn
// sie nicht am Ende sind. Hier wird ohne diese Pruefung kopiert.

inline void bb_CopyStream(int src_stream, int dest_stream, int buffer_size = 1024) {
  if (!bb_stream_exists_(src_stream) || !bb_stream_exists_(dest_stream))
    bb_RuntimeError("Stream does not exist");
  if (buffer_size < 1 || buffer_size > 1024 * 1024) bb_RuntimeError("Illegal buffer size");
  std::vector<char> buf(static_cast<size_t>(buffer_size));
  while (!bb_Eof(src_stream)) {
    const int n = bb_stream_read_(src_stream, buf.data(), buffer_size);
    if (n > 0) bb_stream_write_(dest_stream, buf.data(), n);
    if (n < buffer_size) break;
  }
}

// Beim Programmende alles schliessen.
inline void bb_socket_quit_() {
  const std::vector<int> servers = [] {
    std::vector<int> v;
    for (auto& [h, s] : bb_tcp_servers_) v.push_back(h);
    return v;
  }();
  for (int h : servers) bb_CloseTCPServer(h);
  const std::vector<int> streams = [] {
    std::vector<int> v;
    for (auto& [h, s] : bb_tcp_streams_) v.push_back(h);
    return v;
  }();
  for (int h : streams) bb_CloseTCPStream(h);
  for (auto& [h, u] : bb_udp_streams_) closesocket(u.sock);
  bb_udp_streams_.clear();
}

#endif // BB_SOCKET_H
