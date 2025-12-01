[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/c-Tbil6P)
# Text-Based Chat Program

A simple networking application that enables real-time text communication between multiple users over a local network or the internet. The program allows users to create chat rooms or engage in one-on-one conversations, with features such as username selection, message timestamps, and basic command functionality for actions like joining or leaving chat rooms.

## Features

1. Real-Time Messaging
- **Description:**
  Users can send and receive messages instantly in shared chat rooms or private chats.
- **Acceptance Criteria:**
   - Messages appear immediately for all users in the same room.
   - Each message includes a timestamp and the sender’s username.
   - The program handles multiple clients concurrently.

2. Chat Rooms Management
- **Description:**
  Users can create, join, leave, and list available chat rooms.
- **Acceptance Criteria:**
   - A user can create a room with a unique name.
   - A user can join any existing room.
   - Leaving a room cleanly disconnects the user and broadcasts a status message.
   - Users can list all active rooms.

3.Command System
- **Description:**
   Built-in commands provide enhanced control and navigation inside the chat application.

- **Acceptance Criteria:**
   - Commands start with / (e.g., /join, /leave, /msg, /rooms).
   - Unknown commands are gracefully rejected with a clear message.
   - Command processing does not block message flow.

## Dependencies

- GCC
- Make

## Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/programming-fundamentals-nup-2025/examproject1-Tacitus-SL.git
cd examproject1-Tacitus-SL
```

2. Build the project:
```bash
make
```

3. Run tests:
```bash
make test
```

## Usage Examples
Start the server:
```bash
./server [port]
```

Connect a client:
```bash
./client [server-ip] [port]
```

Common commands inside the chat:
```bash
/join room_name
/leave
/rooms
/msg username Hello!
/quit
```
