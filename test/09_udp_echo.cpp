#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t server_thread;
static cothread_t client_thread;

static socket_t server_fd = kInvalidSocket;
static socket_t client_fd = kInvalidSocket;
static unsigned short port = 0;
static int ready = 0;
static int server_phase = 0;
static int client_phase = 0;

static void server_entry() {
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT(server_fd != kInvalidSocket, "udp server socket failed");
  TEST_ASSERT(bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == 0, "udp bind failed");

  sockaddr_in bound;
  socklen_t blen = sizeof(bound);
  TEST_ASSERT(getsockname(server_fd, (sockaddr*)&bound, &blen) == 0, "udp getsockname failed");
  port = ntohs(bound.sin_port);
  ready = 1;
  co_switch(host_thread);

  sockaddr_in peer;
  socklen_t plen = sizeof(peer);
  char buf[16];
  int n = (int)recvfrom(server_fd, buf, sizeof(buf), 0, (sockaddr*)&peer, &plen);
  TEST_ASSERT(n == 4 && std::memcmp(buf, "ping", 4) == 0, "udp server recv mismatch");
  TEST_ASSERT(sendto(server_fd, "pong", 4, 0, (sockaddr*)&peer, plen) == 4, "udp server send failed");
  server_phase = 1;
  co_switch(host_thread);

  for(;;) co_switch(host_thread);
}

static void client_entry() {
  sockaddr_in to;
  std::memset(&to, 0, sizeof(to));
  to.sin_family = AF_INET;
  to.sin_port = htons(port);
  to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  client_fd = socket(AF_INET, SOCK_DGRAM, 0);
  TEST_ASSERT(client_fd != kInvalidSocket, "udp client socket failed");
  TEST_ASSERT(sendto(client_fd, "ping", 4, 0, (sockaddr*)&to, sizeof(to)) == 4, "udp client send failed");
  client_phase = 1;
  co_switch(host_thread);

  char buf[16];
  sockaddr_in from;
  socklen_t flen = sizeof(from);
  int n = (int)recvfrom(client_fd, buf, sizeof(buf), 0, (sockaddr*)&from, &flen);
  TEST_ASSERT(n == 4 && std::memcmp(buf, "pong", 4) == 0, "udp client recv mismatch");
  client_phase = 2;
  co_switch(host_thread);

  for(;;) co_switch(host_thread);
}

int main() {
  TEST_ASSERT(net_init() == 0, "net_init failed");
  host_thread = co_active();
  server_thread = co_create(kStackSize, server_entry);
  client_thread = co_create(kStackSize, client_entry);
  TEST_ASSERT(server_thread && client_thread, "co_create failed");

  co_switch(server_thread);
  TEST_ASSERT(ready, "udp server not ready");
  co_switch(client_thread);
  co_switch(server_thread);
  co_switch(client_thread);

  TEST_ASSERT(server_phase == 1 && client_phase == 2, "udp phases incomplete");

  close_socket(server_fd);
  close_socket(client_fd);
  co_delete(server_thread);
  co_delete(client_thread);
  net_cleanup();
  std::puts("09_udp_echo: ok");
  return 0;
}
