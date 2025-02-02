# Multi-threaded Chat Server (CS425 - A1 - 2025)

## Overview
This project implements a multi-threaded chat server that supports multiple concurrent client connections, user authentication, broadcast messaging, private messaging, and group functionalities. 

The project is implemented using TCP socket programming, multithreading (via `std::thread`), and data synchronization (using mutexes).

---

## 1. How to Clone and Run the Code

### 1.1 Cloning the Repository
1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/your-repo.git
   ```

2. Navigate into the repository directory:
   ```bash
   cd your-repo
   ```

### 1.2 Building the Server
This project includes a Makefile to simplify the build process.

1. Run the following command to compile the server:
   ```bash
   make
   ```

This will generate an executable named `server_grp`.

### 1.3 Preparing the `users.txt`
Ensure that you have a file named `users.txt` in the project directory. Each line in the file should be in the format:

```
username:password
```

For example:
```
alice:password123
bob:qwerty456
frank:letmein
```

### 1.4 Running the Server
After building the project, start the server by running:
```bash
./server_grp
```

The server will:
* Read user credentials from `users.txt`
* Bind to `127.0.0.1` on port `12345`
* Listen for incoming client connections

## 2. Explaining the Code

### `broadcast_message(const std::string &message, int sender_socket)`
* **Purpose**: Sends a message to all connected clients except the sender.
* **Functionality**: Locks the client list, iterates over all connected client sockets, and sends the message to each client except the one with `sender_socket`.

### `private_message(const std::string &sender_username, const std::string &recipient_username, const std::string &message)`
* **Purpose**: Sends a private message from the sender to a specific recipient.
* **Functionality**: Checks if the recipient is connected by looking up `username_to_socket`. If connected, sends the message; otherwise, sends an error message back to the sender.

### `send_group_message(const std::string &sender_username, const std::string &group_name, const std::string &message)`
* **Purpose**: Sends a message to all members of a specified group, excluding the sender.
* **Functionality**: Validates the existence of the group and iterates through its members to send the message to each member (except the sender).

### `handle_client(int client, const std::unordered_map<std::string, std::string> &users)`
* **Purpose**: Manages the lifecycle of a client connection.
* **Functionality**:
   * Authenticates the client by requesting a username and password
   * Adds the client to the connected client list upon successful authentication
   * Broadcasts a join message
   * Enters a loop to receive and process client commands (broadcast, private message, group message, create group, join group, and leave group)
   * Cleans up and removes the client upon disconnection

### `main()`
* **Purpose**: Serves as the entry point of the server.
* **Functionality**:
   * Sets up the server socket (creates, binds, and listens)
   * Loads valid user credentials from `users.txt`
   * Enters a loop to accept new client connections
   * For each connection, spawns a new thread running `handle_client`
