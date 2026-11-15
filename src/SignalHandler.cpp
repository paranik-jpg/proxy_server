#include "SignalHandler.h"
#include "Logger.h"

#include <csignal>

int SignalHandler::server_fd = -1;

void SignalHandler::registerHandler(int fd) {
    server_fd = fd;
    // Trigerred when Ctrl + C is pressed, SIGINT => Ctrl + C
    signal(SIGINT, SignalHandler::signalHandler); 
}

// OS auto assigns signum = SIGINT
void SignalHandler::signalHandler(int signum) {   
    Logger::info("\n\n[SHUTDOWN] Ctrl+C intercepted! Commencing cleanup...");

    if(server_fd != -1){
        Logger::info("[SHUTDOWN] Closing master socket on port 8888 cleanly.");
        close(server_fd);
    }

    Logger::info("[SHUTDOWN] Exiting process. Port 8888 freed instantly!");
    exit(signum);
}
