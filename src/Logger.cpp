#include "Logger.h"

#include <mutex>
#include <iostream>

static std::mutex print_mutex;

void Logger::info(const std::string& msg) {
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cout << msg << std::endl;
}

void Logger::error(const std::string& msg) {
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cerr << msg << std::endl;
}
