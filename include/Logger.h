#ifndef LOGGER_H
#define LOGGER_H

#include <string>

#ifndef ENABLE_INFO_LOGGING
#define ENABLE_INFO_LOGGING 1
#endif

class Logger {
public:
    // Static => Can be accessed without creating an object using ClassName::member. 
    // Accessing through an object (object.member) is also possible but not preferred.
    static void info(const std::string& msg);
    static void error(const std::string& msg);
};

#endif
