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
