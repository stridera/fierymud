# Clan Scripting Features

This document describes the clan-related variables and functions available in DG Scripts for accessing clan information from scripts attached to mobs, objects, and rooms.

## Available Clan Variables

All clan variables are accessed through character fields using the syntax `%character.field%`. If the character is not in a clan, most fields return empty strings or "0" as appropriate.

### Basic Clan Information

| Variable | Type | Description | Non-clan Return |
|----------|------|-------------|-----------------|
| `%actor.clan%` | string | Clan name | empty string |
| `%actor.clan_rank%` | string | Character's rank title | empty string |
| `%actor.clan_id%` | number | Unique clan ID | "0" |
| `%actor.clan_abbr%` | string | Clan abbreviation | empty string |
| `%actor.clan_description%` | string | Clan description | empty string |
| `%actor.clan_motd%` | string | Clan message of the day | empty string |

### Clan Settings

| Variable | Type | Description | Non-clan Return |
|----------|------|-------------|-----------------|
| `%actor.clan_dues%` | number | Monthly dues amount (platinum) | "0" |
| `%actor.clan_app_fee%` | number | Application fee (platinum) | "0" |
| `%actor.clan_min_level%` | number | Minimum level to apply | "0" |
| `%actor.clan_member_count%` | number | Total number of clan members | "0" |

### Clan Treasury

| Variable | Type | Description | Non-clan Return |
|----------|------|-------------|-----------------|
| `%actor.clan_treasure_total%` | number | Total clan wealth (all coins) | "0" |
| `%actor.clan_treasure_platinum%` | number | Platinum coins in treasury | "0" |
| `%actor.clan_treasure_gold%` | number | Gold coins in treasury | "0" |
| `%actor.clan_treasure_silver%` | number | Silver coins in treasury | "0" |
| `%actor.clan_treasure_copper%` | number | Copper coins in treasury | "0" |

### Clan Facilities

| Variable | Type | Description | Non-clan Return |
|----------|------|-------------|-----------------|
| `%actor.clan_bank_room%` | number | Bank room vnum (-1 if disabled) | "-1" |
| `%actor.clan_chest_room%` | number | Chest room vnum (-1 if disabled) | "-1" |

### Clan Permissions

| Variable | Type | Description | Non-clan Return |
|----------|------|-------------|-----------------|
| `%actor.clan_can_deposit%` | boolean | Can deposit to clan bank (1/0) | "0" |
| `%actor.clan_can_withdraw%` | boolean | Can withdraw from clan bank (1/0) | "0" |
| `%actor.clan_can_store%` | boolean | Can store items in clan chest (1/0) | "0" |
| `%actor.clan_can_retrieve%` | boolean | Can retrieve items from clan chest (1/0) | "0" |

## Script Examples

### Example 1: Clan Welcome Message

```
* Trigger: Greet (100) - Entry trigger on mob
* Check if player has a clan and welcome them appropriately

if %actor.clan%
  say Welcome, %actor.clan_rank% %actor.name% of %actor.clan%!
  say Your clan has %actor.clan_member_count% members.
  if %actor.clan_treasure_total% > 1000
    say Your clan's treasury is doing well with %actor.clan_treasure_total% coins!
  else
    say Perhaps your clan could use some more funds...
  end
else
  say Greetings, clanless wanderer.
  say Consider joining one of our fine guilds!
end
```

### Example 2: Clan Bank Guard

```
* Trigger: Command (100) - Command trigger on guard mob
* Commands: deposit withdraw bank

set room_vnum %actor.clan_bank_room%

if %room_vnum% == -1
  say Your clan's bank access has been disabled.
  halt
end

if %actor.in_room% != %room_vnum%
  say You can only access your clan bank from room %room_vnum%.
  halt
end

if %cmd% == deposit
  if %actor.clan_can_deposit%
    say You may proceed with your deposit.
  else
    say You don't have permission to deposit funds.
  end
elseif %cmd% == withdraw
  if %actor.clan_can_withdraw%
    say You may proceed with your withdrawal.
  else
    say You don't have permission to withdraw funds.
  end
else
  say Use 'deposit' or 'withdraw' to access your clan bank.
end
```

### Example 3: Clan Information Board

```
* Trigger: Command (100) - Command trigger on object
* Commands: read info

if %actor.clan%
  %echo% &Y--- %actor.clan% Information ---&n
  %echo% &WClan:&n %actor.clan% (%actor.clan_abbr%)
  %echo% &WYour Rank:&n %actor.clan_rank%
  %echo% &WDescription:&n %actor.clan_description%
  %echo% &WMembers:&n %actor.clan_member_count%
  %echo% &WDues:&n %actor.clan_dues% platinum per month
  %echo% &WApplication Fee:&n %actor.clan_app_fee% platinum
  %echo% &WMinimum Level:&n %actor.clan_min_level%
  %echo% &WTreasury:&n %actor.clan_treasure_platinum%p %actor.clan_treasure_gold%g %actor.clan_treasure_silver%s %actor.clan_treasure_copper%c
  
  if %actor.clan_bank_room% != -1
    %echo% &WBank Room:&n %actor.clan_bank_room%
  else
    %echo% &RBank access is disabled&n
  end
  
  if %actor.clan_chest_room% != -1
    %echo% &WChest Room:&n %actor.clan_chest_room%
  else
    %echo% &RChest access is disabled&n
  end
  
  %echo% &WPermissions:&n
  if %actor.clan_can_deposit%
    %echo%   - Can deposit to bank
  end
  if %actor.clan_can_withdraw%
    %echo%   - Can withdraw from bank
  end
  if %actor.clan_can_store%
    %echo%   - Can store items in chest
  end
  if %actor.clan_can_retrieve%
    %echo%   - Can retrieve items from chest
  end
  
  if %actor.clan_motd%
    %echo% &YMOTD:&n %actor.clan_motd%
  end
else
  %echo% You are not a member of any clan.
end
```

### Example 4: Clan Recruitment Officer

```
* Trigger: Speech (100) - Speech trigger on mob
* Speech: join apply clan

if %actor.clan%
  say You're already a member of %actor.clan%!
  halt
end

* List available clans with their requirements
say Here are the available clans:

* This would need to be customized for each clan in your MUD
say The Warriors Guild requires level 10 and costs 100 platinum to join.
say The Merchants Guild requires level 5 and costs 50 platinum to join.
say The Mages Circle requires level 15 and costs 200 platinum to join.

say Use 'clan apply <clanname>' to submit an application.
```

### Example 5: Conditional Clan Facilities

```
* Trigger: Entry (100) - Room entry trigger
* Only allow clan members into their bank room

if %actor.clan%
  set required_room %actor.clan_bank_room%
  if %required_room% == %self.vnum%
    %echo% %actor.name% enters the %actor.clan% bank.
    %echoaround% %actor% %actor.name% shows their %actor.clan% credentials.
  elseif %required_room% != -1
    %echo% This is not your clan's designated bank room.
    %echo% Your clan's bank is located in room %required_room%.
    %teleport% %actor% %required_room%
  else
    %echo% Your clan's bank access has been disabled.
    %teleport% %actor% 3001
  end
else
  %echo% Only clan members may enter this area.
  %teleport% %actor% 3001
end
```

## Notes

- All clan variables are read-only from scripts
- Characters can only be members of one clan at a time (first membership is used)
- NPCs are never considered clan members
- Permission checks use the character's current rank and individual permissions
- Room vnums of -1 indicate disabled facilities
- Treasury values are returned as integers (coin amounts)
- Boolean permissions return "1" for true, "0" for false

## Integration with Existing Systems

These clan variables integrate seamlessly with the existing DG scripting system and can be used in:

- Mobile triggers (entry, greet, command, speech, etc.)
- Object triggers (command, get, drop, etc.)  
- Room triggers (entry, command, etc.)
- Any context where character variables are available

The implementation follows the existing variable naming conventions and error handling patterns used throughout the DG scripting system.