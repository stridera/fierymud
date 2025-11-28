// ---------- Core Enumerations ----------
export type AbilityKind = "spell" | "skill" | "itemPower" | "trap";

export type TargetScope =
    | "SELF"
    | "SINGLE_TARGET"
    | "ROOM_HOSTILES"
    | "ROOM_ALLIES"
    | "ROOM_ALL"
    | "SINGLE_TARGET_OBJECT"
    | "GROUP"
    | "WORLD";

export type MessageEvent =
    | "on_cast"
    | "on_hit"
    | "on_miss"
    | "on_save_success"
    | "on_save_partial"
    | "on_save_fail"
    | "on_apply_aura"
    | "on_remove_aura"
    | "on_tick"
    | "on_wear_off"
    | "on_fizzle"
    | "on_channel_begin"
    | "on_channel_end";

export type DamageSchool =
    | "physical" | "fire" | "cold" | "lightning" | "acid"
    | "poison" | "holy" | "shadow" | "psychic" | "force";

export type SaveType = "vs_spell" | "vs_breath" | "vs_poison" | "vs_paralysis" | "none";

export type StackingRule = "refresh" | "stack" | "ignore" | "replace";

// ---------- Ability (Spell) ----------
export interface Ability {
    id: string;                 // e.g. "spell.fireball"
    kind: AbilityKind;          // "spell"
    name: string;               // display name
    school?: string;            // e.g. "evocation"
    tier?: number;              // circle/level
    tags?: string[];            // freeform tags, e.g. ["aoe","fire"]

    costs?: {
        stamina?: string;
        cooldown?: string;        // duration, e.g. "8s"
        reagents?: { id: string; qty: number }[];
        concentration?: boolean;
    };

    requirements?: {
        roomTagsAll?: string[];   // e.g. ["outdoors"]
        weatherAny?: string[];    // e.g. ["storming"]
        alignmentAny?: string[];  // ["good","neutral","evil"]
        forbiddenRoomTagsAny?: string[];
        state?: string[];         // e.g. ["!silenced"]
    };

    targeting: {
        scope: TargetScope;
        // Each string is a boolean expression evaluated in context
        // Variables: caster, target, room, ability, aura(), hasEffect(), isHostileToCaster, etc.
        filters?: string[];       // e.g. ["isAlive", "isHostileToCaster"]
        // For SINGLE_TARGET_OBJECT:
        objectFilters?: string[]; // e.g. ["isWeapon", "isEquippedBy(caster)"]
        maxTargets?: number;      // hard cap for ROOM_* / GROUP
    };

    savingThrow?: {
        type: SaveType;                 // e.g. "vs_spell"
        dc?: string;                    // expression e.g. "10 + caster.level/2"
        onSuccess?: "half_damage" | "negate" | "reduced" | "custom";
        // Optional effect labels to which save applies (if omitted, applies to all damage/heal)
        appliesToEffects?: string[];
    };

    // The meat: ordered effects to apply to each selected target (or once per cast if scope=SELF/WORLD)
    effects: Effect[];

    // Audience-specific messaging by event (templates may be omitted; engine will use defaults)
    messages?: Partial<Record<MessageEvent, MessageTemplate>>;

    // Optional scripting escape hatch (pure, deterministic)
    script?: {
        id: string;                  // e.g. "custom.enchantWeapon_v1"
        inputs?: Record<string, unknown>;
    };
}

// ---------- Effects ----------
export type Effect =
    | DamageEffect
    | HealEffect
    | ApplyAuraEffect
    | RemoveAuraEffect
    | ResourceChangeEffect
    | SummonEffect
    | TeleportEffect
    | ObjectModifyEffect
    | DispelEffect
    | ConditionEffect
    | MessageEffect
    | ScriptCallEffect;

export interface BaseEffect {
    id?: string;                // optional label for referencing (saves/appliesToEffects)
    when?: string;              // boolean expression gate, e.g. "target.isUndead"
    tags?: string[];            // for synergy/logging e.g. ["fire","dot"]
}

export interface DamageEffect extends BaseEffect {
    kind: "DAMAGE";
    school?: DamageSchool;
    formula: string;            // e.g. "roll('8d6') + caster.INT * 0.6"
    allowOverkill?: boolean;
}

export interface HealEffect extends BaseEffect {
    kind: "HEAL";
    formula: string;            // e.g. "roll('4d8') + caster.level"
}

export interface ApplyAuraEffect extends BaseEffect {
    kind: "APPLY_AURA";
    auraId: string;             // points to Aura template
    duration: string;           // e.g. "1h", "12s"
    stacks?: { max: number; rule: StackingRule };
    // Optional inline modifiers if you don't want a separate Aura object:
    modifiers?: Record<string, string | number>; // e.g. {"hitroll":"+1"}
}

export interface RemoveAuraEffect extends BaseEffect {
    kind: "REMOVE_AURA";
    auraId: string;             // or category/tag supported by engine
    byCategory?: string;        // e.g. "curse", "poison"
    maxRemoved?: number;        // e.g. 1
}

export interface ResourceChangeEffect extends BaseEffect {
    kind: "RESOURCE_CHANGE";
    resource: "hp" | "stamina" | "rage";
    formula: string;            // may be negative
}

export interface SummonEffect extends BaseEffect {
    kind: "SUMMON";
    npcId: string;
    count?: string;             // expression, e.g. "1 + floor(caster.level/10)"
    duration?: string;          // optional despawn time
}

export interface TeleportEffect extends BaseEffect {
    kind: "TELEPORT";
    destination: string;        // room id or symbolic (e.g. "temple")
}

export interface ObjectModifyEffect extends BaseEffect {
    kind: "OBJECT_MODIFY";
    // generic key/value to let content decide: flags, plus/minus stats, sockets, etc.
    changes: Record<string, unknown>;
    target: "held" | "equipped" | "inventory" | "targetObject";
}

export interface DispelEffect extends BaseEffect {
    kind: "DISPEL";
    category?: string;          // e.g. "magic", "curse"
    chance?: string;            // e.g. "50 + caster.level"
    maxRemoved?: number;
}

export interface ConditionEffect extends BaseEffect {
    kind: "CONDITION";
    cond: string;               // boolean expression
    onTrue: Effect[];
    onFalse?: Effect[];
}

export interface MessageEffect extends BaseEffect {
    kind: "MESSAGE";
    toCaster?: string;
    toTarget?: string;
    toRoom?: string;
}

export interface ScriptCallEffect extends BaseEffect {
    kind: "SCRIPT_CALL";
    scriptId: string;
    args?: Record<string, unknown>;
}

// ---------- Auras ----------
export interface Aura {
    id: string;                        // e.g. "aura.bless"
    name: string;
    dispelCategory?: string;           // "magic", "curse", "poison"
    priority?: number;                 // for dispel ordering
    tickRate?: string;                 // e.g. "3s"
    stacks?: { max: number; rule: StackingRule };
    // Stat changes, flags, resistances, skill mods, etc.
    modifiers?: Record<string, string | number>;
    // Optional tick/enter/exit payloads
    onApply?: Effect[];
    onTick?: Effect[];
    onExpire?: Effect[];
    messages?: Partial<Record<MessageEvent, MessageTemplate>>;
}

// ---------- Messaging ----------
export interface MessageTemplate {
    caster?: string;   // sent to caster (2nd person)
    target?: string;   // sent to each affected target (2nd person)
    room?: string;     // sent once to bystanders (excludes caster & all targets)
    // Optional condition to select this variant (e.g. only on crit)
    when?: string;     // boolean expression over context
}

// ---------- Notes on expressions & tokens ----------
// - Expressions are side-effect-free: numbers, booleans, dice: roll('XdY+Z'), math.
// - Message tokens available: {ability},{caster},{target},{amount},{damage},{healed},
//   {aura},{school},{save_type},{stack},{list(targets)},{name(who)},{mask(who)},
//   {they(who)},{them(who)},{their(who)},{fmt(n)},{if(cond,A,B)}.
