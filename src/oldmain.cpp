#include <iostream>
#include <sys/socket.h> // Contains core socket system calls (socket, bind, listen, accept)
#include <netinet/in.h> // Contains structures to store IP addresses and ports
#include <unistd.h>     // Contains system utilities like close()
#include <cstring>      // For clearing memory structures (memset)
#include <netdb.h>      // For gethostbyname()
#include <arpa/inet.h>  // For inet_ntoa()
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <algorithm>    // For to_string()
#include <csignal>

int global_server_fd = -1;
std::vector<std::jthread> active_threads;
std::mutex thread_mutex; // Protects the vector from simultaneous access


// Helper function to print messages without jumbled text
void safe_print(std::ostream& stream, const std::string& msg){
    static std::mutex print_mutex;
    std::lock_guard<std::mutex> lock(print_mutex);
    stream << msg << std::endl;
}


// The simple shutdown function
void signal_handler(int signum){
    safe_print(std::cout, "\n\n[SHUTDOWN] Ctrl+C intercepted! Commencing cleanup...");

    if(global_server_fd != -1){
        safe_print(std::cout, "[SHUTDOWN] Closing master socket on port 8080 cleanly.");
        close(global_server_fd);
    }

    safe_print(std::cout, "[SHUTDOWN] Exiting process. Port 8080 freed instantly!");
    exit(signum);
}


void cleanup_finished_threads() {
    std::lock_guard<std::mutex> lock(thread_mutex);

    // std::erase_if (C++20) removes threads that are no longer joinable (finished)
    std::erase_if(active_threads, [](std::jthread& t){
        return !t.joinable();
    });
}


void handle_client(int client_fd){
        // Buffer to store the incoming data
        char buffer[4096] = {0};
        // recv() pulls data from the Kernel into your application memory
        int valread = recv(client_fd, buffer, 4096, 0);

        // EDGE CASE GUARD: Did the browser disconnect or send nothing?
        if(valread <=0){
            close(client_fd);
            return;
        }

        // Safely create the string using EXACTLY the number of bytes read
        std::string request(buffer,valread);
        // HTTP request format is "GET [URL] HTTP/1.1"
        size_t pos1 = request.find(' ');
        size_t pos2 = request.find(' ', pos1+1);
        if(pos1 != std::string::npos && pos2 != std::string::npos){
            std::string url = request.substr(pos1 + 1, pos2 - pos1 - 1);
            safe_print(std::cout,"[PARSED URL]: " + url);
        }


        // Look for the "Host:" line in your buffer string
        size_t host_pos = request.find("Host: ");
        if(host_pos != std::string::npos){
            size_t host_end = request.find("\r\n", host_pos);
            std::string host = request.substr(host_pos + 6, host_end - (host_pos + 6));

            // STRIP THE PORT: If there's a ':', cut it off
            size_t colon_pos = host.find(':');
            if(colon_pos != std::string::npos){
                host = host.substr(0,colon_pos);
            }
            safe_print(std::cout,"[TARGET HOST]: " + host);


            // DNS Resolution (The "Phonebook" lookup)
            struct addrinfo hints, *res;     // We want the system to resolve address
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;       // Support IPv4
            hints.ai_socktype = SOCK_STREAM; // TCP

            if(getaddrinfo(host.c_str(), "80", &hints, &res) == 0){
                char ip_str[INET_ADDRSTRLEN];
                struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
                inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);

                safe_print(std::cout,"[DNS RESOLVED]: " + std::string(ip_str));

                // 1. Create a new socket to talk to the real internet server
                int remote_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
                if (remote_fd >= 0){

                    // 2. Connect to the real target server (e.g., example.com:80)
                    if(connect(remote_fd, res->ai_addr, res->ai_addrlen) >= 0){
                        safe_print(std::cout,"[SUCCESS] Connected to target remote server!");

                        // 3. Forward the browser's original request to the internet
                        send(remote_fd, buffer, valread, 0);

                        // 4. Clear the buffer to receive the internet server's response
                        memset(buffer, 0, 4096);
                        int remote_read;
                        // 5. Loop to receive all chunks of data
                        while ((remote_read  = recv(remote_fd, buffer, 4096, 0)) > 0){
                            safe_print(std::cout,"[RELAY] Received response from internet. Piping back to browser...");

                            // Pipe each chunk back to the browser immediately
                            send(client_fd, buffer, remote_read, 0);
                        }
                    } else {
                        safe_print(std::cerr,"[ERROR]: Failed to connect to remote server!");
                    }
                    // Always close your outbound descriptors
                    close(remote_fd);
                }
                // Free the memory allocated by getaddrinfo
                freeaddrinfo(res);
            } else {
                safe_print(std::cerr,"[ERROR]: Could not resolve host!");
             }
        }

        // Cleanup: Close descriptor
        close(client_fd);
}


int main() {
    // Register the shutdown handler immediately
    signal(SIGINT, signal_handler);

    safe_print(std::cout,"[PROXY] Initializing Proxy Server Engine...");
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_fd < 0) {
        safe_print(std::cerr,"[ERROR] Failed to create socket system descriptor!");
        return 1;
    }

    // Save the socket key globally so signal_handler can see it
    global_server_fd = server_fd;

    int opt = 1;
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0){
        safe_print(std::cerr,"[ERROR]: setsockopt failed!");
        return 1;
    }
    safe_print(std::cout,"[SUCCESS] Socket descriptor created. FD Number: " + std::to_string(server_fd));


    // Define the server address structure
    struct sockaddr_in server_addr;               // We are manually specifying an IPv4 address and port
    memset(&server_addr, 0, sizeof(server_addr)); // Clear memory to avoid garbage data
    server_addr.sin_family = AF_INET;             // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;     // Listen on all network interfaces (localhost, Wi-Fi, etc.)
    server_addr.sin_port = htons(8080);           // Convert 8080 to Network Byte Order
    

    // Bind the socket to the port
    if(bind(server_fd, (struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        safe_print(std::cerr,"[ERROR] Failed to bind to port 8080!");
        return 1;
    }
    safe_print(std::cout,"[SUCCESS] Socket bound to port 8080.");


    // Start listening (10 is the backlog size - how many connections can wait in queue)
    if(listen(server_fd,10)<0){
        safe_print(std::cerr,"[ERROR] Failed to start listening!");
        return 1;
    }
    safe_print(std::cout,"[INFO] Proxy server is now listening on port 8080...");


    while(true){
        safe_print(std::cout,"[WAITING] Ready for connection...");
        // This will pause execution until a browser makes a request
        int client_fd = accept(server_fd, nullptr, nullptr);
        if(client_fd<0){
           safe_print(std::cerr,"[ERROR] Failed to accept client connection!");
           continue;
        }

        // 1. Clear out any dead threads to save memory
        cleanup_finished_threads();

        safe_print(std::cout,"[THREADING] Spawning worker thread for FD: " + std::to_string(client_fd));

        // 2. Safely add the new worker to the active list
        {
            std::lock_guard<std::mutex> lock(thread_mutex);
            active_threads.emplace_back(handle_client, client_fd);
        }
    } 
    close(server_fd);

    return 0;
}