/**
 * @file
 * @brief Random and unrandom artefact functions.
**/

#include "AppHdr.h"

#include "artefact.h"
#include "art-enum.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "areas.h"
#include "branch.h"
#include "colour.h"
#include "coordit.h"
#include "database.h"
#include "itemname.h"
#include "itemprop.h"
#include "items.h"
#include "libutil.h"
#include "makeitem.h"
#include "religion.h"
#include "shout.h"
#include "spl-book.h"
#include "state.h"
#include "stringutil.h"
#include "unicode.h"

static bool _god_fits_artefact(const god_type which_god, const item_def &item,
                               bool name_check_only = false)
{
    if (which_god == GOD_NO_GOD)
        return false;

    // Jellies can't eat artefacts, so their god won't make any.
    if (which_god == GOD_JIYVA)
        return false;

    // First check the item's base_type and sub_type, then check the
    // item's brand and other randart properties.
    bool type_bad = false;
    switch (which_god)
    {
    case GOD_ELYVILON:
        // Peaceful healer god: no weapons, no berserking.
        if (item.base_type == OBJ_WEAPONS)
            type_bad = true;

        if (item.is_type(OBJ_JEWELLERY, AMU_RAGE))
            type_bad = true;
        break;

    case GOD_SHINING_ONE:
        // Crusader god: holiness, honourable combat.
        if (item.is_type(OBJ_JEWELLERY, RING_STEALTH))
            type_bad = true;
        break;

    case GOD_OKAWARU:
        // Precision fighter god: no inaccuracy.
        if (item.is_type(OBJ_JEWELLERY, AMU_INACCURACY))
            type_bad = true;
        break;

    case GOD_SIF_MUNA:
    case GOD_VEHUMET:
        // The magic gods: no weapons, no preventing spellcasting.
        if (item.base_type == OBJ_WEAPONS)
            type_bad = true;
        break;

    case GOD_TROG:
        // Anti-magic god: no spell use, no enhancing magic.
        if (item.base_type == OBJ_BOOKS)
            type_bad = true;

        if (item.base_type == OBJ_JEWELLERY
            && (item.sub_type == RING_WIZARDRY
             || item.sub_type == RING_FIRE
             || item.sub_type == RING_ICE
             || item.sub_type == RING_MAGICAL_POWER))
        {
            type_bad = true;
        }
        break;

    case GOD_CHEIBRIADOS:
        // Slow god: no quick blades, no berserking.
        if (item.is_type(OBJ_WEAPONS, WPN_QUICK_BLADE))
            type_bad = true;

        if (item.is_type(OBJ_JEWELLERY, AMU_RAGE))
            type_bad = true;
        break;

    case GOD_DITHMENOS:
        // Shadow god: no reducing stealth.
        if (item.is_type(OBJ_JEWELLERY, RING_LOUDNESS))
            type_bad = true;
        break;

    default:
        break;
    }

    if (type_bad && !name_check_only)
    {
        die("%s attempting to gift invalid type of item.",
            god_name(which_god).c_str());
    }

    if (type_bad)
        return false;

    const int brand = get_weapon_brand(item);
    const int ego   = get_armour_ego_type(item);

    if (is_evil_god(which_god) && brand == SPWPN_HOLY_WRATH)
        return false;
    else if (is_good_god(which_god)
             && (brand == SPWPN_DRAINING
                 || brand == SPWPN_PAIN
                 || brand == SPWPN_VAMPIRISM
                 || brand == SPWPN_REAPING
                 || brand == SPWPN_CHAOS
                 || is_demonic(item)
                 || artefact_property(item, ARTP_CURSED) != 0))
    {
        return false;
    }

    switch (which_god)
    {
    case GOD_ELYVILON:
        // Peaceful healer god: no berserking.
        if (artefact_property(item, ARTP_ANGRY)
            || artefact_property(item, ARTP_BERSERK))
        {
            return false;
        }
        break;

    case GOD_ZIN:
        // Lawful god: no mutagenics.
        if (artefact_property(item, ARTP_MUTAGENIC))
            return false;
        break;

    case GOD_SHINING_ONE:
        // Crusader god: holiness, honourable combat.
        if (item.base_type == OBJ_WEAPONS && brand != SPWPN_HOLY_WRATH)
            return false;

        if (artefact_property(item, ARTP_INVISIBLE)
            || artefact_property(item, ARTP_STEALTH) > 0)
        {
            return false;
        }
        break;

    case GOD_LUGONU:
        // Abyss god: corruption.
        if (item.base_type == OBJ_WEAPONS && brand != SPWPN_DISTORTION)
            return false;
        break;

    case GOD_KIKUBAAQUDGHA:
        // Necromancy god.
        if (item.base_type == OBJ_WEAPONS && brand != SPWPN_PAIN)
            return false;
    case GOD_SIF_MUNA:
    case GOD_VEHUMET:
        // The magic gods: no preventing spellcasting.
        if (artefact_property(item, ARTP_PREVENT_SPELLCASTING))
            return false;
        break;

    case GOD_TROG:
        // Anti-magic god: no spell use, no enhancing magic.
        if (brand == SPWPN_PAIN) // Pain involves necromantic spell use.
            return false;

        if (artefact_property(item, ARTP_MAGICAL_POWER))
            return false;
        break;

    case GOD_FEDHAS:
        // Fedhas forbids necromancy involving corpses, only reaping
        // really applies.
        if (brand == SPWPN_REAPING)
            return false;
        break;

    case GOD_CHEIBRIADOS:
        // Slow god: no speed, no berserking.
        if (brand == SPWPN_SPEED)
            return false;

        if (ego == SPARM_RUNNING)
            return false;

        if (artefact_property(item, ARTP_ANGRY)
            || artefact_property(item, ARTP_BERSERK))
        {
            return false;
        }
        break;

    case GOD_ASHENZARI:
        // Cursed god: no holy wrath (since that brand repels curses).
        if (brand == SPWPN_HOLY_WRATH)
            return false;
        break;

    case GOD_DITHMENOS:
        // No fiery weapons.
        if (item.base_type == OBJ_WEAPONS
            && brand == SPWPN_FLAMING)
        {
            return false;
        }
        // No reducing stealth.
        if (artefact_property(item, ARTP_STEALTH) < 0)
            return false;
        break;

    default:
        break;
    }

    return true;
}

string replace_name_parts(const string &name_in, const item_def& item)
{
    string name = name_in;

    god_type god_gift;
    (void) origin_is_god_gift(item, &god_gift);

    // Don't allow "player's Death" type names for god gifts (except
    // for those from Xom).
    if (name.find("@player_death@", 0) != string::npos
        || name.find("@player_doom@", 0) != string::npos)
    {
        if (god_gift == GOD_NO_GOD || god_gift == GOD_XOM)
        {
            name = replace_all(name, "@player_death@",
                               "@player_name@"
                               + getRandNameString("killer_name"));
            name = replace_all(name, "@player_doom@",
                               "@player_name@'s "
                               + getRandNameString("death_or_doom"));
        }
        else
        {
            // Simply overwrite the name with one of type "God's Favour".
            name = "of ";
            name += god_name(god_gift, false);
            name += "'s ";
            name += getRandNameString("divine_esteem");
        }
    }
    name = replace_all(name, "@player_name@", you.your_name);

    name = replace_all(name, "@player_species@",
                 species_name(static_cast<species_type>(you.species), true));

    if (name.find("@branch_name@", 0) != string::npos)
    {
        string place = branches[random2(NUM_BRANCHES)].longname;
        if (!place.empty())
            name = replace_all(name, "@branch_name@", place);
    }

    // Occasionally use long name for Xom (see religion.cc).
    name = replace_all(name, "@xom_name@", god_name(GOD_XOM, coinflip()));

    if (name.find("@god_name@", 0) != string::npos)
    {
        god_type which_god;

        // God gifts will always get the gifting god's name.
        if (god_gift != GOD_NO_GOD)
            which_god = god_gift;
        else
        {
            do
            {
                which_god = random_god(false); // Fedhas in ZotDef only
            }
            while (!_god_fits_artefact(which_god, item, true));
        }

        name = replace_all(name, "@god_name@", god_name(which_god, false));
    }

    // copied from apostrophise() (libutil.cc):
    // The proper possessive for a word ending in an "s" is to
    // put an apostrophe after the "s": "Chris" -> "Chris'",
    // not "Chris" -> "Chris's".  Stupid English language...
    name = replace_all(name, "s's", "s'");

    return name;
}

// Remember: disallow unrandart creation in Abyss/Pan.

// Functions defined in art-func.h are referenced in art-data.h
#include "art-func.h"

static const unrandart_entry unranddata[] =
{
#include "art-data.h"
};

static const unrandart_entry *_seekunrandart(const item_def &item);

bool is_known_artefact(const item_def &item)
{
    if (!item_type_known(item))
        return false;

    return is_artefact(item);
}

bool is_artefact(const item_def &item)
{
    return item.flags & ISFLAG_ARTEFACT_MASK;
}

// returns true is item is a pure randart
bool is_random_artefact(const item_def &item)
{
    return item.flags & ISFLAG_RANDART;
}

/** Is this an unrandart, and if so which one?
 *
 *  @param item The item to be checked.
 *  @param which The unrand enum to be checked against (default 0).
 *  @returns true if item is an unrand, and if which is not 0, if it is the unrand
 *           specfied by enum in which.
 */
bool is_unrandom_artefact(const item_def &item, int which)
{
    return item.flags & ISFLAG_UNRANDART
           && (!which || which == item.special);
}

bool is_special_unrandom_artefact(const item_def &item)
{
    return item.flags & ISFLAG_UNRANDART
           && (_seekunrandart(item)->flags & UNRAND_FLAG_SPECIAL);
}

bool is_randapp_artefact(const item_def &item)
{
    return item.flags & ISFLAG_UNRANDART
           && !(item.flags & ISFLAG_KNOW_TYPE)
           && (_seekunrandart(item)->flags & UNRAND_FLAG_RANDAPP);
}

void autoid_unrand(item_def &item)
{
    if (!(item.flags & ISFLAG_UNRANDART) || item.flags & ISFLAG_KNOW_TYPE)
        return;
    const uint16_t uflags = _seekunrandart(item)->flags;
    if (uflags & UNRAND_FLAG_RANDAPP || uflags & UNRAND_FLAG_UNIDED)
        return;

    set_ident_flags(item, ISFLAG_IDENT_MASK | ISFLAG_NOTED_ID);
}

unique_item_status_type get_unique_item_status(int art)
{
    ASSERT_RANGE(art, UNRAND_START + 1, UNRAND_LAST);
    return you.unique_items[art - UNRAND_START];
}

static void _set_unique_item_status(int art, unique_item_status_type status)
{
    ASSERT_RANGE(art, UNRAND_START + 1, UNRAND_LAST);
    you.unique_items[art - UNRAND_START] = status;
}

void set_unique_item_status(const item_def& item,
                            unique_item_status_type status)
{
    if (item.flags & ISFLAG_UNRANDART)
        _set_unique_item_status(item.special, status);
}

/**
 * Fill out the inherent ARTPs corresponding to a given type of armour.
 *
 * @param arm           The armour_type of the armour in question.
 * @param proprt[out]   The properties list to be populated.
 */
static void _populate_armour_intrinsic_artps(const armour_type arm,
                                             artefact_properties_t &proprt)
{
    proprt[ARTP_FIRE] += armour_type_prop(arm, ARMF_RES_FIRE);
    proprt[ARTP_COLD] += armour_type_prop(arm, ARMF_RES_COLD);
    proprt[ARTP_NEGATIVE_ENERGY] += armour_type_prop(arm, ARMF_RES_NEG);
    proprt[ARTP_POISON] += armour_type_prop(arm, ARMF_RES_POISON);
    proprt[ARTP_ELECTRICITY] += armour_type_prop(arm, ARMF_RES_ELEC);
    proprt[ARTP_MAGIC] += armour_type_prop(arm, ARMF_RES_MAGIC);
    proprt[ARTP_STEALTH] += armour_type_prop(arm, ARMF_STEALTH);
    proprt[ARTP_REGENERATION] += armour_type_prop(arm, ARMF_REGENERATION);
}

/// The artefact properties corresponding to a given piece of jewellery.
struct jewellery_fake_artp
{
    /// The artp matching the jewellery (e.g. ARTP_AC for RING_PROTECTION)
    artefact_prop_type  artp;
    /// The value of the artp. (E.g. '9' for RING_MAGICAL_POWER.) If set to 0, uses item.plus instead.
    int                 plus;
};

static map<jewellery_type, vector<jewellery_fake_artp>> jewellery_artps = {
    { AMU_RAGE, { { ARTP_BERSERK, 1 } } },
    { AMU_WARDING, { { ARTP_NEGATIVE_ENERGY, 1 } } },
    { AMU_REGENERATION, { { ARTP_REGENERATION, 1 } } },

    { RING_INVISIBILITY, { { ARTP_INVISIBLE, 1 } } },
    { RING_MAGICAL_POWER, { { ARTP_MAGICAL_POWER, 9 } } },
    { RING_FLIGHT, { { ARTP_FLY, 1 } } },
    { RING_SEE_INVISIBLE, { { ARTP_EYESIGHT, 1 } } },
    { RING_STEALTH, { { ARTP_STEALTH, 1 } } },
    { RING_LOUDNESS, { { ARTP_STEALTH, -1 } } },

    { RING_PROTECTION_FROM_FIRE, { { ARTP_FIRE, 1 } } },
    { RING_PROTECTION_FROM_COLD, { { ARTP_COLD, 1 } } },
    { RING_POISON_RESISTANCE, { { ARTP_POISON, 1 } } },
    { RING_LIFE_PROTECTION, { { ARTP_NEGATIVE_ENERGY, 1 } } },
    { RING_PROTECTION_FROM_MAGIC, { { ARTP_MAGIC, 1 } } },

    { RING_FIRE, { { ARTP_FIRE, 1 }, { ARTP_COLD, -1 } } },
    { RING_ICE, { { ARTP_COLD, 1 }, { ARTP_FIRE, -1 } } },

    { RING_STRENGTH, { { ARTP_STRENGTH, 0 } } },
    { RING_INTELLIGENCE, { { ARTP_INTELLIGENCE, 0 } } },
    { RING_DEXTERITY, { { ARTP_DEXTERITY, 0 } } },
    { RING_PROTECTION, { { ARTP_AC, 0 } } },
    { RING_EVASION, { { ARTP_EVASION, 0 } } },
    { RING_SLAYING, { { ARTP_SLAYING, 0 } } },
};

/**
 * Fill out the inherent ARTPs corresponding to a given type of jewellery.
 *
 * @param arm           The jewellery in question.
 * @param proprt[out]   The properties list to be populated.
 * @param known[out]    The props which are known.
 */
static void _populate_jewel_intrinsic_artps(const item_def &item,
                                              artefact_properties_t &proprt,
                                              artefact_known_props_t &known)
{
    const jewellery_type jewel = (jewellery_type)item.sub_type;
    vector<jewellery_fake_artp> *props = map_find(jewellery_artps, jewel);
    if (!props)
        return;

    const bool id_props = item_ident(item, ISFLAG_KNOW_PROPERTIES)
                            || item_ident(item, ISFLAG_KNOW_TYPE);

    for (const auto &fake_artp : *props)
    {
        proprt[fake_artp.artp] += fake_artp.plus ? fake_artp.plus : item.plus;
        if (id_props)
            known[fake_artp.artp] = true;
    }
}


/**
 * Fill out the inherent ARTPs corresponding to a given item.
 *
 * @param arm           The item in question.
 * @param proprt[out]   The properties list to be populated.
 * @param known[out]    The props which are known.
 */
static void _populate_item_intrinsic_artps(const item_def &item,
                                             artefact_properties_t &proprt,
                                             artefact_known_props_t &known)
{
    switch (item.base_type)
    {
        case OBJ_ARMOUR:
            _populate_armour_intrinsic_artps((armour_type)item.sub_type,
                                             proprt);
            break;
        case OBJ_JEWELLERY:
            _populate_jewel_intrinsic_artps(item, proprt, known);
            break;
        default:
            break;
    }
}

void artefact_desc_properties(const item_def &item,
                              artefact_properties_t &proprt,
                              artefact_known_props_t &known)
{
    // Randart books have no randart properties.
    if (item.base_type == OBJ_BOOKS)
        return;

    // actual artefact properties
    artefact_properties(item, proprt, known);

    // fake artefact properties (intrinsics)
    _populate_item_intrinsic_artps(item, proprt, known);
}

static void _add_randart_weapon_brand(const item_def &item,
                                    artefact_properties_t &item_props)
{
    const int item_type = item.sub_type;

    if (is_range_weapon(item))
    {
        item_props[ARTP_BRAND] = random_choose_weighted(
            2, SPWPN_SPEED,
            4, SPWPN_VENOM,
            4, SPWPN_VORPAL,
            4, SPWPN_FLAMING,
            4, SPWPN_FREEZING,
            0);

        if (item_type == WPN_BLOWGUN)
            item_props[ARTP_BRAND] = coinflip() ? SPWPN_SPEED : SPWPN_EVASION;
        else if (item_attack_skill(item) == SK_CROSSBOWS)
        {
            // Penetration and electrocution are only allowed on
            // crossbows.  This may change in future.
            if (one_chance_in(5))
                item_props[ARTP_BRAND] = SPWPN_ELECTROCUTION;
            else if (one_chance_in(5))
                item_props[ARTP_BRAND] = SPWPN_PENETRATION;
        }
    }
    else if (is_demonic(item) && x_chance_in_y(7, 9))
    {
        item_props[ARTP_BRAND] = random_choose(
            SPWPN_DRAINING,
            SPWPN_FLAMING,
            SPWPN_FREEZING,
            SPWPN_ELECTROCUTION,
            SPWPN_VAMPIRISM,
            SPWPN_PAIN,
            SPWPN_VENOM);
        // fall back to regular melee brands 2/9 of the time
    }
    else
    {
        item_props[ARTP_BRAND] = random_choose_weighted(
            73, SPWPN_VORPAL,
            34, SPWPN_FLAMING,
            34, SPWPN_FREEZING,
            26, SPWPN_VENOM,
            26, SPWPN_DRAINING,
            13, SPWPN_HOLY_WRATH,
            13, SPWPN_ELECTROCUTION,
            13, SPWPN_SPEED,
            13, SPWPN_VAMPIRISM,
            13, SPWPN_PAIN,
            13, SPWPN_ANTIMAGIC,
             3, SPWPN_DISTORTION,
             0);
    }

    // no brand = magic flag to reject and retry
    if (!is_weapon_brand_ok(item_type, item_props[ARTP_BRAND], true))
        item_props[ARTP_BRAND] = SPWPN_NORMAL;
}

static bool _artp_can_go_on_item(artefact_prop_type prop, const item_def &item)
{
    artefact_properties_t intrinsic_proprt;
    intrinsic_proprt.init(0);
    artefact_known_props_t _;
    _populate_item_intrinsic_artps(item, intrinsic_proprt, _);
    if (intrinsic_proprt[prop])
        return false; // don't duplicate intrinsic props

    artefact_properties_t proprt;
    proprt.init(0);
    artefact_desc_properties(item, proprt, _);

    const object_class_type item_class = item.base_type;
    const int item_type = item.sub_type;

    switch (prop)
    {
        case ARTP_SLAYING:
            return item_class != OBJ_WEAPONS; // they already have slaying!
        case ARTP_POISON:
        case ARTP_EYESIGHT:
            return !item.is_type(OBJ_ARMOUR, ARM_NAGA_BARDING);
            // naga already have rPois & sInv!
        case ARTP_RCORR:
        case ARTP_REGENERATION:
            return item_class == OBJ_ARMOUR; // limit availability to armour
        case ARTP_BERSERK:
        case ARTP_ANGRY:
        case ARTP_NOISES:
            return item_class != OBJ_WEAPONS || !is_range_weapon(item);
            // works poorly with ranged weapons
        case ARTP_CAUSE_TELEPORTATION:
            return item_type != OBJ_WEAPONS
                    && !crawl_state.game_is_sprint()
                    && !proprt[ARTP_PREVENT_TELEPORTATION];
            // no tele in sprint, and too annoying on weapons (swappable)
            // and obv we shouldn't generate contradictory props
        case ARTP_PREVENT_TELEPORTATION:
            return !item.is_type(OBJ_JEWELLERY, RING_TELEPORT_CONTROL)
                    && !proprt[ARTP_BLINK]
                    && !proprt[ARTP_CAUSE_TELEPORTATION];
            // no contradictory props/item types
        case ARTP_BLINK:
            return !proprt[ARTP_PREVENT_TELEPORTATION];
            // no contradictory props
        default:
            return true;
    }
}


/// Generation info for a type of artefact property.
struct artefact_prop_data
{
    /// The name of the prop, as displayed on item annotations, etc.
    const char *name;
    /// The types of values this prop can have (e.g. bool, positive int, int)
    artp_value_type value_types;
    /// Randomly generate a 'good' value; null if this prop is never good
    function<int ()> gen_good_value;
    /// Randomly generate a 'bad' value; null if this prop is never good
    function<int ()> gen_bad_value;
};


/// Generate 'good' values for stat artps (e.g. ARTP_STRENGTH)
static int _gen_good_stat_artp() {
    // normally 2-4, max 6
    return 2 + random2(3) + (one_chance_in(4) ? random2(3) : 0);
}

/// Generate 'bad' values for stat artps (e.g. ARTP_STRENGTH)
static int _gen_bad_stat_artp() { return -_gen_good_stat_artp(); }

/// Generate 'good' values for resist-ish artps (e.g. ARTP_FIRE)
static int _gen_good_res_artp() { return 1 + one_chance_in(5); }

/// Generate 'bad' values for resist-ish artps (e.g. ARTP_FIRE)
static int _gen_bad_res_artp() { return -1; }

/// Generate 'good' values for ARTP_HP/ARTP_MAGICAL_POWER
static int _gen_good_hpmp_artp() { return 9; }

/// Generate 'bad' values for ARTP_HP/ARTP_MAGICAL_POWER
static int _gen_bad_hpmp_artp() { return -_gen_good_hpmp_artp(); }

/// Generation info for artefact properties.
static const artefact_prop_data artp_data[] =
{
    { "Brand", ARTP_VAL_POS,
        nullptr, nullptr }, // ARTP_BRAND,
    { "AC", ARTP_VAL_ANY,
        nullptr, nullptr }, // ARTP_AC,
    { "EV", ARTP_VAL_ANY,
        nullptr, nullptr }, // ARTP_EVASION,
    { "Str", ARTP_VAL_ANY,
        _gen_good_stat_artp, _gen_bad_stat_artp }, // ARTP_STRENGTH,
    { "Int", ARTP_VAL_ANY,
        _gen_good_stat_artp, _gen_bad_stat_artp }, // ARTP_INTELLIGENCE,
    { "Dex", ARTP_VAL_ANY,
        _gen_good_stat_artp, _gen_bad_stat_artp }, // ARTP_DEXTERITY,
    { "rF", ARTP_VAL_ANY,
        _gen_good_res_artp, _gen_bad_res_artp }, // ARTP_FIRE,
    { "rC", ARTP_VAL_ANY,
        _gen_good_res_artp, _gen_bad_res_artp }, // ARTP_COLD,
    { "rElec", ARTP_VAL_BOOL,
        []() { return 1; }, nullptr }, // ARTP_ELECTRICITY,
    { "rPois", ARTP_VAL_ANY,
        []() { return 1; }, _gen_bad_res_artp }, // ARTP_POISON,
    { "rN", ARTP_VAL_ANY,
        _gen_good_res_artp,  _gen_bad_res_artp }, // ARTP_NEGATIVE_ENERGY,
    { "MR", ARTP_VAL_ANY,
        _gen_good_res_artp, _gen_bad_res_artp }, // ARTP_MAGIC,
    { "SInv", ARTP_VAL_BOOL,
        []() { return 1; }, nullptr }, // ARTP_EYESIGHT,
    { "+Inv", ARTP_VAL_BOOL,
        []() { return 1; }, nullptr }, // ARTP_INVISIBLE,
    { "+Fly", ARTP_VAL_BOOL,
        []() { return 1; }, nullptr }, // ARTP_FLY,
#if TAG_MAJOR_VERSION > 34
    { "+Fog", ARTP_VAL_BOOL,
        nullptr, nullptr }, // ARTP_FOG,
#endif
    { "+Blink", ARTP_VAL_BOOL,
        []() { return 1; }, nullptr }, // ARTP_BLINK,
    { "+Rage", ARTP_VAL_BOOL,
        []() { return 1; }, nullptr }, // ARTP_BERSERK,
    { "Noisy", ARTP_VAL_POS,
        nullptr, []() { return 2; } }, // ARTP_NOISES,
    { "-Cast", ARTP_VAL_BOOL,
        nullptr, []() { return 1; } }, // ARTP_PREVENT_SPELLCASTING,
    { "*Tele", ARTP_VAL_POS,
        nullptr, []() { return 8; } }, // ARTP_CAUSE_TELEPORTATION,
    { "-Tele", ARTP_VAL_BOOL,
        nullptr, []() { return 1; } }, // ARTP_PREVENT_TELEPORTATION,
    { "*Rage", ARTP_VAL_POS,
        nullptr, []() { return 5; } }, // ARTP_ANGRY,
#if TAG_MAJOR_VERSION == 34
    { "Hungry", ARTP_VAL_POS,
        nullptr, nullptr }, // ARTP_METABOLISM,
#endif
    { "Contam", ARTP_VAL_POS,
        nullptr, []() { return 1; } }, // ARTP_MUTAGENIC,
#if TAG_MAJOR_VERSION == 34
    { "Acc", ARTP_VAL_ANY,
        nullptr, nullptr }, // ARTP_ACCURACY,
#endif
    { "Slay", ARTP_VAL_ANY,
      []() { return 2 + random2(3) + random2(3); },
      []() { return -(2 + random2(3) + random2(3)); } }, // ARTP_SLAYING,
    { "Curse", ARTP_VAL_POS,
        nullptr, nullptr }, // ARTP_CURSED,
    { "Stlth", ARTP_VAL_ANY,
        _gen_good_res_artp, _gen_bad_res_artp }, // ARTP_STEALTH,
    { "MP", ARTP_VAL_ANY,
        _gen_good_hpmp_artp, _gen_bad_hpmp_artp }, // ARTP_MAGICAL_POWER,
    { "Delay", ARTP_VAL_ANY,
        nullptr, nullptr }, // ARTP_BASE_DELAY,
    { "HP", ARTP_VAL_ANY,
        _gen_good_hpmp_artp, _gen_bad_hpmp_artp }, // ARTP_HP,
    { "Clar", ARTP_VAL_BOOL,
        nullptr, nullptr }, // ARTP_CLARITY,
    { "BAcc", ARTP_VAL_ANY,
        nullptr, nullptr }, // ARTP_BASE_ACC,
    { "BDam", ARTP_VAL_ANY,
        nullptr, nullptr }, // ARTP_BASE_DAM,
    { "RMsl", ARTP_VAL_BOOL,
        nullptr, nullptr }, // ARTP_RMSL,
#if TAG_MAJOR_VERSION == 34
    { "+Fog", ARTP_VAL_BOOL,
        nullptr, nullptr }, // ARTP_FOG,
#endif
    { "Regen", ARTP_VAL_POS,
        []() { return 1; }, nullptr }, // ARTP_REGENERATION,
    { "SustAb", ARTP_VAL_BOOL,
        nullptr, nullptr }, // ARTP_SUSTAB,
    { "nupgr", ARTP_VAL_BOOL,
        nullptr, nullptr }, // ARTP_NO_UPGRADE,
    { "rCorr", ARTP_VAL_BOOL,
        []() { return 1; }, nullptr }, // ARTP_RCORR,
    { "rMut", ARTP_VAL_BOOL,
        nullptr, nullptr }, // ARTP_RMUT,
    { "+Twstr", ARTP_VAL_BOOL,
        []() { return 1; }, nullptr }, // ARTP_TWISTER,
};
COMPILE_CHECK(ARRAYSZ(artp_data) == ARTP_NUM_PROPERTIES);

/**
 * Is it possible for the given artp to be generated with 'good' values
 * (generally helpful to the player?
 * E.g. ARTP_SLAYING, ARTP_FIRE, ARTP_REGENERATION, etc.
 *
 * @param prop      The artefact property in question.
 * @return      true if the ARTP is ever generated as a 'good' prop; false
 *              otherwise.
 */
bool artp_potentially_good(artefact_prop_type prop)
{
    return artp_data[prop].gen_good_value != nullptr;
}

/**
 * Is it possible for the given artp to be generated with 'bad' values
 * (generally harmful to the player?
 * E.g. ARTP_SLAYING, ARTP_AC, ARTP_CONTAM, etc.
 *
 * @param prop      The artefact property in question.
 * @return      true if the ARTP is ever generated as a 'bad' prop; false
 *              otherwise.
 */
bool artp_potentially_bad(artefact_prop_type prop)
{
    return artp_data[prop].gen_bad_value != nullptr;
}

/**
 * What type of values can this prop have?
 *
 * Positive, boolean (0 or 1), or any (integer).
 *
 * There should be a better way of expressing this...
 *
 * @param prop      The prop type in question.
 * @return          Possible value types for the prop.
 */
artp_value_type artp_potential_value_types(artefact_prop_type prop)
{
    ASSERT_RANGE(prop, 0, ARRAYSZ(artp_data));
    return artp_data[prop].value_types;
}

/**
 * Return the name for a given artefact property.
 *
 * @param prop      The artp in question.
 * @return          A name; e.g. rCorr, Slay, HP, etc.
 */
const char *artp_name(artefact_prop_type prop)
{
    ASSERT_RANGE(prop, 0, ARRAYSZ(artp_data));
    return artp_data[prop].name;
}

static void _get_randart_properties(const item_def &item,
                                    artefact_properties_t &item_props)
{
    const object_class_type item_class = item.base_type;

    // initialize an array of artefact properties to randomly pick from
    vector<artefact_prop_type> art_props;
    for (int i = 0; i < ARTP_NUM_PROPERTIES; ++i)
        art_props.push_back(static_cast<artefact_prop_type>(i));

    // number of good properties to assign -- avg 2.4, min 1.
    int good = max(1, binomial(6, 45));

    // number of bad properties to assign. Chance increases w/ number of good
    // properties. Average is .24 bad per good, up to 1.44 for 6 good.
    int bad = binomial(3, 8 * good);

    // Add some extra good props if the artifact is a weapon and has a bad base
    // enchantment 0 through -2: +1 good; -3 to -5: +2 good; -6 to -8: +3 good
    if (item_class == OBJ_WEAPONS && item.plus <= 0)
        good += 1 + (-1 * item.plus / 3);

    item_props.init(0);

    // make sure all weapons have a brand
    if (item_class == OBJ_WEAPONS)
        _add_randart_weapon_brand(item, item_props);

    // randomly pick properties from the list, assign values and all that,
    // then subtract them from the good/bad count as needed
    shuffle_array(art_props);
    for (artefact_prop_type prop : art_props)
    {
        if (good <= 0 && bad <= 0)
            break;

        if (!_artp_can_go_on_item(prop, item))
            continue;

        // awkwardly cut the spawn rate of this property to one third.
        // once proper weighting comes in, do that instead.
        if (prop == ARTP_TWISTER && !one_chance_in(5))
            continue;

        // should we try to generate a good or bad version of the prop?
        const bool can_gen_good = good > 0 && artp_potentially_good(prop);
        const bool can_gen_bad = bad > 0 && artp_potentially_bad(prop);
        const bool gen_good = can_gen_good && (!can_gen_bad || coinflip());

        if (gen_good)
        {
            item_props[prop] = artp_data[prop].gen_good_value();
            --good;
            if (prop == ARTP_TWISTER)
                --good; // costs a little extra goodness.
        }
        else if (can_gen_bad)
        {
            item_props[prop] = artp_data[prop].gen_bad_value();
            --bad;
        }
    }
}

static bool _redo_book(item_def &book)
{
    int num_spells  = 0;
    int num_unknown = 0;

    for (spell_type spell : spellbook_template(static_cast<book_type>(book.sub_type)))
    {
        num_spells++;
        if (!you.seen_spell[spell])
            num_unknown++;
    }

    if (num_spells <= 5 && num_unknown == 0)
        return true;
    else if (num_spells > 5 && num_unknown <= 1)
        return true;

    return false;
}

static bool _init_artefact_book(item_def &book)
{
    ASSERT(book.sub_type == BOOK_RANDART_LEVEL
           || book.sub_type == BOOK_RANDART_THEME);
    ASSERT(book.book_param != 0);

    god_type god;
    bool redo = (!origin_is_god_gift(book, &god) || god != GOD_XOM);

    // plus contains a parameter to make_book_foo_randart(), which might get
    // changed after the book has been made into a randart, so reset it on each
    // iteration of the loop.
    // XXX: ...is this really necessary...?
    const int book_param = book.book_param;
    bool book_good = false;
    for (int i = 0; i < 4; i++)
    {
        book.book_param = book_param;

        if (book.sub_type == BOOK_RANDART_LEVEL)
            book_good = make_book_level_randart(book, book.book_param);
        else
            book_good = make_book_theme_randart(book);

        if (!book_good)
            continue;

        if (redo && _redo_book(book))
            continue;

        break;
    }

    return book_good;
}

void setup_unrandart(item_def &item, bool creating)
{
    ASSERT(is_unrandom_artefact(item));
    CrawlVector &rap = item.props[ARTEFACT_PROPS_KEY].get_vector();
    const unrandart_entry *unrand = _seekunrandart(item);

    if (unrand->prpty[ARTP_NO_UPGRADE] && !creating)
        return; // don't mangle mutable items

    for (int i = 0; i < ART_PROPERTIES; i++)
        rap[i] = static_cast<short>(unrand->prpty[i]);

    item.base_type = unrand->base_type;
    item.sub_type  = unrand->sub_type;
    item.plus      = unrand->plus;
}

static bool _init_artefact_properties(item_def &item)
{
    ASSERT(is_artefact(item));

    if (is_unrandom_artefact(item))
    {
        setup_unrandart(item);
        return true;
    }

    CrawlVector &rap = item.props[ARTEFACT_PROPS_KEY].get_vector();
    for (vec_size i = 0; i < ART_PROPERTIES; i++)
        rap[i] = static_cast<short>(0);

    if (item.base_type == OBJ_BOOKS)
        return _init_artefact_book(item);

    artefact_properties_t prop;
    prop.init(0);
    _get_randart_properties(item, prop);

    for (int i = 0; i < ART_PROPERTIES; i++)
    {
        if (i == ARTP_CURSED && prop[i] < 0)
        {
            do_curse_item(item);
            continue;
        }
        rap[i] = static_cast<short>(prop[i]);
    }

    return true;
}

void artefact_properties(const item_def &item,
                         artefact_properties_t  &proprt,
                         artefact_known_props_t &known)
{
    ASSERT(is_artefact(item));
    if (!item.props.exists(KNOWN_PROPS_KEY))
        return;

    const CrawlStoreValue &_val = item.props[KNOWN_PROPS_KEY];
    ASSERT(_val.get_type() == SV_VEC);
    const CrawlVector &known_vec = _val.get_vector();
    ASSERT(known_vec.get_type()     == SV_BOOL);
    ASSERT(known_vec.size()         == ART_PROPERTIES);
    ASSERT(known_vec.get_max_size() == ART_PROPERTIES);

    if (item_ident(item, ISFLAG_KNOW_PROPERTIES))
    {
        for (vec_size i = 0; i < ART_PROPERTIES; i++)
            known[i] = static_cast<bool>(true);
    }
    else
    {
        for (vec_size i = 0; i < ART_PROPERTIES; i++)
            known[i] = known_vec[i];
    }

    if (item.props.exists(ARTEFACT_PROPS_KEY))
    {
        const CrawlVector &rap_vec =
            item.props[ARTEFACT_PROPS_KEY].get_vector();
        ASSERT(rap_vec.get_type()     == SV_SHORT);
        ASSERT(rap_vec.size()         == ART_PROPERTIES);
        ASSERT(rap_vec.get_max_size() == ART_PROPERTIES);

        for (vec_size i = 0; i < ART_PROPERTIES; i++)
            proprt[i] = rap_vec[i].get_short();
    }
    else if (is_unrandom_artefact(item))
    {
        const unrandart_entry *unrand = _seekunrandart(item);

        for (int i = 0; i < ART_PROPERTIES; i++)
            proprt[i] = static_cast<short>(unrand->prpty[i]);
    }
    else
        _get_randart_properties(item, proprt);
}

void artefact_properties(const item_def &item,
                         artefact_properties_t &proprt)
{
    artefact_known_props_t known;

    artefact_properties(item, proprt, known);
}

int artefact_property(const item_def &item, artefact_prop_type prop,
                      bool &_known)
{
    artefact_properties_t  proprt;
    artefact_known_props_t known;
    proprt.init(0);
    known.init(0);

    artefact_properties(item, proprt, known);

    _known = known[prop];

    return proprt[prop];
}

int artefact_property(const item_def &item, artefact_prop_type prop)
{
    bool known;

    return artefact_property(item, prop, known);
}

int artefact_known_property(const item_def &item, artefact_prop_type prop)
{
    artefact_properties_t  proprt;
    artefact_known_props_t known;
    proprt.init(0);
    known.init(0);

    artefact_properties(item, proprt, known);

    if (known[prop])
        return proprt[prop];
    else
        return 0;
}

static int _artefact_num_props(const artefact_properties_t &proprt)
{
    int num = 0;

    // Count all properties, but exclude self-cursing.
    for (int i = 0; i < ARTP_NUM_PROPERTIES; ++i)
        if (i != ARTP_CURSED && proprt[i] != 0)
            num++;

    return num;
}

void artefact_learn_prop(item_def &item, artefact_prop_type prop)
{
    ASSERT(is_artefact(item));
    ASSERT(item.props.exists(KNOWN_PROPS_KEY));
    CrawlStoreValue &_val = item.props[KNOWN_PROPS_KEY];
    ASSERT(_val.get_type() == SV_VEC);
    CrawlVector &known_vec = _val.get_vector();
    ASSERT(known_vec.get_type()     == SV_BOOL);
    ASSERT(known_vec.size()         == ART_PROPERTIES);
    ASSERT(known_vec.get_max_size() == ART_PROPERTIES);

    if (item_ident(item, ISFLAG_KNOW_PROPERTIES))
        return;

    known_vec[prop] = static_cast<bool>(true);
}

static string _get_artefact_type(const item_def &item, bool appear = false)
{
    switch (item.base_type)
    {
    case OBJ_BOOKS:
        return "book";
    case OBJ_WEAPONS:
        return "weapon";
    case OBJ_ARMOUR:
        if (item.sub_type == ARM_ROBE)
            return "robe";
        if (get_item_slot(item) == EQ_BODY_ARMOUR)
            return "body armour";
        return "armour";
    case OBJ_JEWELLERY:
        // Distinguish between amulets and rings only in appearance.
        if (!appear)
            return "jewellery";

        if (jewellery_is_amulet(item))
            return "amulet";
        else
            return "ring";
     default:
        return "artefact";
    }
}

static bool _pick_db_name(const item_def &item)
{
    switch (item.base_type)
    {
    case OBJ_WEAPONS:
    case OBJ_ARMOUR:
        return coinflip();
    case OBJ_JEWELLERY:
        return one_chance_in(5);
    default:
        return false;
    }
}

static string _artefact_name_lookup(const item_def &item, const string &lookup)
{
    const string name = getRandNameString(lookup);
    return name.empty() ? name : replace_name_parts(name, item);
}

static bool _artefact_name_lookup(string &result, const item_def &item,
                                  const string &lookup)
{
    result = _artefact_name_lookup(item, lookup);
    return !result.empty();
}

string make_artefact_name(const item_def &item, bool appearance)
{
    ASSERT(is_artefact(item));

    ASSERT(item.base_type == OBJ_WEAPONS
           || item.base_type == OBJ_ARMOUR
           || item.base_type == OBJ_JEWELLERY
           || item.base_type == OBJ_BOOKS);

    if (is_unrandom_artefact(item))
    {
        const unrandart_entry *unrand = _seekunrandart(item);
        if (!appearance)
            return unrand->name;
        if (!(unrand->flags & UNRAND_FLAG_RANDAPP))
            return unrand->unid_name;
    }

    string lookup;
    string result;

    // Use prefix of gifting god, if applicable.
    bool god_gift = false;
    int item_orig = 0;
    if (!appearance)
    {
        // Divine gifts don't look special, so this is only necessary
        // for actually naming an item.
        item_orig = item.orig_monnum;
        if (item_orig < 0)
            item_orig = -item_orig;
        else
            item_orig = 0;

        if (item_orig > GOD_NO_GOD && item_orig < NUM_GODS)
        {
            lookup += god_name(static_cast<god_type>(item_orig), false) + " ";
            god_gift = true;
        }
    }

    // get base type
    lookup += _get_artefact_type(item, appearance);

    if (appearance)
    {
        string appear = getRandNameString(lookup, " appearance");
        if (appear.empty())
        {
            appear = getRandNameString("general appearance");
            if (appear.empty()) // still nothing found?
                appear = "non-descript";
        }

        result += appear;
        result += " ";
        result += item_base_name(item);
        return result;
    }

    if (_pick_db_name(item))
    {
        result += item_base_name(item) + " ";

        int tries = 100;
        string name;
        do
        {
            (_artefact_name_lookup(name, item, lookup)

             // If nothing found, try god name alone.
             || (god_gift
                 && _artefact_name_lookup(name, item,
                                          god_name(
                                             static_cast<god_type>(item_orig),
                                             false)))

             // If still nothing found, try base type alone.
             || _artefact_name_lookup(name, item, _get_artefact_type(item)));
        }
        while (--tries > 0 && strwidth(name) > 25);

        if (name.empty()) // still nothing found?
            result += "of Bugginess";
        else
            result += name;
    }
    else
    {
        // construct a unique name
        const string st_p = make_name(random_int(), false);
        result += item_base_name(item);

        if (one_chance_in(3))
        {
            result += " of ";
            result += st_p;
        }
        else
        {
            result += " \"";
            result += st_p;
            result += "\"";
        }
    }

    return result;
}

static const unrandart_entry *_seekunrandart(const item_def &item)
{
    return get_unrand_entry(item.special);
}

string get_artefact_name(const item_def &item, bool force_known)
{
    ASSERT(is_artefact(item));

    if (item_type_known(item) || force_known)
    {
        // print artefact's real name
        if (item.props.exists(ARTEFACT_NAME_KEY))
            return item.props[ARTEFACT_NAME_KEY].get_string();
        // unrands don't use cached names
        if (is_unrandom_artefact(item))
            return _seekunrandart(item)->name;
        return make_artefact_name(item, false);
    }
    // print artefact appearance
    if (item.props.exists(ARTEFACT_APPEAR_KEY))
        return item.props[ARTEFACT_APPEAR_KEY].get_string();
    return make_artefact_name(item, true);
}

void set_artefact_name(item_def &item, const string &name)
{
    ASSERT(is_artefact(item));
    ASSERT(!name.empty());
    item.props[ARTEFACT_NAME_KEY].get_string() = name;
}

int find_unrandart_index(const item_def& artefact)
{
    return artefact.special;
}

const unrandart_entry* get_unrand_entry(int unrand_index)
{
    unrand_index -= UNRAND_START;

    if (unrand_index <= -1 || unrand_index >= NUM_UNRANDARTS)
        return &unranddata[0];  // dummy unrandart
    else
        return &unranddata[unrand_index];
}

int find_okay_unrandart(uint8_t aclass, uint8_t atype, bool in_abyss)
{
    int ret = -1;

    // Pick randomly among not-yet-existing unrandarts with the proper
    // base_type and sub_type.
    for (int i = 0, count = 0; i < NUM_UNRANDARTS; i++)
    {
        const int              index = i + UNRAND_START;
        const unrandart_entry* entry = &unranddata[i];

        // Skip dummy entries.
        if (entry->base_type == OBJ_UNASSIGNED)
            continue;

        const unique_item_status_type status =
            get_unique_item_status(index);

        if (in_abyss && status != UNIQ_LOST_IN_ABYSS
            || !in_abyss && status != UNIQ_NOT_EXISTS)
        {
            continue;
        }

        // Never randomly generated until lost in the abyss.
        if ((!in_abyss || status != UNIQ_LOST_IN_ABYSS)
            && entry->flags & UNRAND_FLAG_NOGEN)
        {
            continue;
        }

        if (entry->base_type != aclass
            || atype != OBJ_RANDOM && entry->sub_type != atype
               // Acquirement.
               && (aclass != OBJ_WEAPONS
                   || item_attack_skill(entry->base_type, atype) !=
                      item_attack_skill(entry->base_type, entry->sub_type)
                   || hands_reqd(&you, entry->base_type,
                                 atype) !=
                      hands_reqd(&you, entry->base_type,
                                 entry->sub_type)))
        {
            continue;
        }

        count++;

        if (one_chance_in(count))
            ret = index;
    }

    return ret;
}

int get_unrandart_num(const char *name)
{
    string quoted = "\"";
    quoted += name;
    quoted += "\"";
    lowercase(quoted);

    for (unsigned int i = 0; i < ARRAYSZ(unranddata); ++i)
    {
        string art = unranddata[i].name;
        art = replace_all(art, " ", "_");
        art = replace_all(art, "'", "");
        lowercase(art);
        if (art == name || art.find(quoted) != string::npos)
            return UNRAND_START + i;
    }
    return SPWPN_NORMAL;
}

static bool _randart_is_redundant(const item_def &item,
                                   artefact_properties_t &proprt)
{
    if (item.base_type != OBJ_JEWELLERY)
        return false;

    artefact_prop_type provides  = ARTP_NUM_PROPERTIES;

    switch (item.sub_type)
    {
    case RING_PROTECTION:
        provides = ARTP_AC;
        break;

    case RING_FIRE:
    case RING_PROTECTION_FROM_FIRE:
        provides = ARTP_FIRE;
        break;

    case RING_POISON_RESISTANCE:
        provides = ARTP_POISON;
        break;

    case RING_ICE:
    case RING_PROTECTION_FROM_COLD:
        provides = ARTP_COLD;
        break;

    case RING_STRENGTH:
        provides = ARTP_STRENGTH;
        break;

    case RING_SLAYING:
        provides  = ARTP_SLAYING;
        break;

    case RING_SEE_INVISIBLE:
        provides = ARTP_EYESIGHT;
        break;

    case RING_INVISIBILITY:
        provides = ARTP_INVISIBLE;
        break;

    case RING_STEALTH:
        provides = ARTP_STEALTH;
        break;

    case RING_TELEPORTATION:
        provides = ARTP_CAUSE_TELEPORTATION;
        break;

    case RING_EVASION:
        provides = ARTP_EVASION;
        break;

    case RING_DEXTERITY:
        provides = ARTP_DEXTERITY;
        break;

    case RING_INTELLIGENCE:
        provides = ARTP_INTELLIGENCE;
        break;

    case RING_MAGICAL_POWER:
        provides = ARTP_MAGICAL_POWER;
        break;

    case RING_FLIGHT:
        provides = ARTP_FLY;
        break;

    case RING_LIFE_PROTECTION:
        provides = ARTP_NEGATIVE_ENERGY;
        break;

    case RING_PROTECTION_FROM_MAGIC:
        provides = ARTP_MAGIC;
        break;

    case AMU_RAGE:
        provides = ARTP_BERSERK;
        break;

    case AMU_INACCURACY:
        provides = ARTP_SLAYING;
        break;

    case AMU_STASIS:
        provides = ARTP_PREVENT_TELEPORTATION;
        break;

    case AMU_REGENERATION:
        provides = ARTP_REGENERATION;
        break;
    }

    if (provides == ARTP_NUM_PROPERTIES)
        return false;

    if (proprt[provides] != 0)
        return true;

    return false;
}

static bool _randart_is_conflicting(const item_def &item,
                                     artefact_properties_t &proprt)
{
    if (item.base_type == OBJ_WEAPONS
        && get_weapon_brand(item) == SPWPN_HOLY_WRATH
        && (is_demonic(item)
            || proprt[ARTP_CURSED] != 0))
    {
        return true;
    }

    if (item.base_type != OBJ_JEWELLERY)
        return false;

    if (item.sub_type == AMU_STASIS
        && (proprt[ARTP_BLINK] != 0
            || proprt[ARTP_CAUSE_TELEPORTATION] != 0
            || proprt[ARTP_ANGRY] != 0
            || proprt[ARTP_BERSERK] != 0))
    {
        return true;
    }

    if (item.sub_type == RING_WIZARDRY && proprt[ARTP_INTELLIGENCE] < 0)
        return true;

    artefact_prop_type conflicts = ARTP_NUM_PROPERTIES;

    switch (item.sub_type)
    {
    case RING_LOUDNESS:
        conflicts = ARTP_STEALTH;
        break;

    case RING_FIRE:
    case RING_ICE:
    case RING_WIZARDRY:
    case RING_MAGICAL_POWER:
        conflicts = ARTP_PREVENT_SPELLCASTING;
        break;

    case RING_TELEPORTATION:
    case RING_TELEPORT_CONTROL:
        conflicts = ARTP_PREVENT_TELEPORTATION;
        break;

    case AMU_RESIST_MUTATION:
        conflicts = ARTP_MUTAGENIC;
        break;

    case AMU_RAGE:
        conflicts = ARTP_STEALTH;
        break;
    }

    if (conflicts == ARTP_NUM_PROPERTIES)
        return false;

    if (proprt[conflicts] != 0)
        return true;

    return false;
}

bool randart_is_bad(const item_def &item, artefact_properties_t &proprt)
{
    if (item.base_type == OBJ_BOOKS)
        return false;

    if (_artefact_num_props(proprt) == 0)
        return true;

    // Weapons must have a brand and at least one other property.
    if (item.base_type == OBJ_WEAPONS
        && (proprt[ARTP_BRAND] == SPWPN_NORMAL
            || _artefact_num_props(proprt) < 2))
    {
        return true;
    }

    return _randart_is_redundant(item, proprt)
           || _randart_is_conflicting(item, proprt);
}

bool randart_is_bad(const item_def &item)
{
    artefact_properties_t proprt;
    proprt.init(0);
    artefact_properties(item, proprt);

    return randart_is_bad(item, proprt);
}

static void _artefact_setup_prop_vectors(item_def &item)
{
    CrawlHashTable &props = item.props;
    if (!props.exists(ARTEFACT_PROPS_KEY))
        props[ARTEFACT_PROPS_KEY].new_vector(SV_SHORT).resize(ART_PROPERTIES);

    CrawlVector &rap = props[ARTEFACT_PROPS_KEY].get_vector();
    rap.set_max_size(ART_PROPERTIES);

    for (vec_size i = 0; i < ART_PROPERTIES; i++)
        rap[i].get_short() = 0;

    if (!item.props.exists(KNOWN_PROPS_KEY))
    {
        props[KNOWN_PROPS_KEY].new_vector(SV_BOOL).resize(ART_PROPERTIES);
        CrawlVector &known = item.props[KNOWN_PROPS_KEY].get_vector();
        known.set_max_size(ART_PROPERTIES);
        for (vec_size i = 0; i < ART_PROPERTIES; i++)
            known[i] = static_cast<bool>(false);
    }
}

// If force_mundane is true, normally mundane items are forced to
// nevertheless become artefacts.
bool make_item_randart(item_def &item, bool force_mundane)
{
    if (item.base_type != OBJ_WEAPONS
        && item.base_type != OBJ_ARMOUR
        && item.base_type != OBJ_JEWELLERY
        && item.base_type != OBJ_BOOKS)
    {
        return false;
    }

    if (item.base_type == OBJ_BOOKS)
    {
        if (item.sub_type != BOOK_RANDART_LEVEL
            && item.sub_type != BOOK_RANDART_THEME)
        {
            return false;
        }
    }

    // This item already is a randart.
    if (item.flags & ISFLAG_RANDART)
        return true;

    // Not a truly random artefact.
    if (item.flags & ISFLAG_UNRANDART)
        return false;

    // Mundane items are much less likely to be artefacts.
    if (!force_mundane && item.is_mundane() && !one_chance_in(5))
        return false;

    _artefact_setup_prop_vectors(item);
    item.flags |= ISFLAG_RANDART;

    god_type god_gift;
    (void) origin_is_god_gift(item, &god_gift);

    int randart_tries = 500;
    do
    {
        // Now that we found something, initialise the props array.
        if (--randart_tries <= 0 || !_init_artefact_properties(item))
        {
            // Something went wrong that no amount of rerolling will fix.
            item.special = 0;
            item.props.erase(ARTEFACT_PROPS_KEY);
            item.props.erase(KNOWN_PROPS_KEY);
            item.flags &= ~ISFLAG_RANDART;
            return false;
        }
    }
    while (randart_is_bad(item)
           || god_gift != GOD_NO_GOD && !_god_fits_artefact(god_gift, item));

    // get true artefact name
    if (item.props.exists(ARTEFACT_NAME_KEY))
        ASSERT(item.props[ARTEFACT_NAME_KEY].get_type() == SV_STR);
    else
        set_artefact_name(item, make_artefact_name(item, false));

    // get artefact appearance
    if (item.props.exists(ARTEFACT_APPEAR_KEY))
        ASSERT(item.props[ARTEFACT_APPEAR_KEY].get_type() == SV_STR);
    else
        item.props[ARTEFACT_APPEAR_KEY].get_string() =
            make_artefact_name(item, true);

    return true;
}

static void _make_faerie_armour(item_def &item)
{
    item_def doodad;
    for (int i=0; i<100; i++)
    {
        doodad.clear();
        doodad.base_type = item.base_type;
        doodad.sub_type = item.sub_type;
        if (!make_item_randart(doodad))
        {
            i--; // Forbidden props are not absolute, artefactness is.
            continue;
        }

        // -CAST makes no sense on someone called "the Enchantress",
        // +TELE is not implemented for monsters yet.
        if (artefact_property(doodad, ARTP_PREVENT_SPELLCASTING)
            || artefact_property(doodad, ARTP_CAUSE_TELEPORTATION))
        {
            continue;
        }

        if (one_chance_in(20))
            artefact_set_property(doodad, ARTP_CLARITY, 1);
        if (one_chance_in(20))
            artefact_set_property(doodad, ARTP_MAGICAL_POWER, 1 + random2(10));
        if (one_chance_in(20))
            artefact_set_property(doodad, ARTP_HP, random2(21) - 10);

        break;
    }
    ASSERT(is_artefact(doodad));
    ASSERT(doodad.sub_type == item.sub_type);

    doodad.props[ARTEFACT_APPEAR_KEY].get_string()
        = item.props[ARTEFACT_APPEAR_KEY].get_string();
    doodad.props.erase(ARTEFACT_NAME_KEY);
    item.props = doodad.props;
    item.plus = random2(6) + random2(6) - 2;
}

static jewellery_type octoring_types[8] =
{
    RING_SEE_INVISIBLE, RING_PROTECTION_FROM_FIRE, RING_PROTECTION_FROM_COLD,
    RING_SUSTAIN_ABILITIES, RING_STEALTH, RING_WIZARDRY, RING_MAGICAL_POWER,
    RING_LIFE_PROTECTION
};

static void _make_octoring(item_def &item)
{
    if (you.octopus_king_rings == 255)
    {
        ASSERT(you.wizard || crawl_state.test);
        item.sub_type = octoring_types[random2(8)];
        return;
    }

    int which = 0;
    do which = random2(8); while (you.octopus_king_rings & (1 << which));

    item.sub_type = octoring_types[which];

    // Save that we've found that particular type
    you.octopus_king_rings |= 1 << which;

    // If there are any types left, unset the 'already found' flag
    if (you.octopus_king_rings != 255)
        _set_unique_item_status(UNRAND_OCTOPUS_KING_RING, UNIQ_NOT_EXISTS);
}

bool make_item_unrandart(item_def &item, int unrand_index)
{
    ASSERT_RANGE(unrand_index, UNRAND_START + 1, (UNRAND_START + NUM_UNRANDARTS));

    item.special = unrand_index;

    const unrandart_entry *unrand = &unranddata[unrand_index - UNRAND_START];

    item.flags |= ISFLAG_UNRANDART;
    _artefact_setup_prop_vectors(item);
    _init_artefact_properties(item);

    if (unrand->prpty[ARTP_CURSED] != 0)
        do_curse_item(item);

    // get artefact appearance
    ASSERT(!item.props.exists(ARTEFACT_APPEAR_KEY));
    if (!(unrand->flags & UNRAND_FLAG_RANDAPP))
        item.props[ARTEFACT_APPEAR_KEY].get_string() = unrand->unid_name;
    else
    {
        item.props[ARTEFACT_APPEAR_KEY].get_string() = make_artefact_name(item, true);
        item_colour(item);
    }

    _set_unique_item_status(unrand_index, UNIQ_EXISTS);

    if (unrand_index == UNRAND_VARIABILITY)
        item.plus = random_range(-4, 16);
    else if (unrand_index == UNRAND_FAERIE)
        _make_faerie_armour(item);
    else if (unrand_index == UNRAND_OCTOPUS_KING_RING)
        _make_octoring(item);
    else if (unrand_index == UNRAND_WOE && you.species != SP_FELID
             && !you.could_wield(item, true, true))
    {
        // always wieldable, always 2-handed
        item.sub_type = WPN_BROAD_AXE;
    }

    if (!(unrand->flags & UNRAND_FLAG_RANDAPP)
        && !(unrand->flags & UNRAND_FLAG_UNIDED)
        && !strcmp(unrand->name, unrand->unid_name))
    {
        set_ident_flags(item, ISFLAG_IDENT_MASK | ISFLAG_NOTED_ID);
    }

    return true;
}

void unrand_reacts()
{
    item_def*  weapon     = you.weapon();
    const int  old_plus   = weapon ? weapon->plus : 0;

    for (int i = 0; i < NUM_EQUIP; i++)
    {
        if (you.unrand_reacts[i])
        {
            item_def&        item  = you.inv[you.equip[i]];
            const unrandart_entry* entry = get_unrand_entry(item.special);

            entry->world_reacts_func(&item);
        }
    }

    if (weapon && (old_plus != weapon->plus))
        you.wield_change = true;
}

void artefact_set_property(item_def          &item,
                            artefact_prop_type prop,
                            int                val)
{
    ASSERT(is_artefact(item));
    ASSERT(item.props.exists(ARTEFACT_PROPS_KEY));

    CrawlVector &rap_vec = item.props[ARTEFACT_PROPS_KEY].get_vector();
    ASSERT(rap_vec.get_type()     == SV_SHORT);
    ASSERT(rap_vec.size()         == ART_PROPERTIES);
    ASSERT(rap_vec.get_max_size() == ART_PROPERTIES);

    rap_vec[prop].get_short() = val;
}

template<typename Z>
static inline void artefact_pad_store_vector(CrawlVector &vec, Z value)
{
    if (vec.get_max_size() < ART_PROPERTIES)
    {
        // Authentic tribal dance to propitiate the asserts in store.cc:
        const int old_size = vec.get_max_size();
        vec.set_max_size(VEC_MAX_SIZE);
        vec.resize(ART_PROPERTIES);
        vec.set_max_size(ART_PROPERTIES);
        for (int i = old_size; i < ART_PROPERTIES; ++i)
            vec[i] = value;
    }
}

void artefact_fixup_props(item_def &item)
{
    CrawlHashTable &props = item.props;
    if (props.exists(ARTEFACT_PROPS_KEY))
        artefact_pad_store_vector(props[ARTEFACT_PROPS_KEY], short(0));

    if (props.exists(KNOWN_PROPS_KEY))
        artefact_pad_store_vector(props[KNOWN_PROPS_KEY], false);
}
