# DNS Server

Basic recursive DNS server written in C that serves AAAA queries. Unlike most DNS implementations, this project uses TCP instead of UDP. It caches queries in memory to improve response time.

## Building and running

The project can be built using the provided Makefile. 
```
Usage: ./dns_svr [server-ip] [server-port]

[server-ip]: IP address of an upstream DNS server
[server-port]: port of an upstream DNS server
```

## Sample output
Given the following queries:
```
AAAA www1.ci.test.of.comp30023-c-1.unimelb.comp30023
AAAA a.bcd.efgh.ijklm.comp30023
```

The program outputs:
```
2021-05-06T13:01:42+0000 requested www1.ci.test.of.comp30023-c-1.unimelb.comp30023
2021-05-06T13:01:42+0000 www1.ci.test.of.comp30023-c-1.unimelb.comp30023 is at 2001:388:6074::7547:1234
2021-05-06T13:01:42+0000 requested a.bcd.efgh.ijklm.comp30023
2021-05-06T13:01:42+0000 a.bcd.efgh.ijklm.comp30023 is at 2001:388:6074::7547:9999
```
