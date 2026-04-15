#include "cli.hpp"
#include "commands/cmd_init.hpp"
#include "commands/cmd_add.hpp"
#include "commands/cmd_commit.hpp"
#include "commands/cmd_status.hpp"
#include "commands/cmd_log.hpp"
#include "commands/cmd_branch.hpp"
#include "commands/cmd_checkout.hpp"
#include "commands/cmd_diff.hpp"
#include "commands/cmd_merge.hpp"
#include "commands/cmd_reset.hpp"
#include "commands/cmd_stash.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <string>

namespace kit::cli {

int run(int argc, char** argv) {
    logger::init_from_env();

    if (argc < 2) {
        std::cout << "kit version 2.0.0\nRun 'kit help' for usage.\n";
        return 0;
    }

    std::string cmd = argv[1];

    try {
        if      (cmd == "init")     return cmd_init::run(argc - 1, argv + 1);
        else if (cmd == "add")      return cmd_add::run(argc - 1, argv + 1);
        else if (cmd == "commit")   return cmd_commit::run(argc - 1, argv + 1);
        else if (cmd == "status")   return cmd_status::run(argc - 1, argv + 1);
        else if (cmd == "log")      return cmd_log::run(argc - 1, argv + 1);
        else if (cmd == "branch")   return cmd_branch::run(argc - 1, argv + 1);
        else if (cmd == "checkout") return cmd_checkout::run(argc - 1, argv + 1);
        else if (cmd == "diff")     return cmd_diff::run(argc - 1, argv + 1);
        else if (cmd == "merge")    return cmd_merge::run(argc - 1, argv + 1);
        else if (cmd == "reset")    return cmd_reset::run(argc - 1, argv + 1);
        else if (cmd == "stash")    return cmd_stash::run(argc - 1, argv + 1);
        else if (cmd == "help" || cmd == "--help" || cmd == "-h") {
            std::cout << R"(
Usage: kit <command> [options]

Commands:
  init                               Initialize a new repository
  add <file>...                      Stage files
  commit -m <message>                Commit staged files
  status                             Show working directory status
  log                                Show commit history
  branch [name]                      List or create branch
  branch -d <name>                   Delete branch
  checkout <branch>                  Switch branch
  diff                               Diff working dir vs HEAD
  merge <branch>                     Merge branch into current
  reset [--soft|--mixed|--hard] <h>  Reset HEAD
  stash                              Stash changes
  stash pop                          Restore stash
)" << "\n";
            return 0;
        } else {
            logger::error("Unknown command: " + cmd + ". Run 'kit help'.");
            return 1;
        }
    } catch (const std::exception& e) {
        logger::error(e.what());
        return 1;
    }
}

} // namespace kit::cli
