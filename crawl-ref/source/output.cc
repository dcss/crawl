/**
 * @file
 * @brief Functions used to print player related info.
**/

#include "AppHdr.h"

#include "output.h"

#include <cmath>
#include <cstdlib>
#include <sstream>

#include "ability.h"
#include "art-enum.h"
#include "areas.h"
#include "branch.h"
#include "colour.h"
#include "describe.h"
#ifndef USE_TILE_LOCAL
#endif
#include "english.h"
#include "env.h"
#include "files.h"
#include "god-abil.h"
#include "god-passive.h"
#include "initfile.h"
#include "item-name.h"
#include "item-prop.h"
#include "jobs.h"
#include "lang-fake.h"
#include "libutil.h"
#include "localise.h"
#include "menu.h"
#include "message.h"
#include "misc.h"
#include "mutation.h"
#include "notes.h"
#include "player-equip.h"
#include "player-stats.h"
#include "prompt.h"
#include "religion.h"
#include "scroller.h"
#include "showsymb.h"
#include "skills.h"
#include "state.h"
#include "status.h"
#include "stringutil.h"
#include "tag-version.h"
#include "throw.h"
#include "tiles-build-specific.h"
#include "transform.h"
#include "viewchar.h"
#include "view.h"
#include "xom.h"


#ifdef USE_TILE_LOCAL

/*
 * this glorious piece of code works by:
    - overriding cgotoxy and cprintf
    - mapping the x,y coordinate of each part of the HUD to a
      value in the touchui_states enum and storing the current value
    - using the current state to decide what and where to actually
      render each part of the HUD

  12345678901234567890
1 Nameaname
2 TrDk(Mak)
3  St Dx In
4  nn nn nn
5  AC SH EV
6  nn nn nn
7 W: foobar
8 Q: 20 baz
9 XXXXXXXXX      status lights
.
y  HPP MPP
 */
#include <stdarg.h>
#define CGOTOXY _cgotoxy_touchui
#define CPRINTF _cprintf_touchui
#define NOWRAP_EOL_CPRINTF _nowrap_eol_cprintf_touchui

enum touchui_states
{
    TOUCH_S_INIT  = 0x0,
    TOUCH_S_NULL  = 0x1,
    TOUCH_T_MP    = 0x0104,
    TOUCH_T_AC    = 0x0105,
    TOUCH_T_EV    = 0x0106,
    TOUCH_T_SH    = 0x0107,
    TOUCH_T_STR   = 0x1305,
    TOUCH_T_INT   = 0x1306,
    TOUCH_T_DEX   = 0x1307,
    TOUCH_V_PLACE = 0x1308,
    TOUCH_T_HP    = 0x0103,
    TOUCH_V_HP    = 0x0203, // dummy location
    TOUCH_V_MP    = 0x0904,
    TOUCH_V_AC    = 0x0505,
    TOUCH_V_EV    = 0x0506,
    TOUCH_V_SH    = 0x0507,
    TOUCH_V_STR   = 0x1805,
    TOUCH_V_INT   = 0x1806,
    TOUCH_V_DEX   = 0x1807,
    TOUCH_V_XL    = 0x0108,
    TOUCH_T_WP    = 0x0109,
    TOUCH_T_QV    = 0x010A,
    TOUCH_V_WP    = 0x0209, // dummy
    TOUCH_V_QV    = 0x020A, // dummy
    TOUCH_V_TITLE = 0x0101,
    TOUCH_V_TITL2 = 0x0102,
    TOUCH_V_LIGHT = 0x010B,
};
touchui_states TOUCH_UI_STATE = TOUCH_S_INIT;
static void _cgotoxy_touchui(int x, int y, GotoRegion region = GOTO_CRT)
{
//    printf("go to (%d,%d): ",x,y);
    if (tiles.is_using_small_layout())
        TOUCH_UI_STATE = (touchui_states)((x<<8)+y);
//    printf("[%x]: ",TOUCH_UI_STATE);
    switch (TOUCH_UI_STATE)
    {
        case TOUCH_V_HP:
        case TOUCH_T_MP:
        case TOUCH_V_TITLE:
        case TOUCH_V_TITL2:
        case TOUCH_V_XL:
        case TOUCH_V_PLACE:
        case TOUCH_S_NULL:
            // no special behaviour for these
            break;
        case TOUCH_T_STR:
            x = 1; y = 3;
            break;
        case TOUCH_T_INT:
            x = 4; y = 3;
            break;
        case TOUCH_T_DEX:
            x = 7; y = 3;
            break;
        case TOUCH_T_AC:
            x = 1; y = 5;
            break;
        case TOUCH_T_EV:
            x = 4; y = 5;
            break;
        case TOUCH_T_SH:
            x = 7; y = 5;
            break;
        case TOUCH_V_STR:
            x = 1; y = 4;
            break;
        case TOUCH_V_INT:
            x = 4; y = 4;
            break;
        case TOUCH_V_DEX:
            x = 7; y = 4;
            break;
        case TOUCH_V_AC:
            x = 2; y = 6;
            break;
        case TOUCH_V_EV:
            x = 5; y = 6;
            break;
        case TOUCH_V_SH:
            x = 8; y = 6;
            break;
        case TOUCH_T_WP:
            x = 1; y = 7;
            break;
        case TOUCH_T_QV:
            x = 1; y = 8;
            break;
        case TOUCH_V_WP:
            x = 4; y = 7;
            break;
        case TOUCH_V_QV:
            x = 4; y = 8;
            break;
        case TOUCH_V_LIGHT:
            x = 1; y = 9;
            break;
        case TOUCH_T_HP:
            x = 2; y = crawl_view.hudsz.y;
            break;
        case TOUCH_V_MP:
            x = 6; y = crawl_view.hudsz.y;
            break;
        default:
            // reset state
            TOUCH_UI_STATE = TOUCH_S_INIT;
    }
//    printf("(%d,%d): ",x,y);
    cgotoxy(x,y,region);
}

static void _cprintf_touchui(const char *format, ...)
{
    va_list args;
    string  buf;
    va_start(args, format);
    buf = vmake_stringf(format, args);

    switch (TOUCH_UI_STATE)
    {
        case TOUCH_T_MP:
        case TOUCH_V_TITL2:
        case TOUCH_V_XL:
        case TOUCH_V_PLACE:
        case TOUCH_S_NULL:
            // don't draw these
//            printf("X! %s\n",buf.c_str());
            break;
        case TOUCH_T_HP:
            TOUCH_UI_STATE = TOUCH_V_HP;
            break;
        case TOUCH_V_TITLE:
            cprintf("%s", you.your_name.c_str());
            break;
        case TOUCH_V_HP:
        case TOUCH_V_MP:
            // suppress everything after initial print; rjustify
            cprintf("%3s", buf.c_str());
            TOUCH_UI_STATE = TOUCH_S_NULL;
            break;
        case TOUCH_V_STR:
        case TOUCH_V_INT:
        case TOUCH_V_DEX:
            // rjustify to 3 chars on these
            cprintf("%3s", buf.c_str());
            break;
        case TOUCH_T_WP:
            TOUCH_UI_STATE = TOUCH_V_WP;
            cprintf("%s", buf.c_str());
            break;
        case TOUCH_T_QV:
            TOUCH_UI_STATE = TOUCH_V_QV;
            cprintf("%s", buf.c_str());
            break;
        case TOUCH_V_WP:
        case TOUCH_V_QV:
            // get rid of the hotkey; somewhat pointless in a touch-screen ui :)
            cprintf(buf.substr(3,10).c_str());
            break;

        default:
//            printf("p: %s\n",buf.c_str());
            cprintf("%s", buf.c_str());
    }
    va_end(args);
}

static void _nowrap_eol_cprintf_touchui(const char *format, ...)
{
    va_list args;
    string  buf;
    va_start(args, format);
    buf = vmake_stringf(format, args);

    // N.B. this should really be factored out and merged with the other switch-case above
    switch (TOUCH_UI_STATE)
    {
        case TOUCH_S_NULL:
            // don't print these
            break;
        case TOUCH_V_TITL2:
            cprintf(localise("%s%s %.4s", species::get_abbrev(you.species),
                                          get_job_abbrev(you.char_class),
                                          god_name(you.religion)).c_str());
            TOUCH_UI_STATE = TOUCH_S_NULL; // suppress whatever else it was going to print
            break;
        default:
//            printf("q: %s\n",buf.c_str());
            nowrap_eol_cprintf("%s", buf.c_str());
    }
    va_end(args);
}

#else
#define CGOTOXY cgotoxy
#define CPRINTF wrapcprintf
#define NOWRAP_EOL_CPRINTF nowrap_eol_cprintf
#endif

static string _god_powers();
static string _god_asterisks();
static int _god_status_colour(int default_colour);

// Colour for captions like 'Health:', 'Str:', etc.
#define HUD_CAPTION_COLOUR Options.status_caption_colour

// Colour for values, which come after captions.
static const auto HUD_VALUE_COLOUR = LIGHTGREY;

// ----------------------------------------------------------------------
// colour_bar
// ----------------------------------------------------------------------

class colour_bar
{
    typedef unsigned short colour_t;
public:
    colour_t m_default;
    colour_t m_change_pos;
    colour_t m_change_neg;
    colour_t m_empty;
    int horiz_bar_width;

    colour_bar(colour_t default_colour,
               colour_t change_pos,
               colour_t change_neg,
               colour_t empty)
        : m_default(default_colour), m_change_pos(change_pos),
          m_change_neg(change_neg), m_empty(empty),
          horiz_bar_width(-1),
          m_old_disp(-1),
          m_request_redraw_after(0)
    {
        // m_old_disp < 0 means it's invalid and needs to be initialised.
    }

    bool wants_redraw() const
    {
        return m_request_redraw_after
               && you.num_turns >= m_request_redraw_after;
    }

    void draw(int ox, int oy, int val, int max_val, int sub_val = 0)
    {
        ASSERT(val <= max_val);
        if (max_val <= 0)
        {
            m_old_disp = -1;
            return;
        }
        const int width = (horiz_bar_width != -1) ?
                                  horiz_bar_width :
                                  crawl_view.hudsz.x - (ox - 1);
        const int sub_disp = (width * val / max_val);
        int disp  = width * max(0, val - sub_val) / max_val;
        const int old_disp = (m_old_disp < 0) ? sub_disp : m_old_disp;
        m_old_disp = sub_disp;

        // Always show at least one sliver of the sub-bar, if it exists
        if (sub_val)
            disp = max(0, min(sub_disp - 1, disp));

        CGOTOXY(ox, oy, GOTO_STAT);

        textcolour(BLACK);
        for (int cx = 0; cx < width; cx++)
        {
#ifdef USE_TILE_LOCAL
            // Maybe this should use textbackground too?
            textcolour(BLACK + m_empty * 16);

            if (cx < disp)
                textcolour(BLACK + m_default * 16);
            else if (cx < sub_disp)
                textcolour(BLACK + YELLOW * 16);
            else if (old_disp >= sub_disp && cx < old_disp)
                textcolour(BLACK + m_change_neg * 16);
            putwch(' ');
#else
            if (cx < disp && cx < old_disp)
            {
                textcolour(m_default);
                putwch('=');
            }
            else if (cx < disp)
            {
                textcolour(m_change_pos);
                putwch('=');
            }
            else if (cx < sub_disp)
            {
                textcolour(YELLOW);
                putwch('=');
            }
            else if (cx < old_disp)
            {
                textcolour(m_change_neg);
                putwch('-');
            }
            else
            {
                textcolour(m_empty);
                putwch('-');
            }
#endif

            // If some change colour was rendered, redraw in a few
            // turns to clear it out.
            if (old_disp != disp)
                m_request_redraw_after = you.num_turns + 4;
            else
                m_request_redraw_after = 0;
        }

        textcolour(LIGHTGREY);
        textbackground(BLACK);
        // the cursor position is now invalid, because we are past the end of
        // the stat region: leave it somewhere valid.
        CGOTOXY(ox, oy, GOTO_STAT);
    }

    void vdraw(int ox, int oy, int val, int max_val)
    {
        // ox is width from l/h edge; oy is height from top
        // bars are 3chars wide and render down to hudsz.y-1
        const int bar_width = 3;
        const int obase     = crawl_view.hudsz.y-1;

        ASSERT(val <= max_val);
        if (max_val <= 0)
        {
            m_old_disp = -1;
            return;
        }

        const int height   = bar_width * (obase-oy+1);
        const int disp     = height * val / max_val;
        const int old_disp = (m_old_disp < 0) ? disp : m_old_disp;

        CGOTOXY(ox, obase, GOTO_STAT);

        textcolour(WHITE);
        for (int cx = 0; cx < height; cx++)
        {
            // Maybe this should use textbackground too?
            textcolour(BLACK + m_empty * 16);

            if (cx < disp)
                textcolour(BLACK + m_default * 16);
            else if (old_disp > disp && cx < old_disp)
                textcolour(BLACK + m_change_neg * 16);
            putwch(' ');

            // move up a line if we've drawn this bit of the bar
            if ((cx+1) % bar_width == 0)
                CGOTOXY(ox, obase-cx/bar_width, GOTO_STAT);

            // If some change colour was rendered, redraw in a few
            // turns to clear it out.
            if (old_disp != disp)
                m_request_redraw_after = you.num_turns + 4;
            else
                m_request_redraw_after = 0;
        }

        textcolour(LIGHTGREY);
        textbackground(BLACK);
    }

    void reset()
    {
        m_old_disp = -1;
        m_request_redraw_after = 0;
    }

 private:
    int m_old_disp;
    int m_request_redraw_after; // force a redraw at this turn count
};

static colour_bar HP_Bar(LIGHTGREEN, GREEN, RED, DARKGREY);

#ifdef USE_TILE_LOCAL
static colour_bar MP_Bar(BLUE, BLUE, LIGHTBLUE, DARKGREY);
#else
static colour_bar MP_Bar(LIGHTBLUE, BLUE, MAGENTA, DARKGREY);
#endif

#ifdef USE_TILE_LOCAL
static colour_bar Noise_Bar(WHITE, LIGHTGREY, LIGHTGREY, DARKGREY);
#else
static colour_bar Noise_Bar(LIGHTGREY, LIGHTGREY, MAGENTA, DARKGREY);
#endif

void reset_hud()
{
    HP_Bar.reset();
    MP_Bar.reset();
    Noise_Bar.reset();
}

// ----------------------------------------------------------------------
// Status display
// ----------------------------------------------------------------------

static bool _boosted_hp()
{
    return you.duration[DUR_DIVINE_VIGOUR]
           || you.berserk();
}

static bool _boosted_mp()
{
    return you.duration[DUR_DIVINE_VIGOUR];
}

static bool _boosted_ac()
{
    return you.armour_class() > you.base_ac(1);
}

static bool _boosted_ev()
{
    return you.duration[DUR_AGILITY]
           || you.props.exists(WU_JIAN_HEAVENLY_STORM_KEY)
           || acrobat_boost_active();
}

static bool _boosted_sh()
{
    return you.duration[DUR_DIVINE_SHIELD]
           || qazlal_sh_boost() > 0
           || (you.get_mutation_level(MUT_CONDENSATION_SHIELD)
                && !you.duration[DUR_ICEMAIL_DEPLETED]);
}

#ifdef DGL_SIMPLE_MESSAGING
void update_message_status()
{
    if (!SysEnv.have_messages)
        return;

    static const char * const msg = "(Hit _)";

    textcolour(LIGHTBLUE);

    CGOTOXY(crawl_view.hudsz.x - strwidth(msg) + 1, 1, GOTO_STAT);
    CPRINTF(localise(msg).c_str());

    textcolour(LIGHTGREY);
}
#endif

void update_turn_count()
{
    if (crawl_state.game_is_arena())
        return;

    // Don't update turn counter when running/resting/traveling to
    // prevent pointless screen updates.
    if (mouse_control::current_mode() == MOUSE_MODE_NORMAL
        && (you.running > 0 || you.running < 0 && Options.travel_delay == -1))
    {
        return;
    }

    const int turncount_start_x = 19 + 6;
    int ypos = 9;
    // TODO: unify this with the calculation in print_stats
    if (you.has_mutation(MUT_HP_CASTING))
        ypos--;
#ifdef USE_TILE_LOCAL
    if (tiles.is_using_small_layout())
        ypos--;
#endif
    CGOTOXY(turncount_start_x, ypos, GOTO_STAT);

    textcolour(HUD_VALUE_COLOUR);
    string time = Options.show_game_time
        ? localise("%.1f", you.elapsed_time / 10.0)
        : localise("%d", you.num_turns);
    time += localise(" (%.1f)",
            (you.elapsed_time - you.elapsed_time_at_last_input) / 10.0);

    CPRINTF("%s",
        chop_string(time, crawl_view.hudsz.x - turncount_start_x + 1).c_str());

    textcolour(LIGHTGREY);
}

static int _count_digits(int val)
{
    if (val > 999)
        return 4;
    else if (val > 99)
        return 3;
    else if (val > 9)
        return 2;
    return 1;
}

static const equipment_type e_order[] =
{
    EQ_WEAPON, EQ_SHIELD, EQ_BODY_ARMOUR, EQ_HELMET, EQ_CLOAK,
    EQ_GLOVES, EQ_BOOTS, EQ_AMULET, EQ_LEFT_RING, EQ_RIGHT_RING,
    EQ_RING_ONE, EQ_RING_TWO, EQ_RING_THREE, EQ_RING_FOUR,
    EQ_RING_FIVE, EQ_RING_SIX, EQ_RING_SEVEN, EQ_RING_EIGHT,
    EQ_RING_AMULET,
};

static void _print_stats_equip(int x, int y)
{
    CGOTOXY(x, y, GOTO_STAT);
    textcolour(HUD_CAPTION_COLOUR);
    string prefix = species::arm_count(you.species) > 2 ? "Eq: " : "Equip: ";
    cprintf(localise(prefix).c_str());
    textcolour(LIGHTGREY);
    for (equipment_type eqslot : e_order)
    {
        if (you_can_wear(eqslot))
        {
            if (you.slot_item(eqslot))
            {
                cglyph_t g = get_item_glyph(*(you.slot_item(eqslot)));
                g.col = element_colour(g.col, !Options.animate_equip_bar);
                formatted_string::parse_string(glyph_to_tagstr(g)).display();
            }
            else if (!you_can_wear(eqslot, true))
                cprintf(" ");
            else
                cprintf(".");
        }
    }
    you.gear_change = false;
}

/*
 * Print the noise bar to the HUD with appropriate coloring.
 * if in wizmode, also print the numeric noise value.
 */
static void _print_stats_noise(int x, int y)
{
    CGOTOXY(x, y, GOTO_STAT);
    if (mouse_control::current_mode() == MOUSE_MODE_NORMAL
        && (you.running > 0 || you.running < 0 && Options.travel_delay == -1))
    {
        return;
    }

    bool silence = silenced(you.pos());
    int level = silence ? 0 : you.get_noise_perception(true);
    textcolour(HUD_CAPTION_COLOUR);
    cprintf(localise("Noise: ").c_str());
    colour_t noisecolour;

    // This is calibrated roughly so that in an open-ish area:
    //   LIGHTGREY = not very likely to carry outside of your los
    //               (though it is possible depending on terrain).
    //   YELLOW = likely to carry outside of your los, up to double.
    //   RED = likely to carry at least 16 spaces, up to much further.
    //   LIGHTMAGENTA = really f*cking loud. (Gong, etc.)
    // In more enclosed areas, these values will be attenuated,
    // and this isn't represented.
    // See player::get_noise_perception for the mapping from internal noise
    // values to this 0-1000 scale.
    // NOTE: This color scheme is duplicated in player.js.
    if (level <= 333)
        noisecolour = LIGHTGREY;
    else if (level <= 666)
        noisecolour = YELLOW;
    else if (level < 1000)
        noisecolour = RED;
    else
        noisecolour = LIGHTMAGENTA;

    int bar_position;
    if (you.wizard && !silence)
    {
        Noise_Bar.horiz_bar_width = 6;
        bar_position = 10;

        // numeric noise level, basically the internal value used by noise
        // propagation (see shout.cc:noisy). The exact value is too hard to
        // interpret to show outside of wizmode, because noise propagation is
        // very complicated.
        CGOTOXY(x + bar_position - 3, y, GOTO_STAT);
        textcolour(noisecolour);
        CPRINTF("%2d ", you.get_noise_perception(false));
    }
    else
    {
        Noise_Bar.horiz_bar_width = 9;
        bar_position = 7;
    }

    if (silence)
    {
        CGOTOXY(x + bar_position, y, GOTO_STAT);
        textcolour(LIGHTMAGENTA);

        // This needs to be one extra wide in case silence happens
        // immediately after super-loud (magenta) noise
        CPRINTF(localise("Silenced  ").c_str());
        Noise_Bar.reset(); // so it doesn't display a change bar after silence ends
    }
    else
    {
        if (level == 1000)
        {
            // the bar goes up to 11 for extra loud sounds! (Well, really 10.)
            Noise_Bar.horiz_bar_width += 1;
        }
        else
        {
            CGOTOXY(x + 16, y, GOTO_STAT);
            CPRINTF(" "); // clean up after the extra wide bar
        }
#ifndef USE_TILE_LOCAL
        // use the previous color for negative change in console; there's a
        // visual difference in bar width. Negative change doesn't get shown
        // in local tiles.
        Noise_Bar.m_change_neg = Noise_Bar.m_default;
#endif
        Noise_Bar.m_default = noisecolour;
        Noise_Bar.m_change_pos = noisecolour;
        Noise_Bar.draw(x + bar_position, y,
                       div_round_up((level * Noise_Bar.horiz_bar_width), 1000),
                       Noise_Bar.horiz_bar_width);
    }
    // intentional non-reset: after it has started drawing, we always redraw
    // noise. There's not a lot of cost to this, and the logic for detecting
    // if/when silenced status has changed is extremely annoying. So
    // you.redraw_noise is esentially only used to keep the noise bar from
    // drawing while the game is starting up. (If someone can figure out how
    // to correctly detect all the silence special cases, feel free to add that
    // in and I will see if you succeeded -advil.)
}

static void _print_stats_gold(int x, int y)
{
    CGOTOXY(x, y, GOTO_STAT);
    textcolour(HUD_CAPTION_COLOUR);
    CPRINTF(localise("Gold:").c_str());
    CGOTOXY(x+6, y, GOTO_STAT);
    if (you.duration[DUR_GOZAG_GOLD_AURA])
        textcolour(LIGHTBLUE);
    else
        textcolour(HUD_VALUE_COLOUR);
    CPRINTF("%-6d", you.gold);
}

static void _print_stats_mp(int x, int y)
{
    CGOTOXY(x, y, GOTO_STAT);
    if (you.has_mutation(MUT_HP_CASTING))
    {
        clear_to_end_of_line();
        return;
    }
    // Calculate colour
    short mp_colour = HUD_VALUE_COLOUR;

    const bool boosted = _boosted_mp();

    if (boosted)
        mp_colour = LIGHTBLUE;
    else
    {
        int mp_percent = (you.max_magic_points == 0
                          ? 100
                          : (you.magic_points * 100) / you.max_magic_points);

        for (const auto &entry : Options.mp_colour)
            if (mp_percent <= entry.first)
                mp_colour = entry.second;
    }

    textcolour(HUD_CAPTION_COLOUR);
    CPRINTF(localise(player_drained() ? "MP: " : "Magic:  ").c_str());
    textcolour(mp_colour);
    CPRINTF("%d", you.magic_points);
    if (!boosted)
        textcolour(HUD_VALUE_COLOUR);
    CPRINTF("/%d", you.max_magic_points);

    int col = _count_digits(you.magic_points)
              + _count_digits(you.max_magic_points) + 1;

    int real_mp = get_real_mp(false);
    if (you.species == SP_DEEP_DWARF
        && real_mp != you.max_magic_points)
    {
        CPRINTF(" (%d)", real_mp);
        col += _count_digits(real_mp) + 3;
    }

    if (boosted)
        textcolour(HUD_VALUE_COLOUR);

    for (int i = 11-col; i > 0; i--)
        CPRINTF(" ");

#ifdef TOUCH_UI
    if (tiles.is_using_small_layout())
        MP_Bar.vdraw(6, 10, you.magic_points, you.max_magic_points);
    else
#endif
    MP_Bar.draw(19, y, you.magic_points, you.max_magic_points);

    you.redraw_magic_points = false;
}

static void _print_stats_hp(int x, int y)
{
    int max_max_hp = get_real_hp(true, false);

    // Calculate colour
    short hp_colour = HUD_VALUE_COLOUR;

    const bool boosted = _boosted_hp();

    if (boosted)
        hp_colour = LIGHTBLUE;
    else
    {
        const int hp_percent =
            (you.hp * 100) / get_real_hp(true, true);

        for (const auto &entry : Options.hp_colour)
            if (hp_percent <= entry.first)
                hp_colour = entry.second;
    }

    // 01234567890123456789
    // Health: xxx/yyy (zzz)
    CGOTOXY(x, y, GOTO_STAT);
    textcolour(HUD_CAPTION_COLOUR);
    CPRINTF(localise(player_drained() ? "HP: " : "Health: ").c_str());
    textcolour(hp_colour);
    CPRINTF("%d", you.hp);
    if (!boosted)
        textcolour(HUD_VALUE_COLOUR);
    CPRINTF("/%d", you.hp_max);
    if (max_max_hp != you.hp_max)
        CPRINTF(" (%d)", max_max_hp);
    if (boosted)
        textcolour(HUD_VALUE_COLOUR);

    int col = wherex() - crawl_view.hudp.x;
    for (int i = 18-col; i > 0; i--)
        CPRINTF(" ");

#ifdef USE_TILE_LOCAL
    if (tiles.is_using_small_layout())
        HP_Bar.vdraw(2, 10, you.hp, you.hp_max);
    else
#endif
        HP_Bar.draw(19, y, you.hp, you.hp_max, you.hp - max(0, poison_survival()));

    you.redraw_hit_points = false;
}

static short _get_stat_colour(stat_type stat)
{
    if (you.duration[stat_zero_duration(stat)])
        return LIGHTRED;

    // Check the stat_colour option for warning thresholds.
    for (const auto &entry : Options.stat_colour)
        if (you.stat(stat) <= entry.first)
            return entry.second;

    // Stat is magically increased.
    if (you.duration[DUR_DIVINE_STAMINA])
        return LIGHTBLUE;  // no end of effect warning

    // Stat is degenerated.
    if (you.stat_loss[stat] > 0)
        return YELLOW;

    return HUD_VALUE_COLOUR;
}

static void _print_stat(stat_type stat, int x, int y)
{
    CGOTOXY(x+5, y, GOTO_STAT);

    textcolour(_get_stat_colour(stat));
    CPRINTF("%d", you.stat(stat, false));

    if (you.stat_loss[stat] > 0)
        CPRINTF(" (%d)  ", you.max_stat(stat));
    else
        CPRINTF("       ");
}

static void _print_stats_ac(int x, int y)
{
    // AC:
    auto text_col = HUD_VALUE_COLOUR;
    if (_boosted_ac())
        text_col = LIGHTBLUE;
    else if (you.duration[DUR_CORROSION])
        text_col = RED;

    string ac = make_stringf("%2d ", you.armour_class());
#ifdef WIZARD
    if (you.wizard)
        ac += make_stringf("(%d%%) ", you.gdr_perc());
#endif
    textcolour(text_col);
    CGOTOXY(x+4, y, GOTO_STAT);
    CPRINTF("%-12s", ac.c_str());

    // SH: (two lines lower)
    text_col = HUD_VALUE_COLOUR;
    if (you.incapacitated() && you.shielded())
        text_col = RED;
    else if (_boosted_sh())
        text_col = LIGHTBLUE;

    string sh = make_stringf("%2d ", player_displayed_shield_class());
    textcolour(text_col);
    CGOTOXY(x+4, y+2, GOTO_STAT);
    CPRINTF("%-12s", sh.c_str());

    you.redraw_armour_class = false;
}

static void _print_stats_ev(int x, int y)
{
    CGOTOXY(x+4, y, GOTO_STAT);
    textcolour(you.duration[DUR_PETRIFYING]
               || you.cannot_move() ? RED
                                    : _boosted_ev() ? LIGHTBLUE
                                                    : HUD_VALUE_COLOUR);
    CPRINTF("%2d ", you.evasion());

    you.redraw_evasion = false;
}

/**
 * Get the appropriate colour for the UI text describing the player's weapon.
 * (Or hands/ice fists/etc, as appropriate.)
 *
 * @return     A colour enum for the player's weapon.
 */
static int _wpn_name_colour()
{
    if (you.duration[DUR_CORROSION])
        return RED;

    if (you.weapon())
    {
        const item_def& wpn = *you.weapon();

        const string prefix = item_prefix(wpn);
        const int prefcol = menu_colour(wpn.name(DESC_INVENTORY), prefix, "stats");
        if (prefcol != -1)
            return prefcol;
        return LIGHTGREY;
    }

    return get_form()->uc_colour;
}

/**
 * Print a description of the player's weapon (or lack thereof) to the UI.
 *
 * @param y     The y-coordinate to print the description at.
 */
static void _print_stats_wp(int y)
{
    string text;
    if (mouse_control::current_mode() == MOUSE_MODE_NORMAL
        && (you.running > 0 || you.running < 0 && Options.travel_delay == -1))
    {
        return;
    }

    CGOTOXY(1, y, GOTO_STAT);

    if (you.weapon())
    {
        item_def wpn = *you.weapon(); // copy

        if (you.duration[DUR_CORROSION] && wpn.base_type == OBJ_WEAPONS)
            wpn.plus -= 4 * you.props["corrosion_amount"].get_int();

        text = wpn.name(DESC_PLAIN, true, false, true);
    }
    else
        text = you.unarmed_attack_name();

    text = localise(text);

    textcolour(HUD_CAPTION_COLOUR);
    const char slot_letter = you.weapon() ? index_to_letter(you.weapon()->link)
                                          : '-';
    const string slot_name = make_stringf("%c) ", slot_letter);
    CPRINTF("%s", slot_name.c_str());
    textcolour(_wpn_name_colour());
    const int max_name_width = crawl_view.hudsz.x - slot_name.size();

    // If there is a launcher, but something unrelated is quivered, show the
    // launcher's ammo in the line with the weapon
    if (you.weapon() && is_range_weapon(*you.weapon())
        && *you.launcher_action.get() != *you.quiver_action.get())
    {
        formatted_string lammo;
        if (you.launcher_action.is_empty()
            || !you.launcher_action.get()->is_valid())
        {
            // the player has no ammo for the wielded launcher, or has
            // explicitly unquivered it
            lammo = quiver::action().quiver_description(true);
        }
        else
            lammo = you.launcher_action.get()->quiver_description(true);

        const int trimmed_size = max_name_width - lammo.tostring().size() - 3;
        CPRINTF("%s ", chop_string(text, trimmed_size).c_str());
        textcolour(LIGHTGREY);
        CPRINTF("(");
        lammo.display();
        textcolour(LIGHTGREY);
        CPRINTF(")");
    }
    else
        CPRINTF("%s", chop_string(text, max_name_width).c_str());
    textcolour(LIGHTGREY);

    you.wield_change  = false;
}

static void _print_stats_qv(int y)
{
    CGOTOXY(1, y, GOTO_STAT);
    if (mouse_control::current_mode() == MOUSE_MODE_NORMAL
        && (you.running > 0 || you.running < 0 && Options.travel_delay == -1))
    {
        return;
    }

    formatted_string qdesc = quiver::get_secondary_action()->quiver_description();
#ifdef USE_TILE_LOCAL
    const int max_width = crawl_view.hudsz.x - (tiles.is_using_small_layout() ? 0 : 4);
#else
    const int max_width = crawl_view.hudsz.x - 4;
#endif
    qdesc.chop(max_width, true).display();

    you.redraw_quiver = false;
}

struct status_light
{
    status_light(int c, string t) : colour(c), text(t) {}
    colour_t colour;
    string text;
};

static void _add_status_light_to_out(int i, vector<status_light>& out)
{
    status_info inf;

    if (fill_status_info(i, inf) && !inf.light_text.empty())
    {
        status_light sl(inf.light_colour, localise(inf.light_text));
        out.push_back(sl);
    }
}

// The colour scheme for these flags is currently:
//
// - yellow, "orange", red      for bad conditions
// - light grey, white          for god based conditions
// - green, light green         for good conditions
// - blue, light blue           for good enchantments
// - magenta, light magenta     for "better" enchantments (deflect, fly)
//
// Note the usage of bad_ench_colour() correspond to levels that
// can be found in player.cc, ie those that the player can tell by
// using the '@' command. Things like confusion and sticky flame
// hide their amounts and are thus always the same colour (so
// we're not really exposing any new information). --bwr
static void _get_status_lights(vector<status_light>& out)
{
#ifdef DEBUG_DIAGNOSTICS
    if (mouse_control::current_mode() != MOUSE_MODE_NORMAL
        || !(you.running > 0 || you.running < 0 && Options.travel_delay == -1))
    {
        static char static_pos_buf[80];
        snprintf(static_pos_buf, sizeof(static_pos_buf),
                 "%2d,%2d", you.pos().x, you.pos().y);
        out.emplace_back(LIGHTGREY, static_pos_buf);
    }
#endif

    // We used to have to hardcode every status, now we just hardcode the
    // statuses important enough to appear first. (Rightmost)
    const unsigned int important_statuses[] =
    {
        STATUS_ORB,
        STATUS_ZOT,
        STATUS_STR_ZERO, STATUS_INT_ZERO, STATUS_DEX_ZERO,
        STATUS_ALIVE_STATE,
        DUR_PARALYSIS,
        DUR_CONF,
        DUR_PETRIFYING,
        DUR_PETRIFIED,
        DUR_BERSERK,
        DUR_TELEPORT,
        DUR_HASTE,
        DUR_SLOW,
        STATUS_SPEED,
        DUR_DEATHS_DOOR,
        DUR_BERSERK_COOLDOWN,
        DUR_EXHAUSTED,
        DUR_WORD_OF_CHAOS_COOLDOWN,
        DUR_DEATHS_DOOR_COOLDOWN,
        DUR_QUAD_DAMAGE,
        STATUS_SERPENTS_LASH,
    };

    bitset<STATUS_LAST_STATUS + 1> done;
    for (unsigned important : important_statuses)
    {
        _add_status_light_to_out(important, out);
        done.set(important);
    }

    for (unsigned status = 0; status <= STATUS_LAST_STATUS ; ++status)
        if (!done[status])
            _add_status_light_to_out(status, out);
}

static void _print_status_lights(int y)
{
    vector<status_light> lights;
    static int last_number_of_lights = 0;
    _get_status_lights(lights);
    if (lights.empty() && last_number_of_lights == 0)
    {
        you.redraw_status_lights = false;
        return;
    }
    last_number_of_lights = lights.size();

    size_t line_cur = y;
    const size_t line_end = crawl_view.hudsz.y+1;

    CGOTOXY(1, line_cur, GOTO_STAT);
#ifdef ASSERTS
    if (wherex() != crawl_view.hudp.x)
    {
        save_game(false); // should be safe
        die("misaligned HUD (is %d, should be %d)", wherex(), crawl_view.hudp.x);
    }
#endif

#ifdef USE_TILE_LOCAL
    if (!tiles.is_using_small_layout())
    {
#endif
    size_t i_light = 0;
    while (true)
    {
        const int end_x = (wherex() - crawl_view.hudp.x)
                + (i_light < lights.size() ? strwidth(lights[i_light].text)
                                           : 10000);

        if (end_x <= crawl_view.hudsz.x)
        {
            textcolour(lights[i_light].colour);
            CPRINTF("%s", lights[i_light].text.c_str());
            if (end_x < crawl_view.hudsz.x)
                CPRINTF(" ");
            ++i_light;
        }
        else
        {
            clear_to_end_of_line();
            ++line_cur;
            // Careful not to trip the )#(*$ CGOTOXY ASSERT
            if (line_cur == line_end)
                break;
            CGOTOXY(1, line_cur, GOTO_STAT);
        }
    }
#ifdef USE_TILE_LOCAL
    }
    else
    {
        size_t i_light = 0;
        if (lights.size() == 1)
        {
            textcolour(lights[0].colour);
            CPRINTF("%s", lights[0].text.c_str());
        }
        else
        {
            while (i_light < lights.size() && (int)i_light < crawl_view.hudsz.x - 1)
            {
                textcolour(lights[i_light].colour);
                if (i_light == lights.size() - 1
                    && strwidth(lights[i_light].text) < crawl_view.hudsz.x - wherex())
                {
                    CPRINTF("%s",lights[i_light].text.c_str());
                }
                else if ((int)lights.size() > crawl_view.hudsz.x / 2)
                    CPRINTF("%.1s",lights[i_light].text.c_str());
                else
                    CPRINTF("%.1s ",lights[i_light].text.c_str());
                ++i_light;
            }
        }
        clear_to_end_of_line();
    }
#endif

    you.redraw_status_lights = false;
}

static void _draw_wizmode_flag(const string& word)
{
    textcolour(LIGHTMAGENTA);
    // 3+ for the " **"
    CGOTOXY(1 + crawl_view.hudsz.x - (3 + strwidth(word)), 1, GOTO_STAT);
    CPRINTF(" *%s*", word.c_str());
}

static void _redraw_title()
{
    const unsigned int WIDTH = crawl_view.hudsz.x;
    string title = you.your_name + " " +
                   filtered_lang(localise_player_title(player_title()));
    const bool small_layout =
#ifdef USE_TILE_LOCAL
                              tiles.is_using_small_layout();
#else
                              false;
#endif

    if (small_layout)
        title = you.your_name;
    else
    {
        unsigned int in_len = strwidth(title);
        if (in_len > WIDTH)
        {
            in_len -= 3;  // strwidth(" the ") - strwidth(", ")

            const unsigned int name_len = strwidth(you.your_name);
            string trimmed_name = you.your_name;
            // Squeeze name if required, the "- 8" is to not squeeze too much.
            if (in_len > WIDTH && (name_len - 8) > (in_len - WIDTH))
            {
                trimmed_name = chop_string(trimmed_name,
                                           name_len - (in_len - WIDTH) - 1);
            }

            title = trimmed_name + ", " + 
                    filtered_lang(localise_player_title(player_title(false)));
        }
    }

    // Line 1: Foo the Bar    *WIZARD*
    CGOTOXY(1, 1, GOTO_STAT);
    textcolour(small_layout && you.wizard ? LIGHTMAGENTA : YELLOW);
    CPRINTF("%s", chop_string(title, WIDTH).c_str());
    if (you.wizard && !small_layout)
        _draw_wizmode_flag(localise("WIZARD"));
    else if (you.suppress_wizard && !small_layout)
        _draw_wizmode_flag(localise("EX-WIZARD"));
    else if (you.explore && !small_layout)
        _draw_wizmode_flag(localise("EXPLORE"));
#ifdef DGL_SIMPLE_MESSAGING
    update_message_status();
#endif

    // Line 2:
    // Minotaur [of God] [Piety]
    textcolour(YELLOW);
    CGOTOXY(1, 2, GOTO_STAT);
    string species = species::name(you.species);
    if (you_worship(GOD_NO_GOD))
    {
        NOWRAP_EOL_CPRINTF("%s", localise(species).c_str());
        if (you.char_class == JOB_MONK
            && !you.has_mutation(MUT_FORLORN) // XX is this necessary?
            && !had_gods())
        {
            string godpiety = "**....";
            textcolour(DARKGREY);
            if ((unsigned int)(strwidth(species) + strwidth(godpiety) + 1) <= WIDTH)
                NOWRAP_EOL_CPRINTF(" %s", godpiety.c_str());
            clear_to_end_of_line();
        }
        else
        {
            // Still need to clear in case the player was excommunicated
            clear_to_end_of_line();
        }
    }
    else
    {
        string god = you_worship(GOD_JIYVA) ? god_name_jiyva(true)
                                            : god_name(you.religion);
        string of_god = localise(" of %s", god); // noloc
        NOWRAP_EOL_CPRINTF("%s%s", localise(species).c_str(), of_god.c_str());

        string piety = _god_asterisks();
        textcolour(_god_status_colour(YELLOW));
        const unsigned int textwidth = (unsigned int)(strwidth(species) + strwidth(god) + strwidth(piety) + 1);
        if (textwidth <= WIDTH)
            NOWRAP_EOL_CPRINTF(" %s", piety.c_str());
        else if (textwidth == (WIDTH + 1))
        {
            //mottled draconian of TSO doesn't fit by one symbol,
            //so we remove leading space.
            NOWRAP_EOL_CPRINTF("%s", piety.c_str());
        }
        clear_to_end_of_line();
        if (you_worship(GOD_GOZAG))
        {
            // "Mottled Draconian of Gozag  Gold: 99999" just fits
            _print_stats_gold(textwidth + 2, 2);
        }
    }

    textcolour(LIGHTGREY);

    you.redraw_title = false;
}

void print_stats()
{
#ifndef USE_TILE_LOCAL
    if (crawl_state.smallterm)
        return;
#endif
    int ac_pos = 5;
    int ev_pos = ac_pos + 1;

    cursor_control coff(false);
    textcolour(LIGHTGREY);

    // Displayed evasion is tied to dex/str.
    if (you.redraw_stats[STAT_DEX]
        || you.redraw_stats[STAT_STR])
    {
        you.redraw_evasion = true;
    }

    if (HP_Bar.wants_redraw())
        you.redraw_hit_points = true;

    if (MP_Bar.wants_redraw())
        you.redraw_magic_points = true;

    // Poison display depends on regen rate, so should be redrawn every turn.
    if (you.duration[DUR_POISONING])
    {
        you.redraw_hit_points = true;
        you.redraw_status_lights = true;
    }

    if (you.redraw_title)
        _redraw_title();
    if (you.redraw_hit_points)
        _print_stats_hp(1, 3);

    int rows_hidden = 0;
    // hide the MP bar for djinni
    if (you.has_mutation(MUT_HP_CASTING))
        rows_hidden++;
    else if (you.redraw_magic_points)
        _print_stats_mp(1, 4);

    // several of the following field names are printed in draw_border, not
    // here. It's supposed to be for things that don't ever change, but it's
    // mostly just a mess.
    if (you.redraw_armour_class)
        _print_stats_ac(1, ac_pos - rows_hidden);
    if (you.redraw_evasion)
        _print_stats_ev(1, ev_pos - rows_hidden);

    for (int i = 0; i < NUM_STATS; ++i)
        if (you.redraw_stats[i])
            _print_stat(static_cast<stat_type>(i), 19, 5 + i - rows_hidden);
    you.redraw_stats.init(false);

    if (you.redraw_experience)
    {
        CGOTOXY(1, 8 - rows_hidden, GOTO_STAT);
        textcolour(Options.status_caption_colour);
        CPRINTF(localise("XL: ").c_str());
        textcolour(HUD_VALUE_COLOUR);
        CPRINTF("%2d ", you.experience_level);
        if (you.experience_level >= you.get_max_xl())
            CPRINTF("%10s", "");
        else
        {
            textcolour(Options.status_caption_colour);
            CPRINTF(localise("Next: ").c_str());
            textcolour(HUD_VALUE_COLOUR);
            CPRINTF("%2d%% ", get_exp_progress());
        }
        you.redraw_experience = false;
    }

    // Line 9 is Noise and Turns
#ifdef USE_TILE_LOCAL
    if (tiles.is_using_small_layout())
        rows_hidden++;
    else
#endif
    {
        if (Options.equip_bar)
        {
             if (you.gear_change || you.wield_change)
                _print_stats_equip(1, 9 - rows_hidden);
        }
        else if (you.redraw_noise)
            _print_stats_noise(1, 9 - rows_hidden);
    }

    if (you.wield_change)
        _print_stats_wp(10 - rows_hidden);

    if (you.redraw_quiver)
        _print_stats_qv(11 - rows_hidden);

    if (you.redraw_status_lights)
        _print_status_lights(12 - rows_hidden);

#ifndef USE_TILE_LOCAL
    assert_valid_cursor_pos();
#endif
}

static string _level_description_string_hud()
{
    const PlaceInfo& place = you.get_place_info();
    string short_name = branches[place.branch].shortname;

    if (brdepth[place.branch] > 1)
        short_name += make_stringf(":%d", you.depth);
    // Indefinite articles
    else if (place.branch != BRANCH_PANDEMONIUM
             && place.branch != BRANCH_DESOLATION
             && !is_connected_branch(place.branch))
    {
        short_name = article_a(short_name);
    }
    return short_name;
}

void print_stats_level()
{
    int ypos = 8;
    // TODO: unify this with the calculation in print_stats
    if (you.has_mutation(MUT_HP_CASTING))
        ypos--;
#ifdef USE_TILE_LOCAL
    if (tiles.is_using_small_layout())
        ypos--;
#endif

    cgotoxy(19, ypos, GOTO_STAT);
    textcolour(HUD_CAPTION_COLOUR);
    CPRINTF(localise("Place: ").c_str());

    textcolour(HUD_VALUE_COLOUR);
#ifdef DEBUG_DIAGNOSTICS
    CPRINTF("(%d) ", env.absdepth0 + 1);
#endif
    CPRINTF("%s", _level_description_string_hud().c_str());
    clear_to_end_of_line();
}

void draw_border()
{
    textcolour(HUD_CAPTION_COLOUR);
    clrscr();

    textcolour(Options.status_caption_colour);

    int ac_pos;
    // TODO: unify this calculation with rows_hidden in print_stats in a
    // non-insane way
    if (you.has_mutation(MUT_HP_CASTING))
        ac_pos = 4;
    else
        ac_pos = 5;

    int ev_pos = ac_pos + 1;
    int sh_pos = ac_pos + 2;
    int str_pos = ac_pos;
    int int_pos = ev_pos;
    int dex_pos = sh_pos;

    // "Health:" and "Magic:" printed elsewhere
    CGOTOXY(1, ac_pos, GOTO_STAT); CPRINTF(localise("AC:").c_str());
    CGOTOXY(1, ev_pos, GOTO_STAT); CPRINTF(localise("EV:").c_str());
    CGOTOXY(1, sh_pos, GOTO_STAT); CPRINTF(localise("SH:").c_str());

    CGOTOXY(19, str_pos, GOTO_STAT); CPRINTF(localise("Str:").c_str());
    CGOTOXY(19, int_pos, GOTO_STAT); CPRINTF(localise("Int:").c_str());
    CGOTOXY(19, dex_pos, GOTO_STAT); CPRINTF(localise("Dex:").c_str());

    // "XL:" and "Place:" printed elsewhere
    // "Noise:" printed elsewhere
    CGOTOXY(19, ac_pos + 4, GOTO_STAT);
    CPRINTF(localise(Options.show_game_time ? "Time:" : "Turn:").c_str());
}

#ifndef USE_TILE_LOCAL
void smallterm_warning()
{
    clrscr();
    CGOTOXY(1,1, GOTO_CRT);
    string msg = localise("Your terminal window is too small; please resize to at least %d,%d", MIN_COLS, MIN_LINES);
    CPRINTF(msg.c_str());
}
#endif

void redraw_screen(bool show_updates)
{
    if (!crawl_state.need_save)
    {
        // If the game hasn't started, don't do much.
        clrscr();
        return;
    }

#ifdef USE_TILE_WEB
    if (!ui::has_layout())
        tiles.pop_all_ui_layouts();
#endif

#ifndef USE_TILE_LOCAL
    if (crawl_state.smallterm)
    {
        smallterm_warning();
        return;
    }
#endif

    draw_border();

    you.redraw_stats.init(true);
    you.redraw_title         = true;
    you.redraw_hit_points    = true;
    you.redraw_magic_points  = true;
    you.redraw_armour_class  = true;
    you.redraw_evasion       = true;
    you.redraw_experience    = true;
    you.wield_change         = true;
    you.redraw_quiver        = true;
    you.redraw_status_lights = true;
    you.redraw_noise         = true;
    you.gear_change          = true;

    print_stats();

    {
        no_notes nx;
        print_stats_level();
#ifdef DGL_SIMPLE_MESSAGING
        update_message_status();
#endif
        update_turn_count();
    }

    if (Options.messages_at_top)
    {
        display_message_window();
        viewwindow(show_updates);
    }
    else
    {
        viewwindow(show_updates);
        display_message_window();
    }

#ifndef USE_TILE_LOCAL
    assert_valid_cursor_pos();
#endif
}

// ----------------------------------------------------------------------
// Monster pane
// ----------------------------------------------------------------------

static string _get_monster_name(const monster_info& mi, int count, bool fullname)
{
    string desc = "";

    const char * const adj = mi.attitude == ATT_FRIENDLY ? "friendly"
                           : mi.attitude == ATT_HOSTILE  ? nullptr
                                                         : "neutral";

    string monpane_desc;
    int col;
    mi.to_string(count, monpane_desc, col, fullname, adj);

    if (count == 1)
    {
        if (!mi.is(MB_NAME_THE))
            desc = (is_vowel(monpane_desc[0]) ? "an " : "a ") + desc;
        else if (adj || !mi.is(MB_NAME_UNQUALIFIED))
            desc = "the " + desc;
    }

    desc += monpane_desc;
    return desc;
}

// If past is true, the messages should be printed in the past tense
// because they're needed for the morgue dump.
string mpr_monster_list(bool past)
{
    // noloc section start (only used in morgue and for debugging)
    // Get monsters via the monster_pane_info, sorted by difficulty.
    vector<monster_info> mons;
    get_monster_info(mons);

    string msg = "";
    if (mons.empty())
    {
        msg = (past
               ? "There were no monsters in sight!"
               : "There are no monsters in sight!");

        return msg;
    }

    vector<string> describe;

    int count = 0;
    for (unsigned int i = 0; i < mons.size(); ++i)
    {
        if (i > 0 && monster_info::less_than(mons[i-1], mons[i]))
        {
            describe.push_back(_get_monster_name(mons[i-1], count, true).c_str());
            count = 0;
        }
        count++;
    }

    describe.push_back(_get_monster_name(mons[mons.size()-1], count, true).c_str());

    string monster_list;
    if (describe.size() == 1)
        monster_list = describe[0];
    else
        monster_list = comma_separated_line(describe.begin(), describe.end());

    if (past)
        msg = make_stringf("You could see %s.", monster_list.c_str());
    else
        msg = make_stringf("You can see %s.", monster_list.c_str());

    return msg;
    // noloc section end
}

#ifndef USE_TILE_LOCAL
static void _print_next_monster_desc(const vector<monster_info>& mons,
                                     int& start, bool zombified = false)
{
    // Skip forward to past the end of the range of identical monsters.
    unsigned int end;
    for (end = start + 1; end < mons.size(); ++end)
    {
        // Array is sorted, so if !(m1 < m2), m1 and m2 are "equal".
        if (monster_info::less_than(mons[start], mons[end], zombified, zombified))
            break;
    }
    // Postcondition: all monsters in [start, end) are "equal"

    // Print info on the monsters we've found.
    {
        int printed = 0;

        // One glyph for each monster.
        for (unsigned int i_mon = start; i_mon < end; i_mon++)
        {
            monster_info mi = mons[i_mon];
            cglyph_t g = get_mons_glyph(mi);
            textcolour(g.col);
            CPRINTF("%s", stringize_glyph(g.ch).c_str());
            ++printed;

            // Printing too many looks pretty bad, though.
            if (i_mon > 6)
                break;
        }
        textcolour(LIGHTGREY);

        const int count = (end - start);

        if (count == 1)  // Print an icon representing damage level.
        {
            CPRINTF(" ");

            monster_info mi = mons[start];
#ifdef TARGET_OS_WINDOWS
            textcolour(real_colour(dam_colour(mi) | COLFLAG_ITEM_HEAP));
#else
            textcolour(real_colour(dam_colour(mi) | COLFLAG_REVERSE));
#endif
            CPRINTF(" ");
            textbackground(BLACK);
            textcolour(LIGHTGREY);
            CPRINTF(" ");
            printed += 3;
        }
        else
        {
            textcolour(LIGHTGREY);
            CPRINTF("  ");
            printed += 2;
        }

        if (printed < crawl_view.mlistsz.x)
        {
            int desc_colour;
            string desc;
            mons_to_string_pane(desc, desc_colour, zombified,
                                mons, start, count);
            desc = localise(desc);
            textcolour(desc_colour);
            if (static_cast<int>(desc.length()) > crawl_view.mlistsz.x - printed)
            {
                ASSERT(crawl_view.mlistsz.x - 2 - printed >= 0);
                desc.resize(crawl_view.mlistsz.x - 2 - printed, ' ');
                desc += "…)";
            }
            else
                desc.resize(crawl_view.mlistsz.x - printed, ' ');
            CPRINTF("%s", localise(desc).c_str());
        }
    }

    // Set start to the next un-described monster.
    start = end;
    textcolour(LIGHTGREY);
}
#endif

#ifndef USE_TILE_LOCAL
// #define BOTTOM_JUSTIFY_MONSTER_LIST
// Returns -1 if the monster list is empty, 0 if there are so many monsters
// they have to be consolidated, and 1 otherwise.
int update_monster_pane()
{
    if (!map_bounds(you.pos()) && !crawl_state.game_is_arena())
        return -1;

    const int max_print = crawl_view.mlistsz.y;
    textbackground(BLACK);

    if (max_print <= 0)
        return -1;

    {
        save_cursor_pos save;

        vector<monster_info> mons;
        get_monster_info(mons);

        // Count how many groups of monsters there are.
        unsigned int lines_needed = mons.size();
        for (unsigned int i = 1; i < mons.size(); i++)
            if (!monster_info::less_than(mons[i-1], mons[i]))
                --lines_needed;

        bool full_info = true;
        if (lines_needed > (unsigned int) max_print)
        {
            full_info = false;

            // Use type names rather than full names ("small zombie" vs
            // "rat zombie") in order to take up fewer lines.

            lines_needed = mons.size();
            for (unsigned int i = 1; i < mons.size(); i++)
                if (!monster_info::less_than(mons[i-1], mons[i], false, false))
                    --lines_needed;
        }

    #ifdef BOTTOM_JUSTIFY_MONSTER_LIST
        const int skip_lines = max<int>(0, crawl_view.mlistsz.y-lines_needed);
    #else
        const int skip_lines = 0;
    #endif

        // Print the monsters!
        string blank;
        blank.resize(crawl_view.mlistsz.x, ' ');
        int i_mons = 0;
        for (int i_print = 0; i_print < max_print; ++i_print)
        {
            CGOTOXY(1, 1 + i_print, GOTO_MLIST);
            // i_mons is incremented by _print_next_monster_desc
            if (i_print >= skip_lines && i_mons < (int) mons.size())
                _print_next_monster_desc(mons, i_mons, full_info);
            else
                CPRINTF("%s", blank.c_str());
        }

        if (i_mons < (int)mons.size())
        {
            // Didn't get to all of them.
            CGOTOXY(crawl_view.mlistsz.x - 2, crawl_view.mlistsz.y, GOTO_MLIST);
            textbackground(COLFLAG_REVERSE);
            CPRINTF("(…)");
            textbackground(BLACK);
        }

        assert_valid_cursor_pos();

        if (mons.empty())
            return -1;

        return full_info;
    }
}
#else
// FIXME: Implement this for Tiles!
int update_monster_pane()
{
    return false;
}
#endif

// Converts a numeric resistance to its symbolic counterpart.
// Can handle any maximum level. The default is for single level resistances
// (the most common case). Negative resistances are allowed.
// Resistances with a maximum of up to 4 are spaced (arbitrary choice), and
// starting at 5 levels, they are continuous.
// params:
//  level : actual resistance level
//  max : maximum number of levels of the resistance
//  immune : overwrites normal pip display for full immunity
static string _itosym(int level, int max = 1, bool immune = false)
{
    if (max < 1)
        return "";

    if (immune)
        return "∞";

    string sym;
    bool spacing = (max >= 5) ? false : true;

    while (max > 0)
    {
        if (level == 0)
            sym += ".";
        else if (level > 0)
        {
            sym += "+";
            --level;
        }
        else // negative resistance
        {
            sym += "x"; // noloc
            ++level;
        }
        sym += (spacing) ? " " : "";
        --max;
    }
    return sym;
}

// noloc section start
// (only translated in lowercase form, and the specific ring slots aren't translated at all)
static const char *s_equip_slot_names[] =
{
    "Weapon", "Cloak",  "Helmet", "Gloves", "Boots",
    "Shield", "Armour", "Left Ring", "Right Ring", "Amulet",
    "First Ring", "Second Ring", "Third Ring", "Fourth Ring",
    "Fifth Ring", "Sixth Ring", "Seventh Ring", "Eighth Ring",
    "Amulet Ring"
};

const char *equip_slot_to_name(int equip)
{
    COMPILE_CHECK(ARRAYSZ(s_equip_slot_names) == NUM_EQUIP);

    if (equip == EQ_RINGS
        || equip >= EQ_FIRST_JEWELLERY && equip <= EQ_LAST_JEWELLERY && equip != EQ_AMULET)
    {
        return "Ring";
    }

    if (equip == EQ_BOOTS && you.wear_barding())
        return "Barding";

    if (equip < EQ_FIRST_EQUIP || equip >= NUM_EQUIP)
        return "";

    return s_equip_slot_names[equip];
}
// noloc section end

int equip_name_to_slot(const char *s)
{
    for (int i = EQ_FIRST_EQUIP; i < NUM_EQUIP; ++i)
        if (!strcasecmp(s_equip_slot_names[i], s))
            return i;

    return -1;
}

// Colour the string according to the level of an ability/resistance.
// Take maximum possible level into account.
static const char* _determine_colour_string(int level, int max_level,
                                            bool immune = false)
{
    if (immune)
        return "<lightgreen>";

    // No colouring for larger bars.
    if (max_level > 3)
        return "<lightgrey>";

    switch (level)
    {
    case 3:
    case 2:
        if (max_level > 1)
            return "<lightgreen>";
        // else fall-through
    case 1:
        return "<green>";
    case -2:
    case -3:
        if (max_level > 1)
            return "<lightred>";
        // else fall-through
    case -1:
        return "<red>";
    default:
        return "<lightgrey>";
    }
}

int stealth_pips()
{
    // round up.
    return (player_stealth() + STEALTH_PIP - 1) / STEALTH_PIP;
}

static string _stealth_bar(int sw)
{
    string bar;
    //no colouring
    bar += _determine_colour_string(0, 5);
    bar += localise("Stlth    ");

    const int unadjusted_pips = stealth_pips();
    const int bar_len = 10;
    const int num_high_pips = unadjusted_pips % bar_len;
    static const vector<string> pip_tiers = { ".", "+", "*", "#", "!" };
    const int max_tier = pip_tiers.size() - 1;
    const int low_tier = min(unadjusted_pips / bar_len, max_tier);
    const int high_tier = min(low_tier + 1, max_tier);

    for (int i = 0; i < num_high_pips; i++)
        bar += pip_tiers[high_tier];
    for (int i = num_high_pips; i < bar_len; i++)
        bar += pip_tiers[low_tier];
    bar += "\n";
    linebreak_string(bar, sw);
    return bar;
}
static string _status_mut_rune_list(int sw);

// helper for print_overview_screen
static void _print_overview_screen_equip(column_composer& cols,
                                         vector<char>& equip_chars,
                                         int sw)
{
    sw = min(max(sw, 79), 640);

    for (equipment_type eqslot : e_order)
    {
        // leave space for all the ring slots
        if (species::arm_count(you.species) > 2
            && eqslot != EQ_WEAPON
            && !you_can_wear(eqslot))
        {
            continue;
        }

        if (species::arm_count(you.species) <= 2
            && eqslot >= EQ_RING_ONE && eqslot <= EQ_RING_EIGHT)
        {
            continue;
        }

        if (eqslot == EQ_RING_AMULET && !you_can_wear(eqslot))
            continue;

        const string slot_name_lwr = lowercase_string(equip_slot_to_name(eqslot));

        string str;

        if (you.slot_item(eqslot))
        {
            // The player has something equipped.
            const item_def& item = *you.slot_item(eqslot);
            const bool melded    = you.melded[eqslot];
            const string prefix = item_prefix(item);
            const int prefcol = menu_colour(item.name(DESC_INVENTORY), prefix);
            const int col = prefcol == -1 ? LIGHTGREY : prefcol;

            // Colour melded equipment dark grey.
            string colname = melded ? "darkgrey" : colour_to_str(col); // noloc

            const int item_idx   = you.equip[eqslot];
            const char equip_char = index_to_letter(item_idx);
            string item_name = item.name(DESC_PLAIN, true);
            item_name = localise("%s"+item_name, melded ? "melded " : "");
            item_name = chop_string(item_name, sw - 36, false);

            str = make_stringf(
                     "<w>%c</w> - <%s>%s</%s>", // noloc
                     equip_char,
                     colname.c_str(),
                     item_name.c_str(),
                     colname.c_str());
            equip_chars.push_back(equip_char);
        }
        else if (eqslot == EQ_WEAPON
                 && you.skill(SK_UNARMED_COMBAT))
        {
            str = "  - " + localise("Unarmed");
        }
        else if (eqslot == EQ_WEAPON
                 && you.form == transformation::blade_hands)
        {
            const bool plural = you.arm_count() > 1;
            str = "  - " + localise(plural ? "Blade Hands" : "Blade Hand");
        }
        else
        {
            str = "<darkgrey>("; // noloc
            if (eqslot == EQ_BOOTS && you.wear_barding())
                str += localise("no " + slot_name_lwr); // noloc
            else if (!you_can_wear(eqslot))
                str += localise("%s unavailable", slot_name_lwr);
            else if (!you_can_wear(eqslot, true))
                str += localise("%s currently unavailable", slot_name_lwr);
            else if (you_can_wear(eqslot) == MB_MAYBE)
                str += localise("%s restricted", slot_name_lwr);
            else
                str += localise("no " + slot_name_lwr); // noloc
            str += + ")</darkgrey>"; // noloc
        }
        cols.add_formatted(2, str.c_str(), false);
    }
}

static string _overview_screen_title(int sw)
{
    string title = " " + localise_player_title(player_title()) + " ";

    string species_job = localise("(%s %s)",
                                      species::name(you.species).c_str(),
                                      get_job_name(you.char_class));

    handle_real_time();
    string time_turns = localise(" Turns: %d, Time: ", you.num_turns)
                      + make_time_string(you.real_time(), true);

    const int char_width = strwidth(species_job);
    const int title_width = strwidth(title);

    int linelength = strwidth(you.your_name) + title_width
                   + char_width + strwidth(time_turns);

    if (linelength >= sw)
    {
        species_job = localise("(%s%s)",
                                    species::get_abbrev(you.species),
                                    get_job_abbrev(you.char_class));
        linelength -= (char_width - strwidth(species_job));
    }

    // Still not enough?
    if (linelength >= sw)
    {
        title = " ";
        linelength -= (title_width - 1);
    }

    string text;
    text = "<yellow>";
    text += you.your_name;
    text += title;
    text += species_job;

    const int num_spaces = sw - linelength - 1;
    if (num_spaces > 0)
        text += string(num_spaces, ' ');

    text += time_turns;
    text += "</yellow>\n";

    return text;
}

#ifdef WIZARD
static string _wiz_god_powers()
{
    string godpowers = god_name(you.religion);
    return make_stringf("%s %d (%d)", localise(god_name(you.religion)).c_str(),
                                      you.piety,
                                      you.duration[DUR_PIETY_POOL]);
}
#endif

static string _god_powers()
{
    if (you_worship(GOD_NO_GOD))
        return "";

    const string name = localise(god_name(you.religion));
    if (you_worship(GOD_GOZAG))
        return colour_string(name, _god_status_colour(god_colour(you.religion)));

    return colour_string(chop_string(name, 20, false)
              + " [" + _god_asterisks() + "]",
              _god_status_colour(god_colour(you.religion)));
}

static string _god_asterisks()
{
    if (you_worship(GOD_NO_GOD))
        return "";

    if (player_under_penance())
        return "*";

    if (you_worship(GOD_GOZAG))
        return "";

    if (you_worship(GOD_XOM))
    {
        const int p_rank = xom_favour_rank() - 1;
        if (p_rank >= 0)
        {
            return string(p_rank, '.') + "*"
                   + string(NUM_PIETY_STARS - 1 - p_rank, '.');
        }
        else
            return string(NUM_PIETY_STARS, '.'); // very special plaything
    }
    else
    {
        const int prank = piety_rank();
        return string(prank, '*') + string(NUM_PIETY_STARS - prank, '.');
    }
}

/**
 * What colour should the god status display be?
 *
 * @param default_colour   The default colour, if not under penance or boredom.
 * @return                 A colour for the god status display.
 */
static int _god_status_colour(int default_colour)
{
    if (player_under_penance())
        return RED;

    if (you_worship(GOD_XOM) && you.gift_timeout <= 1)
        return you.gift_timeout ? RED : LIGHTRED;

    return default_colour;
}

static bool _player_statrotted()
{
    return you.strength(false) != you.max_strength()
        || you.intel(false) != you.max_intel()
        || you.dex(false) != you.max_dex();
}

static vector<formatted_string> _get_overview_stats()
{
    formatted_string entry;

    // 4 columns
    int col1 = 20;
    int col2 = 10;
    int col3 = 11;

    if (player_drained())
        col1 += 1;

    if (_player_statrotted())
        col3 += 2;

    column_composer cols(4, col1, col1 + col2, col1 + col2 + col3);

    entry.textcolour(HUD_CAPTION_COLOUR);
    if (player_drained())
        entry.cprintf(localise("HP:   "));
    else
        entry.cprintf(localise("Health: "));

    if (_boosted_hp())
        entry.textcolour(LIGHTBLUE);
    else
        entry.textcolour(HUD_VALUE_COLOUR);

    entry.cprintf("%d/%d", you.hp, you.hp_max);
    if (player_drained())
        entry.cprintf(" (%d)", get_real_hp(true, false));

    cols.add_formatted(0, entry.to_colour_string(), false);
    entry.clear();

    if (!you.has_mutation(MUT_HP_CASTING))
    {
        entry.textcolour(HUD_CAPTION_COLOUR);
        if (player_drained())
            entry.cprintf(localise("MP:   "));
        else
            entry.cprintf(localise("Magic:  "));

        if (_boosted_mp())
            entry.textcolour(LIGHTBLUE);
        else
            entry.textcolour(HUD_VALUE_COLOUR);

        entry.cprintf("%d/%d", you.magic_points, you.max_magic_points);
        if (you.species == SP_DEEP_DWARF
            && get_real_mp(false) != you.max_magic_points)
        {
            entry.cprintf(" (%d)", get_real_mp(false));
        }

        cols.add_formatted(0, entry.to_colour_string(), false);
        entry.clear();
    }

    entry.textcolour(HUD_CAPTION_COLOUR);
    if (player_drained())
        entry.cprintf(localise("Gold: "));
    else
        entry.cprintf(localise("Gold:   "));

    entry.textcolour(HUD_VALUE_COLOUR);

    entry.cprintf("%d", you.gold);

    cols.add_formatted(0, entry.to_colour_string(), false);
    entry.clear();

    entry.textcolour(HUD_CAPTION_COLOUR);
    entry.cprintf(localise("AC: "));

    if (_boosted_ac())
        entry.textcolour(LIGHTBLUE);
    else
        entry.textcolour(HUD_VALUE_COLOUR);

    entry.cprintf("%2d", you.armour_class());

    cols.add_formatted(1, entry.to_colour_string(), false);
    entry.clear();

    entry.textcolour(HUD_CAPTION_COLOUR);
    entry.cprintf(localise("EV: "));

    if (_boosted_ev())
        entry.textcolour(LIGHTBLUE);
    else
        entry.textcolour(HUD_VALUE_COLOUR);

    entry.cprintf("%2d", you.evasion());

    cols.add_formatted(1, entry.to_colour_string(), false);
    entry.clear();

    entry.textcolour(HUD_CAPTION_COLOUR);
    entry.cprintf(localise("SH: "));

    if (_boosted_sh())
        entry.textcolour(LIGHTBLUE);
    else
        entry.textcolour(HUD_VALUE_COLOUR);

    entry.cprintf("%2d", player_displayed_shield_class());

    cols.add_formatted(1, entry.to_colour_string(), false);
    entry.clear();

    entry.textcolour(HUD_CAPTION_COLOUR);
    entry.cprintf(localise("Str: "));

    entry.textcolour(_get_stat_colour(STAT_STR));

    entry.cprintf("%2d", you.strength(false));
    if (you.strength(false) != you.max_strength())
        entry.cprintf(" (%d)", you.max_strength());

    cols.add_formatted(2, entry.to_colour_string(), false);
    entry.clear();

    entry.textcolour(HUD_CAPTION_COLOUR);
    entry.cprintf(localise("Int: "));

    entry.textcolour(_get_stat_colour(STAT_INT));

    entry.cprintf("%2d", you.intel(false));
    if (you.intel(false) != you.max_intel())
        entry.cprintf(" (%d)", you.max_intel());

    cols.add_formatted(2, entry.to_colour_string(), false);
    entry.clear();

    entry.textcolour(HUD_CAPTION_COLOUR);
    entry.cprintf(localise("Dex: "));

    entry.textcolour(_get_stat_colour(STAT_DEX));

    entry.cprintf("%2d", you.dex(false));
    if (you.dex(false) != you.max_dex())
        entry.cprintf(" (%d)", you.max_dex());

    cols.add_formatted(2, entry.to_colour_string(), false);
    entry.clear();

    entry.textcolour(HUD_CAPTION_COLOUR);
    entry.cprintf(localise("XL:     "));

    entry.textcolour(HUD_VALUE_COLOUR);
    entry.cprintf("%d", you.experience_level);

    if (you.experience_level < you.get_max_xl())
    {
        entry.textcolour(HUD_CAPTION_COLOUR);
        entry.cprintf(localise("   Next: "));

        entry.textcolour(HUD_VALUE_COLOUR);
        entry.cprintf("%d%%", get_exp_progress());
    }

    cols.add_formatted(3, entry.to_colour_string(), false);
    entry.clear();

    entry.textcolour(HUD_CAPTION_COLOUR);
    entry.cprintf(localise("God:    "));

    entry.textcolour(HUD_VALUE_COLOUR);

    string godpowers = _god_powers();
#ifdef WIZARD
    if (you.wizard)
        godpowers = _wiz_god_powers();
#endif
    entry += formatted_string::parse_string(godpowers);

    cols.add_formatted(3, entry.to_colour_string(), false);
    entry.clear();

    if (!you.has_mutation(MUT_INNATE_CASTER))
    {
        entry.textcolour(HUD_CAPTION_COLOUR);
        entry.cprintf(localise("Spells: "));

        entry.textcolour(HUD_VALUE_COLOUR);
        entry.cprintf(localise("%d/%d levels left",
                      player_spell_levels(), player_total_spell_levels()));

        cols.add_formatted(3, entry.to_colour_string(), false);
        entry.clear();
    }

    if (you.has_mutation(MUT_MULTILIVED))
    {
        entry.textcolour(HUD_CAPTION_COLOUR);
        entry.cprintf(localise("Lives:  "));

        entry.textcolour(HUD_VALUE_COLOUR);
        entry.cprintf("%d", you.lives);

        entry.textcolour(HUD_CAPTION_COLOUR);
        entry.cprintf(localise("   Deaths: "));

        entry.textcolour(HUD_VALUE_COLOUR);
        entry.cprintf("%d", you.deaths);

        cols.add_formatted(3, entry.to_colour_string(), false);
        entry.clear();
    }

    return cols.formatted_lines();
}

// generator of resistance strings:
// params :
//      name : name of the resist, correct spacing is handled here
//      spacing : width of the name column
//      value : actual value of the resistance (can be negative)
//      max : maximum value of the resistance (for colour AND representation),
//          default is the most common case (1)
//      pos_resist : false for "bad" resistances (no tele, random tele, *Rage),
//          inverts the value for the colour choice
//      immune : overwrites normal pip display for full immunity
static string _resist_composer(const char* name, int spacing, int value,
                               int max = 1, bool pos_resist = true,
                               bool immune = false)
{
    string out;
    out += _determine_colour_string(pos_resist ? value : -value, max, immune);
    out += chop_string(localise(name), spacing);
    out += _itosym(value, max, immune);

    return out;
}

static vector<formatted_string> _get_overview_resistances(
    vector<char> &equip_chars, bool calc_unid, int sw)
{
    // 3 columns, splits at columns 20, 33
    column_composer cols(3, 20, 33);
    // First column, resist name is up to 9 chars
    int cwidth = 9;
    string out;

    const int rfire = player_res_fire(calc_unid);
    out += _resist_composer("rFire", cwidth, rfire, 3) + "\n";

    const int rcold = player_res_cold(calc_unid);
    out += _resist_composer("rCold", cwidth, rcold, 3) + "\n";

    const int rlife = player_prot_life(calc_unid);
    out += _resist_composer("rNeg", cwidth, rlife, 3) + "\n";

    const int rpois = player_res_poison(calc_unid);
    out += _resist_composer("rPois", cwidth, rpois, 1, true, rpois == 3) + "\n";

    const int relec = player_res_electricity(calc_unid);
    out += _resist_composer("rElec", cwidth, relec) + "\n";

    const int rcorr = you.res_corr(calc_unid);
    out += _resist_composer("rCorr", cwidth, rcorr) + "\n";

    const int rmuta = (you.rmut_from_item(calc_unid)
                       || you.get_mutation_level(MUT_MUTATION_RESISTANCE) == 3);
    if (rmuta)
        out += _resist_composer("rMut", cwidth, rmuta) + "\n";

    const int rmagi = player_willpower(calc_unid) / WL_PIP;
    out += _resist_composer("Will", cwidth, rmagi, 5, true,
                            player_willpower(calc_unid) == WILL_INVULN) + "\n";

    out += _stealth_bar(20) + "\n";

    const int regen = player_regen(); // round up
    out += localise("HPRegen  %03.2f/turn", regen/100.0);
    out += "\n";

#if TAG_MAJOR_VERSION == 34
    const bool etheric = player_equip_unrand(UNRAND_ETHERIC_CAGE);
    const int mp_regen = player_mp_regen() //round up
                         + (etheric ? 50 : 0); // on average
    out += localise("MPRegen  %03.2f/turn", mp_regen / 100.0);
    out += (etheric ? "*\n" : "\n");
#else
    const int mp_regen = player_mp_regen(); // round up
    out += localise("MPRegen  %03.2f/turn", mp_regen / 100.0);
    out += "\n";
#endif

    cols.add_formatted(0, out, false);

    // Second column, resist name is 9 chars
    out.clear();
    cwidth = 9;
    const int rinvi = you.can_see_invisible(calc_unid);
    out += _resist_composer("SeeInvis", cwidth, rinvi) + "\n";

    const int faith = you.faith(calc_unid);
    out += _resist_composer("Faith", cwidth, faith) + "\n";

    const int rspir = you.spirit_shield(calc_unid);
    out += _resist_composer("Spirit", cwidth, rspir) + "\n";

    const item_def *sh = you.shield();
    const int reflect = you.reflection(calc_unid)
                        || sh && shield_reflects(*sh);
    out += _resist_composer("Reflect", cwidth, reflect) + "\n";

    const int harm = you.extra_harm(calc_unid);
    out += _resist_composer("Harm", cwidth, harm) + "\n";

    const int rampage = you.rampaging(calc_unid);
    out += _resist_composer("Rampage", cwidth, rampage, 1, true,
                            player_equip_unrand(UNRAND_SEVEN_LEAGUE_BOOTS))
           + "\n";

    const int archmagi = you.archmagi(calc_unid);
    if (archmagi)
        out += _resist_composer("Archmagi", cwidth, archmagi) + "\n";

    const int rclar = you.clarity(calc_unid);
    const int stasis = you.stasis();
    // TODO: what about different levels of anger/berserkitis?
    const bool show_angry = (you.angry(calc_unid)
                             || you.get_mutation_level(MUT_BERSERK))
                            && !rclar && !stasis
                            && !you.is_lifeless_undead();
    if (show_angry || rclar)
    {
        out += show_angry ? _resist_composer("Rnd*Rage", cwidth, 1, 1, false)
                            + "\n"
                          : _resist_composer("Clarity", cwidth, rclar) + "\n";
    }

    // Fo don't need a reminder that they can't teleport
    if (!you.stasis())
    {
        if (you.no_tele(calc_unid))
            out += _resist_composer("NoTele", cwidth, 1, 1, false) + "\n";
        else if (player_teleport(calc_unid))
            out += _resist_composer("Rnd*Tele", cwidth, 1, 1, false) + "\n";
    }

    const int no_cast = you.no_cast(calc_unid);
    if (no_cast)
        out += _resist_composer("NoCast", cwidth, 1, 1, false);

    cols.add_formatted(1, out, false);

    _print_overview_screen_equip(cols, equip_chars, sw);

    return cols.formatted_lines();
}

class overview_popup : public formatted_scroller
{
public:
    overview_popup() {};
    vector<char> equip_chars;
private:
    bool process_key(int ch) override
    {
        if (find(equip_chars.begin(), equip_chars.end(), ch) != equip_chars.end())
        {
            item_def& item = you.inv[letter_to_index(ch)];
            if (!describe_item(item))
                return false;
            return true;
        }
        return formatted_scroller::process_key(ch);
    };
};

void print_overview_screen()
{
    // TODO: this should handle window resizes
    constexpr int num_cols = 80;
    bool calc_unid = false;
    overview_popup overview;

    overview.set_more();
    overview.set_tag("resists");

    overview.add_text(_overview_screen_title(num_cols));

    for (const formatted_string &bline : _get_overview_stats())
        overview.add_formatted_string(bline, true);
    overview.add_text("\n");

    {
        vector<formatted_string> blines =
            _get_overview_resistances(overview.equip_chars, calc_unid, num_cols);

        for (unsigned int i = 0; i < blines.size(); ++i)
            overview.add_text(blines[i].to_colour_string() + "\n");
    }

    overview.add_text("\n");
    overview.add_text(_status_mut_rune_list(num_cols));
    overview.show();
}

string dump_overview_screen(bool full_id)
{
    string text = formatted_string::parse_string(_overview_screen_title(80));
    text += "\n";

    for (const formatted_string &bline : _get_overview_stats())
    {
        text += bline;
        text += "\n";
    }
    text += "\n";

    vector<char> equip_chars;
    for (const formatted_string &bline
            : _get_overview_resistances(equip_chars, full_id, 640))
    {
        text += bline;
        text += "\n";
    }
    text += "\n";

    text += formatted_string::parse_string(_status_mut_rune_list(80));

    string ability_list = formatted_string::parse_string(print_abilities());
    linebreak_string(ability_list, 80);
    text += ability_list;

    text += "\n";

    return text;
}

/// Creates rows of short descriptions for current status effects, mutations,
/// and runes/Orbs of Zot.
string _status_mut_rune_list(int sw)
{
    // print status information
    string text = "<w>@:</w> "; // noloc
    vector<string> status;

    status_info inf;
    for (unsigned i = 0; i <= STATUS_LAST_STATUS; ++i)
    {
        if (fill_status_info(i, inf) && !inf.short_text.empty())
            status.emplace_back(localise(inf.short_text));
    }

    int move_cost = (player_speed() * player_movement_speed()) / 10;
    if (move_cost != 10)
    {
        const char *help = (move_cost <   8) ? "very quick" :
                           (move_cost <  10) ? "quick" :
                           (move_cost <  13) ? "slow"
                                             : "very slow";
        status.emplace_back(localise(help));
    }

    if (status.empty())
        text += localise("no status effects");
    else
        text += comma_separated_line(status.begin(), status.end(), ", ", ", ");
    text += "\n";

    // print mutation information
    text += "<w>A:</w> "; // noloc

    text += terse_mutation_list();

    // print the Orb
    if (player_has_orb())
    {
        text += "\n<w>0:</w> "; // noloc
        text += localise("Orb of Zot");
    }

    // print runes
    vector<string> runes;
    for (int i = 0; i < NUM_RUNE_TYPES; i++)
        if (you.runes[i])
            runes.emplace_back(rune_type_name(i));
    if (!runes.empty())
    {
        text += make_stringf("\n<w>%s:</w> ", // noloc
                    stringize_glyph(get_item_symbol(SHOW_ITEM_MISCELLANY)).c_str());
        text += localise("%d/%d runes: ", (int)runes.size(), you.obtainable_runes);
        text += localise(comma_separated_line(runes.begin(), runes.end(), ", ", ", "));


    }

    linebreak_string(text, sw);

    return text;
}
