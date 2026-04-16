# Progressive libco tests

This folder contains 12 progressive tests/examples that explain libco from basics to networking.

1. `01_hello_world` — first coroutine switch and return to host.
2. `02_ping_pong` — two coroutines switching back and forth.
3. `03_goroutine_style_spawn` — running multiple worker-style coroutines.
4. `04_round_robin_scheduler` — simple host-driven round-robin scheduling.
5. `05_producer_consumer` — cooperative producer/consumer flow.
6. `06_fan_in` — fan-in pattern from multiple coroutine producers.
7. `07_tcp_echo` — TCP echo request/response with coroutine roles.
8. `08_tcp_two_clients` — TCP server serving two clients.
9. `09_udp_echo` — UDP ping/pong with coroutine coordination.
10. `10_udp_batch` — UDP multi-datagram batch handling.
11. `11_http_get` — minimal HTTP/1.1 GET handling over TCP.
12. `12_http_two_clients` — HTTP server handling two clients.

## Run on Linux

```bash
cd test
make run
```

## Run on Windows (MSVC Developer PowerShell)

```powershell
cd test
./run-tests.ps1
```
