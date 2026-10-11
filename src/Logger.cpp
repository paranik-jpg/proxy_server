#include "Logger.h"

#include <mutex>
#include <iostream>

static std::mutex print_mutex;

void Logger::info(const std::string& msg) {
#if ENABLE_INFO_LOGGING
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cout << msg << '\n'; // '\n' avoids unnecessary flushes, std::endl auto flushes
#endif
}

void Logger::error(const std::string& msg) {
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cerr << msg << '\n';
}
