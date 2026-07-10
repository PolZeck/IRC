_This project has been created as part of the 42 curriculum by pledieu, lcosson_

## ft_irc

A lightweight Internet Relay Chat (IRC) server written in C++98 as part of the 42 curriculum.

# Description

`ft_irc` is a custom IRC server implementing the core features of the IRC protocol using non-blocking sockets and `poll()`.

The project focuses on:

* TCP/IP networking
* Multiplexing with `poll()`
* Non-blocking I/O
* Client/server architecture
* IRC protocol parsing
* Multi-client management
* Channel and operator handling

The server is compatible with standard IRC clients such as HexChat, irssi, or netcat.

---

## Features

* Multiple client connections
* Non-blocking sockets
* `poll()`-based event loop
* IRC authentication system
* Channel creation and management
* Private and channel messaging
* Operator management
* IRC command parsing
* Clean memory management

---

## Implemented Commands

| Command | Description              |
| ------- | ------------------------ |
| PASS    | Server authentication    |
| NICK    | Set nickname             |
| USER    | Set username             |
| JOIN    | Join a channel           |
| PART    | Leave a channel          |
| PRIVMSG | Send messages            |
| QUIT    | Disconnect               |
| KICK    | Remove user from channel |

---

## Channel Modes

| Mode | Description                   |
| ---- | ----------------------------- |
| +i   | Invite-only channel           |
| +t   | Restricted topic modification |
| +k   | Channel password              |
| +o   | Channel operator              |
| +l   | User limit                    |

---

# Instructions

```bash
make
```

This will generate:

```bash
./ircserv
```

---

## Usage

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 pass
```

---

## Testing with Netcat

Open a second terminal:

```bash
nc localhost 6667
```

Then send IRC commands:

```text
PASS pass
NICK user1
USER user1 0 * :User One
JOIN #42
```

Open another terminal for a second client:

```bash
nc localhost 6667
```

```text
PASS pass
NICK user2
USER user2 0 * :User Two
JOIN #42
PRIVMSG #42 :Hello everyone
```

---

## Testing with HexChat

You can also connect using an IRC client such as HexChat.

Connection settings:

* Server: `localhost`
* Port: `6667`
* Password: `pass`

---

## Project Structure

```text
includes/
srcs/
    commands/
    main.cpp
    Server.cpp
    Client.cpp
    Channel.cpp
Makefile
```

---

## Technical Concepts

This project covers:

* Socket programming
* TCP communication
* Event-driven architecture
* IRC protocol basics
* File descriptor multiplexing
* Buffer management
* Dynamic memory management

---

## Memory Management

The server was tested with Valgrind:

```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-fds=yes \
         ./ircserv 6667 pass
```


# Resources

We used AI as a peer-to-peer system. We configured it this way: we explained our design, our vision, and our understanding of the project and its components to it.
It allowed us to raise questions and make changes, accompanied by explanations, during our interactions with it.

We used as reference : RFC 2812 : https://www.rfc-editor.org/info/rfc2812/