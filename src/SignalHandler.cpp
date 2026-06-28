#include "SignalHandler.h"
#include "Logger.h"

#include <csignal>

int SignalHandler::server_fd = -1;

void SignalHandler::registerHandler(int fd) {
    server_fd = fd;
    signal(SIGINT, SignalHandler::signalHandler);
}

void SignalHandler::signalHandler(int signum) {
    Logger::info("\n\n[SHUTDOWN] Ctrl+C intercepted! Commencing cleanup...");

    if(server_fd != -1){
        Logger::info("[SHUTDOWN] Closing master socket on port 8080 cleanly.");
        close(server_fd);
    }

    Logger::info("[SHUTDOWN] Exiting process. Port 8080 freed instantly!");
    exit(signum);
}