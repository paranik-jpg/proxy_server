#ifndef LOGGER_H
#define LOGGER_H

#include <string>

class Logger {
public:
    static void info(const std::string& msg);
    static void error(const std::string& msg);
};

#endif