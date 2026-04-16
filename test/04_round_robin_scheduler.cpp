#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t a_thread;
static cothread_t b_thread;
static int a_count = 0;
static int b_count = 0;

static void a_entry() {
  for(int i = 0; i < 5; ++i) {
    ++a_count;
    co_switch(host_thread);
  }
  for(;;) co_switch(host_thread);
}

static void b_entry() {
  for(int i = 0; i < 5; ++i) {
    ++b_count;
    co_switch(host_thread);
  }
  for(;;) co_switch(host_thread);
}

int main() {
  host_thread = co_active();
  a_thread = co_create(kStackSize, a_entry);
  b_thread = co_create(kStackSize, b_entry);
  TEST_ASSERT(a_thread && b_thread, "co_create failed");

  for(int i = 0; i < 5; ++i) {
    co_switch(a_thread);
    co_switch(b_thread);
  }

  TEST_ASSERT(a_count == 5 && b_count == 5, "round robin counts mismatch");

  co_delete(a_thread);
  co_delete(b_thread);
  std::puts("04_round_robin_scheduler: ok");
  return 0;
}
