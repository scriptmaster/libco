#ifndef LIBCO_TEST_COMMON_HPP
#define LIBCO_TEST_COMMON_HPP

#include "../libco.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
static const socket_t kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int socket_t;
static const socket_t kInvalidSocket = -1;
#endif

static const unsigned int kStackSize = 65536;

#define TEST_ASSERT(cond, msg) do { \
  if(!(cond)) { \
    std::fprintf(stderr, "ASSERT FAILED: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
    std::exit(1); \
  } \
} while(0)

static inline void close_socket(socket_t s) {
#ifdef _WIN32
  if(s != INVALID_SOCKET) closesocket(s);
#else
  if(s >= 0) close(s);
#endif
}

static inline int net_init() {
#ifdef _WIN32
  WSADATA wsa;
  return WSAStartup(MAKEWORD(2,2), &wsa);
#else
  return 0;
#endif
}

static inline void net_cleanup() {
#ifdef _WIN32
  WSACleanup();
#endif
}

static inline int socket_would_block() {
#ifdef _WIN32
  int e = WSAGetLastError();
  return e == WSAEWOULDBLOCK;
#else
  return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
}

static inline int set_reuseaddr(socket_t s) {
  int yes = 1;
  return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, (socklen_t)sizeof(yes));
}

#endif
