#include "test_common.hpp"

static cothread_t host_thread;
static cothread_t producer_thread;
static cothread_t consumer_thread;

static int queue_data[4];
static int queue_len = 0;
static int produced = 0;
static int consumed_sum = 0;

static void producer_entry() {
  for(int i = 1; i <= 4; ++i) {
    while(queue_len == 4) co_switch(host_thread);
    queue_data[queue_len++] = i;
    ++produced;
    co_switch(host_thread);
  }
  for(;;) co_switch(host_thread);
}

static void consumer_entry() {
  int consumed = 0;
  while(consumed < 4) {
    while(queue_len == 0) co_switch(host_thread);
    int value = queue_data[0];
    for(int i = 1; i < queue_len; ++i) queue_data[i - 1] = queue_data[i];
    --queue_len;
    consumed_sum += value;
    ++consumed;
    co_switch(host_thread);
  }
  for(;;) co_switch(host_thread);
}

int main() {
  host_thread = co_active();
  producer_thread = co_create(kStackSize, producer_entry);
  consumer_thread = co_create(kStackSize, consumer_entry);
  TEST_ASSERT(producer_thread && consumer_thread, "co_create failed");

  while(produced < 4 || consumed_sum != 10) {
    co_switch(producer_thread);
    co_switch(consumer_thread);
  }

  TEST_ASSERT(consumed_sum == 10, "consumer sum mismatch");
  co_delete(producer_thread);
  co_delete(consumer_thread);
  std::puts("05_producer_consumer: ok");
  return 0;
}
