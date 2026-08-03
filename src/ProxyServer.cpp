#include "ProxyServer.h"
#include "Logger.h"
#include "SignalHandler.h"
#include "HttpParser.h"

#include <sys/socket.h> // Contains core socket system calls (socket, bind, listen, accept)
#include <netinet/in.h> // Contains structures to store IP addresses and ports
#include <cstring>      // For clearing memory structures (memset)
#include <netdb.h>      // For addrinfo
#include <unistd.h>     // Contains system utilities like close()
#include <arpa/inet.h>  // For inet_ntoa()
#include <cerrno>

ProxyServer::ProxyServer(int port) : port(port), server_fd(-1), pool(std::thread::hardware_concurrency()) {
}

void ProxyServer::handleClient(int client_fd) {

    // --- TIMEOUT LOGIC ---
    struct timeval tv; // timeval is a struct to store time-interval value upto microsecond precision
    tv.tv_sec = 5;     // Stores sec
    tv.tv_usec = 0;    // Stores microsec

    // This tells the kernel to stop waiting for data after 5 seconds AND free the thread
    if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) { // setsockopt() takes char as option-val pointer
        Logger::error("[ERROR] Failed to set client socket timeout");
    }
    // ---------------------

    // Buffer to store the incoming data
    char buffer[4096];
    // recv() pulls data from the Kernel into your application memory
    int valread = recv(client_fd, buffer, sizeof(buffer), 0); // Returns number of bytes read

    // EDGE CASE GUARD: Did the browser disconnect or send nothing?
    if(valread <= 0) {
        close(client_fd);
        return;
    }

    // Safely create the string using EXACTLY the number of bytes read !
    std::string request(buffer, valread);

    std::string url = HttpParser::extractURL(request);
    Logger::info("[PARSED URL]: " + url);

    std::string host = HttpParser::extractHost(request);

    if(host.empty()) {
        close(client_fd);
        return;
    }

    int target_port = HttpParser::extractPort(request);

    std::string portStr = std::to_string(target_port);

    // DNS Resolution (The "Phonebook" lookup)
    struct addrinfo hints, *res;     // We want the system to resolve address
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // Support IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP


    // By getaddrinfo, OS will filter addresses with 'hints' type and the result will be stored as a LL,
    // res will point to the Head of that LL
    if(getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) { // c_str() is same as (const char*)
        Logger::error("[ERROR] Could not resolve host: " + host);
        close(client_fd);
        return;
    }


    // 1. Try to connect to the real target server with fallback IPs
    struct addrinfo* p;
    int remote_fd = -1;
    int connected = 0;

    // Looping over LL
    for(p = res; p != NULL; p = p->ai_next) {
        // Create a new socket for this attempt
        remote_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if(remote_fd < 0) {
          Logger::error(
              "[WARN] Failed to create socket for this IP, trying next...");
          continue;
        }

        if(setsockopt(remote_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) < 0)) {
            Logger::error(
                "[WARN] Failed to set remote socket timeout");
        }

        // 2. Try to connect to this IP
        if(connect(remote_fd, p->ai_addr, p->ai_addrlen) == 0) {
          char ip_str[INET_ADDRSTRLEN];
          struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
          inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);
          Logger::info("[DNS RESOLVED & CONNECTED]: " + std::string(ip_str));
          connected = 1;
          break;
        }

        Logger::error("[WARN] Failed to connect to this IP, trying next...");
        close(remote_fd);
        remote_fd = -1;
    }

    freeaddrinfo(res);

    if(!connected || remote_fd < 0) {
      Logger::error("[ERROR] Failed to connect to any IP for host: " + host);
      close(client_fd);
      return;
    }

    Logger::info("[SUCCESS] Connected to target remote server!");

    // 3. Forward the browser's original request to the internet
    int sent = send(remote_fd, buffer, valread, 0);

    if(sent < 0) {
      Logger::error("[ERROR] Failed to send request to remote server.");
      close(remote_fd);
      freeaddrinfo(res);
      close(client_fd);
      return;
    }

        int remote_read;
        size_t totalBytes = 0;
        int chunks = 0;

        // 4. Relay the response back to the client
        while ((remote_read = recv(remote_fd, buffer, sizeof(buffer), 0)) > 0) {
            if(send(client_fd, buffer, remote_read, 0) < 0) {
                Logger::error("[ERROR] Failed to relay data to client.");
                break;
            }

            totalBytes += remote_read;
            chunks++;
        }

        if (remote_read == 0) {
            Logger::info(
            "[RELAY COMPLETE] Sent " +
            std::to_string(totalBytes) +
            " bytes in " +
            std::to_string(chunks) +
            " chunks."
            );
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            Logger::info(
            "[RELAY COMPLETE] Connection timed out after sending " +
            std::to_string(totalBytes) +
            " bytes in " +
            std::to_string(chunks) +
            " chunks."
            );
        }
        else {
            Logger::error(
            "[RELAY ERROR] recv() failed. errno = " +
            std::to_string(errno)
            );
        }

    // Always close your outbound descriptors
    close(remote_fd);

    // Free the memory allocated by getaddrinfo
    freeaddrinfo(res);

    // Cleanup: Close descriptor
    close(client_fd);
}

bool ProxyServer::start() {

    Logger::info("[PROXY] Initializing Proxy Server Engine...");
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        Logger::error("[ERROR] Failed to create socket system descriptor!");
        return false;
    }

    // for keeping a copy
    SignalHandler::registerHandler(server_fd);

    int opt = 1;
    // Forcefully binding socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::error("[ERROR]: Failed to enable SO_REUSEADDR!");
        return false;
    }

    // Allowing multiple sockets to bind to port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        Logger::error("[ERROR]: Failed to enable SO_REUSEPORT!");
        return false;
    }

    Logger::info("[SUCCESS] Socket descriptor created. FD Number: " + std::to_string(server_fd));


    // Define the server address structure
    struct sockaddr_in server_addr;               // We are manually specifying an IPv4 address and port
    memset(&server_addr, 0, sizeof(server_addr)); // Clear memory to avoid garbage data
    server_addr.sin_family = AF_INET;             // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;     // Listen on all network interfaces (localhost, Wi-Fi, etc.) -> deeper => I.P.
    server_addr.sin_port = htons(port);           // Convert 8080 to Network Byte Order


    // Bind the socket to the port
    // Assigns an address to the socket
    if(bind(server_fd, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0) { // bind accepts only sockaddr*
        Logger::error("[ERROR] Failed to bind to port " + std::to_string(port) + "!");
        return false;
    }

    Logger::info("[SUCCESS] Socket bound to port " + std::to_string(port));


    // Start listening (10 is the backlog size -> how many connections can wait in queue before refusal)
    if(listen(server_fd, 10) < 0) {
        Logger::error("[ERROR] Failed to start listening!");
        return false;
    }

    Logger::info("[INFO] Proxy server is now listening on port " + std::to_string(port) + "...");


    while(true) {
        Logger::info("[WAITING] Ready for connection...");
        // This will pause execution until a browser makes a request
        int client_fd = accept(server_fd, nullptr, nullptr); // nullptr -> ignore client's address information
        // client_fd is a new socket fd !!!
        if(client_fd < 0) {
           Logger::error("[ERROR] Failed to accept client connection!");
           continue;
        }

        Logger::info("[THREADING] Spawning worker thread for FD: " + std::to_string(client_fd));

        pool.enqueue([this, client_fd]() { // 'this' is used to use 'handleClient()'
            handleClient(client_fd);
        });
    }

    return true;
}
