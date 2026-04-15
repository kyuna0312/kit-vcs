#include "logger.hpp"
#include <iostream>
#include <cstdlib>

namespace kit::logger {

static Level current_level = Level::INFO;

void set_level(Level l) { current_level = l; }
Level get_level() { return current_level; }

void init_from_env() {
    const char* env = std::getenv("KIT_LOG_LEVEL");
    if (!env) return;
    std::string val(env);
    if      (val == "DEBUG") set_level(Level::DEBUG);
    else if (val == "INFO")  set_level(Level::INFO);
    else if (val == "WARN")  set_level(Level::WARN);
    else if (val == "ERROR") set_level(Level::ERR);
}

void debug(const std::string& msg) {
    if (current_level <= Level::DEBUG)
        std::cout << "[kit debug] " << msg << "\n";
}
void info(const std::string& msg) {
    if (current_level <= Level::INFO)
        std::cout << "[kit] " << msg << "\n";
}
void warn(const std::string& msg) {
    if (current_level <= Level::WARN)
        std::cerr << "[kit warn] " << msg << "\n";
}
void error(const std::string& msg) {
    if (current_level <= Level::ERR)
        std::cerr << "[kit error] " << msg << "\n";
}

} // namespace kit::logger
