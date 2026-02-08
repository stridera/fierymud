#include "composer_system.hpp"

#include <fmt/format.h>

#include "core/logging.hpp"
#include "core/player.hpp"

ComposerSystem::ComposerSystem(std::weak_ptr<Player> player, ComposerConfig config)
    : player_(std::move(player)), config_(std::move(config)) {}

void ComposerSystem::start() {
    if (state_ != ComposerState::Inactive) {
        Log::warn("ComposerSystem::start() called when already active");
        return;
    }

    state_ = ComposerState::Composing;
    lines_.clear();

    // Send header and initial help
    send_message(config_.header_message);
    send_message("(Type ~h for help)");
    send_prompt();
}

void ComposerSystem::process_input(std::string_view input) {
    if (state_ != ComposerState::Composing) {
        Log::warn("ComposerSystem::process_input() called when not composing");
        return;
    }

    // Check for special commands
    if (input == ".") {
        cmd_save();
        return;
    }

    if (input == "~q" || input == "~Q") {
        cmd_cancel();
        return;
    }

    if (input == "~p" || input == "~P") {
        cmd_preview();
        return;
    }

    if (input == "~c" || input == "~C") {
        cmd_clear();
        return;
    }

    if (input == "~h" || input == "~H" || input == "~?") {
        cmd_help();
        return;
    }

    // Regular text input - add to buffer
    if (lines_.size() >= static_cast<size_t>(config_.max_lines)) {
        send_message(fmt::format("Maximum of {} lines reached. Use . to save or ~c to clear.", config_.max_lines));
        send_prompt();
        return;
    }

    // Truncate overly long lines
    if (input.size() > static_cast<size_t>(config_.max_line_length)) {
        send_message(fmt::format("Line truncated to {} characters.", config_.max_line_length));
        lines_.push_back(std::string(input.substr(0, config_.max_line_length)));
    } else {
        lines_.push_back(std::string(input));
    }

    send_prompt();
}

void ComposerSystem::cmd_save() { finish(true); }

void ComposerSystem::cmd_cancel() {
    send_message(config_.cancel_message);
    finish(false);
}

void ComposerSystem::cmd_preview() {
    if (lines_.empty()) {
        send_message("(No text entered yet)");
    } else {
        send_message("--- Preview ---");
        for (size_t i = 0; i < lines_.size(); ++i) {
            send_message(fmt::format("{:3}: {}", i + 1, lines_[i]));
        }
        send_message(fmt::format("--- {} line{} ---", lines_.size(), lines_.size() == 1 ? "" : "s"));
    }
    send_prompt();
}

void ComposerSystem::cmd_clear() {
    lines_.clear();
    send_message("Text cleared. Start fresh.");
    send_prompt();
}

void ComposerSystem::cmd_help() {
    send_message("Composer Commands:");
    send_message("  .   - Save and finish");
    send_message("  ~q  - Cancel and discard all text");
    send_message("  ~p  - Preview current text");
    send_message("  ~c  - Clear all text and start over");
    send_message("  ~h  - Show this help");
    send_message("");
    send_message("Any other input adds a new line to your text.");
    send_prompt();
}

void ComposerSystem::send_message(std::string_view message) {
    if (auto player = player_.lock()) {
        player->send_message(message);
    }
}

void ComposerSystem::send_prompt() {
    if (auto player = player_.lock()) {
        player->send_message(config_.prompt);
    }
}

void ComposerSystem::finish(bool success) {
    if (success) {
        state_ = ComposerState::Completed;
        send_message(config_.save_message);
    } else {
        state_ = ComposerState::Cancelled;
    }

    // Build result
    ComposerResult result;
    result.success = success;
    result.lines = lines_;
    result.combined_text = build_combined_text();

    // Invoke callback if set
    if (completion_callback_) {
        completion_callback_(result);
    }
}

std::string ComposerSystem::build_combined_text() const {
    if (lines_.empty()) {
        return "";
    }

    std::string result;
    for (size_t i = 0; i < lines_.size(); ++i) {
        if (i > 0) {
            result += '\n';
        }
        result += lines_[i];
    }
    return result;
}
