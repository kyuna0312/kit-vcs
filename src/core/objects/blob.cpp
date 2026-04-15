#include "blob.hpp"
#include "utils/hash.hpp"
#include "utils/fs_utils.hpp"
#include <stdexcept>

namespace kit {

std::string Blob::serialize() const {
    return "blob " + std::to_string(content.size()) + "\n" + content;
}

std::string Blob::hash() const {
    return kit::hash::sha1(serialize());
}

Blob Blob::deserialize(const std::string& raw) {
    auto nl = raw.find('\n');
    if (nl == std::string::npos)
        throw std::runtime_error("Invalid blob: missing newline");
    return Blob{raw.substr(nl + 1)};
}

Blob Blob::from_file(const std::string& path) {
    return Blob{kit::fs::read_file(path)};
}

} // namespace kit
