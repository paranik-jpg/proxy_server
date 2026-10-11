#include "HttpParser.h"

namespace {
    std::string getHostHeader(const std::string& request) {
      size_t line_start = request.find('\n');

      if (line_start == std::string::npos) {
        return "";
      }

      ++line_start;

      while (line_start < request.size()) {
        size_t line_end = request.find('\n', line_start);

        std::string line = request.substr(
            line_start,
            line_end == std::string::npos
                ? std::string::npos
                : line_end - line_start
        );

        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }

        if (line.empty()) {
          break;
        }

        size_t colon = line.find(':');

        if(colon != std::string::npos) {
            std::string name = line.substr(0, colon);

            std::transform(name.begin(), name.end(), name.begin(),
                           [](unsigned char c) {
                             return static_cast<char>(std::tolower(c));
                           });

            if (name == "host") {
              std::string value = line.substr(colon + 1);

              size_t start = value.find_first_not_of(" \t");
              size_t end = value.find_last_not_of(" \t");

              if (start == std::string::npos) {
                return "";
              }

              return value.substr(start, end - start + 1);
            }
        }

        if (line_end == std::string::npos) {
          break;
        }

        line_start = line_end + 1;
      }

      return "";
    }

    int parsePort(const std::string& value) {
        if (value.empty()) {
          return 80;
        }

        int port = 0;

        for (unsigned char c : value) {
            if (!std::isdigit(c)) {
              return 80;
            }

            port = port * 10 + (c - '0');

            if (port > 65535) {
              return 80;
            }
        }

        return (port >= 1) ? port : 80;
    }

    std::string getAbsoluteURI(const std::string& request) {
      size_t first_space = request.find(' ');

      if (first_space == std::string::npos) {
        return "";
      }

      size_t second_space = request.find(' ', first_space + 1);

      if (second_space == std::string::npos) {
        return "";
      }

      std::string target =
          request.substr(first_space + 1, second_space - first_space - 1);

      if (target.rfind("http://", 0) == 0 || target.rfind("https://", 0) == 0) {
        return target;
      }

      return "";
    }

    std::string getAbsoluteAuthority(const std::string& request) {
        std::string uri = getAbsoluteURI(request);

        if (uri.empty()) {
          return "";
        }

        size_t scheme_end = uri.find("://");
        size_t authority_start = scheme_end + 3;

        size_t authority_end = uri.find_first_of("/?#", authority_start);

        return uri.substr(
            authority_start,
            authority_end == std::string::npos
                ? std::string::npos
                : authority_end - authority_start
        );
    }
}

std::string HttpParser::extractURL(const std::string& request) {
    size_t pos1 = request.find(' ');
    size_t pos2 = request.find(' ', pos1 + 1);

    if(pos1 == std::string::npos || pos2 == std::string::npos) {
      return "";
    }

    return request.substr(pos1 + 1, pos2 - pos1 - 1);
}

// Gives Host => "localhost:8888" -> "localhost"
std::string HttpParser::extractHost(const std::string& request) {
  std::string authority = getAbsoluteAuthority(request);

  if (authority.empty()) {
    authority = getHostHeader(request);
  }

  // IPv6: [::1]:3000 -> ::1
  if(!authority.empty() && authority.front() == '[') {
    size_t closing_bracket = authority.find(']');

    if (closing_bracket != std::string::npos) {
      return authority.substr(1, closing_bracket - 1);
    }

    return authority;
  }

  // Strip a port only when there is exactly one colon.
  // Multiple colons can indicate unbracketed IPv6 address.
  size_t colon = authority.find(':');

  if (colon != std::string::npos && authority.find(':', colon + 1) == std::string::npos) {
    return authority.substr(0, colon);
  }

  return authority;
}

int HttpParser::extractPort(const std::string& request) {
  std::string uri = getAbsoluteURI(request);
  std::string authority = getAbsoluteAuthority(request);

  if (!uri.empty()) {
    // Absolute-form: default port depends on the scheme.
    if (authority.empty()) {
      return 80;
    }

    size_t scheme_end = uri.find("://");
    std::string scheme = uri.substr(0, scheme_end);

    int default_port = (scheme == "https") ? 443 : 80;
    // Reuse your existing IPv6 and hostname port-parsing logic,
    // but use default_port instead of 80 when no port is present.
  } else {
    authority = getHostHeader(request);
  }

  // IPv6: [::1]:3000
  if(!authority.empty() && authority.front() == '[') {
    size_t closing_bracket = authority.find(']');

    if (closing_bracket == std::string::npos) {
      return 80;
    }

    if(closing_bracket + 1 < authority.size() &&
       authority[closing_bracket + 1] == ':') {
      return parsePort(authority.substr(closing_bracket + 2));
    }

    return 80;
  }

  // Ordinary hostnome or IPv4: host:port
  size_t colon = authority.find(':');

  if (colon != std::string::npos && authority.find(':', colon + 1) == std::string::npos) {
    return parsePort(authority.substr(colon + 1));
  }

  return 80;
}
