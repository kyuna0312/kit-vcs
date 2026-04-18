#pragma once
#include <string>

namespace kit::logger {

enum class Level { DEBUG = 0, INFO = 1, WARN = 2, ERR = 3 };

void set_level(Level l);
Level get_level();
void init_from_env(); // reads KIT_LOG_LEVEL env var

void debug(const std::string& msg);
void info(const std::string& msg);
void warn(const std::string& msg);
void error(const std::string& msg); // writes to stderr

} // namespace kit::logger
