#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <algorithm>

#define BUFFER_SIZE 1024

// Mutex used for safely printing to console
std::mutex cout_mutex;

// Mutex used for protecting shared data structures (connected_clients, username_to_socket, etc.)
std::mutex clients_mutex;

// Vector to store the socket descriptors of all connected clients
std::vector<int> connected_clients;

// Maps a username to its socket descriptor for quick lookup
std::unordered_map<std::string, int> username_to_socket;

// Maps a group name to a set of usernames belonging to that group
std::unordered_map<std::string, std::unordered_set<std::string>> group_members;

// Mutex used to protect operations on the group_members data structure
std::mutex groups_mutex;

/**
 * @brief Sends a message to all connected clients except the sender.
 * 
 * @param message The text message to be sent
 * @param sender_socket The socket descriptor of the sender (excluded from receiving)
 */
void broadcast_message(const std::string &message, int sender_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client_socket : connected_clients) {
        if (client_socket != sender_socket) {
            send(client_socket, message.c_str(), message.size(), 0);
        }
    }
}

/**
 * @brief Sends a private message to a specific user.
 * 
 * @param sender_username The username of the sender
 * @param recipient_username The username of the recipient
 * @param message The text message to be sent
 */
void private_message(const std::string &sender_username, const std::string &recipient_username, const std::string &message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    // Check if recipient is currently connected
    if (username_to_socket.find(recipient_username) != username_to_socket.end()) {
        int recipient_socket = username_to_socket[recipient_username];
        std::string formatted_message = "Private message from " + sender_username + ": " + message;
        send(recipient_socket, formatted_message.c_str(), formatted_message.size(), 0);
    } else {
        // If recipient is not connected, inform the sender
        int sender_socket = username_to_socket[sender_username];
        std::string error_msg = "Error: User '" + recipient_username + "' not found or not connected.\n";
        send(sender_socket, error_msg.c_str(), error_msg.size(), 0);
    }
}

/**
 * @brief Sends a message to all members of a specified group (except the sender).
 * 
 * @param sender_username The username of the sender
 * @param group_name Name of the group to which the message is sent
 * @param message The text message to be sent
 */
void send_group_message(const std::string &sender_username, const std::string &group_name, const std::string &message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    // Check if the group exists
    if (group_members.find(group_name) != group_members.end()) {
        std::string formatted_message = "Group message from " + sender_username + " in " + group_name + ": " + message;
        // Send the message to all members in the group (except the sender)
        for (const auto &member : group_members[group_name]) {
            if (member != sender_username && username_to_socket.find(member) != username_to_socket.end()) {
                int member_socket = username_to_socket[member];
                send(member_socket, formatted_message.c_str(), formatted_message.size(), 0);
            }
        }
    } else {
        // Group not found, log this in the server console
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Group " << group_name << " does not exist." << std::endl;
    }
}

/**
 * @brief Handles interaction with an individual client.
 *        Performs user authentication and processes commands.
 * 
 * @param client Socket descriptor for the connected client
 * @param users A map of valid username-password pairs for authentication
 */
void handle_client(int client, const std::unordered_map<std::string, std::string> &users) {
    char buffer[BUFFER_SIZE];

    // Prompt for username
    const char *auth = "Enter Username\n";
    send(client, auth, strlen(auth), 0);

    memset(buffer, 0, BUFFER_SIZE);
    recv(client, buffer, BUFFER_SIZE, 0);
    std::string received_username(buffer);
    // Remove trailing newline characters if any
    received_username.erase(std::remove(received_username.begin(), received_username.end(), '\n'), received_username.end());

    // Prompt for password
    auth = "Enter Password\n";
    send(client, auth, strlen(auth), 0);

    memset(buffer, 0, BUFFER_SIZE);
    recv(client, buffer, BUFFER_SIZE, 0);
    std::string received_password(buffer);
    // Remove trailing newline characters if any
    received_password.erase(std::remove(received_password.begin(), received_password.end(), '\n'), received_password.end());

    // Check authentication
    const char *message;
    auto it = users.find(received_username);
    if (it != users.end() && it->second == received_password) {
        message = "Authentication Successful\n";
        send(client, message, strlen(message), 0);
    } else {
        message = "Authentication failed\n";
        send(client, message, strlen(message), 0);
        close(client); // Disconnect if authentication fails
        return;
    }

    {
        // Print to server console that the client has authenticated
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Client " << received_username << " authenticated successfully." << std::endl;
    }

    {
        // Add client to the list of connected clients and record their username
        std::lock_guard<std::mutex> lock(clients_mutex);
        connected_clients.push_back(client);
        username_to_socket[received_username] = client;
    }

    {
        // Broadcast that this user has joined the chat
        std::string joining_chat = received_username + " has joined the chat.\n";
        broadcast_message(joining_chat, client);  
    }

    // Communication loop to receive commands/messages from the client
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client, buffer, BUFFER_SIZE, 0);
        
        // If the client disconnects or an error occurs
        if (bytes_received <= 0) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Client " << received_username << " disconnected." << std::endl;
            std::string exit_message = received_username + " has exited the chat.";
            broadcast_message(exit_message, client);
            
            {
                // Clean up client references
                std::lock_guard<std::mutex> lock(clients_mutex);
                connected_clients.erase(std::remove(connected_clients.begin(), connected_clients.end(), client), connected_clients.end());
                username_to_socket.erase(received_username);

                // Remove this user from all groups they belong to
                for (auto &[group_name, members] : group_members) {
                    members.erase(received_username);
                }
            }
            close(client);
            break;
        }

        // Convert the client message to a std::string and remove trailing newline
        std::string client_message(buffer);
        client_message.erase(std::remove(client_message.begin(), client_message.end(), '\n'), client_message.end());

        // Print the received message in the server console
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Message from " << received_username << ": " << client_message << std::endl;
        }

        // Check which command was sent

        // 1) Broadcast command
        if (client_message.rfind("/broadcast ", 0) == 0) {
            std::string broadcast_content = client_message.substr(11);
            std::string formatted_message = "Broadcast from " + received_username + ": " + broadcast_content;
            broadcast_message(formatted_message, client);
        }

        // 2) Private message command
        else if (client_message.rfind("/msg ", 0) == 0) {
            std::istringstream iss(client_message.substr(5));
            std::string recipient_username, message_content;
            iss >> recipient_username;            // read the recipient's username
            std::getline(iss, message_content);   // read the rest of the line as message content

            if (!recipient_username.empty() && !message_content.empty()) {
                private_message(received_username, recipient_username, message_content);
            } else {
                const char *error_msg = "Invalid command format. Use: /msg <username> <message>\n";
                send(client, error_msg, strlen(error_msg), 0);
            }
        }

        // 3) Group message command
        else if (client_message.rfind("/group_msg ", 0) == 0) {
            std::istringstream iss(client_message.substr(11));
            std::string group_name, group_message;
            iss >> group_name;
            std::getline(iss, group_message);

            if (group_name.empty() || group_message.empty()) {
                const char *error_msg = "Error: Group name or message cannot be empty.\n";
                send(client, error_msg, strlen(error_msg), 0);
            }
            else {
                // Check if group exists, and if the user is a member of that group
                std::lock_guard<std::mutex> lock(groups_mutex);
                if (group_members.find(group_name) != group_members.end()) {
                    if (group_members[group_name].find(received_username) != group_members[group_name].end()) {
                        send_group_message(received_username, group_name, group_message);
                    } else {
                        const char *error_msg = "Error: You are not a member of the group.\n";
                        send(client, error_msg, strlen(error_msg), 0);
                    }
                } else {
                    const char *error_msg = "Group does not exist\n";
                    send(client, error_msg, strlen(error_msg), 0);
                }
            }
        }

        // 4) Create group command
        else if (client_message.rfind("/create_group ", 0) == 0) {
            std::string group_name = client_message.substr(14);
            if (group_name.empty()) {
                const char *error_msg = "Error: Group name cannot be empty.\n";
                send(client, error_msg, strlen(error_msg), 0);
            }
            else {
                std::lock_guard<std::mutex> lock(groups_mutex);
                if (group_members.find(group_name) == group_members.end()) {
                    // Create the group and add this user to it
                    group_members[group_name].insert(received_username);
                    const char *success_msg = "Group created successfully.\n";
                    send(client, success_msg, strlen(success_msg), 0);
                } else {
                    const char *error_msg = "Group already exists.\n";
                    send(client, error_msg, strlen(error_msg), 0);
                }
            }
        }

        // 5) Join group command
        else if (client_message.rfind("/join_group ", 0) == 0) {
            std::string group_name = client_message.substr(12);
            if (group_name.empty()) {
                const char *error_msg = "Error: Group name cannot be empty.\n";
                send(client, error_msg, strlen(error_msg), 0);
            }
            else {
                std::lock_guard<std::mutex> lock(groups_mutex);
                if (group_members.find(group_name) != group_members.end()) {
                    // Check if the user is already in the group
                    if (group_members[group_name].find(received_username) != group_members[group_name].end()) {
                        const char *error_msg = "You are already a member of this group.\n";
                        send(client, error_msg, strlen(error_msg), 0);
                    } else {
                        group_members[group_name].insert(received_username);
                        const char *success_msg = "Joined group successfully.\n";
                        send(client, success_msg, strlen(success_msg), 0);
                    }
                } else {
                    const char *error_msg = "Group does not exist.\n";
                    send(client, error_msg, strlen(error_msg), 0);
                }
            }
        }

        // 6) Leave group command
        else if (client_message.rfind("/leave_group ", 0) == 0) {
            std::string group_name = client_message.substr(13);
            if (group_name.empty()) {
                const char *error_msg = "Error: Group name cannot be empty.\n";
                send(client, error_msg, strlen(error_msg), 0);
            }
            else {
                std::lock_guard<std::mutex> lock(groups_mutex);
                // Check if the group exists and if the user is a member
                if (group_members.find(group_name) != group_members.end() &&
                    group_members[group_name].find(received_username) != group_members[group_name].end()) {
                    group_members[group_name].erase(received_username);
                    const char *success_msg = "Left group successfully.\n";
                    send(client, success_msg, strlen(success_msg), 0);
                } else {
                    const char *error_msg = "Group does not exists or you are not part of this group.\n";
                    send(client, error_msg, strlen(error_msg), 0);
                }
            }
        }

        // If the command doesn't match any valid pattern, send error to client
        else {
            const char *error_msg = "Incorrect Format of message\n";
            send(client, error_msg, strlen(error_msg), 0);
        }
    }
}

/**
 * @brief Main function to set up the server, load user credentials, and accept client connections.
 * 
 * @return int Exit status
 */
int main() {
    int server_socket;
    sockaddr_in server_address{};

    // Create the server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    } else
        std::cout << "Socket successfully created.\n";

    // Configure server address parameters
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(12345);           // Port number
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Loopback IP address

    // Bind the socket to our specified IP and port
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) != 0) {
        std::cerr << "Bind failed";
        return 1;
    } else
        std::cout << "Bind Successful.\n";

    // Listen for incoming client connections
    if (listen(server_socket, 5) != 0) {
        std::cerr << "Listen Failed";
        return 1;
    } else
        std::cout << "Listen Successful.\n";

    // Load valid usernames and passwords from users.txt
    std::unordered_map<std::string, std::string> users;
    std::ifstream file("users.txt");
    std::string line;
    std::string username, password;

    while (std::getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            username = line.substr(0, pos);
            password = line.substr(pos + 1);
            users[username] = password;
        }
    }

    // Vector to hold threads for each connected client
    std::vector<std::thread> threads;

    // Continuously accept new connections in a loop
    while (true) {
        int client = accept(server_socket, nullptr, nullptr);
        if (client < 0) {
            std::cerr << "Connection Failed" << std::endl;
            continue;
        }
        std::cout << "Client connected!\n";

        // Spawn a new thread to handle each client
        threads.emplace_back(handle_client, client, std::cref(users));
    }

    // Join all threads (in practice, this might never be reached unless the server is shut down)
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Close the server socket
    close(server_socket);
    return 0;
}
