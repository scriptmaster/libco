#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t server_thread;
static cothread_t client_thread;

static socket_t listen_fd = kInvalidSocket;
static socket_t server_fd = kInvalidSocket;
static socket_t client_fd = kInvalidSocket;
static unsigned short port = 0;
static int server_ready = 0;
static int client_phase = 0;
static int server_phase = 0;
static char server_buf[16];
static char client_buf[16];

static void server_entry() {
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT(listen_fd != kInvalidSocket, "server socket failed");
  TEST_ASSERT(set_reuseaddr(listen_fd) == 0, "set_reuseaddr failed");
  TEST_ASSERT(bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) == 0, "bind failed");
  TEST_ASSERT(listen(listen_fd, 2) == 0, "listen failed");

  sockaddr_in bound;
  socklen_t blen = sizeof(bound);
  TEST_ASSERT(getsockname(listen_fd, (sockaddr*)&bound, &blen) == 0, "getsockname failed");
  port = ntohs(bound.sin_port);
  server_ready = 1;
  co_switch(host_thread);

  sockaddr_in peer;
  socklen_t plen = sizeof(peer);
  server_fd = accept(listen_fd, (sockaddr*)&peer, &plen);
  TEST_ASSERT(server_fd != kInvalidSocket, "accept failed");
  int n = (int)recv(server_fd, server_buf, sizeof(server_buf), 0);
  TEST_ASSERT(n == 4 && std::memcmp(server_buf, "ping", 4) == 0, "server recv mismatch");
  TEST_ASSERT(send(server_fd, "pong", 4, 0) == 4, "server send failed");
  server_phase = 1;
  co_switch(host_thread);

  for(;;) co_switch(host_thread);
}

static void client_entry() {
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT(client_fd != kInvalidSocket, "client socket failed");
  TEST_ASSERT(connect(client_fd, (sockaddr*)&addr, sizeof(addr)) == 0, "connect failed");
  TEST_ASSERT(send(client_fd, "ping", 4, 0) == 4, "client send failed");
  client_phase = 1;
  co_switch(host_thread);

  int n = (int)recv(client_fd, client_buf, sizeof(client_buf), 0);
  TEST_ASSERT(n == 4 && std::memcmp(client_buf, "pong", 4) == 0, "client recv mismatch");
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
  TEST_ASSERT(server_ready && port != 0, "server not ready");
  co_switch(client_thread);
  TEST_ASSERT(client_phase == 1, "client phase mismatch");
  co_switch(server_thread);
  TEST_ASSERT(server_phase == 1, "server phase mismatch");
  co_switch(client_thread);
  TEST_ASSERT(client_phase == 2, "client response missing");

  close_socket(server_fd);
  close_socket(client_fd);
  close_socket(listen_fd);
  co_delete(server_thread);
  co_delete(client_thread);
  net_cleanup();
  std::puts("07_tcp_echo: ok");
  return 0;
}
