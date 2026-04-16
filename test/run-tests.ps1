$ErrorActionPreference = "Stop"

$tests = @(
  "01_hello_world",
  "02_ping_pong",
  "03_goroutine_style_spawn",
  "04_round_robin_scheduler",
  "05_producer_consumer",
  "06_fan_in",
  "07_tcp_echo",
  "08_tcp_two_clients",
  "09_udp_echo",
  "10_udp_batch",
  "11_http_get",
  "12_http_two_clients"
)

cl /nologo /I .. /Zc:__STDC__ /c ..\libco.c /Fo:libco.obj

foreach ($test in $tests) {
  cl /nologo /EHsc /std:c++14 /I .. "$test.cpp" libco.obj /link ws2_32.lib /OUT:"$test.exe"
}

foreach ($test in $tests) {
  Write-Host "==> $test"
  & ".\$test.exe"
}
