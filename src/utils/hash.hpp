#pragma once
#include <string>

namespace kit::hash {
    // Returns lowercase hex SHA1 of data
    std::string sha1(const std::string& data);
} // namespace kit::hash
