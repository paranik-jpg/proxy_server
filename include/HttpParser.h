#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include <string>

class HttpParser {
public:
    static std::string extractURL(const std::string& request);

    static std::string extractHost(const std::string& request);

    static int extractPort(const std::string& request);
    
};

#endif