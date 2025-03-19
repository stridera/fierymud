/***************************************************************************
 *   File: money.h                                        Part of FieryMUD *
 *  Usage: header file for money, I guess                                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998, 1999, 2000 by the Fiery Consortium        *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#pragma once

#include "structs.hpp"
#include "sysdep.hpp"

#define PLATINUM_SCALE 1000
#define GOLD_SCALE 100
#define SILVER_SCALE 10
#define COPPER_SCALE 1

constexpr int platinum_scale = 1000;
constexpr int gold_scale = 100;
constexpr int silver_scale = 10;
constexpr int copper_scale = 1;

class Money {
  public:
    Money() = default;
    Money(int platinum, int gold, int silver, int copper)
        : platinum_(platinum), gold_(gold), silver_(silver), copper_(copper) {}
    Money(const nlohmann::json &j) noexcept {
        platinum_ = j.value("platinum", 0);
        gold_ = j.value("gold", 0);
        silver_ = j.value("silver", 0);
        copper_ = j.value("copper", 0);
    }

    // Return the total value of the money in copper
    [[nodiscard]] int value() const noexcept {
        return platinum_ * platinum_scale + gold_ * gold_scale + silver_ * silver_scale + copper_ * copper_scale;
    }
    [[nodiscard]] bool is_zero() const noexcept { return platinum_ == 0 && gold_ == 0 && silver_ == 0 && copper_ == 0; }

    [[nodiscard]] int platinum() const noexcept { return platinum_; }
    [[nodiscard]] int gold() const noexcept { return gold_; }
    [[nodiscard]] int silver() const noexcept { return silver_; }
    [[nodiscard]] int copper() const noexcept { return copper_; }

    [[nodiscard]] bool can_afford(const Money &cost) const noexcept { return value() >= cost.value(); }
    [[nodiscard]] bool can_afford(int cost) const noexcept { return value() >= cost; }

    // Charge them for the cost, starting with the highest coin type
    [[nodiscard]] bool charge(int cost) noexcept {
        if (cost <= 0 || value() < cost) {
            return false;
        }

        int coin_scales[] = {platinum_scale, gold_scale, silver_scale, copper_scale};

        int remaining = cost;
        for (int i = 0; i < NUM_COIN_TYPES; ++i) {
            while (remaining >= coin_scales[i] && (*this)[i] > 0) {
                remaining -= coin_scales[i];
                (*this)[i]--;
            }
        }
        return true;
    }
    [[nodiscard]] bool charge(const Money &cost) noexcept { return charge(cost.value()); }

    // Operators for arithmetic
    Money operator+(const Money &other) const noexcept {
        return Money(platinum_ + other.platinum_, gold_ + other.gold_, silver_ + other.silver_,
                     copper_ + other.copper_);
    }
    Money operator+=(const Money &other) noexcept {
        platinum_ += other.platinum_;
        gold_ += other.gold_;
        silver_ += other.silver_;
        copper_ += other.copper_;
        return *this;
    }
    Money operator-(const Money &other) const noexcept {
        return Money(platinum_ - other.platinum_, gold_ - other.gold_, silver_ - other.silver_,
                     copper_ - other.copper_);
    }
    Money operator-=(const Money &other) noexcept {
        platinum_ -= other.platinum_;
        gold_ -= other.gold_;
        silver_ -= other.silver_;
        copper_ -= other.copper_;
        return *this;
    }
    Money operator*(int multiplier) const noexcept {
        return Money(platinum_ * multiplier, gold_ * multiplier, silver_ * multiplier, copper_ * multiplier);
    }
    Money operator*=(int multiplier) noexcept {
        platinum_ *= multiplier;
        gold_ *= multiplier;
        silver_ *= multiplier;
        copper_ *= multiplier;
        return *this;
    }
    Money operator/(int divisor) const noexcept {
        return Money(platinum_ / divisor, gold_ / divisor, silver_ / divisor, copper_ / divisor);
    }
    Money operator/=(int divisor) noexcept {
        platinum_ /= divisor;
        gold_ /= divisor;
        silver_ /= divisor;
        copper_ /= divisor;
        return *this;
    }

    // Get operator by index
    int operator[](int index) const noexcept {
        switch (index) {
        case PLATINUM:
            return platinum_;
        case GOLD:
            return gold_;
        case SILVER:
            return silver_;
        case COPPER:
            return copper_;
        default:
            return 0;
        }
    }
    // Set operator by index
    int &operator[](int index) noexcept {
        switch (index) {
        case PLATINUM:
            return platinum_;
        case GOLD:
            return gold_;
        case SILVER:
            return silver_;
        case COPPER:
            return copper_;
        default:
            throw std::out_of_range("Invalid coin type");
        }
    }
    // Comparison operators
    bool operator==(const Money &other) const noexcept {
        return platinum_ == other.platinum_ && gold_ == other.gold_ && silver_ == other.silver_ &&
               copper_ == other.copper_;
    }
    bool operator!=(const Money &other) const noexcept { return !(*this == other); }
    bool operator<(const Money &other) const noexcept {
        return platinum_ < other.platinum_ && gold_ < other.gold_ && silver_ < other.silver_ && copper_ < other.copper_;
    }
    bool operator<=(const Money &other) const noexcept { return *this < other || *this == other; }
    bool operator>(const Money &other) const noexcept { return !(*this <= other); }
    bool operator>=(const Money &other) const noexcept { return !(*this < other); }

    // Json serialization
    [[nodiscard]] nlohmann::json to_json() const noexcept {
        return nlohmann::json{{"platinum", platinum_}, {"gold", gold_}, {"silver", silver_}, {"copper", copper_}};
    }

  private:
    int platinum_{0};
    int gold_{0};
    int silver_{0};
    int copper_{0};
};

struct CoinDef {
    const char *name;
    const char *shortname;
    const char *initial;
    const char *color;
    const char *abbrev;
    int scale;
};

extern CoinDef coindefs[NUM_COIN_TYPES];

#define VALID_COIN(coin) (coin >= 0 && coin < NUM_COIN_TYPES)
#define COIN_NAME(coin) (VALID_COIN(coin) ? coindefs[coin].name : "coin")
#define COIN_SHORTNAME(coin) (VALID_COIN(coin) ? coindefs[coin].shortname : "c")
bool is_coin_name(const char *name, int cointype);
#define COIN_COLOR(coin) (VALID_COIN(coin) ? coindefs[coin].color : "&9&b")
#define COIN_INITIAL(coin) (VALID_COIN(coin) ? coindefs[coin].initial : "?")
#define COIN_SCALE(coin) (VALID_COIN(coin) ? coindefs[coin].scale : 1)

/* Old below */

#define PLATINUM_SCALE 1000
#define GOLD_SCALE 100
#define SILVER_SCALE 10
#define COPPER_SCALE 1

#define PLATINUM_PART(amt) ((amt) / PLATINUM_SCALE)
#define GOLD_PART(amt) (((amt) % PLATINUM_SCALE) / GOLD_SCALE)
#define SILVER_PART(amt) (((amt) % GOLD_SCALE) / SILVER_SCALE)
#define COPPER_PART(amt) (((amt) % SILVER_SCALE) / COPPER_SCALE)
#define CASH_VALUE(coins)                                                                                              \
    ((coins)[PLATINUM] * PLATINUM_SCALE + (coins)[GOLD] * GOLD_SCALE + (coins)[SILVER] * SILVER_SCALE +                \
     (coins)[COPPER] * COPPER_SCALE)

[[nodiscard]] std::string statemoney(const Money coins) noexcept;
[[nodiscard]] std::optional<Money> parse_money(std::string_view input) noexcept;
[[nodiscard]] std::string briefmoney(int spaces, int amt) noexcept;
[[nodiscard]] ObjData *create_money(const Money coins);
[[nodiscard]] int parse_coin_type(std::string_view name) noexcept;

// Charge a character an amount of money, returning true if successful.
[[nodiscard]] bool charge_char(CharData *ch, int amount) noexcept;