#ifndef SPL_OTHER_H
#define SPL_OTHER_H

void identify(int power, int item_slot = -1);
int cast_healing(int pow, bool divine_ability = false,
                 const coord_def& where = coord_def(0, 0),
                 bool not_self = false, targ_mode_type mode = TARG_NUM_MODES);

void antimagic();

void cast_detect_secret_doors(int pow);
int detect_traps(int pow);
int detect_items(int pow);
int detect_creatures(int pow, bool telepathic = false);
bool remove_curse(bool suppress_msg);
bool detect_curse(int scroll, bool suppress_msg);

bool entomb(int pow);
bool cast_imprison(int pow, monsters *monster, int source);

bool cast_smiting(int power, const coord_def& where);

#endif
