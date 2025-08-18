#include "src/clan.hpp"
#include <iostream>
int main() {
    std::cout << "enum_name: " << magic_enum::enum_name(ClanPermission::KICK_MEMBERS) << std::endl;
    std::cout << "fmt::format result: " << fmt::format("You need the {} permission to do that.", magic_enum::enum_name(ClanPermission::KICK_MEMBERS)) << std::endl;
    return 0;
}
