# libwebsock

A simple C library for processing WebSocket protocol messages.

This library allows a developer to quickly develop WebSocket servers by focusing
on the actual logic of your WebSocket implementation instead of the details
of the WebSocket protocol.

## Features

* Easy to use
* No dependencies on any external libraries
* No failures on Autobahn Test suite
* Valgrind clean
* Single-threaded (synchronization is developers responsibility)
* To get started, have a look at [autobahn-echo.c][1] in the examples directory of the package. A sample echo server is implemented.
* Current Travis-CI build status:
[![Build Status][4]][5]

Note: As this library is only for processing WebSocket protocol messages, building the communication layer is the responsibility of the developer.

 [1]: https://github.com/samiahmedshaik/libwebsock/blob/master/examples/autobahn-echo.c
 [3]: http://paydensutherland.com/autobahn
 [4]: https://travis-ci.org/samiahmedshaik/libwebsock.png
 [5]: https://travis-ci.org/samiahmedshaik/libwebsock
 [6]: https://github.com/payden/libwebsock/wiki/Installation
 [7]: https://github.com/payden/libwebsock/wiki/API
