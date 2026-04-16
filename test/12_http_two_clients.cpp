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
static int done1 = 0;
static int done2 = 0;

static void handle_http(socket_t fd, const char* path) {
  char req[256];
  int n = (int)recv(fd, req, sizeof(req) - 1, 0);
  TEST_ASSERT(n > 0, "http recv request failed");
  req[n] = 0;
  TEST_ASSERT(std::strstr(req, path) != 0, "unexpected http path");

  const char* body = (std::strcmp(path, "GET /a HTTP/1.1") == 0) ? "A" : "B";
  char resp[256];
  std::snprintf(resp, sizeof(resp),
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 1\r\n"
    "Connection: close\r\n"
    "\r\n"
    "%s", body);
  TEST_ASSERT(send(fd, resp, (int)std::strlen(resp), 0) == (int)std::strlen(resp), "http send response failed");
}

static void server_entry() {
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  TEST_ASSERT(listen_fd != kInvalidSocket, "http listen socket failed");
  TEST_ASSERT(set_reuseaddr(listen_fd) == 0, "set_reuseaddr failed");
  TEST_ASSERT(bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) == 0, "http bind failed");
  TEST_ASSERT(listen(listen_fd, 4) == 0, "http listen failed");

  sockaddr_in bound;
  socklen_t blen = sizeof(bound);
  TEST_ASSERT(getsockname(listen_fd, (sockaddr*)&bound, &blen) == 0, "http getsockname failed");
  port = ntohs(bound.sin_port);
  ready = 1;
  co_switch(host_thread);

  sockaddr_in peer;
  socklen_t plen = sizeof(peer);
  server_fd1 = accept(listen_fd, (sockaddr*)&peer, &plen);
  TEST_ASSERT(server_fd1 != kInvalidSocket, "accept client1 failed");
  handle_http(server_fd1, "GET /a HTTP/1.1");
  co_switch(host_thread);

  server_fd2 = accept(listen_fd, (sockaddr*)&peer, &plen);
  TEST_ASSERT(server_fd2 != kInvalidSocket, "accept client2 failed");
  handle_http(server_fd2, "GET /b HTTP/1.1");
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
  const char* req = "GET /a HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
  TEST_ASSERT(send(client_fd1, req, (int)std::strlen(req), 0) == (int)std::strlen(req), "client1 send failed");
  co_switch(host_thread);

  char resp[256];
  int n = (int)recv(client_fd1, resp, sizeof(resp) - 1, 0);
  TEST_ASSERT(n > 0, "client1 recv failed");
  resp[n] = 0;
  TEST_ASSERT(std::strstr(resp, "\r\n\r\nA") != 0, "client1 body mismatch");
  done1 = 1;
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
  const char* req = "GET /b HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
  TEST_ASSERT(send(client_fd2, req, (int)std::strlen(req), 0) == (int)std::strlen(req), "client2 send failed");
  co_switch(host_thread);

  char resp[256];
  int n = (int)recv(client_fd2, resp, sizeof(resp) - 1, 0);
  TEST_ASSERT(n > 0, "client2 recv failed");
  resp[n] = 0;
  TEST_ASSERT(std::strstr(resp, "\r\n\r\nB") != 0, "client2 body mismatch");
  done2 = 1;
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
  TEST_ASSERT(ready, "http server not ready");

  co_switch(client1_thread);
  co_switch(server_thread);
  co_switch(client1_thread);

  co_switch(client2_thread);
  co_switch(server_thread);
  co_switch(client2_thread);

  TEST_ASSERT(done1 && done2, "http clients incomplete");

  close_socket(server_fd1);
  close_socket(server_fd2);
  close_socket(client_fd1);
  close_socket(client_fd2);
  close_socket(listen_fd);
  co_delete(server_thread);
  co_delete(client1_thread);
  co_delete(client2_thread);
  net_cleanup();
  std::puts("12_http_two_clients: ok");
  return 0;
}
