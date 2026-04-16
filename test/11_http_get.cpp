#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t server_thread;
static cothread_t client_thread;

static socket_t listen_fd = kInvalidSocket;
static socket_t server_fd = kInvalidSocket;
static socket_t client_fd = kInvalidSocket;
static unsigned short port = 0;
static int ready = 0;
static int client_done = 0;

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
  TEST_ASSERT(listen(listen_fd, 2) == 0, "http listen failed");

  sockaddr_in bound;
  socklen_t blen = sizeof(bound);
  TEST_ASSERT(getsockname(listen_fd, (sockaddr*)&bound, &blen) == 0, "http getsockname failed");
  port = ntohs(bound.sin_port);
  ready = 1;
  co_switch(host_thread);

  sockaddr_in peer;
  socklen_t plen = sizeof(peer);
  server_fd = accept(listen_fd, (sockaddr*)&peer, &plen);
  TEST_ASSERT(server_fd != kInvalidSocket, "http accept failed");

  char req[256];
  int n = (int)recv(server_fd, req, sizeof(req) - 1, 0);
  TEST_ASSERT(n > 0, "http recv request failed");
  req[n] = 0;
  TEST_ASSERT(std::strstr(req, "GET /hello HTTP/1.1") != 0, "http request line mismatch");

  const char* resp =
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 5\r\n"
    "Connection: close\r\n"
    "\r\n"
    "hello";
  TEST_ASSERT(send(server_fd, resp, (int)std::strlen(resp), 0) == (int)std::strlen(resp), "http send response failed");
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
  TEST_ASSERT(client_fd != kInvalidSocket, "http client socket failed");
  TEST_ASSERT(connect(client_fd, (sockaddr*)&addr, sizeof(addr)) == 0, "http connect failed");

  const char* req =
    "GET /hello HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n";
  TEST_ASSERT(send(client_fd, req, (int)std::strlen(req), 0) == (int)std::strlen(req), "http send request failed");
  co_switch(host_thread);

  char resp[256];
  int n = (int)recv(client_fd, resp, sizeof(resp) - 1, 0);
  TEST_ASSERT(n > 0, "http recv response failed");
  resp[n] = 0;
  TEST_ASSERT(std::strstr(resp, "HTTP/1.1 200 OK") != 0, "http status mismatch");
  TEST_ASSERT(std::strstr(resp, "\r\n\r\nhello") != 0, "http body mismatch");
  client_done = 1;
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
  TEST_ASSERT(ready, "http server not ready");
  co_switch(client_thread);
  co_switch(server_thread);
  co_switch(client_thread);
  TEST_ASSERT(client_done, "http client did not complete");

  close_socket(server_fd);
  close_socket(client_fd);
  close_socket(listen_fd);
  co_delete(server_thread);
  co_delete(client_thread);
  net_cleanup();
  std::puts("11_http_get: ok");
  return 0;
}
