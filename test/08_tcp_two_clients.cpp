#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t server_thread;
static cothread_t client1_thread;
static cothread_t client2_thread;

static socket_t listen_fd = kInvalidSocket;
static socket_t server_fd1 = kInvalidSocket;
static socket_t server_fd2 = kInvalidSocket;
static socket_t client_fd1 = kInvalidSocket;
static socket_t client_fd2 = kInvalidSocket;
static unsigned short port = 0;
static int ready = 0;
static int c1_phase = 0;
static int c2_phase = 0;
static int s_phase = 0;

static void server_entry() {
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT(listen_fd != kInvalidSocket, "listen socket failed");
  TEST_ASSERT(set_reuseaddr(listen_fd) == 0, "set_reuseaddr failed");
  TEST_ASSERT(bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) == 0, "bind failed");
  TEST_ASSERT(listen(listen_fd, 4) == 0, "listen failed");

  sockaddr_in bound;
  socklen_t blen = sizeof(bound);
  TEST_ASSERT(getsockname(listen_fd, (sockaddr*)&bound, &blen) == 0, "getsockname failed");
  port = ntohs(bound.sin_port);
  ready = 1;
  co_switch(host_thread);

  sockaddr_in peer;
  socklen_t plen = sizeof(peer);
  char buf[16];

  server_fd1 = accept(listen_fd, (sockaddr*)&peer, &plen);
  TEST_ASSERT(server_fd1 != kInvalidSocket, "accept client1 failed");
  int n1 = (int)recv(server_fd1, buf, sizeof(buf), 0);
  TEST_ASSERT(n1 == 2 && std::memcmp(buf, "c1", 2) == 0, "server recv c1 mismatch");
  TEST_ASSERT(send(server_fd1, "ok", 2, 0) == 2, "server send c1 failed");
  s_phase = 1;
  co_switch(host_thread);

  server_fd2 = accept(listen_fd, (sockaddr*)&peer, &plen);
  TEST_ASSERT(server_fd2 != kInvalidSocket, "accept client2 failed");
  int n2 = (int)recv(server_fd2, buf, sizeof(buf), 0);
  TEST_ASSERT(n2 == 2 && std::memcmp(buf, "c2", 2) == 0, "server recv c2 mismatch");
  TEST_ASSERT(send(server_fd2, "ok", 2, 0) == 2, "server send c2 failed");
  s_phase = 2;
  co_switch(host_thread);

  for(;;) co_switch(host_thread);
}

static void client1_entry() {
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  client_fd1 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT(client_fd1 != kInvalidSocket, "client1 socket failed");
  TEST_ASSERT(connect(client_fd1, (sockaddr*)&addr, sizeof(addr)) == 0, "client1 connect failed");
  TEST_ASSERT(send(client_fd1, "c1", 2, 0) == 2, "client1 send failed");
  c1_phase = 1;
  co_switch(host_thread);

  char buf[8];
  int n = (int)recv(client_fd1, buf, sizeof(buf), 0);
  TEST_ASSERT(n == 2 && std::memcmp(buf, "ok", 2) == 0, "client1 recv failed");
  c1_phase = 2;
  co_switch(host_thread);

  for(;;) co_switch(host_thread);
}

static void client2_entry() {
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  client_fd2 = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT(client_fd2 != kInvalidSocket, "client2 socket failed");
  TEST_ASSERT(connect(client_fd2, (sockaddr*)&addr, sizeof(addr)) == 0, "client2 connect failed");
  TEST_ASSERT(send(client_fd2, "c2", 2, 0) == 2, "client2 send failed");
  c2_phase = 1;
  co_switch(host_thread);

  char buf[8];
  int n = (int)recv(client_fd2, buf, sizeof(buf), 0);
  TEST_ASSERT(n == 2 && std::memcmp(buf, "ok", 2) == 0, "client2 recv failed");
  c2_phase = 2;
  co_switch(host_thread);

  for(;;) co_switch(host_thread);
}

int main() {
  TEST_ASSERT(net_init() == 0, "net_init failed");
  host_thread = co_active();
  server_thread = co_create(kStackSize, server_entry);
  client1_thread = co_create(kStackSize, client1_entry);
  client2_thread = co_create(kStackSize, client2_entry);
  TEST_ASSERT(server_thread && client1_thread && client2_thread, "co_create failed");

  co_switch(server_thread);
  TEST_ASSERT(ready, "server not ready");

  co_switch(client1_thread);
  co_switch(server_thread);
  co_switch(client1_thread);

  co_switch(client2_thread);
  co_switch(server_thread);
  co_switch(client2_thread);

  TEST_ASSERT(c1_phase == 2 && c2_phase == 2 && s_phase == 2, "phases incomplete");

  close_socket(server_fd1);
  close_socket(server_fd2);
  close_socket(client_fd1);
  close_socket(client_fd2);
  close_socket(listen_fd);
  co_delete(server_thread);
  co_delete(client1_thread);
  co_delete(client2_thread);
  net_cleanup();
  std::puts("08_tcp_two_clients: ok");
  return 0;
}
