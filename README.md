# QFakeMail

QFakeMail is a simple mailer written with Qt for educational purposes:
it speaks SMTP directly over a TCP socket, and it shows how to use sockets,
lists, windows, and signals and slots.

It sends a plain text message, optionally with attachments, through any SMTP server,
and lets you choose the sender address.

## Build

Requirements: CMake 3.19 or newer and Qt 6 (Widgets, Network).

```
cmake -B build
cmake --build build
```

## License

Copyright (C) 2005-2007 Matteo Croce

QFakeMail is released under the GNU General Public License version 3 or later, see [COPYING](COPYING).
