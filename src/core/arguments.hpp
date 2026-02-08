#pragma once

#include <limits>
#include <optional>
#include <string>

class Arguments {
    std::string static_args_; // This maintains the life of the string
    std::string_view arg_;

  public:
    // Constructors
    // This constructor is used for strings that will be owned by the Arguments class.
    explicit Arguments(std::string argument) noexcept : static_args_(std::move(argument)), arg_(static_args_) {}
    // This is for strings not owned by the Arguments class.
    explicit Arguments(std::string_view argument) : arg_(argument) {}
    // This is for C-style strings.
    explicit Arguments(const char *argument) noexcept : arg_(argument) {}

    // std::string copy constructor
    Arguments(const std::string &argument) : arg_(argument) {}

    static constexpr int MAX_ITEMS = std::numeric_limits<int>::max();

    // Get the ramaining argument list.
    [[nodiscard]] std::string_view get() const;

    [[nodiscard]] bool empty() const { return get().empty(); }

    // When parsing commands, we want to allow things like ";hi" or ".gossip" to allow aliases for say/gossip, etc.
    // This command will return the single character if it's not alpha, and shift the argument list.
    // If strict is true, it will only return the single character if it's not alpha.  Otherwise, it will
    // return the first word or quoted string like shift().
    [[nodiscard]] std::string_view command_shift(bool strict = false);

    // Shift out one argument from the argument list.  This will return the first word or quoted string
    // and remove it from the argument list.
    [[nodiscard]] std::string_view shift();

    // Shift out one argument from the argument list, and replace any "$$" with "$".
    // This should follow the same rules as remove_double_dollars.
    [[nodiscard]] std::string shift_clean();

    // Try to shift out a number from the argument list.  If the first argument is a positive int, it will be shifted
    // and returned.  Otherwise, nothing will be shifted and an empty optional will be returned.
    [[nodiscard]] std::optional<int> try_shift_number();

    // Try to shift a number off the beginning of the next argument.  It will return a pair of the number and the
    // remaining argument list.  If the number portion is not there, it will default to 1 item.
    // If the number is 'all', it will return the maximum number of items.
    [[nodiscard]] std::optional<std::pair<int, std::string_view>> try_shift_number_and_arg();
};
