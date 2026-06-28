#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

class SignalHandler {
public:
    static void registerHandler(int server_fd);

private:
    static int server_fd;
    static void signalHandler(int signum);
};

#endif