#include "ids.hpp"

/**
 * Parse EntityId from string in format "zone:id" or just "id".
 * If only a single number is provided and default_zone is specified, uses that zone.
 * @param str String to parse (e.g., "30:45" or "45")
 * @param default_zone Optional zone to use if only local_id is provided
 * @return Parsed EntityId, or invalid EntityId if parsing fails
 */
EntityId EntityId::parse(std::string_view str, std::optional<std::uint32_t> default_zone) {
    auto colon_pos = str.find(':');
    if (colon_pos != std::string_view::npos) {
        // Full zone:id format
        try {
            int zone_id = std::stoi(std::string(str.substr(0, colon_pos)));
            int local_id = std::stoi(std::string(str.substr(colon_pos + 1)));
            if (zone_id >= 0 && local_id >= 0) {
                return EntityId(static_cast<std::uint32_t>(zone_id), static_cast<std::uint32_t>(local_id));
            }
        } catch (const std::exception &) {
            // Fall through to return invalid
        }
        return EntityId{};
    }

    // Single number format - use default zone if provided
    try {
        int local_id = std::stoi(std::string(str));
        if (local_id >= 0 && default_zone.has_value()) {
            return EntityId(*default_zone, static_cast<std::uint32_t>(local_id));
        }
    } catch (const std::exception &) {
        // Fall through to return invalid
    }
    return EntityId{};
}

std::string EntityId::to_string() const { return std::to_string(zone_id_) + ":" + std::to_string(local_id_); }

std::size_t EntityId::Hash::operator()(const EntityId &id) const noexcept {
    if (!id.valid_)
        return 0;
    // Combine hash of both components for better distribution
    std::size_t h1 = std::hash<std::uint32_t>{}(id.zone_id_);
    std::size_t h2 = std::hash<std::uint32_t>{}(id.local_id_);
    return h1 ^ (h2 << 1);
}
