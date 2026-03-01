#pragma once

#include "core/result.hpp"

enum class CommandResult;

namespace HousingCommands {

/** Register all housing commands with the command system. */
Result<void> register_commands();

} // namespace HousingCommands
