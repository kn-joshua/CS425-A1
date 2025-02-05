# Multi-threaded Chat Server (CS425 - A1 - 2025)

## 1. How to Clone and Run the Code

### 1.1 Cloning the Repository
1. **Clone** the repository from GitHub:
   ```bash
   git clone https://github.com/kn-joshua/CS425-A1.git
   ```

2. **Navigate** into the cloned directory:
   ```bash
   cd CS425-A1
   ```

### 1.2 Building the Server and Client
A `Makefile` is provided to compile both the **server** (`server_grp.cpp`) and the **client** (`client_grp.cpp`) in one step:

1. Run:
   ```bash
   make
   ```

This will generate two executables:
* `server_grp`
* `client_grp`

### 1.3 Preparing `users.txt`
* Create or edit the `users.txt` file in the same folder with the format:
  ```
  username:password
  ```
* Example:
  ```
  alice:password123
  bob:qwerty456
  frank:letmein
  ```

Users not present in `users.txt` will fail authentication when they try to connect.

### 1.4 Running the Server
1. **Launch the server** in a terminal:
   ```bash
   ./server_grp
   ```

2. The server will:
   * Bind to `127.0.0.1` on port `12345`
   * Listen for incoming connections
   * Load user credentials from `users.txt`

### 1.5 Running the Client(s)
To interact with the server, you need at least one client. You can open multiple terminals to run more clients for testing group features:
```bash
./client_grp
```

When the client connects, you'll be asked for a username and password. If valid (based on `users.txt`), you can start sending commands, such as:
* `/broadcast <message>`
* `/msg <username> <message>`
* `/create_group <groupName>`
* `/join_group <groupName>`
* `/leave_group <groupName>`
* `/group_msg <groupName> <message>`

**Note**: You can run as many `client_grp` instances (in separate terminals) as you want to simulate multiple connected users.

## 2. Explaining the Code (Server Side)
Below is a brief explanation of the key functions in `server_grp.cpp`:

### `broadcast_message(const std::string &message, int sender_socket)`
* **Purpose**: Broadcasts a message to all connected clients except the sender.
* **How It Works**: Locks the shared client list, iterates over connected clients, and calls `send()` on each socket that is not the sender's socket.

### `private_message(const std::string &sender_username, const std::string &recipient_username, const std::string &message)`
* **Purpose**: Sends a direct (private) message from one user to another.
* **How It Works**: Looks up the recipient's socket in the `username_to_socket` map. If the recipient is online, sends the message. Otherwise, sends an error back to the sender.

### `send_group_message(const std::string &sender_username, const std::string &group_name, const std::string &message)`
* **Purpose**: Delivers a message to all members of a specified group, excluding the sender.
* **How It Works**: Checks if the group exists in `group_members`. If yes, sends the message to each group member's socket (except the sender); otherwise, logs an error on the server side.

### `handle_client(int client, const std::unordered_map<std::string, std::string> &users)`
* **Purpose**: Manages a single client's lifecycle — from authentication to command processing.
* **How It Works**:
   1. **Authentication**: Prompts the connected client for username/password and verifies against the `users` map.
   2. **Post-Auth**: On success, adds the client to `connected_clients`, notifies others that a new user has joined, and enters a loop to:
      * **Receive** commands (e.g., `/broadcast`, `/msg`, `/create_group`, etc.)
      * **Parse** and execute the corresponding actions (e.g., sending messages, creating/joining/leaving groups)
      * **Remove** the client from all data structures if they disconnect

### `main()`
* **Purpose**: Entry point of the server application.
* **How It Works**:
   1. **Socket Setup**: Creates a TCP socket, binds it to `127.0.0.1:12345`, and starts listening.
   2. **Load Credentials**: Reads `users.txt` and stores username-password pairs in a map.
   3. **Accept Loop**: Continuously accepts incoming client connections and spawns a new thread (`std::thread`) that calls `handle_client` for each connected client.
   4. **Thread Management**: Keeps track of all client-handler threads; each thread remains active until its client disconnects.

# Additional Documentation

---

## 3. Assignment Features Implemented / Not Implemented

- **Implemented Features:**
  - **Basic Server Functionality:**  
    The server uses TCP sockets to listen on port `12345` and accepts multiple concurrent client connections.
  - **User Authentication:**  
    Clients are prompted for a username and password, and authentication is performed against the contents of `users.txt`.
  - **Messaging Features:**  
    Supports broadcast messages (`/broadcast`), private messages (`/msg`), and group messages (`/group_msg`).
  - **Group Management:**  
    Clients can create (`/create_group`), join (`/join_group`), and leave (`/leave_group`) groups. The server maintains a mapping of group names to their members.
  - **Multithreading:**  
    Each client connection is handled in a separate thread, ensuring simultaneous interactions.

- **Not Implemented / Future Enhancements:**
  - **Advanced Error Recovery:**  
    More robust error handling and reconnection strategies could be implemented.
  - **Message History & Persistence:**  
    Currently, messages are not stored; adding persistent logging or message history is a potential enhancement.
  - **Enhanced Security:**  
    Encryption for messages and secure storage of passwords (e.g., hashing) is not implemented.

---

## 4. Design Decisions

- **Thread per Connection:**  
  A new thread is created for each client connection. This design simplifies the handling of concurrent clients since each thread independently processes its client's commands. Using threads (with `std::thread`) allows for a straightforward implementation of concurrent I/O without the complexity of managing processes.
  
- **Synchronization Mechanisms:**  
  Shared data structures (like the client list and group membership maps) are protected using `std::mutex` and `std::lock_guard` to avoid race conditions. This ensures that simultaneous modifications by multiple threads do not lead to undefined behavior.
  
- **Socket Programming:**  
  The server uses standard POSIX sockets to implement the TCP server. The use of a loop in `main()` to accept connections and spawn threads provides a simple yet effective model for managing multiple client connections.

---

## 5. Implementation Overview

- **High-Level Functionality:**
  - **Server Initialization:**  
    The server sets up a TCP socket, binds it to `127.0.0.1:12345`, and listens for incoming connections.
  - **User Authentication:**  
    Upon connection, the server requests a username and password and verifies these credentials against a map populated from `users.txt`.
  - **Message Handling:**  
    The server supports various commands for broadcasting, sending private messages, and managing groups.
  - **Multithreading:**  
    Each new connection spawns a thread which runs the `handle_client` function, responsible for processing messages and commands.

## 6. Testing

**Correctness Testing:**
Multiple clients were connected (using client_grp) in separate terminal windows. Each client was tested for:
- Successful authentication.
- Correct message delivery for broadcast, private, and group messaging.
- Proper handling of group creation, joining, and leaving.

**Stress Testing:**
The server was tested with several simultaneous client connections to ensure that multithreading and synchronization handled concurrent requests without data races or crashes.

## 7. Restrictions in the Server

**Maximum Clients:**
There is no explicit hard-coded limit on the number of clients, but system resources (e.g., available file descriptors and CPU) may impose practical limits.

**Maximum Groups:**
There is no strict limit on the number of groups. However, each new group is stored in memory, so available memory is the practical constraint.

**Group Members:**
Similarly, there is no hard-coded limit on the number of members per group, with the system's memory being the main restriction.

**Message Size:**
The buffer size for a message is defined as 1024 bytes. Messages larger than this may be truncated or require additional handling.

## 8. Challenges Faced

**Concurrency Issues:**
Handling multiple client connections concurrently introduced potential race conditions. This was addressed by using mutexes (std::mutex) and lock guards (std::lock_guard).

**Command Parsing:**
Ensuring that commands (especially those with multiple parameters) are parsed correctly was challenging. Edge cases such as missing parameters or extra spaces had to be handled carefully.

**Testing Multithreaded Code:**
Stress testing the server with many simultaneous connections required careful coordination to simulate real-world usage without causing resource exhaustion.

## 9. Contribution of Each Member

**Kapu Nirmal Joshua:**
- Role: Testing and Documentation
- Contribution: 30%
- Performed comprehensive testing on the server and client implementations.
- Prepared and built the complete documentation (this README).

**Aravind S and Sumay Avi:**
- Role: Implementation
- Contribution: 70% (equal to both)
- Handled the overall design and coding of both the server (server_grp.cpp) and client (client_grp.cpp).
- Managed multithreading and synchronization aspects of the assignment.

## 10. Declaration

I hereby declare that this assignment is entirely my own work and does not contain any plagiarized content. All external sources have been properly acknowledged.  
