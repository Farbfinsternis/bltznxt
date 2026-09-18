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
// UDP (CreateUDPStream und Verwandte) und die DirectPlay-Befehle fehlen
// weiterhin.

#include "bb_file.h"
#include "bb_string.h"
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

inline const bool bb_sock_hooks_reg_ = [] {
  bb_sock_read_hook_  = bb_tcp_read_;
  bb_sock_write_hook_ = bb_tcp_write_;
  bb_sock_avail_hook_ = bb_tcp_avail_;
  bb_sock_eof_hook_   = bb_tcp_eof_;
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
}

#endif // BB_SOCKET_H
