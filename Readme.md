# Text-Based Chat Program

A simple networking application written in C that enables real-time text communication between multiple users over a local network or the internet. The program allows users to create chat rooms or engage in one-on-one conversations, with features such as username selection, message timestamps, and basic command functionality for actions like joining or leaving chat rooms.

## Educational Project
Developed as part of Programming Fundamentals course at NUP 2025.

## Code Style

This project follows the **K&R (Kernighan and Ritchie)** coding style.

## Features

1. Real-Time Messaging
    - **Description:** Users can send and receive messages instantly in shared chat rooms or private chats.

2. Chat Rooms Management
    - **Description:** Users can create, join, leave, and list available chat rooms.

3. Command System
    - **Description:** Built-in commands provide enhanced control and navigation inside the chat application.

## Documentation

All functions are documented using **Doxygen** docstring format.

## Dependencies

- GCC
- Make

## Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/programming-fundamentals-nup-2025/examproject1-Tacitus-SL.git
cd examproject1-Tacitus-SL
```

2. Install dependencies:
```bash
make install_deps
```

3. Build the project:
```bash
make
```

4. Run tests:
```bash
make test
```

## Usage Examples
1. Starting the Server
```bash
./build/server -p 8080
```

2. Starting the Client

Open a new terminal window for each client.
```bash
./build/client -p 8080 -a 127.0.0.1
```

3. Shutting down
To stop the server and disconnect all clients, press: `CTRL+C`

**Step-by-step usage flow:**
- **Server:** Start `./build/server -p 8080`
- **Client 1:** Start `./build/client -p 8080 -a 127.0.0.1`
  - Type: `/name Alice`
- **Client 2:** Start `./build/client -p 8080 -a 127.0.0.1`
  - Type: `/name Bob`
- **Chat:** Alice types "Hello!", Bob sees it instantly.

## Installation

Install to `/usr/local/bin`:
```bash
sudo make install
```

Uninstall:
```bash
sudo make uninstall
```

## Project Structure
```
.
├── src/
│   ├── client.c              # TCP client implementation
│   ├── server.c              # TCP server main loop and event handling
│   ├── server_utils.c/h      # Server utilities (client management, rooms, commands)
│   ├── protocol.h            # Protocol definitions and constants
│   └── colors.h              # ANSI color codes for terminal output
├── tests/
│   ├── run_tests.sh          # Test runner script
│   └── unit_tests.c          # Unit tests
├── Makefile                  # Build system
├── README.md
└── .gitignore
```
