#include <catch2/catch_test_macros.hpp>
#include "core/object.hpp"
#include "core/actor.hpp"
#include "commands/builtin_commands.hpp"

TEST_CASE("Object Description System", "[unit][descriptions]") {
    
    SECTION("Basic object description") {
        auto sword_result = Object::create(EntityId{1001}, "sword", ObjectType::Weapon);
        REQUIRE(sword_result.has_value());
        auto sword = std::shared_ptr<Object>(sword_result.value().release());
        
        sword->set_short_description("a sharp sword");
        sword->set_description("This is a well-crafted weapon with a gleaming blade.");
        
        // Create a dummy actor for the viewer parameter
        auto actor_result = Mobile::create(EntityId{2001}, "TestActor");
        REQUIRE(actor_result.has_value());
        auto actor = std::shared_ptr<Actor>(actor_result.value().release());
        
        std::string description = BuiltinCommands::Helpers::format_object_description(sword, actor);
        
        REQUIRE(description.find("a sharp sword") != std::string::npos);
        REQUIRE(description.find("This is a well-crafted weapon with a gleaming blade.") != std::string::npos);
    }

    SECTION("Container description with state") {
        auto chest_result = Container::create(EntityId{1002}, "chest", 10);
        REQUIRE(chest_result.has_value());
        auto chest_unique = std::move(chest_result.value());
        
        chest_unique->set_short_description("a wooden chest");
        chest_unique->set_description("A sturdy wooden chest sits here.");
        
        // Configure as closeable and lockable
        ContainerInfo info;
        info.capacity = 10;
        info.closeable = true;
        info.closed = true;
        info.lockable = true;
        info.locked = false;
        
        chest_unique->set_container_info(info);
        
        // Convert to shared_ptr<Object>
        auto chest = std::shared_ptr<Object>(chest_unique.release());
        auto actor_result = Mobile::create(EntityId{2002}, "TestActor");
        REQUIRE(actor_result.has_value());
        auto actor = std::shared_ptr<Actor>(actor_result.value().release());
        std::string description = BuiltinCommands::Helpers::format_object_description(chest, actor);
        
        REQUIRE(description.find("a wooden chest") != std::string::npos);
        REQUIRE(description.find("A sturdy wooden chest sits here.") != std::string::npos);
        REQUIRE(description.find("appears to be closed") != std::string::npos);
        REQUIRE(description.find("unlocked") != std::string::npos);
    }

    SECTION("Weapon description with damage") {
        auto weapon_result = Weapon::create(EntityId{1003}, "mace");
        REQUIRE(weapon_result.has_value());
        auto mace_unique = weapon_result.value().release();
        auto mace = std::shared_ptr<Object>(mace_unique);
        
        mace->set_short_description("a heavy mace");
        mace->set_description("A brutal weapon designed for crushing blows.");
        
        // Set damage profile
        DamageProfile damage;
        damage.dice_count = 2;
        damage.dice_sides = 6;
        damage.damage_bonus = 3;
        mace->set_damage_profile(damage);
        
        auto actor_result = Mobile::create(EntityId{2003}, "TestActor");
        REQUIRE(actor_result.has_value());
        auto actor = std::shared_ptr<Actor>(actor_result.value().release());
        std::string description = BuiltinCommands::Helpers::format_object_description(mace, actor);
        
        REQUIRE(description.find("a heavy mace") != std::string::npos);
        REQUIRE(description.find("A brutal weapon designed for crushing blows.") != std::string::npos);
        REQUIRE(description.find("Weapon damage: 2d6+3") != std::string::npos);
    }

    SECTION("Armor description with AC") {
        auto armor_result = Armor::create(EntityId{1004}, "chainmail");
        REQUIRE(armor_result.has_value());
        auto chainmail_unique = armor_result.value().release();
        auto chainmail = std::shared_ptr<Object>(chainmail_unique);
        
        chainmail->set_short_description("a suit of chainmail");
        chainmail->set_description("Fine steel rings woven into protective armor.");
        chainmail->set_armor_class(15);
        
        auto actor_result = Mobile::create(EntityId{2004}, "TestActor");
        REQUIRE(actor_result.has_value());
        auto actor = std::shared_ptr<Actor>(actor_result.value().release());
        std::string description = BuiltinCommands::Helpers::format_object_description(chainmail, actor);
        
        REQUIRE(description.find("a suit of chainmail") != std::string::npos);
        REQUIRE(description.find("Fine steel rings woven into protective armor.") != std::string::npos);
        REQUIRE(description.find("Armor class: 15") != std::string::npos);
    }

    SECTION("Object with extra descriptions") {
        auto obj_result = Object::create(EntityId{1005}, "statue", ObjectType::Other);
        REQUIRE(obj_result.has_value());
        auto statue = std::shared_ptr<Object>(obj_result.value().release());
        
        statue->set_short_description("a marble statue");
        statue->set_description("A beautifully carved marble statue stands here.");
        
        // Add extra descriptions
        ExtraDescription face_extra;
        face_extra.keywords = {"face", "expression"};
        face_extra.description = "The statue's face shows a serene, peaceful expression.";
        statue->add_extra_description(face_extra);
        
        ExtraDescription base_extra;
        base_extra.keywords = {"base", "pedestal"};
        base_extra.description = "The marble base is inscribed with ancient runes.";
        statue->add_extra_description(base_extra);
        
        auto actor_result = Mobile::create(EntityId{2005}, "TestActor");
        REQUIRE(actor_result.has_value());
        auto actor = std::shared_ptr<Actor>(actor_result.value().release());
        std::string description = BuiltinCommands::Helpers::format_object_description(statue, actor);
        
        REQUIRE(description.find("a marble statue") != std::string::npos);
        REQUIRE(description.find("A beautifully carved marble statue stands here.") != std::string::npos);
        REQUIRE(description.find("You notice some details you could examine more closely:") != std::string::npos);
        REQUIRE(description.find("face") != std::string::npos);
        REQUIRE(description.find("base") != std::string::npos);
    }

    SECTION("Weight and value display") {
        auto obj_result = Object::create(EntityId{1006}, "gem", ObjectType::Treasure);
        REQUIRE(obj_result.has_value());
        auto gem = std::shared_ptr<Object>(obj_result.value().release());
        
        gem->set_short_description("a precious gem");
        gem->set_description("A sparkling gemstone catches the light.");
        gem->set_weight(1);
        gem->set_value(500);
        
        auto actor_result = Mobile::create(EntityId{2006}, "TestActor");
        REQUIRE(actor_result.has_value());
        auto actor = std::shared_ptr<Actor>(actor_result.value().release());
        std::string description = BuiltinCommands::Helpers::format_object_description(gem, actor);
        
        REQUIRE(description.find("It weighs 1 pounds") != std::string::npos);
        REQUIRE(description.find("worth 500 gold coins") != std::string::npos);
    }
}