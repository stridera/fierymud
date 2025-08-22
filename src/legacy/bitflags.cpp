#include "bitflags.hpp"

#include "logging.hpp"
#include "structs.hpp"
#include "utils.hpp"

#include <string_view>

std::string sprintbit(long bitvector, const std::string_view names[]) {
    std::string result;

    /* Assuming 8 bits to a byte... */
    for (long i = 0; names[i].data()[0] != '\n'; i++) {
        if (IS_SET(bitvector, (1 << i))) {
            result += names[i];
            result += ' ';
        }
    }

    if (result.empty())
        result = "NO BITS";
    else
        result.pop_back(); /* Remove the trailing space */

    return result;
}

//  template <std::size_t N> std::string sprinttype(int type, const std::array<std::string_view, N> &names) {
//      return std::string{names[type]};
//  }

std::string sprinttype(int type, const std::string_view names[]) {

    std::string result;
    int nr = 0;

    while (type && names[nr][0] != '\n') {
        type--;
        nr++;
    }

    if (names[nr] != "\n")
        result = names[nr];
    else {
        result = "UNDEFINED";
        log("SYSERR: Unknown type {} in sprinttype.", type);
    }
    return result;
}

//  template <std::size_t N> std::string sprintflag(flagvector flags[], const std::array<std::string_view, N> &names) {
//      std::string result;

//      for (int i = 0; i < names.size(); ++i) {
//          if (IS_FLAGGED(flags, i)) {
//              result += names[i];
//              result += ' ';
//          }
//      }

//      if (result.empty())
//          result = "NO FLAGS";
//      else
//          result.pop_back(); /* Remove the trailing space */

//      return result;
//  }

std::string sprintflag(flagvector flags[], int num_flags, const std::string_view names[]) {
    int nr = 0;
    std::string result;

    for (int i = 0; i < num_flags; ++i) {
        if (IS_FLAGGED(flags, i)) {
            if (names[nr] != "\n")
                result += names[nr];
            else
                result += "UNDEFINED";
            result += ' ';
        }
        if (names[nr][0] != '\n')
            ++nr;
    }

    if (result.empty())
        result = "NO FLAGS";
    else
        result.pop_back(); /* Remove the trailing space */

    return result;
}

std::string sprintascii(flagvector bits) {
    int i, j = 0;
    /* 32 bits, don't just add letters to try to get more unless flagvector is also as large. */
    const std::string_view flags = "abcdefghijklmnopqrstuvwxyzABCDEF";
    std::string out;

    for (i = 0; flags[i]; ++i)
        if (bits & (1 << i))
            out += flags[i];

    if (j == 0) /* Didn't write anything. */
        out += '0';

    return out;
}