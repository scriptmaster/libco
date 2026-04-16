#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t hello_thread;
static int step = 0;

static void hello_entry() {
  step = 1;
  std::puts("hello from coroutine");
  co_switch(host_thread);
  for(;;) co_switch(host_thread);
}

int main() {
  host_thread = co_active();
  hello_thread = co_create(kStackSize, hello_entry);
  TEST_ASSERT(hello_thread != 0, "co_create failed");

  co_switch(hello_thread);
  TEST_ASSERT(step == 1, "hello coroutine did not run");

  co_delete(hello_thread);
  std::puts("01_hello_world: ok");
  return 0;
}
