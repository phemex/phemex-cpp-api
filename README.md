# Phemex C++ API
This is project includes 2 parts,
* Phemex market data & trading API in C++
* A simple runnable demo to Phemex market data & trading services

### Depends

* GCC >= 8.2, supports C++17
* Boost 1.70.0
* OpenSSL 1.1.1.d-2
* [nlohmann json 3.7.3](https://github.com/nlohmann/json)

### Build
```
$ make -j2
```

### Run
The demo shows how to subscribe Phemex order book, klines, trades market data information.

```
$ ./phemex-cpp-api
```
