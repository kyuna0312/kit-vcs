#pragma once
#include <optional>
#include <string>

namespace kit {

template<typename T>
struct Result {
    std::optional<T> value;
    std::string error;
    bool ok() const { return value.has_value(); }
    static Result success(T v) { return {std::move(v), ""}; }
    static Result failure(std::string e) { return {std::nullopt, std::move(e)}; }
};

template<>
struct Result<void> {
    bool success;
    std::string error;
    bool ok() const { return success; }
    static Result ok_result() { return {true, ""}; }
    static Result failure(std::string e) { return {false, std::move(e)}; }
};

} // namespace kit
