# C++ IRC-Style Chat Server

This is a multithreaded and modular IRC-style chat server written in modern C++20. It's designed for performance and scalability, using a thread pool to handle multiple client connections asynchronously.

---

## Core Features

* **Asynchronous & Multithreaded**: Built with a non-blocking I/O model and a thread pool to efficiently manage numerous concurrent clients.
* **Configurable**: Server settings like port, max users, max channels, server name, and the Message of the Day (MOTD) can be easily configured via an external `.ini` file.
* **Modern C++**: Leverages C++20 features, smart pointers for safe memory management (`std::unique_ptr`, `std::shared_ptr`), and standard library threading primitives (`std::mutex`, `std::atomic`) for thread safety.

---

## Build & Run

### Prerequisites

* A C++20 compatible compiler (e.g., GCC 10+, Clang 12+)
* CMake (version 3.16+)
* Make

### Building the Server

```bash
# Clone the repository
git clone [https://github.com/Bolty232/boltchat-server.git](https://github.com/Bolty232/boltchat-server.git)
cd boltchat-server

# Create a build directory
mkdir build && cd build

# Configure and build the project
cmake ..
make
```

## Running the Server
```bash
# Run with default settings
./Server

# Run with a custom configuration file
./Server --configpath /path/to/your/server.conf
```
A sample server.ini might look like this:
```Ini
port=6667
maxusers=500
maxchannels=100
servername=MyCoolChatServer
motd=Welcome!
```

## Implemented Commands

| Command                      | Description                                                  |
| ---------------------------- | ------------------------------------------------------------ |
| `/nick <name>`               | Change your nickname.                                        |
| `/join <#channel>`           | Join a channel. Creates it if it doesn't exist.              |
| `/part <#channel>`           | Leave a channel.                                             |
| `/msg <#channel\|user> <msg>` | Send a message to a channel or a private message to a user.  |
| `/list`                      | List all active channels and their member counts.            |
| `/who [#channel]`            | List users online (server-wide or in a specific channel).    |
| `/motd`                      | Show the server's Message of the Day.                        |
| `/quit [message]`            | Disconnect from the server with an optional message.         |
| `/help`                      | Displays the list of available commands.                     |

## Roadmap & Potential Improvements

Future enhancements could include:

* **Core Features**:
    * **Channel Operators**: Implement user roles (`@operator`) with privileges like `/kick` and `/ban`.
    * **Channel Modes**: Add support for standard channel modes like `+i` (invite-only), `+m` (moderated), and `/topic` protection.
    * **More Commands**: Implement `/whois`, `/away`, and `/invite`.

* **Security**:
    * **SSL/TLS Encryption**: Secure client-server communication by wrapping sockets in an SSL layer.
    * **User Authentication**: Add password protection for server access and nickname registration (a "NickServ"-like feature).

* **Quality of Life**:
    * **Persistence**: Use a lightweight database (e.g., SQLite) to save channel states and registered users across server restarts.
    * **Logging**: Implement robust logging for chat history, server events, and errors.
    * **Keep-Alive**: Add a `PING`/`PONG` mechanism to detect and close dead connections more quickly.
    * **More configurability**: Add more settings to be processed in the `config.ini`
