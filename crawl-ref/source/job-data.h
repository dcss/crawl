enum weapon_choice
{
    WCHOICE_NONE,   ///< No weapon choice
    WCHOICE_PLAIN,  ///< Normal weapon choice
    WCHOICE_GOOD,   ///< Chooses from "good" weapons
    WCHOICE_RANGED, ///< Choice of ranged weapon
};

struct job_def
{
    const char* abbrev; ///< Two-letter abbreviation
    const char* name; ///< Long name
    int s, i, d; ///< Starting Str, Dex, and Int
    vector<species_type> recommended_species; ///< Which species are good at it
    /// Guaranteed starting equipment. Uses vault spec syntax, with the plus:,
    /// charges:, q:, and ego: tags supported.
    vector<string> equipment;
    weapon_choice wchoice; ///< how the weapon is chosen, if any
    vector<pair<skill_type, int>> skills; ///< starting skills
};

static const map<job_type, job_def> job_data =
{

{ JOB_ABYSSAL_KNIGHT, {
    "AK", "Abyssal Knight",
    4, 4, 4,
    { SP_HILL_ORC, SP_SPRIGGAN, SP_TROLL, SP_MERFOLK, SP_BASE_DRACONIAN,
      SP_DEMONSPAWN, },
    { "leather armour" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_AIR_ELEMENTALIST, {
    "AE", "Air Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_TENGU, SP_BASE_DRACONIAN, SP_NAGA, SP_VINE_STALKER, },
    { "robe", "book of Air" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_ARCANE_MARKSMAN, {
    "AM", "Arcane Marksman",
    2, 5, 5,
    { SP_FORMICID, SP_DEEP_ELF, SP_KOBOLD, SP_SPRIGGAN, SP_TROLL, SP_CENTAUR, },
    { "robe", "book of Debilitation" },
    WCHOICE_RANGED,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_ARTIFICER, {
    "Ar", "Artificer",
    4, 3, 5,
    { SP_DEEP_DWARF, SP_HALFLING, SP_KOBOLD, SP_SPRIGGAN, SP_BASE_DRACONIAN,
      SP_DEMONSPAWN, },
    { "short sword", "leather armour", "wand of flame charges:15",
      "wand of enslavement charges:15", "wand of random effects charges:15" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_ASSASSIN, {
    "As", "Assassin",
    3, 3, 6,
    { SP_TROLL, SP_HALFLING, SP_SPRIGGAN, SP_DEMONSPAWN, SP_VAMPIRE,
      SP_VINE_STALKER, },
    { "dagger plus:2", "blowgun", "robe", "cloak", "needle ego:poisoned q:8",
      "needle ego:curare q:2" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_BERSERKER, {
    "Be", "Berserker",
    9, -1, 4,
    { SP_HILL_ORC, SP_HALFLING, SP_OGRE, SP_MERFOLK, SP_MINOTAUR, SP_GARGOYLE,
      SP_DEMONSPAWN, },
    { "animal skin" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_CHAOS_KNIGHT, {
    "CK", "Chaos Knight",
    4, 4, 4,
    { SP_HILL_ORC, SP_TROLL, SP_CENTAUR, SP_MERFOLK, SP_MINOTAUR,
      SP_BASE_DRACONIAN, SP_DEMONSPAWN, },
    { "leather armour plus:2" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_CONJURER, {
    "Cj", "Conjurer",
    0, 7, 5,
    { SP_DEEP_ELF, SP_NAGA, SP_TENGU, SP_BASE_DRACONIAN, SP_DEMIGOD, },
    { "robe", "book of Conjurations" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_EARTH_ELEMENTALIST, {
    "EE", "Earth Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_DEEP_DWARF, SP_SPRIGGAN, SP_GARGOYLE, SP_DEMIGOD,
      SP_GHOUL, SP_OCTOPODE, },
    { "book of Geomancy", "stone q:30", "robe", },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_ENCHANTER, {
    "En", "Enchanter",
    0, 7, 5,
    { SP_DEEP_ELF, SP_FELID, SP_KOBOLD, SP_SPRIGGAN, SP_NAGA, SP_VAMPIRE, },
    { "dagger plus:1", "robe", "book of Maledictions" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_FIGHTER, {
    "Fi", "Fighter",
    8, 0, 4,
    { SP_DEEP_DWARF, SP_HILL_ORC, SP_TROLL, SP_MINOTAUR, SP_GARGOYLE,
      SP_CENTAUR, },
    { "scale mail", "shield", "potion of might" },
    WCHOICE_GOOD,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_FIRE_ELEMENTALIST, {
    "FE", "Fire Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_HILL_ORC, SP_NAGA, SP_TENGU, SP_DEMIGOD, SP_GARGOYLE, },
    { "robe", "book of Flames" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_GLADIATOR, {
    "Gl", "Gladiator",
    6, 0, 6,
    { SP_DEEP_DWARF, SP_HILL_ORC, SP_MERFOLK, SP_MINOTAUR, SP_GARGOYLE,
      SP_CENTAUR, },
    { "leather armour", "helmet", "throwing net q:3" },
    WCHOICE_GOOD,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_HUNTER, {
    "Hu", "Hunter",
    4, 3, 5,
    { SP_HILL_ORC, SP_HALFLING, SP_KOBOLD, SP_OGRE, SP_TROLL, SP_CENTAUR, },
    { "short sword", "leather armour" },
    WCHOICE_RANGED,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_ICE_ELEMENTALIST, {
    "IE", "Ice Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_MERFOLK, SP_NAGA, SP_BASE_DRACONIAN, SP_DEMIGOD,
      SP_GARGOYLE, },
    { "robe", "book of Frost" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_MONK, {
    "Mo", "Monk",
    3, 2, 7,
    { SP_DEEP_DWARF, SP_HILL_ORC, SP_TROLL, SP_CENTAUR, SP_MERFOLK,
      SP_GARGOYLE, SP_DEMONSPAWN, },
    { "robe" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_NECROMANCER, {
    "Ne", "Necromancer",
    0, 7, 5,
    { SP_DEEP_ELF, SP_DEEP_DWARF, SP_HILL_ORC, SP_DEMONSPAWN, SP_MUMMY,
      SP_VAMPIRE, },
    { "robe", "book of Necromancy" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_SKALD, {
    "Sk", "Skald",
    3, 5, 4,
    { SP_HALFLING, SP_CENTAUR, SP_MERFOLK, SP_BASE_DRACONIAN, SP_VAMPIRE, },
    { "leather armour", "book of Battle" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_SUMMONER, {
    "Su", "Summoner",
    0, 7, 5,
    { SP_DEEP_ELF, SP_HILL_ORC, SP_VINE_STALKER, SP_MERFOLK, SP_TENGU,
      SP_VAMPIRE, },
    { "robe", "book of Callings" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_TRANSMUTER, {
    "Tm", "Transmuter",
    2, 5, 5,
    { SP_NAGA, SP_MERFOLK, SP_BASE_DRACONIAN, SP_DEMIGOD, SP_DEMONSPAWN,
      SP_TROLL, },
    { "arrow q:12", "robe", "book of Changes" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_VENOM_MAGE, {
    "VM", "Venom Mage",
    0, 7, 5,
    { SP_DEEP_ELF, SP_SPRIGGAN, SP_NAGA, SP_MERFOLK, SP_TENGU, SP_FELID,
      SP_DEMONSPAWN, },
    { "robe", "Young Poisoner's Handbook" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_WANDERER, {
    "Wn", "Wanderer",
    0, 0, 0, // Randomised
    { SP_HILL_ORC, SP_SPRIGGAN, SP_CENTAUR, SP_MERFOLK, SP_BASE_DRACONIAN,
      SP_HUMAN, SP_DEMONSPAWN, },
    { }, // Randomised
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_WARPER, {
    "Wr", "Warper",
    3, 5, 4,
    { SP_FELID, SP_HALFLING, SP_DEEP_DWARF, SP_SPRIGGAN, SP_CENTAUR,
      SP_BASE_DRACONIAN, },
    { "leather armour", "book of Spatial Translocations", "scroll of blinking",
      "tomahawk ego:dispersal q:5" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },

{ JOB_WIZARD, {
    "Wz", "Wizard",
    -1, 10, 3,
    { SP_DEEP_ELF, SP_NAGA, SP_BASE_DRACONIAN, SP_OCTOPODE, SP_HUMAN,
      SP_MUMMY, },
    { "robe", "hat", "book of Minor Magic" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 22}, { SK_SHORT_BLADES, 22}, { SK_LONG_BLADES, 22},
      { SK_AXES, 22}, { SK_MACES_FLAILS, 22}, { SK_POLEARMS, 22},
      { SK_STAVES, 22}, { SK_SLINGS, 22}, { SK_BOWS, 22}, { SK_CROSSBOWS, 22},
      { SK_THROWING, 22}, { SK_ARMOUR, 22}, { SK_DODGING, 22}, { SK_STEALTH, 22},
      { SK_SHIELDS, 22}, { SK_UNARMED_COMBAT, 22}, { SK_SPELLCASTING, 22},
      { SK_CONJURATIONS, 22}, { SK_HEXES, 22}, { SK_CHARMS, 22},
      { SK_SUMMONINGS, 22}, { SK_NECROMANCY, 22}, { SK_TRANSLOCATIONS, 22},
      { SK_TRANSMUTATIONS, 22}, { SK_FIRE_MAGIC, 22}, { SK_ICE_MAGIC, 22},
      { SK_AIR_MAGIC, 22}, { SK_EARTH_MAGIC, 22}, { SK_POISON_MAGIC, 22},
      { SK_INVOCATIONS, 22}, { SK_EVOCATIONS, 22}, },
} },
#if TAG_MAJOR_VERSION == 34
{ JOB_DEATH_KNIGHT, {
    "DK", "Death Knight",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_HEALER, {
    "He", "Healer",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_JESTER, {
    "Jr", "Jester",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_PRIEST, {
    "Pr", "Priest",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_STALKER, {
    "St", "Stalker",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },
#endif
};
