// Minimal declarations necessary for nontrivial command headers

#pragma once

#include "core/result.hpp"
#include "database/generated/db_room.hpp"

// Silence spurious warnings in <functional> header
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <functional>
#pragma GCC diagnostic pop

/** Message formatting and delivery options */
enum class MessageType {
    Normal,   // Standard output
    Error,    // Error messages (red text)
    Success,  // Success messages (green text)
    Warning,  // Warning messages (yellow text)
    Info,     // Informational messages (blue text)
    System,   // System messages (cyan text)
    Debug,    // Debug messages (gray text)
    Combat,   // Combat-related messages
    Social,   // Social action messages
    Tell,     // Private tell messages
    Say,      // Say/speech messages
    Emote,    // Emote/action messages
    Channel,  // Channel communication
    Broadcast // System-wide broadcasts
};

class CommandContext;
enum class CommandResult;
enum class MessageType;
class Actor;
class Object;
class Room;

using Direction = db::Direction;

using CommandHandler = std::function<Result<CommandResult>(const CommandContext &context)>;
