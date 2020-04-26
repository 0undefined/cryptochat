# Crypto Chat

WIP peer to peer encrypted chat system. You are free to use, copy, make edits
and distribute this project however you like.


# Install procedure

Currently there's only support for Unix-like systems. There not planned any
windows support in the near future.

Prerequisites:
* [OpenSSL](https://www.openssl.org/source/)
* [NCurses](https://www.gnu.org/software/ncurses/ncurses.html)
* [PThread](https://linux.die.net/man/7/pthreads)

Most of the prerequisites are available in most package managers.


## Roadmap

* [ ] Encryption layer
* [ ] TUI
* [ ] Implement commands
  + [ ] Ditch current system and use BYacc and Flex instead
* [ ] Better async execution
* [ ] Tab completion


## Commands

* `:w :whisper`
    `:w[hisper] [recipient] [message]`
    Send message to a specific peer
* `:c :connect`
    `:c[onnect] [server:[port]]`
    Establishes connection to a peer
* `:s :say`
    `:s[ay] [message]`
    Broadcast message to all connected peers
