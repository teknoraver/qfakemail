# QFakeMail

QFakeMail is a simple mailer written with Qt for educational purposes:
it speaks SMTP directly over a TCP socket, and it shows how to use sockets,
lists, windows, and signals and slots.

It sends a plain text message, optionally with attachments, through any SMTP server,
and lets you choose the sender address.

<p align="center"><img src="https://github.com/user-attachments/assets/be379d9e-5829-47f1-a35f-79c62edf0759" alt="QFakeMail main window" width="322"></p>

## Build

Requirements: CMake 3.19 or newer and Qt 6 (Widgets, Network).

```
cmake -B build
cmake --build build
```

## License

Copyright (C) 2005-2007 Matteo Croce

QFakeMail is released under the GNU General Public License version 3 or later, see [COPYING](COPYING).
