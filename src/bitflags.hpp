#include "structs.hpp"

std::string sprintbit(long vektor, const std::string_view names[]);
std::string sprinttype(int type, const std::string_view names[]);
std::string sprintflag(flagvector flags[], int num_flags, const std::string_view names[]);
std::string sprintascii(flagvector bits);