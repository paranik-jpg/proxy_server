cmake_minimum_required(VERSION 3.16)

project(ProxyServer VERSION 1.0 LANGUAGES CXX)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ProxyServer
    src/main.cpp
    src/ProxyServer.cpp
    src/ThreadPool.cpp
    src/Logger.cpp
    src/SignalHandler.cpp
    src/HttpParser.cpp
)

target_link_libraries(ProxyServer PRIVATE Threads::Threads)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang")
    target_compile_options(ProxyServer PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(ENABLE_SANITIZERS)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang")
        target_compile_options(ProxyServer PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )

        target_link_options(ProxyServer PRIVATE
            -fsanitize=address,undefined
        )
    else()
        message(FATAL_ERROR "Sanitizers are supported here only with GCC or Clang")
    endif()
endif()

target_include_directories(ProxyServer PRIVATE include)

# Linux networking uses POSIX sockets.
# No extra libraries are needed on modern Linux.
