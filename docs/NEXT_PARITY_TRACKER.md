# FieryMUD Next Parity Tracker

Tracking feature parity between legacy (main branch) and next branch.
Last updated: 2026-02-27

## Command Parity Summary

| Category | Legacy | Next | Status |
|----------|--------|------|--------|
| Socials | 192 | 194 (from DB) | Parity (only `hi5`, `screw` missing) |
| OLC Commands | 17 | 0 | Intentionally removed (Muditor) |
| DG Script (m\*) | ~25 | 0 | Replaced by Lua triggers |
| Admin/Wizard | ~37 | Partial | Most replaced by MCP/API |
| Core Gameplay | ~100 | ~275 (incl aliases) | Near parity |

---

## Missing Player Commands

### Combat Skills

| Command | Legacy Handler | Description | Priority | Status |
|---------|---------------|-------------|----------|--------|
| `bandage` | do_bandage | First aid / wound treatment | High | DONE |
| `berserk` | do_berserk | Berserker rage ability | High | DONE |
| `breathe` | do_breathe | Dragon breath attacks | High | DONE |
| `buck` | do_buck | Mounted combat - throw rider | Medium | DONE |
| `cartwheel` | do_cartwheel | Evasive combat maneuver | Medium | DONE |
| `claw` | do_claw | Claw attack (shapeshift form) | High | DONE |
| `disarm` | do_disarm | Disarm opponent's weapon | High | DONE |
| `disengage` | do_disengage | Leave combat gracefully | High | DONE |
| `doorbash` | do_doorbash | Break down doors | Medium | DONE |
| `electrify` | do_electrify | Electric shield ability | Medium | DONE |
| `gouge` | do_eye_gouge | Eye gouge attack | Medium | DONE |
| `guard` | do_guard | Guard/protect another player | High | DONE |
| `hitall` | do_hitall | Attack all enemies in room | High | DONE |
| `layhands` | do_layhand | Paladin healing ability | High | DONE |
| `rend` | do_rend | Tearing attack | Medium | DONE |
| `retreat` | do_retreat | Directional flee | High | DONE |
| `roar` | do_roar | Intimidation ability | Medium | DONE |
| `roundhouse` | do_roundhouse | Roundhouse kick | Medium | DONE |
| `springleap` | do_springleap | Monk spring attack | Medium | DONE |
| `stomp` | do_stomp | Stomp attack | Medium | DONE |
| `sweep` | do_sweep | Leg sweep | Medium | DONE |
| `throatcut` | do_throatcut | Throat cut assassination | High | DONE |
| `tripup` | do_tripup | Trip opponent | Medium | DONE |

### Utility / Movement Skills

| Command | Legacy Handler | Description | Priority | Status |
|---------|---------------|-------------|----------|--------|
| `conceal` | do_conceal | Hide objects from view | Medium | DONE |
| `corner` | do_corner | Corner/trap someone | Low | DONE |
| `douse` | do_douse | Extinguish fires/light | Medium | DONE |
| `drag` | do_drag | Drag objects/bodies | Low | DONE |
| `hunt` | do_hunt | Track a target (active) | High | DONE |
| `track` | do_track | Track a target (passive) | High | DONE |
| `lure` | do_lure | Lure mobs | Low | DONE |
| `palm` | do_palm | Sleight of hand | Medium | DONE |
| `pick` | do_gen_door | Pick locks | High | DONE |
| `steal` | do_steal | Steal from targets | Medium | DONE |
| `stow` | do_stow | Quickly sheathe weapon | Low | DONE |
| `tame` | do_tame | Tame animals | Low | DONE |
| `visible` | do_visible | Cancel invisibility | High | DONE |
| `walk` | do_move | Walk (vs fly) | Medium | DONE |

### Information Commands

| Command | Legacy Handler | Description | Priority | Status |
|---------|---------------|-------------|----------|--------|
| `experience` | do_experience | Detailed XP breakdown | Medium | DONE |
| `level` | do_level | Level requirements info | Medium | DONE |
| `songs` | do_songs | Bard song list | Low | DONE |
| `spells` | do_spells | Spell list by circle with slot status | Medium | DONE |
| `skills` | do_skills | Skill list with proficiency bars | Medium | DONE |
| `trophy` | do_trophy | Trophy/kill display | Low | DONE |
| `whoami` | do_gen_ps | Display own name | Low | DONE |
| `world` | do_world | World statistics | Low | DONE |
| `uptime` | do_date | Server uptime | Low | DONE |

### Communication

| Command | Legacy Handler | Description | Priority | Status |
|---------|---------------|-------------|----------|--------|
| `music` | do_music | Music channel | Medium | DONE |
| `ctell` | do_ctell | Clan tell | Medium | DONE |
| `insult` | do_insult | Insult a target | Low | DONE |

### Misc Gameplay

| Command | Legacy Handler | Description | Priority | Status |
|---------|---------------|-------------|----------|--------|
| `first aid` | do_first_aid | First aid skill | High | DONE |
| `perform` | do_cast | Cast songs/chants (cast expanded to accept SONG/CHANT types) | High | DONE |
| `house` | do_house | Player housing | Low | DONE |

---

## Missing Aliases (Quick Wins)

These are aliases for existing commands - trivial to add.

| Alias | Maps To | Notes | Status |
|-------|---------|-------|--------|
| `bodyslam` | `bash` | Alternative name | DONE |
| `maul` | `bash` | Bear form variant | DONE |
| `murder` | `kill` / `hit` | PK flagging variant | DONE |
| `tantrum` | `hitall` | Hit-all variant | DONE |
| `howl` | `roar` | Intimidation variant | DONE |
| `shadow` | `follow` | Stealth follow | DONE |

---

## Intentionally Not Porting

### OLC Commands (Replaced by Muditor)
olc, edit, oedit, medit, redit, zedit, sedit, hedit, sdedit, tedit, gedit,
iedit, boardadmin, rcopy, mcopy, ocopy, trigcopy

### DG Script Commands (Replaced by Lua Triggers)
masound, mat, mcast, mchant, mdamage, mecho, mechoaround, mexp, mforce,
mgold, mgoto, mjunk, mkill, mload, mmobflag, mobjflag, mperform, mpurge,
mroomflag, msave, msend, mskillset, mteleport, m_run_room_trig,
attach, detach, varset, varunset

### Admin Internals (Replaced by MCP/API)
freeze, force, switch, snoop, at, ban, unban, grant, ungrant, advance,
dc, page, peace, purge, restore, reroll, rename, revoke, notitle, mute,
wizlock, xnames, coredump, hotboot, inctime, linkload, objupdate,
pfilemaint, terminate, thaw, infodump

### Legacy Admin Search/List (Replaced by Muditor/API)
clist, csearch, elist, esearch, estat, ksearch, nlist, olist, olocate,
onum, rlist, rnum, rsearch, snum, ssearch, tsearch, tnum, vlist, vnum,
vsearch, vstat, vwear, vitem, zlist, znum, zsearch, slist (admin variant)

### Misc Legacy-Only
last, lastgos, log, show, players, pscan, viewdam, wizhelp, wizlist,
restat, rrestore, rpain, pain, skillset, flag commands

---

## Visual / Color Comparison

### Color System Architecture

| Feature | Legacy | Next | Verdict |
|---------|--------|------|---------|
| **Color codes** | Dual-code: `&X` (relative) + `@X` (absolute) | XML-like tags: `<red>`, `<b:green>`, `</>` | Next is cleaner, more readable |
| **Color depth** | 16 ANSI colors only (8 normal + 8 bright) | 16 ANSI + 256-color + 24-bit RGB | Next far superior |
| **Terminal detect** | None (assumes ANSI) | MTTS/GMCP/TTYPE detection + adaptive output | Next far superior |
| **Unicode** | No support | Adaptive (unicode for capable clients, ASCII fallback) | Next far superior |
| **Semantic colors** | None (hardcoded per-use) | Named constants: `Colors::Health`, `Colors::Damage`, etc. | Next far superior |
| **Nesting** | Flat (reset to normal) | Stack-based (pop restores previous color) | Next far superior |
| **Item quality** | No rarity coloring | 5-tier: Common/Uncommon/Rare/Epic/Legendary | New feature |
| **Environment** | No semantic env colors | Named: Grass, Water, Fire, Stone, Wood, Metal | New feature |

### Area-by-Area Comparison

#### 1. Room Descriptions (`look`)

**Legacy:**
```
{cyan}Room Name{reset}
Room description text.

{red}A circle of fire burns wildly here, surrounding the area.{reset}
{bright green}Thick foliage appears to have overgrown the whole area.{reset}
{yellow}A soft {white}glow{reset}{yellow} suffuses the area with {bold}light{reset}{yellow}.{reset}

[Exits: n e s w]

Objects on ground (no special formatting)
Actors with status colors (hp-based condition text)
```
- Room name: cyan
- Special room effects: individually colored (red for fire, green for nature, yellow for light)
- Objects: no color coding
- Actor condition: graduated color scale (green excellent -> red awful)

**Next:**
```
{green}Room Name{reset}
Room description text.

{cyan}Obvious exits:{reset} {bright cyan}north{reset}, {bright cyan}east{reset}

{yellow}You see:{reset}
  Object descriptions

  {indicators}Actor presence descriptions
```
- Room name: green (less vibrant than cyan)
- Exits: cyan with bright cyan direction names
- Objects: prefixed with yellow "You see:" header
- Actor indicators: colored per-type (magenta=poison, red=burning, yellow=AFK)

**Status: MOSTLY DONE** - Room name colored by sector type (get_sector_color_tag: City=b:yellow, Forest=b:green, Mountains=b:white, Water=b:cyan, Underground=dim, Lava=b:red, etc.). Atmosphere descriptions added for exotic sectors (Underwater=cyan, Lava=b:red, Fire=red, Ice=b:cyan, Swamp=b:green, Underground=dim, Astral=b:magenta, Spirit=b:magenta, Void=dim, Lightning=b:yellow). Darkness message upgraded to `<dim>It is pitch black...</>`.

**Remaining gaps:**
- [x] Infravision display: red-tinted room name, heat signature descriptions, "The red shape of [name] is here"
- [ ] Room magical effect coloring (fire circle=red, fog, foliage=green, illumination=yellow) - rooms don't have effect system yet

#### 2. Score / Character Sheet

**Legacy:**
```
Character attributes for Name

Level: {yellow}20{reset}  Class: Sorcerer
Race: Elf  Size: {yellow}Medium{reset}  Gender: {yellow}Male{reset}
Age: {bold yellow}25{reset}{yellow} years{reset}, {bold yellow}3{reset}{yellow} months{reset}

Str: {bold yellow}18{reset}({yellow}20{reset})     Int: {bold yellow}16{reset}({yellow}16{reset})     Wis: ...
Dex: {bold yellow}14{reset}({yellow}14{reset})     Con: {bold yellow}12{reset}({yellow}12{reset})     Cha: ...

Hit points: {bold red}150{reset}[{red}200{reset}] ({yellow}75{reset})
Moves: {bold green}50{reset}[{green}100{reset}] ({yellow}50{reset})

Armor class: {yellow}-5{reset}
Hitroll: {bold yellow}12{reset}  Damroll: {bold yellow}8{reset}  Focus: {yellow}15{reset}
Perception: {bold yellow}Good{reset}  Concealment: {bold yellow}Fair{reset}

Alignment: {bold yellow}350{reset}  Status: {yellow}Standing{reset}

{bold}*{red}*{bold yellow}* You are on {bold red}FIRE{bold white}! {bold yellow}*{red}*{yellow}*{reset}

Playing time: {bold yellow}5{reset}{yellow} days{reset} and {bold yellow}3{reset}{yellow} hours{reset}

Coins carried: 5p 12g 30s 45c
Coins in bank:  100p 0g 0s 0c
```

**Next:**
```
            Character attributes for Name

Level: 20  Class: Sorcerer  Race: Elf  Size: Medium  Gender: Male
Age: 40 years, 0 months  Height: 5'10"  Weight: 180 lbs
Str: 18    Int: 16     Wis: 14
Dex: 12    Con: 10     Cha: 8
Hit points: 150/200   Stamina: 50/100
Condition: {green}Nourished{reset}   {cyan}Refreshed{reset}   {magenta}Buzzed{reset}
Accuracy: 12   Evasion: 8   Attack Power: 15
Armor Rating: 25   Damage Reduction: 10%
Alignment: Good (350)  Status: Standing
Encumbrance: 45/200 lbs  Exp: 5000 / 10000  [{green}=========={reset}{red}----------{reset}] 50%
Coins: 5p 12g 30s 45c
Location: Town Square [30.1]

{cyan}Active Effects:{reset}
  {green}Shield{reset} (+2 armor_rating) - 2 hours
```

**Status: DONE** - Score heavily colorized: stats in b:yellow, HP/stamina with green/yellow/red gradient based on %, alignment colored by value, position status colors, encumbrance coloring, on-fire display, colored headers.

#### 3. Combat Messages

**Legacy:**
```
(Scaled by % of max HP - 10+ tiers)
Graze:     "$n grazes $N as $e #W $M."
Normal:    "$n #W $N."
Hard:      "$n #W $N hard."
Very hard: "$n #W $N very hard."
Massacre:  "$n massacres $N to small fragments with $s #w."
Deadly:    "$n nearly rips $N in two with $s deadly #w!!"

Damage verbs rotate based on weapon type (#w/#W placeholders)
Blind combat has special {dark gray} messages
Elemental unarmed attacks (fire/ice/lightning/acid) with unique flavor text
Death: Standard kill message
```

**Next:**
```
Miss:     "Your attack misses {cyan}Target{reset}."
Normal:   "You hit {cyan}Target{reset} for {yellow}42{reset} damage."
Glancing: "You {dim}glancingly{reset} hit {cyan}Target{reset} for {dim}15{reset} damage."
Critical: "You {bold red}CRITICALLY{reset} hit {cyan}Target{reset} for {bold red}89{reset} damage."

Death:    "You have killed {cyan}Target{reset}! You gain {gold}500{reset} experience."
          "{bold red}You are DEAD!{reset} You feel your spirit leave your body..."

Optional dice roll details (for debugging):
  {color}[Roll: 15+3=18 vs 8+2=10, margin 8]{reset}
  {color}[Base: 25.0 -> Soak: -5 -> DR: 10% -> Final: 18.0]{reset}
```

**Status: DONE** - 9-tier damage system (miss/graze/barely hit/hit/hit hard/hit very hard/hit extremely hard/massacre/OBLITERATE) with graduated colors (dim→white→yellow→b:yellow→b:red). Critical/glancing prefixes preserved. Miss messages dimmed.

**Remaining gaps:**
- [x] Weapon-type-specific verbs (slash/pierce/crush/burn/freeze/corrode/shock/poison) - full verb table with per-tier messages
- [x] Elemental flavor text for attacks (fire=red flames, cold=cyan frost, acid=green sizzle, shock=yellow lightning, poison=magenta venom)
- [x] Blind combat messages (attacker: "wildly slash something", target: "something slashes you", miss: "swing wildly in the darkness")

#### 4. Who List

**Legacy:**
```
[{color}  Overlord  {reset}] Strider the Almighty
[{red}QuasiDeity{reset}] Bob the Builder
[{bright red}MajorDeity{reset}] Alice the Wise
[Level 50 Sor] Jane the Sorcerer  (AFK) (hidden)

Immortals have unique colored rank titles
Summary: "There are 5 visible deities and 12 visible mortals."
```

**Next:**
```
Players currently online (3):
  Aragorn (Level 20)
  Gandalf (Level 30)
  Legolas (Level 18)
```

**Status: DONE** - Immortals/mortals separated, colored rank titles (Overlord/SoulForger/etc), level-based color gradient for mortals, class display, status flags (AFK/invis/hidden), summary line with breakdown.

#### 5. Equipment / Inventory

**Legacy:**
```
You are using:
<worn on head>       a shiny crown
<wielded>            a flaming sword
<worn on body>       {glowing} mythril plate armor
<floating nearby>    {hovering} a scrying orb

Object flags with color:
  ({blue}floating{reset})
  {yellow}({bold}illuminated{yellow}){reset}
  ({magenta}glowing{reset})
  ({red}({cyan}humming{red}){reset})
  ({bright blue}magic{reset})
  ({magenta}poisoned{reset})
  ({red}Red Aura{reset}) / ({yellow}Gold Aura{reset})
  ({cyan}{bold}({magenta}hovering{cyan}{bold}){reset})
```

**Next:**
```
You are using:
{bright cyan}<used as light>{reset} a torch
{cyan}<worn on head>{reset} a helm of might
{yellow}<wielded>{reset} a longsword
{yellow}<held>{reset} a shield
{cyan}<worn on body>{reset} plate armor
{magenta}<floating nearby>{reset} a scrying orb
```

**Status: DONE** - Object::flag_indicators() method with always-visible flags (Glow=magenta, Hum=cyan, Float=blue, Invisible=dim, Decomposing=dim) and detection-based flags (Magic=b:blue, Poison=b:magenta, AntiEvil=b:yellow Gold Aura, AntiGood=red Red Aura, Bless=cyan). Integrated into inventory, equipment, and room object displays.

**Remaining gaps:**
- [ ] Item quality/rarity coloring (system exists in rich_text.hpp but unused)

#### 6. Communication Channels

**Legacy:**
```
Say:    plain text with @0 reset
Tell:   {white}You tell Name{white}, 'message'{reset}
Gossip: {yellow}You gossip, 'message'{reset}
Shout:  {yellow}You shout, 'message'{reset}
Gtell:  {green}You group say, '{reset}message{green}'{reset}
```

**Next:**
```
Say:     {white}You say, 'message'{reset}
Tell:    {cyan}Name tells you, 'message'{reset}
Whisper: {dim}{cyan}Name whispers to you, 'message'{reset}  (dimmed!)
Emote:   {yellow}Name does something{reset}
Ask:     {white}Name asks you, 'message'{reset}
```

**Status: DONE** - Channel colors: gossip=b:yellow, shout=b:red, tell/reply=cyan, whisper=dim cyan, emote=yellow, ask=b:white, clan=b:green, group=b:cyan, wiznet=b:magenta, petition=b:yellow, lasttells=cyan. Whisper dimming (unique to next).

#### 7. Prompts

**Legacy:**
```
Default: "FieryMUD: Set your prompt (see 'help prompt')>"
Supports ~25 format codes: %h/%H (hp), %v/%V (moves), %n (name), %a (alignment),
%o/%O (opponent status), %t/%T (tank status), %l/%L (spell effects),
%e/%E (exp bar/message), %p{h|v} (percent), %c/%C (coins), %w/%W (wealth),
%d{code} (cooldown bars), %x/%X (flags), %z/%Z (zone)
```

**Next:**
```
Default: "<%h/%Hhp %v/%Vs> "
Supports ~24 format codes: %h/%H (hp), %v/%V (stamina), %l (level),
%g (wealth), %x/%X (exp), %t/%T/%o/%O (opponent/tank), %n (newline),
%a/%A (alignment), %p/%P (% hp/stamina), %z/%Z (room name),
%c/%C (coins), %L (spell slots), %d (cooldowns), %_ (space)
```

**Status: DONE** - Expanded from ~12 to ~24 codes. Added: %a (alignment value), %A (colored alignment desc), %p (% HP), %P (% stamina), %o/%t (opponent/tank condition text), %O/%T (opponent/tank name), %z/%Z (room name), %c/%C (coin breakdown brief/colored), %L (spell slots per circle with green/yellow/red color coding and restoring count), %d (active ability cooldowns with remaining seconds), %_ (space). Fixed %X to use proper level calc.

All legacy prompt codes now have next equivalents.

#### 8. HP Condition Display

**Legacy** (graduated color scale):
```
100%:  {green}excellent condition{reset}
88%:   {yellow}a few scratches{reset}
75%:   {bright yellow}small wounds and bruises{reset}
50%:   {bright magenta}quite a few wounds{reset}
30%:   {magenta}big nasty wounds{reset}
15%:   {bright red}pretty hurt{reset}
0%:    {red}awful condition{reset}
<0%:   {red}bleeding awfully from large wounds{reset}
```

**Next:** Not currently implemented as visible text in room descriptions.

**Status: DONE** - Actor::condition_text() with 8 graduated tiers matching legacy (excellent=b:green → scratches=yellow → wounds=b:yellow → wounds=b:magenta → nasty=magenta → hurt=b:red → awful=red → bleeding=red). Shows in room_presence when HP < max, in fighting descriptions with [condition] brackets, and in prompt via %o/%t codes.

---

## Visual Priority Action Items

### Tier 1 - Critical (makes next feel unfinished without these)
1. ~~**Score command colorization**~~ DONE
2. ~~**Who list overhaul**~~ DONE
3. ~~**HP condition text**~~ DONE
4. ~~**Combat damage tiers**~~ DONE
5. ~~**Item flag coloring**~~ DONE

### Tier 2 - Important (noticeably better experience)
6. ~~**Prompt format codes**~~ DONE
7. ~~**Room atmosphere coloring**~~ DONE
8. ~~**Weapon-type combat verbs**~~ DONE - full verb table with 20 damage types
9. **Equipment slot coloring** - Already exists, verify item name coloring too
10. ~~**Channel-specific colors**~~ DONE

### Tier 3 - Polish (makes next feel premium)
11. **Item quality/rarity colors** - Color system defined (Common-Legendary) but no rarity field in DB yet
12. ~~**Infravision mode**~~ DONE - Red-tinted room name, heat signature descriptions, red actor shapes
13. ~~**Blind combat messages**~~ DONE - Attacker/target/miss flavor text for blind actors
14. ~~**Elemental damage flavor**~~ DONE - Fire/cold/acid/shock/poison prefix descriptions
15. **Room effect atmosphere** - Rooms don't have an effect system yet; sector atmospheres already done
16. ~~**"On fire" display**~~ DONE - Multi-color `*<red>*<yellow>* ON FIRE! *<red>*<yellow>*` in room, who, score
