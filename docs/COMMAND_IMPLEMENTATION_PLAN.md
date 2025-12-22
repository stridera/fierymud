# Command Implementation Plan

## Overview

This document maps the disparity between legacy FieryMUD commands (~550 total) and modern implementation (~60), and provides a phased implementation plan.

## Current State Summary

| Category | Legacy | Modern | Gap |
|----------|--------|--------|-----|
| Movement | 6 | 7 | Complete |
| Information | 25+ | 10 | ~15 |
| Communication | 15+ | 6 | ~9 |
| Object/Inventory | 30+ | 17 | ~13 |
| Combat | 35+ | 5 | ~30 |
| Position | 10 | 0 | ~10 |
| Group | 12+ | 0 | ~12 |
| Skills/Abilities | 40+ | 0 | ~40 |
| Magic/Spells | 15+ | 1 | ~14 |
| Shop | 15+ | 3 | ~12 |
| Socials | 200+ | Dynamic | DB-driven |
| Immortal/Admin | 80+ | 10 | ~70 |
| Clan | 15+ | 7 | ~8 |
| Mount | 5+ | 0 | ~5 |
| House | 5+ | 0 | ~5 |
| Misc Player | 30+ | 3 | ~27 |

**Total Gap: ~270 core commands** (excluding socials which are database-driven)

---

## Phase 1: Core Player Experience (Priority: Critical)

Essential commands for basic gameplay. **Target: 45 commands**

### 1.1 Position Commands (10)
Commands that control character stance/position.

| Command | Description | Complexity |
|---------|-------------|------------|
| `sit` | Sit down | Low |
| `stand` | Stand up | Low |
| `rest` | Rest for recovery | Low |
| `sleep` | Sleep for faster recovery | Low |
| `wake` | Wake from sleep | Low |
| `kneel` | Kneel position | Low |
| `recline` | Recline position | Low |
| `fly` | Start flying | Medium |
| `land` | Stop flying | Medium |
| `mount` / `dismount` | Mount handling | Medium |

### 1.2 Additional Information (8)
| Command | Description | Complexity |
|---------|-------------|------------|
| `consider` | Evaluate target difficulty | Medium |
| `diagnose` | Check target health status | Low |
| `glance` | Quick look at target | Low |
| `credits` | Show game credits | Low |
| `motd` | Message of the day | Low |
| `news` | Show game news | Low |
| `policy` | Show game policies | Low |
| `version` | Show version info | Low |

### 1.3 Additional Communication (6)
| Command | Description | Complexity |
|---------|-------------|------------|
| `reply` | Reply to last tell | Low |
| `ask` | Ask someone a question | Low |
| `gtell` / `gsay` | Group communication | Medium |
| `ctell` | Clan tell | Medium |
| `petition` | Contact immortals | Low |
| `lasttells` | Show tell history | Low |

### 1.4 Object Manipulation (10)
| Command | Description | Complexity |
|---------|-------------|------------|
| `hold` | Hold item in off-hand | Low |
| `grab` | Grab/wield item | Low |
| `fill` | Fill container with liquid | Medium |
| `pour` | Pour liquid | Medium |
| `quaff` | Drink potion | Medium |
| `recite` | Use scroll | Medium |
| `use` | Use wand/staff | Medium |
| `junk` | Destroy item | Low |
| `donate` | Donate item | Low |
| `compare` | Compare two items | Medium |

### 1.5 Basic Aliases (11)
| Command | Alias For | Notes |
|---------|-----------|-------|
| `i` | `inventory` | Already exists |
| `eq` | `equipment` | Already exists |
| `l` | `look` | Already exists |
| `k` | `kill` | Combat shortcut |
| `c` | `cast` | Magic shortcut |
| `'` | `say` | Already exists |
| `;` | `gossip` | Channel shortcut |
| `sc` | `score` | Already exists |
| `exa` | `examine` | Already exists |
| `fl` | `flee` | Combat shortcut |
| `re` | `rest` | Position shortcut |

---

## Phase 2: Combat System (Priority: High)

Full combat command set. **Target: 35 commands**

### 2.1 Melee Combat (15)
| Command | Description | Class/Skill |
|---------|-------------|-------------|
| `bash` | Shield bash, stun | Warrior |
| `kick` | Kick attack | All fighters |
| `backstab` | Sneak attack from behind | Rogue |
| `rescue` | Save ally from combat | Warrior |
| `disarm` | Disarm opponent | Fighter |
| `trip` / `tripup` | Trip opponent | Fighter |
| `headbutt` | Headbutt attack | Warrior |
| `gouge` | Eye gouge | Rogue |
| `bodyslam` | Slam attack | Warrior |
| `throatcut` | Lethal sneak attack | Rogue |
| `corner` | Corner opponent | Rogue |
| `sweep` | Leg sweep | Monk |
| `roundhouse` | Spinning kick | Monk |
| `claw` | Claw attack | Shifter |
| `peck` | Peck attack | Shifter |

### 2.2 Combat Tactics (10)
| Command | Description | Complexity |
|---------|-------------|------------|
| `assist` | Join ally's combat | Medium |
| `disengage` | Stop fighting | Low |
| `retreat` | Tactical retreat | Medium |
| `guard` | Guard another player | Medium |
| `hitall` / `tantrum` | Attack all enemies | High |
| `berserk` | Enter berserk mode | Medium |
| `consider` | Check enemy strength | Low |
| `visible` | Cancel invisibility | Low |
| `doorbash` | Bash open doors | Medium |
| `stomp` | Ground stomp AoE | Medium |

### 2.3 Combat Support (10)
| Command | Description | Complexity |
|---------|-------------|------------|
| `bandage` | First aid healing | Medium |
| `first aid` | Alias for bandage | Low |
| `drag` | Drag unconscious ally | Medium |
| `lure` | Lure enemy | Medium |
| `taunt` | Draw enemy attention | Medium |
| `feign` | Feign death | Medium |
| `hide` | Hide from enemies | Medium |
| `sneak` | Move silently | Medium |
| `track` | Track target | Medium |
| `search` | Search for hidden | Medium |

---

## Phase 3: Group & Social Systems (Priority: High)

Group play and social interaction. **Target: 25 commands**

### 3.1 Group Commands (12)
| Command | Description | Complexity |
|---------|-------------|------------|
| `group` | Manage group membership | Medium |
| `follow` | Follow a leader | Medium |
| `order` | Order followers | Medium |
| `report` | Report health to group | Low |
| `greport` | Group report | Low |
| `split` | Split gold with group | Medium |
| `disband` | Disband group | Low |
| `gretreat` | Group retreat | Medium |
| `consent` | Consent to actions | Medium |
| `abandon` | Abandon followers | Low |
| `stay` | Order to stay | Low |
| `summon` | Summon group member | Medium |

### 3.2 Social Interaction (8)
| Command | Description | Complexity |
|---------|-------------|------------|
| `ignore` | Ignore player | Medium |
| `afk` | Set AFK status | Low |
| `title` | Set title | Low |
| `description` | Set description | Medium |
| `prompt` | Customize prompt | Medium |
| `alias` | Command aliases | High |
| `toggle` | Toggle options | Medium |
| `color` | Color settings | Medium |

### 3.3 Consent System (5)
| Command | Description | Complexity |
|---------|-------------|------------|
| `consent` | Give consent | Medium |
| `noconsent` | Revoke consent | Medium |
| `pk` | PK toggle | Medium |
| `peace` | Request peace | Low |
| `truce` | Accept truce | Low |

---

## Phase 4: Magic & Skills (Priority: High)

Spellcasting and skill systems. **Target: 30 commands**

### 4.1 Spellcasting (10)
| Command | Description | Complexity |
|---------|-------------|------------|
| `cast` | Cast a spell | High (exists) |
| `chant` | Alias for cast | Low |
| `memorize` | Memorize spells | High |
| `forget` | Forget spells | Medium |
| `spells` | List known spells | Medium |
| `study` | Study spellbook | Medium |
| `scribe` | Scribe scrolls | High |
| `pray` | Divine casting | Medium |
| `meditate` | Mana recovery | Medium |
| `concentrate` | Spell focus | Medium |

### 4.2 Skills Display (5)
| Command | Description | Complexity |
|---------|-------------|------------|
| `skills` | List skills | Medium |
| `practice` | Practice skills | High |
| `train` | Train attributes | High |
| `innate` | List innate abilities | Medium |
| `abilities` | List all abilities | Medium |

### 4.3 Special Abilities (15)
| Command | Description | Class |
|---------|-------------|-------|
| `layhands` | Paladin healing | Paladin |
| `turn` | Turn undead | Cleric |
| `perform` | Bard performance | Bard |
| `rage` | Barbarian rage | Barbarian |
| `shapechange` | Druid transform | Druid |
| `shadowstep` | Shadow teleport | Rogue |
| `springleap` | Leap ability | Monk |
| `roar` | Intimidate roar | Warrior |
| `electrify` | Electric touch | Various |
| `breathe` | Breath attack | Dragon |
| `rend` | Rend attack | Beast |
| `maul` | Maul attack | Beast |
| `impale` | Impale attack | Various |
| `camp` | Setup camp | All |
| `forage` | Find food/herbs | Ranger |

---

## Phase 5: Economy & Shops (Priority: Medium)

Shop and economy systems. **Target: 20 commands**

### 5.1 Shop Commands (12)
| Command | Description | Complexity |
|---------|-------------|------------|
| `list` | List shop items | Medium (exists) |
| `buy` | Purchase item | Medium (exists) |
| `sell` | Sell item | Medium (exists) |
| `value` | Appraise item | Medium |
| `identify` | Identify item | Medium |
| `repair` | Repair item | Medium |
| `haggle` | Negotiate price | Medium |
| `inspect` | Detailed item view | Low |
| `exchange` | Currency exchange | Medium |
| `check` | Check account | Low |
| `items` | Shop inventory | Low |
| `store` / `retrieve` | Storage | Medium |

### 5.2 Banking (5)
| Command | Description | Complexity |
|---------|-------------|------------|
| `deposit` | Deposit gold | Medium |
| `withdraw` | Withdraw gold | Medium |
| `balance` | Check balance | Low |
| `transfer` | Transfer funds | Medium |
| `statement` | Account history | Medium |

### 5.3 Mail System (3)
| Command | Description | Complexity |
|---------|-------------|------------|
| `mail` | Send mail | Medium |
| `receive` | Check mail | Medium |
| `postage` | Check mail cost | Low |

---

## Phase 6: Clan System Enhancement (Priority: Medium)

Expand clan functionality. **Target: 15 commands**

### 6.1 Clan Management (8)
| Command | Description | Complexity |
|---------|-------------|------------|
| `clan create` | Create clan | High |
| `clan disband` | Disband clan | Medium |
| `clan invite` | Invite member | Medium |
| `clan kick` | Remove member | Medium |
| `clan promote` | Promote member | Medium |
| `clan demote` | Demote member | Medium |
| `clan setmotd` | Set MOTD | Low |
| `clan setdesc` | Set description | Low |

### 6.2 Clan Resources (7)
| Command | Description | Complexity |
|---------|-------------|------------|
| `clan deposit` | Deposit to clan bank | Medium |
| `clan withdraw` | Withdraw from bank | Medium |
| `clan balance` | Check clan funds | Low |
| `clan chest` | Access clan storage | Medium |
| `clan wars` | War declarations | High |
| `clan ally` | Alliance management | Medium |
| `clan log` | View clan log | Medium |

---

## Phase 7: Immortal Commands (Priority: Low)

Administrative tools. **Target: 50 commands**

### 7.1 Player Management (15)
| Command | Description | Level |
|---------|-------------|-------|
| `advance` | Advance player level | Admin |
| `set` | Set player attributes | God |
| `restore` | Restore HP/mana | Immortal |
| `freeze` / `thaw` | Freeze player | God |
| `mute` | Mute player | God |
| `notitle` | Remove title | God |
| `pardon` | Pardon crimes | God |
| `force` | Force action | God |
| `dc` | Disconnect player | Immortal |
| `ban` / `unban` | Ban management | God |
| `jail` | Jail player | Immortal |
| `release` | Release from jail | Immortal |
| `xnames` | Banned names | Admin |
| `rename` | Rename player | Admin |
| `reroll` | Reroll stats | Admin |

### 7.2 World Control (15)
| Command | Description | Level |
|---------|-------------|-------|
| `goto` | Teleport to location | Immortal |
| `transfer` | Transfer player | God |
| `at` | Execute at location | Immortal |
| `load` | Load mob/obj | Builder |
| `purge` | Remove entities | Builder |
| `invis` | Toggle invisibility | Immortal |
| `echo` | Echo to room | Immortal |
| `gecho` | Global echo | God |
| `zreset` | Reset zone | Builder |
| `peace` | Stop all combat | God |
| `dig` | Create room | Builder |
| `zlock` | Lock zone | Builder |
| `weather` | Control weather | God |
| `time` | Control time | God |
| `reload` | Reload data | Coder |

### 7.3 Information (10)
| Command | Description | Level |
|---------|-------------|-------|
| `stat` | Statistics | Immortal |
| `vnum` | Find by vnum | Immortal |
| `where` | Locate entity | Immortal |
| `users` | Online users | Immortal |
| `last` | Last logins | God |
| `show` | Show info | Immortal |
| `syslog` | System log | Immortal |
| `uptime` | Server uptime | All |
| `wizlist` | Wizard list | All |
| `date` | Real date/time | All |

### 7.4 System (10)
| Command | Description | Level |
|---------|-------------|-------|
| `shutdown` | Shutdown server | Coder |
| `reboot` | Reboot server | Coder |
| `copyover` | Hot reboot | Coder |
| `wizlock` | Lock logins | Admin |
| `snoop` | Snoop player | God |
| `switch` | Control mob | God |
| `return` | Return from switch | God |
| `skillset` | Set skills | Admin |
| `poofin` / `poofout` | Set arrival/departure messages | Immortal |
| `wiznet` | Immortal channel | Immortal |

---

## Phase 8: Miscellaneous (Priority: Low)

Remaining commands. **Target: 30 commands**

### 8.1 Housing (5)
| Command | Description | Complexity |
|---------|-------------|------------|
| `house` | House management | High |
| `hcontrol` | House admin | High |
| `hlist` | List houses | Medium |
| `hbuild` | Build house | High |
| `hfurnish` | Furnish house | Medium |

### 8.2 Crafting (5)
| Command | Description | Complexity |
|---------|-------------|------------|
| `craft` | Craft items | High |
| `brew` | Brew potions | High |
| `forge` | Forge weapons | High |
| `enchant` | Enchant items | High |
| `extract` | Extract components | Medium |

### 8.3 Mounts (5)
| Command | Description | Complexity |
|---------|-------------|------------|
| `mount` | Mount creature | Medium |
| `dismount` | Dismount | Medium |
| `tame` | Tame wild mount | High |
| `stable` | Stable mount | Medium |
| `summon mount` | Call mount | Medium |

### 8.4 Music/Bard (5)
| Command | Description | Complexity |
|---------|-------------|------------|
| `play` | Play instrument | Medium |
| `sing` | Sing song | Medium |
| `compose` | Compose music | High |
| `songs` | List songs | Low |
| `music` | Music settings | Low |

### 8.5 Miscellaneous (10)
| Command | Description | Complexity |
|---------|-------------|------------|
| `trophy` | Trophy display | Medium |
| `subclass` | Choose subclass | High |
| `respec` | Respecialize | High |
| `bounty` | Bounty system | High |
| `quest` | Quest log | Medium |
| `achievements` | View achievements | Medium |
| `leaderboard` | Rankings | Medium |
| `calendar` | Event calendar | Medium |
| `bug` / `idea` / `typo` | Submit reports | Low |
| `clear` / `cls` | Clear screen | Low |

---

## Implementation Priority Matrix

```
CRITICAL (Phase 1):  Position, Basic Info, Basic Object
   |
HIGH (Phase 2-4):    Combat, Groups, Magic/Skills
   |
MEDIUM (Phase 5-6):  Economy, Clans
   |
LOW (Phase 7-8):     Immortal, Housing, Crafting
```

## Recommended Implementation Order

1. **Week 1-2**: Phase 1 (Core Player Experience)
   - Position commands (sit, stand, rest, sleep, wake)
   - Additional info (consider, diagnose, motd, news)
   - Object handling (hold, fill, pour, quaff)

2. **Week 3-4**: Phase 2 (Combat)
   - Basic combat (bash, kick, backstab)
   - Combat tactics (assist, retreat, guard)
   - Combat support (bandage, hide, sneak)

3. **Week 5-6**: Phase 3 (Groups & Social)
   - Group commands (group, follow, order)
   - Social features (ignore, afk, toggle)

4. **Week 7-8**: Phase 4 (Magic & Skills)
   - Spell management (memorize, spells, study)
   - Skill display (skills, practice)

5. **Week 9-10**: Phase 5 (Economy)
   - Shop enhancements (value, identify, repair)
   - Banking (deposit, withdraw, balance)

6. **Week 11-12**: Phase 6 (Clans)
   - Management (create, invite, kick)
   - Resources (deposit, chest, wars)

7. **Ongoing**: Phase 7-8 (Admin & Misc)
   - As needed for testing and administration

---

## Technical Considerations

### Command Registration Pattern
```cpp
Commands()
    .command("commandname", cmd_handler)
    .alias("shortcut")
    .category("Category")
    .privilege(PrivilegeLevel::Player)
    .min_position(Position::Standing)
    .description("Description")
    .build();
```

### Required Infrastructure
1. **Position System**: Track and enforce character positions
2. **Combat System**: Combat rounds, damage, effects
3. **Group System**: Group tracking and communication
4. **Skill/Spell System**: Skill checks, spell slots, cooldowns
5. **Economy System**: Currency, shops, banking
6. **Consent System**: PvP permissions

### Dependencies
- Phase 2 (Combat) requires Position System from Phase 1
- Phase 4 (Magic) requires combat targeting from Phase 2
- Phase 5 (Economy) requires object system from Phase 1
- Phase 6 (Clans) requires group concepts from Phase 3

---

## Metrics

| Phase | Commands | Estimated Effort | Priority |
|-------|----------|------------------|----------|
| 1 | 45 | 2 weeks | Critical |
| 2 | 35 | 2 weeks | High |
| 3 | 25 | 2 weeks | High |
| 4 | 30 | 2 weeks | High |
| 5 | 20 | 2 weeks | Medium |
| 6 | 15 | 1 week | Medium |
| 7 | 50 | 3 weeks | Low |
| 8 | 30 | 2 weeks | Low |
| **Total** | **250** | **16 weeks** | - |

---

## Notes

- Socials are database-driven and already implemented dynamically
- OLC commands (medit, oedit, redit, zedit, etc.) are excluded - separate system
- DG Script mob commands (m*) are excluded - scripting system
- Some legacy commands may be deprecated or consolidated
- Modern implementation may combine related commands into subcommands
