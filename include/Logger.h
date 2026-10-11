#ifndef LOGGER_H
#define LOGGER_H

#include <string>

#ifndef ENABLE_INFO_LOGGING
#define ENABLE_INFO_LOGGING 1
#endif

class Logger {
public:
    // static => We can call them even w/o creating objects, but its not preferred !!! (logger. v/s Logger::)
    static void info(const std::string& msg);
    static void error(const std::string& msg);
};

#endif
