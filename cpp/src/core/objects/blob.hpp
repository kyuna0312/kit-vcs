#pragma once
#include <string>

namespace kit {

struct Blob {
    std::string content;

    std::string serialize() const;   // "blob <size>\n<content>"
    std::string hash() const;        // SHA1 of serialize()
    static Blob deserialize(const std::string& raw);
    static Blob from_file(const std::string& path);
};

} // namespace kit
