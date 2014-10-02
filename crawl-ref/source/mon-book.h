/**
 * @file
 * @brief Monster spellbook functions, types, and globals.
**/

#ifndef MONBOOK_H
#define MONBOOK_H

#include <vector>
#include "defines.h"
#include "enum.h"
#include "mon-mst.h"

enum mon_spell_slot_flags
{
    MON_SPELL_NO_FLAGS  = 0,
    MON_SPELL_EMERGENCY = 1 << 0,
    MON_SPELL_INNATE    = 1 << 1,
    MON_SPELL_WIZARD    = 1 << 2,
    MON_SPELL_PRIEST    = 1 << 3,
};

struct mon_spellbook
{
    mon_spellbook_type type;
    mon_spell_slot spells[NUM_MONSTER_SPELL_SLOTS];
};

typedef vector<vector<spell_type> > unique_books;

vector<mon_spellbook_type> get_spellbooks(const monster_info &mon);
unique_books get_unique_spells(const monster_info &mon);
#endif
