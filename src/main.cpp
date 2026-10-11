#include "ProxyServer.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  int port = 8888; // Default port

    if (argc > 2) {
      std::cerr << "Usage: ./ProxyServer [port]\n";
      return 1;
    }

    if (argc == 2) {
        try{
          std::string port_arg = argv[1];
          std::size_t parsed_chars = 0;
          int parsed_port = std::stoi(port_arg, &parsed_chars);

          if (parsed_chars != port_arg.size() || parsed_port < 1 || parsed_port > 65535) {
            std::cerr << "Error: Invalid port. Use 1-65535.\n ";
            return 1;
          }

          port = parsed_port;
        } catch(const std::exception&) {
            std::cerr << "Error: Invalid port. Use 1-65535.\n ";
            return 1;
        }
    }

  ProxyServer server(port);

  if(!server.start()) {
    return 1;  // Abnormal termination OR Error Occured
  }

    return 0;                // Successful Execution
}
