#include "HttpParser.h"

std::string HttpParser::extractURL(const std::string& request) {
    size_t pos1 = request.find(' ');
    size_t pos2 = request.find(' ', pos1 + 1);

    std::string url = "";
    if(pos1 != std::string::npos && pos2 != std::string::npos) {
        url = request.substr(pos1 + 1, pos2 - pos1 - 1);
    }
    return url;
}

// Gives Host => "localhost:8888" -> "localhost"
std::string HttpParser::extractHost(const std::string& request) {
    size_t host_pos = request.find("Host: ");
    std::string host = "";

    if(host_pos != std::string::npos) {
        size_t host_end = request.find("\r\n", host_pos);
        host = request.substr(host_pos + 6, host_end - (host_pos + 6));

        // STRIP THE PORT: If there's a ':', cut it off
        size_t colon_pos = host.find(':');
        if(colon_pos != std::string::npos) {
            host = host.substr(0, colon_pos);
        }
    }

    return host;
}

int HttpParser::extractPort(const std::string& request) {
    return 80;
}
