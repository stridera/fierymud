-- SQL script to add look_message values to AbilityMessages for visual spell effects
-- These messages appear when looking at someone affected by these spells
-- Placeholders: {pronoun.possessive} = His/Her/Their, {pronoun.subjective} = He/She/They

-- Sanctuary (id: 36) - White glowing aura
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} glows with a bright white aura.'
WHERE ability_id = 36;

-- Fireshield (id: 105) - Burning flames
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is surrounded by a shield of burning flames.'
WHERE ability_id = 105;

-- Coldshield (id: 106) - Icy aura
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is surrounded by a shield of bitter frost.'
WHERE ability_id = 106;

-- Blur (id: 69) - Blurred image
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} image appears blurred and difficult to focus on.'
WHERE ability_id = 69;

-- Fly (id: 57) - Floating
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is hovering in the air.'
WHERE ability_id = 57;

-- Haste (id: 68) - Moving fast
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is moving with unnatural speed.'
WHERE ability_id = 68;

-- Invisibility (id: 29) - Semi-transparent
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} appears semi-transparent and shimmering.'
WHERE ability_id = 29;

-- Mass Invisibility (id: 112) - Semi-transparent
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} appears semi-transparent and shimmering.'
WHERE ability_id = 112;

-- Stone Skin (id: 52) - Stone-like skin
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} body appears to be made of stone.'
WHERE ability_id = 52;

-- Barkskin (id: 144) - Bark-like skin
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} skin is thick, brown and wrinkley like bark.'
WHERE ability_id = 144;

-- Poison (id: 33) - Sickly green
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} looks extremely sick and has a greenish pallor.'
WHERE ability_id = 33;

-- Disease (id: 135) - Diseased appearance
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} looks terribly ill and diseased.'
WHERE ability_id = 135;

-- Berserk (id: 280) - Wild/feral
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} eyes are wild with bloodlust and rage.'
WHERE ability_id = 280;

-- Fear (id: 114) - Terrified
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is cowering in abject terror.'
WHERE ability_id = 114;

-- Doom (id: 64) - Divine sanctuary
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} glows with a divine, protective light.'
WHERE ability_id = 64;

-- Terror (id: 364) - Terrified
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is cowering in abject terror.'
WHERE ability_id = 364;

-- Sleep (id: 38) - Sleeping
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is in a deep magical slumber.'
WHERE ability_id = 38;

-- Silence (id: 94) - Silenced
UPDATE "AbilityMessages"
SET look_message = 'A sphere of silence surrounds {pronoun.subjective}.'
WHERE ability_id = 94;

-- Confusion (id: 219) - Dazed
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} looks dazed and confused.'
WHERE ability_id = 219;

-- Web (id: 246) - Webbed
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is covered in sticky webs.'
WHERE ability_id = 246;

-- Feather Fall (id: 103) - Light/airy
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} appears to be light as a feather.'
WHERE ability_id = 103;

-- Charm Person (id: 7) - Glazed eyes
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} eyes have a glazed, vacant look.'
WHERE ability_id = 7;

-- Bless (id: 3) - Blessed
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} looks blessed and protected.'
WHERE ability_id = 3;

-- Vaporform (id: 179) - Misty
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} body appears as swirling mist.'
WHERE ability_id = 179;

-- Wings Of Heaven (id: 126) - Angelic wings
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} has glowing angelic wings sprouting from {pronoun.possessive} back.'
WHERE ability_id = 126;

-- Wings Of Hell (id: 141) - Demonic wings
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} has dark, leathery demonic wings sprouting from {pronoun.possessive} back.'
WHERE ability_id = 141;

-- Soulshield (id: 92) - Soul protection
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is surrounded by an ethereal protective barrier.'
WHERE ability_id = 92;

-- Circle Of Light (id: 115) - Glowing brightly
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is bathed in a brilliant circle of light.'
WHERE ability_id = 115;

-- Enlightenment (id: 123) - Glowing with knowledge
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} eyes glow with inner wisdom.'
WHERE ability_id = 123;

-- Protection from Evil (id: 34) - Protected aura
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is surrounded by a faint protective aura.'
WHERE ability_id = 34;

-- Protection from Good (id: 235) - Dark aura
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is surrounded by a faint dark aura.'
WHERE ability_id = 235;

-- Sphere of Protection (id: 325) - Sphere barrier
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is enclosed in a shimmering protective sphere.'
WHERE ability_id = 325;

-- Earth Blessing (id: 247) - Earthen blessing
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is surrounded by a faint earthy glow.'
WHERE ability_id = 247;

-- Additional armor and visual effect spells

-- Armor Of Gaia (id: 156) - Earthen armor
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is encased in armor of living earth and stone.'
WHERE ability_id = 156;

-- Bone Armor (id: 187) - Bone armor
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is encased in a grotesque armor of bones.'
WHERE ability_id = 187;

-- Ice Armor (id: 172) - Ice armor
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is encased in glistening armor of solid ice.'
WHERE ability_id = 172;

-- Infravision (id: 50) - Glowing red eyes
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} eyes glow with a faint red hue.'
WHERE ability_id = 50;

-- Darkness (id: 73) - Shrouded in darkness
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is shrouded in an aura of magical darkness.'
WHERE ability_id = 73;

-- Divine Essence (id: 129) - Divine glow
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} radiates with pure divine essence.'
WHERE ability_id = 129;

-- Dark Presence (id: 131) - Dark aura
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} exudes an aura of dark malevolence.'
WHERE ability_id = 131;

-- Demonic Aspect (id: 137) - Demonic features
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} has taken on a terrifying demonic appearance.'
WHERE ability_id = 137;

-- Demonic Mutation (id: 140) - Mutated demonic form
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} body writhes with demonic mutations.'
WHERE ability_id = 140;

-- On Fire (id: 337) - Burning
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is engulfed in flames!'
WHERE ability_id = 337;

-- Shadow (id: 344) - Shadowy form
UPDATE "AbilityMessages"
SET look_message = '{pronoun.subjective} is cloaked in swirling shadows.'
WHERE ability_id = 344;

-- Flame Blade (id: 161) - Flaming weapon
UPDATE "AbilityMessages"
SET look_message = '{pronoun.possessive} weapon burns with magical flames.'
WHERE ability_id = 161;

-- Verify what was updated
SELECT a.name, am.look_message
FROM "AbilityMessages" am
JOIN "Ability" a ON am.ability_id = a.id
WHERE am.look_message IS NOT NULL
ORDER BY a.name;
