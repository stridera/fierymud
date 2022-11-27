#include "mud.hpp"

#include <fmt/core.h>

int main(int argc, char **argv) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    std::string_view opt;
    for (const auto &arg : args) {
        if (arg == "-h" || arg == "--help") {
            fmt::print("Usage: {} [options]", argv[0]);
            return EXIT_SUCCESS;
        } else if (arg == "-v" || arg == "--version") {
            fmt::print("{} v{}", argv[0], VERSION);
        }

        if (opt.empty()) {
            if (arg.starts_with('--')) {
                opt = arg.substr(2);
            } else if (arg.starts_with('-')) {
                opt = arg.substr(1);
            }
        }
    }
}
Mud mud();
mud.run();
return EXIT_SUCCESS;
}
