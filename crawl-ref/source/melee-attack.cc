/**
 * @file
 * @brief melee_attack class and associated melee_attack methods
 */

#include "AppHdr.h"

#include "melee-attack.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "areas.h"
#include "art-enum.h"
#include "attitude-change.h"
#include "bloodspatter.h"
#include "chardump.h"
#include "cloud.h"
#include "delay.h"
#include "english.h"
#include "env.h"
#include "exercise.h"
#include "fineff.h"
#include "god-conduct.h"
#include "god-item.h"
#include "god-passive.h" // passive_t::convert_orcs
#include "hints.h"
#include "item-prop.h"
#include "localise.h"
#include "mapdef.h"
#include "message.h"
#include "message-util.h"
#include "mon-behv.h"
#include "mon-poly.h"
#include "mon-tentacle.h"
#include "religion.h"
#include "shout.h"
#include "spl-damage.h"
#include "state.h"
#include "stepdown.h"
#include "stringutil.h"
#include "tag-version.h"
#include "target.h"
#include "terrain.h"
#include "transform.h"
#include "traps.h"
#include "unwind.h"
#include "view.h"
#include "xom.h"

#ifdef NOTE_DEBUG_CHAOS_BRAND
    #define NOTE_DEBUG_CHAOS_EFFECTS
#endif

#ifdef NOTE_DEBUG_CHAOS_EFFECTS
    #include "notes.h"
#endif

/*
 **************************************************
 *             BEGIN PUBLIC FUNCTIONS             *
 **************************************************
*/
melee_attack::melee_attack(actor *attk, actor *defn,
                           int attack_num, int effective_attack_num,
                           bool is_cleaving)
    :  // Call attack's constructor
    ::attack(attk, defn),

    attack_number(attack_num), effective_attack_number(effective_attack_num),
    cleaving(is_cleaving), is_riposte(false), is_projected(false), roll_dist(0),
    wu_jian_attack(WU_JIAN_ATTACK_NONE),
    wu_jian_number_of_targets(1)
{
    attack_occurred = false;
    damage_brand = attacker->damage_brand(attack_number);
    init_attack(SK_UNARMED_COMBAT, attack_number);
    if (weapon && !using_weapon())
        wpn_skill = SK_FIGHTING;

    attack_position = attacker->pos();
}

bool melee_attack::can_reach()
{
    return attk_type == AT_HIT && weapon && weapon_reach(*weapon) > REACH_NONE
           || flavour_has_reach(attk_flavour)
           || is_projected;
}

bool melee_attack::handle_phase_attempted()
{
    // Skip invalid and dummy attacks.
    if (defender && (!adjacent(attack_position, defender->pos())
                     && !can_reach())
        || attk_flavour == AF_CRUSH
           && (!attacker->can_constrict(defender, true)
               || attacker->is_monster() && attacker->mid == MID_PLAYER))
    {
        --effective_attack_number;

        return false;
    }

    if (attacker->is_player() && defender && defender->is_monster())
    {
        // Unrands with secondary effects that can harm nearby friendlies.
        // Don't prompt for confirmation (and leak information about the
        // monster's position) if the player can't see the monster.
        if (weapon && is_unrandom_artefact(*weapon, UNRAND_DEVASTATOR)
            && you.can_see(*defender))
        {

            targeter_smite hitfunc(attacker, 1, 1, 1, false);
            hitfunc.set_aim(defender->pos());

            if (stop_attack_prompt(hitfunc, "attack", nullptr, nullptr,
                                   defender->as_monster()))
            {
                cancel_attack = true;
                return false;
            }
        }
        else if (weapon &&
                ((is_unrandom_artefact(*weapon, UNRAND_SINGING_SWORD)
                  && !silenced(you.pos()))
                 || is_unrandom_artefact(*weapon, UNRAND_VARIABILITY))
                 && you.can_see(*defender))
        {
            targeter_radius hitfunc(&you, LOS_NO_TRANS);

            if (stop_attack_prompt(hitfunc, "attack",
                                   [](const actor *act)
                                   {
                                       return !god_protects(act->as_monster());
                                   }, nullptr, defender->as_monster()))
            {
                cancel_attack = true;
                return false;
            }
        }
        else if (weapon && is_unrandom_artefact(*weapon, UNRAND_TORMENT)
                 && you.can_see(*defender))
        {
            targeter_radius hitfunc(&you, LOS_NO_TRANS);

            if (stop_attack_prompt(hitfunc, "attack",
                                   [] (const actor *m)
                                   {
                                       return !m->res_torment();
                                   },
                                   nullptr, defender->as_monster()))
            {
                cancel_attack = true;
                return false;
            }
        }
        else if (weapon && is_unrandom_artefact(*weapon, UNRAND_ARC_BLADE)
                 && you.can_see(*defender))
        {
            vector<const actor *> exclude;
            if (!safe_discharge(defender->pos(), exclude))
            {
                cancel_attack = true;
                return false;
            }
        }
        else if (weapon && is_unrandom_artefact(*weapon, UNRAND_POWER)
                 && you.can_see(*defender))
        {
            targeter_beam hitfunc(&you, 4, ZAP_SWORD_BEAM, 100, 0, 0);
            hitfunc.beam.aimed_at_spot = false;
            hitfunc.set_aim(defender->pos());

            if (stop_attack_prompt(hitfunc, "attack",
                                   [](const actor *act)
                                   {
                                       return !god_protects(act->as_monster());
                                   }, nullptr, defender->as_monster()))
            {
                cancel_attack = true;
                return false;
            }
        }
        else if (!cleave_targets.empty())
        {
            targeter_cleave hitfunc(attacker, defender->pos());
            if (stop_attack_prompt(hitfunc, "attack"))
            {
                cancel_attack = true;
                return false;
            }
        }
        else if (stop_attack_prompt(defender->as_monster(), false,
                                    attack_position))
        {
            cancel_attack = true;
            return false;
        }
    }

    if (attacker->is_player())
    {
        // Set delay now that we know the attack won't be cancelled.
        if (!is_riposte
             && (wu_jian_attack == WU_JIAN_ATTACK_NONE))
        {
            you.time_taken = you.attack_delay().roll();
        }

        const caction_type cact_typ = is_riposte ? CACT_RIPOSTE : CACT_MELEE;
        if (weapon)
        {
            if (weapon->base_type == OBJ_WEAPONS)
                if (is_unrandom_artefact(*weapon)
                    && get_unrand_entry(weapon->unrand_idx)->type_name)
                {
                    count_action(cact_typ, weapon->unrand_idx);
                }
                else
                    count_action(cact_typ, weapon->sub_type);
            else if (weapon->base_type == OBJ_STAVES)
                count_action(cact_typ, WPN_STAFF);
        }
        else
            count_action(cact_typ, -1, -1); // unarmed subtype/auxtype
    }
    else
    {
        // Only the first attack costs any energy.
        if (!effective_attack_number)
        {
            int energy = attacker->as_monster()->action_energy(EUT_ATTACK);
            int delay = attacker->attack_delay().roll();
            dprf(DIAG_COMBAT, "Attack delay %d, multiplier %1.1f", delay, energy * 0.1);
            ASSERT(energy > 0);
            ASSERT(delay > 0);

            attacker->as_monster()->speed_increment
                -= div_rand_round(energy * delay, 10);
        }

        // Statues and other special monsters which have AT_NONE need to lose
        // energy, but otherwise should exit the melee attack now.
        if (attk_type == AT_NONE)
            return false;
    }

    if (attacker != defender && !is_riposte)
    {
        // Allow setting of your allies' target, etc.
        attacker->attacking(defender);

        check_autoberserk();
    }

    // Xom thinks fumbles are funny...
    if (attacker->fumbles_attack())
    {
        // ... and thinks fumbling when trying to hit yourself is just
        // hilarious.
        xom_is_stimulated(attacker == defender ? 200 : 10);
        return false;
    }
    // Non-fumbled self-attacks due to confusion are still pretty funny, though.
    else if (attacker == defender && attacker->confused())
        xom_is_stimulated(100);

    // Any attack against a monster we're afraid of has a chance to fail
    if (attacker->is_player() && defender &&
        you.afraid_of(defender->as_monster()) && one_chance_in(3))
    {
        mprf("You attempt to attack %s, but flinch away in fear!",
             defender->name(DESC_THE).c_str());
        return false;
    }

    if (attk_flavour == AF_SHADOWSTAB
        && defender && !defender->can_see(*attacker))
    {
        if (you.see_cell(attack_position))
        {
            if (defender->is_player())
            {
                mprf("%s strikes at you from the darkness!",
                    attacker->name(DESC_THE, true).c_str());
            }
            else
            {
                mprf("%s strikes at %s from the darkness!",
                    attacker->name(DESC_THE, true).c_str(),
                    defender->name(DESC_THE).c_str());
            }
        }
        to_hit = AUTOMATIC_HIT;
        needs_message = false;
    }
    else if (attacker->is_monster()
             && attacker->type == MONS_DROWNED_SOUL)
    {
        to_hit = AUTOMATIC_HIT;
    }

    attack_occurred = true;

    // Check for player practicing dodging
    if (defender && defender->is_player())
        practise_being_attacked();

    return true;
}

bool melee_attack::handle_phase_dodged()
{
    did_hit = false;

    if (needs_message)
    {
        // TODO: Unify these, placed player_warn_miss here so I can remove
        // player_attack
        if (attacker->is_player())
            player_warn_miss();
        else if (ev_margin <= -20)
        {
            do_3rd_person_message(attacker, defender,
                                  "%s completely misses %s.");
        }
        else if (ev_margin <= -12)
            do_3rd_person_message(attacker, defender, "%s misses %s.");
        else if (ev_margin <= -6)
            do_3rd_person_message(attacker, defender, "%s closely misses %s.");
        else
            do_3rd_person_message(attacker, defender, "%s barely misses %s.");
    }

    if (attacker->is_player())
    {
        // Upset only non-sleeping non-fleeing monsters if we missed.
        if (!defender->asleep() && !mons_is_fleeing(*defender->as_monster()))
            behaviour_event(defender->as_monster(), ME_WHACK, attacker);
    }

    if (defender->is_player())
        count_action(CACT_DODGE, DODGE_EVASION);

    if (attacker != defender && adjacent(defender->pos(), attack_position)
        && attacker->alive() && defender->can_see(*attacker)
        && !defender->cannot_act() && !defender->confused()
        && (!defender->is_player() || (!you.duration[DUR_LIFESAVING]
                                       && !attacker->as_monster()->neutral()))
        && !mons_aligned(attacker, defender) // confused friendlies attacking
        // Retaliation only works on the first attack in a round.
        // FIXME: player's attack is -1, even for auxes
        && effective_attack_number <= 0)
    {
        if (defender->is_player()
                ? you.has_mutation(MUT_REFLEXIVE_HEADBUTT)
                : mons_species(mons_base_type(*defender->as_monster()))
                    == MONS_MINOTAUR)
        {
            do_minotaur_retaliation();
        }

        // Retaliations can kill!
        if (!attacker->alive())
            return false;

        if (defender->is_player() && player_equip_unrand(UNRAND_STARLIGHT))
            do_starlight();

        if (defender->is_player())
        {
            const bool using_lbl = defender->weapon()
                && item_attack_skill(*defender->weapon()) == SK_LONG_BLADES;
            const bool using_fencers = player_equip_unrand(UNRAND_FENCERS)
                && (!defender->weapon()
                    || is_melee_weapon(*defender->weapon()));
            const int chance = using_lbl + using_fencers;

            if (x_chance_in_y(chance, 3) && !is_riposte) // no ping-pong!
                riposte();

            // Retaliations can kill!
            if (!attacker->alive())
                return false;
        }
    }

    return true;
}

void melee_attack::apply_black_mark_effects()
{
    // Less reliable effects for players.
    if (attacker->is_player()
        && you.has_mutation(MUT_BLACK_MARK)
        && one_chance_in(5)
        || attacker->is_monster()
           && attacker->as_monster()->has_ench(ENCH_BLACK_MARK))
    {
        if (!defender->alive())
            return;

        switch (random2(3))
        {
            case 0:
                antimagic_affects_defender(damage_done * 8);
                break;
            case 1:
                defender->weaken(attacker, 6);
                break;
            case 2:
                defender->drain(attacker, false, damage_done);
                break;
        }
    }
}

void melee_attack::do_ooze_engulf()
{
    if (attacker->is_player()
        && you.has_mutation(MUT_ENGULF)
        && defender->alive()
        && !defender->as_monster()->has_ench(ENCH_WATER_HOLD)
        && attacker->can_constrict(defender, true, true)
        && coinflip())
    {
        defender->as_monster()->add_ench(mon_enchant(ENCH_WATER_HOLD, 1,
                                                     attacker, 1));
        mprf("You engulf %s in ooze!", defender->name(DESC_THE).c_str());
        // Smothers sticky flame.
        defender->expose_to_element(BEAM_WATER, 0);
    }
}

/* An attack has been determined to have hit something
 *
 * Handles to-hit effects for both attackers and defenders,
 * Determines damage and passes off execution to handle_phase_damaged
 * Also applies weapon brands
 *
 * Returns true if combat should continue, false if it should end here.
 */
bool melee_attack::handle_phase_hit()
{
    did_hit = true;
    perceived_attack = true;
    bool hit_woke_orc = false;

    if (attacker->is_player())
    {
        if (crawl_state.game_is_hints())
            Hints.hints_melee_counter++;

        // TODO: Remove this (placed here so I can get rid of player_attack)
        if (have_passive(passive_t::convert_orcs)
            && mons_genus(defender->mons_species()) == MONS_ORC
            && !defender->is_summoned()
            && !defender->as_monster()->is_shapeshifter()
            && you.see_cell(defender->pos()) && defender->asleep())
        {
            hit_woke_orc = true;
        }
    }

    damage_done = 0;
    // Slimify does no damage and serves as an on-hit effect, handle it
    if (attacker->is_player() && you.duration[DUR_SLIMIFY]
        && mon_can_be_slimified(defender->as_monster())
        && !cleaving)
    {
        // Bail out after sliming so we don't get aux unarmed and
        // attack a fellow slime.
        slimify_monster(defender->as_monster());
        you.duration[DUR_SLIMIFY] = 0;

        return false;
    }

    // This does more than just calculate the damage, it also sets up
    // messages, etc. It also wakes nearby creatures on a failed stab,
    // meaning it could have made the attacked creature vanish. That
    // will be checked early in player_monattack_hit_effects
    damage_done += calc_damage();

    bool stop_hit = false;
    // Check if some hit-effect killed the monster.
    if (attacker->is_player())
        stop_hit = !player_monattk_hit_effects();

    // check_unrand_effects is safe to call with a dead defender, so always
    // call it, even if the hit effects said to stop.
    if (stop_hit)
    {
        check_unrand_effects();
        return false;
    }

    if (damage_done > 0 || flavour_triggers_damageless(attk_flavour))
    {
        if (!handle_phase_damaged())
            return false;

        // TODO: Remove this, (placed here to remove player_attack)
        if (attacker->is_player() && hit_woke_orc)
        {
            // Call function of orcs first noticing you, but with
            // beaten-up conversion messages (if applicable).
            beogh_follower_convert(defender->as_monster(), true);
        }
    }
    else if (needs_message)
    {
        string msg;
        if (attacker->is_monster())
        {
            msg = localise(mons_attack_message(), atk_name(DESC_THE),
                           defender_name(true));
            msg += localise(" but does no damage.");
        }
        else
        {
            msg = localise(player_attack_message(0), defender_name(false));
            msg += localise(" but do no damage.");
        }
        mpr_nolocalise(msg);
    }

    // Check for weapon brand & inflict that damage too
    apply_damage_brand();

    // Fireworks when using Serpent's Lash to kill.
    if (!defender->alive()
        && defender->as_monster()->can_bleed()
        && wu_jian_has_momentum(wu_jian_attack))
    {
        blood_spray(defender->pos(), defender->as_monster()->type,
                    damage_done / 5);
        defender->as_monster()->flags |= MF_EXPLODE_KILL;
    }

    if (check_unrand_effects())
        return false;

    if (damage_done > 0)
    {
        apply_black_mark_effects();
        do_ooze_engulf();
    }

    if (attacker->is_player())
    {
        // Always upset monster regardless of damage.
        // However, successful stabs inhibit shouting.
        behaviour_event(defender->as_monster(), ME_WHACK, attacker,
                        coord_def(), !stab_attempt);

        // [ds] Monster may disappear after behaviour event.
        if (!defender->alive())
            return true;
    }
    else if (defender->is_player())
    {
        // These effects (mutations right now) are only triggered when
        // the player is hit, each of them will verify their own required
        // parameters.
        do_passive_freeze();
        emit_foul_stench();
    }

    return true;
}

bool melee_attack::handle_phase_damaged()
{
    if (!attack::handle_phase_damaged())
        return false;

    return true;
}

bool melee_attack::handle_phase_aux()
{
    if (attacker->is_player()
        && !cleaving
        && wu_jian_attack != WU_JIAN_ATTACK_TRIGGERED_AUX
        && !is_projected)
    {
        // returns whether an aux attack successfully took place
        // additional attacks from cleave don't get aux
        if (!defender->as_monster()->friendly()
            && adjacent(defender->pos(), attack_position))
        {
            player_aux_unarmed();
        }

        // Don't print wounds after the first attack with Gyre/Gimble.
        // DUR_CLEAVE and Gyre/Gimble interact poorly together at the moment,
        // so don't try to skip print_wounds in that case.
        if (!(weapon && is_unrandom_artefact(*weapon, UNRAND_GYRE)
              && !you.duration[DUR_CLEAVE]))
        {
            print_wounds(*defender->as_monster());
        }
    }

    return true;
}

/**
 * Handle effects that fire when the defender (the target of the attack) is
 * killed.
 *
 * @return  Not sure; it seems to never be checked & always be true?
 */
bool melee_attack::handle_phase_killed()
{
    // Wyrmbane needs to be notified of deaths, including ones due to aux
    // attacks, but other users of melee_effects() don't want to possibly
    // be called twice. Adding another entry for a single artefact would
    // be overkill, so here we call it by hand. check_unrand_effects()
    // avoided triggering Wyrmbane's death effect earlier in the attack.
    if (unrand_entry && weapon && weapon->unrand_idx == UNRAND_WYRMBANE)
    {
        unrand_entry->melee_effects(weapon, attacker, defender,
                                               true, special_damage);
    }

    return attack::handle_phase_killed();
}

static void _handle_spectral_brand(const actor &attacker, const actor &defender)
{
    if (you.triggered_spectral || !defender.alive())
        return;
    you.triggered_spectral = true;
    spectral_weapon_fineff::schedule(attacker, defender);
}

bool melee_attack::handle_phase_end()
{
    if (!cleave_targets.empty() && !simu)
    {
        attack_cleave_targets(*attacker, cleave_targets, attack_number,
                              effective_attack_number, wu_jian_attack,
                              is_projected);
    }

    // Check for passive mutation effects.
    if (defender->is_player() && defender->alive() && attacker != defender)
    {
        mons_do_eyeball_confusion();
        mons_do_tendril_disarm();
    }

    if (attacker->alive()
        && attacker->is_monster()
        && attacker->as_monster()->has_ench(ENCH_ROLLING))
    {
        attacker->as_monster()->del_ench(ENCH_ROLLING);
    }

    if (attacker->is_player() && defender)
    {
        if (damage_brand == SPWPN_SPECTRAL)
            _handle_spectral_brand(*attacker, *defender);
        // Use the Nessos hack to give the player glaive of the guard spectral too
        if (weapon && is_unrandom_artefact(*weapon, UNRAND_GUARD))
            _handle_spectral_brand(*attacker, *defender);
    }

    return attack::handle_phase_end();
}

/* Initiate the processing of the attack
 *
 * Called from the main code (fight.cc), this method begins the actual combat
 * for a particular attack and is responsible for looping through each of the
 * appropriate phases (which then can call other related phases within
 * themselves).
 *
 * Returns whether combat was completely successful
 *      If combat was not successful, it could be any number of reasons, like
 *      the defender or attacker dying during the attack? or a defender moving
 *      from its starting position.
 */
bool melee_attack::attack()
{
    if (!cleaving)
    {
        cleave_setup();
        if (!handle_phase_attempted())
            return false;
    }

    if (attacker != defender && attacker->is_monster()
        && mons_self_destructs(*attacker->as_monster()))
    {
        attacker->self_destruct();
        return did_hit = perceived_attack = true;
    }

    string saved_gyre_name;
    if (weapon && is_unrandom_artefact(*weapon, UNRAND_GYRE))
    {
        saved_gyre_name = get_artefact_name(*weapon);
        // i18n: No need for explicit translation for these. Handled as if
        // they were random artefacts.
        set_artefact_name(*weapon, cleaving ? "quick blade \"Gimble\"" // @noloc
                                            : "quick blade \"Gyre\""); // @noloc
    }

    // Restore gyre's name before we return. We cannot use an unwind_var here
    // because the precise address of the ARTEFACT_NAME_KEY property might
    // change, for example if a summoned item is reset.
    ON_UNWIND
    {
        if (!saved_gyre_name.empty() && weapon
                && is_unrandom_artefact(*weapon, UNRAND_GYRE))
        {
            set_artefact_name(*weapon, saved_gyre_name);
        }
    };

    // Attacker might have died from effects of cleaving handled prior to this
    if (!attacker->alive())
        return false;

    // We might have killed the kraken target by cleaving a tentacle.
    if (!defender->alive())
    {
        handle_phase_killed();
        handle_phase_end();
        return attack_occurred;
    }

    // Apparently I'm insane for believing that we can still stay general past
    // this point in the combat code, mebe I am! --Cryptic

    // Calculate various ev values and begin to check them to determine the
    // correct handle_phase_ handler.
    const int ev = defender->evasion(ev_ignore::none, attacker);
    ev_margin = test_hit(to_hit, ev, !attacker->is_player());
    bool shield_blocked = attack_shield_blocked(true);

    // Stuff for god conduct, this has to remain here for scope reasons.
    god_conduct_trigger conducts[3];

    if (attacker->is_player() && attacker != defender)
    {
        set_attack_conducts(conducts, *defender->as_monster(),
                            you.can_see(*defender));

        if (player_under_penance(GOD_ELYVILON)
            && god_hates_your_god(GOD_ELYVILON)
            && ev_margin >= 0
            && one_chance_in(20))
        {
            simple_god_message(" blocks your attack.", GOD_ELYVILON);
            handle_phase_end();
            return false;
        }
        // Check for stab (and set stab_attempt and stab_bonus)
        player_stab_check();
        // Make sure we hit if we passed the stab check.
        if (stab_attempt && stab_bonus > 0)
        {
            ev_margin = AUTOMATIC_HIT;
            shield_blocked = false;
        }

        // Serpent's Lash does not miss
        if (wu_jian_has_momentum(wu_jian_attack))
           ev_margin = AUTOMATIC_HIT;
    }

    if (shield_blocked)
        handle_phase_blocked();
    else
    {
        if (attacker != defender
            && (adjacent(defender->pos(), attack_position) || is_projected)
            && !is_riposte)
        {
            // Check for defender Spines
            do_spines();

            // Spines can kill! With Usk's pain bond, they can even kill the
            // defender.
            if (!attacker->alive() || !defender->alive())
                return false;
        }

        if (ev_margin >= 0)
        {
            bool cont = handle_phase_hit();

            attacker_sustain_passive_damage();

            if (!cont)
            {
                if (!defender->alive())
                    handle_phase_killed();
                handle_phase_end();
                return false;
            }
        }
        else
            handle_phase_dodged();
    }

    // don't crash on banishment
    if (!defender->pos().origin())
        handle_noise(defender->pos());

    // Noisy weapons.
    if (attacker->is_player()
        && weapon
        && is_artefact(*weapon)
        && artefact_property(*weapon, ARTP_NOISE))
    {
        noisy_equipment();
    }

    alert_defender();

    if (!defender->alive())
        handle_phase_killed();

    handle_phase_aux();

    handle_phase_end();

    return attack_occurred;
}

void melee_attack::check_autoberserk()
{
    if (defender->is_monster() && mons_is_firewood(*defender->as_monster()))
        return;

    if (attacker->is_player())
    {
        for (int i = EQ_FIRST_EQUIP; i < NUM_EQUIP; ++i)
        {
            const item_def *item = you.slot_item(static_cast<equipment_type>(i));
            if (!item)
                continue;

            if (!is_artefact(*item))
                continue;

            if (x_chance_in_y(artefact_property(*item, ARTP_ANGRY), 100))
            {
                attacker->go_berserk(true);
                return;
            }
        }
    }
    else
    {
        for (int i = MSLOT_WEAPON; i <= MSLOT_JEWELLERY; ++i)
        {
            const item_def *item =
                attacker->as_monster()->mslot_item(static_cast<mon_inv_type>(i));
            if (!item)
                continue;

            if (!is_artefact(*item))
                continue;

            if (x_chance_in_y(artefact_property(*item, ARTP_ANGRY), 100))
            {
                attacker->go_berserk(true);
                return;
            }
        }
    }
}

bool melee_attack::check_unrand_effects()
{
    if (unrand_entry && unrand_entry->melee_effects && weapon)
    {
        const bool died = !defender->alive();

        // Don't trigger the Wyrmbane death effect yet; that is done in
        // handle_phase_killed().
        if (weapon->unrand_idx == UNRAND_WYRMBANE && died)
            return true;

        // Recent merge added damage_done to this method call
        unrand_entry->melee_effects(weapon, attacker, defender,
                                    died, damage_done);
        return !defender->alive(); // may have changed
    }

    return false;
}

class AuxAttackType
{
public:
    AuxAttackType(int _damage, string _name, string _message) :
    damage(_damage), name(_name), message(_message) { };
public:
    virtual int get_damage() const { return damage; };
    virtual int get_brand() const { return SPWPN_NORMAL; };
    virtual string get_name() const { return name; };
    virtual string get_message() const { return message; };
protected:
    const int damage;
    const string name;
    const string message;
};

class AuxConstrict: public AuxAttackType
{
public:
    AuxConstrict()
    : AuxAttackType(0, "your grab", "You grab %s") { };
};

class AuxKick: public AuxAttackType
{
public:
    AuxKick()
    : AuxAttackType(5, "your kick", "You kick %s") { };

    int get_damage() const override
    {
        if (you.has_usable_hooves())
        {
            // Max hoof damage: 10.
            return damage + you.get_mutation_level(MUT_HOOVES) * 5 / 3;
        }

        if (you.has_usable_talons())
        {
            // Max talon damage: 9.
            return damage + 1 + you.get_mutation_level(MUT_TALONS);
        }

        // Max spike damage: 8.
        // ... yes, apparently tentacle spikes are "kicks".
        return damage + you.get_mutation_level(MUT_TENTACLE_SPIKE);
    }

    string get_message() const override
    {
        if (you.has_usable_talons())
            return "You claw %s.";
        if (you.get_mutation_level(MUT_TENTACLE_SPIKE))
            return "You pierce %s.";
        return message;
    }

    string get_name() const override
    {
        if (you.get_mutation_level(MUT_TENTACLE_SPIKE))
            return "your tentacle spike";
        return name;
    }
};

class AuxHeadbutt: public AuxAttackType
{
public:
    AuxHeadbutt()
    : AuxAttackType(5, "your headbutt", "You headbutt %s") { };

    int get_damage() const override
    {
        return damage + you.get_mutation_level(MUT_HORNS) * 3;
    }
};

class AuxPeck: public AuxAttackType
{
public:
    AuxPeck()
    : AuxAttackType(6, "your peck", "You peck %s") { };
};

class AuxTailslap: public AuxAttackType
{
public:
    AuxTailslap()
    : AuxAttackType(6, "your tail-slap", "You tail-slap %s") { };

    int get_damage() const override
    {
        return damage + max(0, you.get_mutation_level(MUT_STINGER) * 2 - 1)
                      + you.get_mutation_level(MUT_ARMOURED_TAIL) * 4;
    }

    int get_brand() const override
    {
        return you.get_mutation_level(MUT_STINGER) ? SPWPN_VENOM : SPWPN_NORMAL;
    }
};

class AuxPunch: public AuxAttackType
{
public:
    AuxPunch()
    : AuxAttackType(5, "your punch", "You punch %s") { };

    int get_damage() const override
    {
        const int base_dam = damage + you.skill_rdiv(SK_UNARMED_COMBAT, 1, 2);

        if (you.form == transformation::blade_hands)
            return base_dam + 6;

        if (you.has_usable_claws())
            return base_dam + roll_dice(you.has_claws(), 3);

        return base_dam;
    }

    string get_name() const override
    {
        if (you.form == transformation::blade_hands)
            return "your slash";

        if (you.has_usable_claws())
            return "your claw";

        if (you.has_usable_tentacles())
            return "your tentacle-slap";

        return name;
    }

    string get_message() const override
    {
        if (you.form == transformation::blade_hands)
            return "You slash %s";

        if (you.has_usable_claws())
            return "You claw %s";

        if (you.has_usable_tentacles())
            return "You tentacle-slap %s";

        return message;
    }

};

class AuxBite: public AuxAttackType
{
public:
    AuxBite()
    : AuxAttackType(0, "your bite", "You bite %s") { };

    int get_damage() const override
    {
        const int fang_damage = you.has_usable_fangs() * 2;
        if (you.get_mutation_level(MUT_ANTIMAGIC_BITE))
            return fang_damage + div_rand_round(you.get_hit_dice(), 3);

        const int str_damage = div_rand_round(max(you.strength()-10, 0), 5);

        if (you.get_mutation_level(MUT_ACIDIC_BITE))
            return fang_damage + str_damage;

        return fang_damage + str_damage;
    }

    int get_brand() const override
    {
        if (you.get_mutation_level(MUT_ANTIMAGIC_BITE))
            return SPWPN_ANTIMAGIC;

        if (you.get_mutation_level(MUT_ACIDIC_BITE))
            return SPWPN_ACID;

        return SPWPN_NORMAL;
    }
};

class AuxPseudopods: public AuxAttackType
{
public:
    AuxPseudopods()
    : AuxAttackType(4, "your bludgeon", "You bludgeon %s") { };

    int get_damage() const override
    {
        return damage * you.has_usable_pseudopods();
    }
};

class AuxTentacles: public AuxAttackType
{
public:
    AuxTentacles()
    : AuxAttackType(12, "your squeeze", "You squeeze %s") { };
};

static const AuxConstrict   AUX_CONSTRICT = AuxConstrict();
static const AuxKick        AUX_KICK = AuxKick();
static const AuxPeck        AUX_PECK = AuxPeck();
static const AuxHeadbutt    AUX_HEADBUTT = AuxHeadbutt();
static const AuxTailslap    AUX_TAILSLAP = AuxTailslap();
static const AuxPunch       AUX_PUNCH = AuxPunch();
static const AuxBite        AUX_BITE = AuxBite();
static const AuxPseudopods  AUX_PSEUDOPODS = AuxPseudopods();
static const AuxTentacles   AUX_TENTACLES = AuxTentacles();

static const AuxAttackType* const aux_attack_types[] =
{
    &AUX_CONSTRICT,
    &AUX_KICK,
    &AUX_HEADBUTT,
    &AUX_PECK,
    &AUX_TAILSLAP,
    &AUX_PUNCH,
    &AUX_BITE,
    &AUX_PSEUDOPODS,
    &AUX_TENTACLES,
};


/* Setup all unarmed (non attack_type) variables
 *
 * Clears any previous unarmed attack information and sets everything from
 * noise_factor to verb and damage. Called after player_aux_choose_uc_attack
 */
void melee_attack::player_aux_setup(unarmed_attack_type atk)
{
    const int num_aux_objs = ARRAYSZ(aux_attack_types);
    const int num_aux_atks = UNAT_LAST_ATTACK - UNAT_FIRST_ATTACK + 1;
    COMPILE_CHECK(num_aux_objs == num_aux_atks);

    ASSERT(atk >= UNAT_FIRST_ATTACK);
    ASSERT(atk <= UNAT_LAST_ATTACK);
    const AuxAttackType* const aux = aux_attack_types[atk - UNAT_FIRST_ATTACK];

    aux_damage = aux->get_damage();
    damage_brand = (brand_type)aux->get_brand();
    aux_attack = aux->get_name();
    aux_message = aux->get_message();

    if (wu_jian_attack != WU_JIAN_ATTACK_NONE)
        wu_jian_attack = WU_JIAN_ATTACK_TRIGGERED_AUX;

    if (atk == UNAT_BITE
        && _vamp_wants_blood_from_monster(defender->as_monster()))
    {
        damage_brand = SPWPN_VAMPIRISM;
    }
}

/**
 * Decide whether the player gets a bonus punch attack.
 *
 * Partially random.
 *
 * @return  Whether the player gets a bonus punch aux attack on this attack.
 */
bool melee_attack::player_gets_aux_punch()
{
    if (!get_form()->can_offhand_punch())
        return false;

    // roll for punch chance based on uc skill & armour penalty
    if (!attacker->fights_well_unarmed(attacker_armour_tohit_penalty
                                       + attacker_shield_tohit_penalty))
    {
        return false;
    }

    // No punching with a shield or 2-handed wpn.
    // Octopodes aren't affected by this, though!
    if (you.arm_count() <= 2 && !you.has_usable_offhand())
        return false;

    // Octopodes get more tentacle-slaps.
    return x_chance_in_y(you.arm_count() > 2 ? 3 : 2, 6);
}

bool melee_attack::player_aux_test_hit()
{
    // XXX We're clobbering did_hit
    did_hit = false;

    const int evasion = defender->evasion(ev_ignore::none, attacker);

    if (player_under_penance(GOD_ELYVILON)
        && god_hates_your_god(GOD_ELYVILON)
        && to_hit >= evasion
        && one_chance_in(20))
    {
        simple_god_message(" blocks your attack.", GOD_ELYVILON);
        return false;
    }

    bool auto_hit = one_chance_in(30);

    if (to_hit >= evasion || auto_hit)
        return true;

    mprf("%s misses %s.", aux_attack.c_str(),
         defender->name(DESC_THE).c_str());

    return false;
}

/* Controls the looping on available unarmed attacks
 *
 * As the master method for unarmed player combat, this loops through
 * available unarmed attacks, determining whether they hit and - if so -
 * calculating and applying their damage.
 *
 * Returns (defender dead)
 */
bool melee_attack::player_aux_unarmed()
{
    unwind_var<brand_type> save_brand(damage_brand);

    for (int i = UNAT_FIRST_ATTACK; i <= UNAT_LAST_ATTACK; ++i)
    {
        if (!defender->alive())
            break;

        unarmed_attack_type atk = static_cast<unarmed_attack_type>(i);

        if (!_extra_aux_attack(atk))
            continue;

        // Determine and set damage and attack words.
        player_aux_setup(atk);

        if (atk == UNAT_CONSTRICT && !attacker->can_constrict(defender, true))
            continue;

        to_hit = random2(calc_your_to_hit_unarmed());

        handle_noise(defender->pos());
        alert_nearby_monsters();

        // [ds] kraken can flee when near death, causing the tentacle
        // the player was beating up to "die" and no longer be
        // available to answer questions beyond this point.
        // handle_noise stirs up all nearby monsters with a stick, so
        // the player may be beating up a tentacle, but the main body
        // of the kraken still gets a chance to act and submerge
        // tentacles before we get here.
        if (!defender->alive())
            return true;

        if (player_aux_test_hit())
        {
            // Upset the monster.
            behaviour_event(defender->as_monster(), ME_WHACK, attacker);
            if (!defender->alive())
                return true;

            if (attack_shield_blocked(true))
                continue;
            if (player_aux_apply(atk))
                return true;
        }
    }

    return false;
}

bool melee_attack::player_aux_apply(unarmed_attack_type atk)
{
    did_hit = true;

    count_action(CACT_MELEE, -1, atk); // aux_attack subtype/auxtype

    aux_damage  = player_stat_modify_damage(aux_damage);

    aux_damage  = random2(aux_damage);

    aux_damage  = player_apply_fighting_skill(aux_damage, true);

    aux_damage  = player_apply_misc_modifiers(aux_damage);

    aux_damage  = player_apply_slaying_bonuses(aux_damage, true);

    aux_damage  = player_apply_final_multipliers(aux_damage);

    if (atk == UNAT_CONSTRICT)
        aux_damage = 0;
    else
        aux_damage = apply_defender_ac(aux_damage);

    aux_damage = inflict_damage(aux_damage, BEAM_MISSILE);
    damage_done = aux_damage;

    if (defender->alive())
    {
        if (atk == UNAT_CONSTRICT)
            attacker->start_constricting(*defender);

        if (damage_done > 0 || atk == UNAT_CONSTRICT)
        {
            player_announce_aux_hit();

            if (damage_brand == SPWPN_ACID)
                defender->splash_with_acid(&you, 3);

            if (damage_brand == SPWPN_VENOM && coinflip())
                poison_monster(defender->as_monster(), &you);

            // Normal vampiric biting attack, not if already got stabbing special.
            if (damage_brand == SPWPN_VAMPIRISM
                && you.has_mutation(MUT_VAMPIRISM)
                && (!stab_attempt || stab_bonus <= 0))
            {
                _player_vampire_draws_blood(defender->as_monster(), damage_done);
            }

            if (damage_brand == SPWPN_ANTIMAGIC && you.has_mutation(MUT_ANTIMAGIC_BITE)
                && damage_done > 0)
            {
                const bool spell_user = defender->antimagic_susceptible();

                antimagic_affects_defender(damage_done * 32);

                string mon_name = defender->as_monster()->name(DESC_THE);

                if (!have_passive(passive_t::no_mp_regen))
                {
                    if (spell_user)
                        mprf("You drain %s magic.", apostrophise(mon_name).c_str());
                    else
                        mprf("You drain %s power.", apostrophise(mon_name).c_str());
                }
                else if (spell_user)
                {
                    // You can't drain MP, but antimagic still applies
                    mprf("You disrupt %s magic.", apostrophise(mon_name).c_str());

                }

                if (!have_passive(passive_t::no_mp_regen)
                    && you.magic_points != you.max_magic_points
                    && !defender->as_monster()->is_summoned()
                    && !mons_is_firewood(*defender->as_monster()))
                {
                    int drain = random2(damage_done * 2) + 1;
                    // Augment mana drain--1.25 "standard" effectiveness at 0 mp,
                    // 0.25 at mana == max_mana
                    drain = (int)((1.25 - you.magic_points / you.max_magic_points)
                                  * drain);
                    if (drain)
                    {
                        mpr("You feel invigorated.");
                        inc_mp(drain);
                    }
                }
            }
        }
        else // no damage was done
        {
            string msg = localise(aux_message, defender->name(DESC_THE));
            if (you.can_see(*defender))
                msg += localise(" but do no damage.");
            else
                msg = add_punctuation(msg, ".", false);
            mpr_nolocalise(msg);
        }
    }
    else // defender was just alive, so this call should be ok?
        player_announce_aux_hit();

    if (defender->as_monster()->hit_points < 1)
    {
        handle_phase_killed();
        return true;
    }

    return false;
}

void melee_attack::player_announce_aux_hit()
{
    string msg = localise(aux_message, defender->name(DESC_THE));
    msg += debug_damage_number();
    attack_strength_message(msg, damage_done, false);
}

string melee_attack::player_why_missed()
{
    const int ev = defender->evasion(ev_ignore::none, attacker);
    const int combined_penalty =
        attacker_armour_tohit_penalty + attacker_shield_tohit_penalty;
    if (to_hit < ev && to_hit + combined_penalty >= ev)
    {
        const bool armour_miss =
            (attacker_armour_tohit_penalty
             && to_hit + attacker_armour_tohit_penalty >= ev);
        const bool shield_miss =
            (attacker_shield_tohit_penalty
             && to_hit + attacker_shield_tohit_penalty >= ev);

        if (armour_miss && !shield_miss)
            return "Your armour prevents you from hitting %s.";
        else if (shield_miss && !armour_miss)
            return "Your shield prevents you from hitting %s.";
        else
            return "Your shield and armour prevent you from hitting %s.";
    }

    return (ev_margin <= -20) ? "You completely miss %s." :
           (ev_margin <= -12) ? "You miss %s." :
           (ev_margin <= -6)  ? "You closely miss %s."
                              : "You barely miss %s.";
}

void melee_attack::player_warn_miss()
{
    did_hit = false;

    mprf(player_why_missed().c_str(),
         defender->name(DESC_THE).c_str());
}

// A couple additive modifiers that should be applied to both unarmed and
// armed attacks.
int melee_attack::player_apply_misc_modifiers(int damage)
{
    if (you.duration[DUR_MIGHT] || you.duration[DUR_BERSERK])
        damage += 1 + random2(10);

    return damage;
}

// Multipliers to be applied to the final (pre-stab, pre-AC) damage.
// It might be tempting to try to pick and choose what pieces of the damage
// get affected by such multipliers, but putting them at the end is the
// simplest effect to understand if they aren't just going to be applied
// to the base damage of the weapon.
int melee_attack::player_apply_final_multipliers(int damage)
{
    // cleave damage modifier
    if (cleaving)
        damage = cleave_damage_mod(damage);

    // martial damage modifier (wu jian)
    damage = martial_damage_mod(damage);

    // Palentonga rolling charge bonus
    if (roll_dist > 0)
    {
        // + 1/3rd base per distance rolled, up to double at dist 3.
        const int extra_dam = damage * roll_dist / 3;
        damage += extra_dam > damage ? damage : extra_dam;
    }

    // not additive, statues are supposed to be bad with tiny toothpicks but
    // deal crushing blows with big weapons
    if (you.form == transformation::statue)
        damage = div_rand_round(damage * 3, 2);

    // Can't affect much of anything as a shadow.
    if (you.form == transformation::shadow)
        damage = div_rand_round(damage, 2);

    if (you.duration[DUR_WEAK])
        damage = div_rand_round(damage * 3, 4);

    if (you.duration[DUR_CONFUSING_TOUCH])
        return 0;

    return damage;
}

string melee_attack::player_attack_message(int damage)
{
    int weap_type = WPN_UNKNOWN;

    if (Options.has_fake_lang(flang_t::grunt))
        damage = HIT_STRONG + 1;

    if (!weapon)
        weap_type = WPN_UNARMED;
    else if (weapon->base_type == OBJ_STAVES)
        weap_type = WPN_STAFF;
    else if (weapon->base_type == OBJ_WEAPONS
             && !is_range_weapon(*weapon))
    {
        weap_type = weapon->sub_type;
    }

    // All weak hits with weapons look the same.
    if (damage < HIT_WEAK
        && weap_type != WPN_UNARMED)
    {
        if (weap_type != WPN_UNKNOWN)
            return "You hit %s";
        else
            return "You clumsily bash %s";
    }

    // Take normal hits into account. If the hit is from a weapon with
    // more than one damage type, randomly choose one damage type from
    // it.
    monster_type defender_genus = mons_genus(defender->type);
    switch (weapon ? single_damage_type(*weapon) : -1)
    {
    case DAM_PIERCE:
        if (damage < HIT_MED)
            return "You puncture %s";
        else if (damage < HIT_STRONG)
            return "You impale %s";
        else
        {
            if (defender->is_monster()
                && defender_visible
                && defender_genus == MONS_HOG)
            {
                return "You spit %s like the proverbial pig";
            }
            else if (defender_genus == MONS_CRAB
                     && Options.has_fake_lang(flang_t::grunt))
            {
                return "You attack %s's weak point";
            }
            else
            {
                static const char * const pierce_desc[] =
                {
                    "You spit %s like a pig",
                    "You skewer %s like a kebab",
                    "You stick %s like a pincushion",
                    "You perforate %s like a sieve"
                };
                const int choice = random2(ARRAYSZ(pierce_desc));
                return pierce_desc[choice];
            }
        }

    case DAM_SLICE:
        if (damage < HIT_MED)
            return "You slash %s";
        else if (damage < HIT_STRONG)
            return "You slice %s";
        else if (defender_genus == MONS_OGRE)
            return "You dice %s like an onion";
        else if (defender_genus == MONS_SKELETON)
            return "You fracture %s into splinters";
        else if (defender_genus == MONS_HOG)
            return "You carve %s like the proverbial ham";
        else if ((defender_genus == MONS_TENGU
                  || get_mon_shape(defender_genus) == MON_SHAPE_BIRD)
                 && one_chance_in(3))
        {
            return "You carve %s like a turkey";
        }
        else if ((defender_genus == MONS_YAK || defender_genus == MONS_YAKTAUR)
                 && Options.has_fake_lang(flang_t::grunt))
        {
            return "You shave %s";
        }
        else
        {
            static const char * const slice_desc[] =
            {
                "You open %s like a pillowcase",
                "You slice %s like a ripe choko",
                "You cut %s into ribbons",
                "You carve %s like a ham",
                "You chop %s into pieces"
            };
            const int choice = random2(ARRAYSZ(slice_desc));
            return slice_desc[choice];
        }

    case DAM_BLUDGEON:
        if (damage < HIT_MED)
            return one_chance_in(4) ? "You thump %s" : "You sock %s";
        else if (damage < HIT_STRONG)
            return "You bludgeon %s";
        else if (defender_genus == MONS_SKELETON)
            return "You shatter %s into splinters";
        else if (defender->type == MONS_GREAT_ORB_OF_EYES)
            return "You splatter %s into a gooey mess";
        else
        {
            static const char * const bludgeon_desc[] =
            {
                "You crush %s like a grape",
                "You beat %s like a drum",
                "You hammer %s like a gong",
                "You pound %s like an anvil",
                "You flatten %s like a pancake"
            };
            const int choice = random2(ARRAYSZ(bludgeon_desc));
            return bludgeon_desc[choice];
        }

    case DAM_WHIP:
        if (damage < HIT_MED)
            return "You whack %s";
        else if (damage < HIT_STRONG)
            return "You thrash %s";
        else
        {
            if (defender->holiness() & (MH_HOLY | MH_NATURAL | MH_DEMONIC))
                return "You punish %s, causing immense pain";
            else
                return "You devastate %s";
        }
    case -1: // unarmed
    {
        const FormAttackVerbs verbs = get_form(you.form)->uc_attack_verbs;
        if (verbs.weak != FAV_DEFAULT)
        {
            form_attack_verb verb;
            if (damage < HIT_WEAK)
                verb = verbs.weak;
            else if (damage < HIT_MED)
                verb = verbs.medium;
            else if (damage < HIT_STRONG)
                verb = verbs.strong;
            else
                verb = verbs.devastating;

            switch (verb)
            {
                case FAV_SLASH: return "You slash %s";
                case FAV_SLICE: return "You slice %s";
                case FAV_SHRED: return "You shred %s";
                case FAV_CLAW: return "You claw %s";
                case FAV_BITE: return "You bite %s";
                case FAV_MAUL: return "You maul %s";
                case FAV_SMACK: return "You smack %s";
                case FAV_PUMMEL: return "You pummel %s";
                case FAV_THRASH: return "You thrash %s";
                case FAV_TOUCH: return "You touch %s";
                case FAV_ENGULF: return "You engulf %s";
                case FAV_RELEASE_SPORES_AT: return "You release spores at %s";
                case FAV_NIP_AT: return "You nip at %s";
                case FAV_GOUGE: return "You gouge %s";
                case FAV_CHOMP: return "You chomp %s";
                case FAV_BUFFET: return "You buffet %s";
                case FAV_BATTER: return "You batter %s";
                case FAV_BLAST: return "You blast %s";
                default: return "You hit %s";
            }
        }

        if (you.damage_type() == DVORP_CLAWING)
        {
            if (damage < HIT_WEAK)
                return "You scratch %s";
            else if (damage < HIT_MED)
                return "You claw %s";
            else if (damage < HIT_STRONG)
                return "You mangle %s";
            else
                return "You eviscerate %s";
        }
        else if (you.damage_type() == DVORP_TENTACLE)
        {
            if (damage < HIT_WEAK)
                return "You tentacle-slap %s";
            else if (damage < HIT_MED)
                return "You bludgeon %s";
            else if (damage < HIT_STRONG)
                return "You batter %s";
            else
                return "You thrash %s";
        }
        else
        {
            if (damage < HIT_WEAK)
                return "You hit %s";
            else if (damage < HIT_MED)
                return "You punch %s";
            else if (damage < HIT_STRONG)
                return "You pummel %s";
            else if (defender->is_monster()
                     && mons_genus(defender->type) == MONS_FORMICID)
            {
                return "You squash %s like the proverbial ant";
            }
            else
            {
                static const char * const punch_desc[] =
                {
                    "You pound %s into fine dust",
                    "You pummel %s like a punching bag",
                    "You pulverise %s",
                    "You squash %s like an ant"
                };
                const int choice = random2(ARRAYSZ(punch_desc));
                // XXX: could this distinction work better?
                if (choice == 0
                    && defender->is_monster()
                    && mons_has_blood(defender->type))
                {
                    return "You beat %s into a bloody pulp";
                }
                else
                    return punch_desc[choice];
            }
        }
    }

    case WPN_UNKNOWN:
    default:
        break;
    }

    return "You hit %s";
}

void melee_attack::player_exercise_combat_skills()
{
    if (defender && defender->is_monster()
        && !mons_is_firewood(*defender->as_monster()))
    {
        practise_hitting(weapon);
    }
}

/*
 * Applies god conduct for weapon ego
 *
 * Using speed brand as a chei worshipper, or holy/unholy/wizardly weapons etc
 */
void melee_attack::player_weapon_upsets_god()
{
    if (weapon
        && (weapon->base_type == OBJ_WEAPONS || weapon->base_type == OBJ_STAVES)
        && god_hates_item_handling(*weapon))
    {
        did_god_conduct(god_hates_item_handling(*weapon), 2);
    }
}

/* Apply player-specific effects as well as brand damage.
 *
 * Called after damage is calculated, but before unrand effects and before
 * damage is dealt.
 *
 * Returns true if combat should continue, false if it should end here.
 */
bool melee_attack::player_monattk_hit_effects()
{
    player_weapon_upsets_god();

    // Don't even check vampire bloodletting if the monster has already
    // been reset (for example, a spectral weapon who noticed in
    // player_stab_check that it shouldn't exist anymore).
    if (defender->type == MONS_NO_MONSTER)
        return false;

    // Thirsty vampires will try to use a stabbing situation to draw blood.
    if (you.has_mutation(MUT_VAMPIRISM)
        && damage_done > 0
        && stab_attempt
        && stab_bonus > 0)
    {
        _player_vampire_draws_blood(defender->as_monster(), damage_done, true);
    }

    if (!defender->alive())
        return false;

    // These effects apply only to monsters that are still alive:

    // Returns true if the hydra was killed by the decapitation, in which case
    // nothing more should be done to the hydra.
    if (consider_decapitation(damage_done))
        return false;

    return true;
}

void melee_attack::handle_noise(const coord_def & pos)
{
    // Successful stabs make no noise.
    if (stab_attempt)
        return;

    int loudness = damage_done / 4;

    // All non-stab melee attacks make some noise.
    loudness = max(1, loudness);

    // Cap melee noise at shouting volume.
    loudness = min(12, loudness);

    noisy(loudness, pos, attacker->mid);
}

/**
 * If appropriate, chop a head off the defender. (Usually a hydra.)
 *
 * @param dam           The damage done in the attack that may or may not chop
  *                     off a head.
 * @param damage_type   The type of damage done in the attack.
 * @return              Whether the defender was killed by the decapitation.
 */
bool melee_attack::consider_decapitation(int dam, int damage_type)
{
    const int dam_type = (damage_type != -1) ? damage_type :
                                               attacker->damage_type();
    if (!attack_chops_heads(dam, dam_type))
        return false;

    decapitate(dam_type);

    if (!defender->alive())
        return true;

    // Only living hydras get to regenerate heads.
    if (!(defender->holiness() & MH_NATURAL))
        return false;

    // What's the largest number of heads the defender can have?
    const int limit = defender->type == MONS_LERNAEAN_HYDRA ? 27
                                                            : MAX_HYDRA_HEADS;

    if (attacker->damage_brand() == SPWPN_FLAMING)
    {
        if (defender_visible)
            mpr("The flame cauterises the wound!");
        return false;
    }

    int heads = defender->heads();
    if (heads >= limit - 1)
        return false; // don't overshoot the head limit!

    simple_monster_message(*defender->as_monster(), " grows two more!");
    defender->as_monster()->num_heads += 2;
    defender->heal(8 + random2(8));

    return false;
}

/**
 * Can the given actor lose its heads? (Is it hydra or hydra-like?)
 *
 * @param defender  The actor in question.
 * @return          Whether the given actor is susceptible to head-choppage.
 */
static bool actor_can_lose_heads(const actor* defender)
{
    if (defender->is_monster()
        && defender->as_monster()->has_hydra_multi_attack()
        && defender->type != MONS_SPECTRAL_THING
        && defender->as_monster()->mons_species() != MONS_SERPENT_OF_HELL)
    {
        return true;
    }

    return false;
}

/**
 * Does this attack chop off one of the defender's heads? (Generally only
 * relevant for hydra defenders)
 *
 * @param dam           The damage done in the attack in question.
 * @param dam_type      The vorpal_damage_type of the attack.
 * @param wpn_brand     The brand_type of the attack.
 * @return              Whether the attack will chop off a head.
 */
bool melee_attack::attack_chops_heads(int dam, int dam_type)
{
    // hydras and hydra-like things only.
    if (!actor_can_lose_heads(defender))
        return false;

    // no decapitate on riposte (Problematic)
    if (is_riposte)
        return false;

    // Monster attackers+defenders have only a 25% chance of making the
    // chop-check to prevent runaway head inflation.
    // XXX: Tentatively making an exception for spectral weapons
    const bool player_spec_weap = attacker->is_monster()
                                    && attacker->type == MONS_SPECTRAL_WEAPON
                                    && attacker->as_monster()->summoner
                                        == MID_PLAYER;
    if (attacker->is_monster() && defender->is_monster()
        && !player_spec_weap && !one_chance_in(4))
    {
        return false;
    }

    // Only cutting implements.
    if (dam_type != DVORP_SLICING && dam_type != DVORP_CHOPPING
        && dam_type != DVORP_CLAWING)
    {
        return false;
    }

    // Small claws are not big enough.
    if (dam_type == DVORP_CLAWING && attacker->has_claws() < 3)
        return false;

    // You need to have done at least some damage.
    if (dam <= 0 || dam < 4 && coinflip())
        return false;

    // ok, good enough!
    return true;
}

/**
 * Decapitate the (hydra or hydra-like) defender!
 *
 * @param dam_type      The vorpal_damage_type of the attack.
 */
void melee_attack::decapitate(int dam_type)
{
    // Player hydras don't gain or lose heads.
    ASSERT(defender->is_monster());

    string the_hydras = apostrophise(defender_name(false));

    int heads = defender->heads();
    if (heads == 1) // will be zero afterwards
    {
        if (defender_visible)
        {
            if (attacker->is_player())
            {
                mprf(dam_type == DVORP_CLAWING
                         ? "You rip %s last head off!"
                         : "You chop %s last head off!",
                     the_hydras.c_str());
            }
            else
            {
                mprf(dam_type == DVORP_CLAWING
                         ? "%s rips %s last head off!"
                         : "%s chops %s last head off!",
                     atk_name(DESC_THE).c_str(),
                     the_hydras.c_str());
            }
        }

        if (!defender->is_summoned())
        {
            bleed_onto_floor(defender->pos(), defender->type,
                             defender->as_monster()->hit_points, true);
        }

        if (!simu)
            defender->hurt(attacker, INSTANT_DEATH);

        return;
    }

    if (defender_visible)
    {
        if (attacker->is_player())
        {
            mprf(dam_type == DVORP_CLAWING
                     ? "You rip one of %s heads off!"
                     : "You chop one of %s heads off!",
                 the_hydras.c_str());
        }
        else
        {
            mprf(dam_type == DVORP_CLAWING
                     ? "%s rips one of %s heads off!"
                     : "%s chops one of %s heads off!",
                 atk_name(DESC_THE).c_str(),
                 the_hydras.c_str());
        }
    }

    defender->as_monster()->num_heads--;
}

/**
 * Apply passive retaliation damage from hitting acid monsters.
 */
void melee_attack::attacker_sustain_passive_damage()
{
    // If the defender has been cleaned up, it's too late for anything.
    if (!defender->alive())
        return;

    if (!mons_class_flag(defender->type, M_ACID_SPLASH))
        return;

    if (attacker->res_acid() >= 3)
        return;

    if (!adjacent(attacker->pos(), defender->pos()) || is_riposte)
        return;

    const int acid_strength = resist_adjust_damage(attacker, BEAM_ACID, 5);

    // Spectral weapons can't be corroded (but can take acid damage).
    const bool avatar = attacker->is_monster()
                        && mons_is_avatar(attacker->as_monster()->type);

    if (!avatar)
    {
        if (x_chance_in_y(acid_strength + 1, 30))
            attacker->corrode_equipment();
    }

    if (attacker->is_player())
        mpr_nolocalise(you.hand_act("%s burns!", "%s burn!"));
    else
    {
        simple_monster_message(*attacker->as_monster(),
                               " is burned by acid!");
    }
    attacker->hurt(defender, roll_dice(1, acid_strength), BEAM_ACID,
                   KILLED_BY_ACID);
}

int melee_attack::staff_damage(skill_type skill)
{
    if (x_chance_in_y(attacker->skill(SK_EVOCATIONS, 200)
                    + attacker->skill(skill, 100), 3000))
    {
        return random2((attacker->skill(skill, 100)
                      + attacker->skill(SK_EVOCATIONS, 50)) / 80);
    }
    return 0;
}

bool melee_attack::apply_staff_damage()
{
    if (!weapon)
        return false;

    if (attacker->is_player() && you.get_mutation_level(MUT_NO_ARTIFICE))
        return false;

    if (weapon->base_type != OBJ_STAVES)
        return false;

    skill_type sk = staff_skill(static_cast<stave_type>(weapon->sub_type));

    switch (weapon->sub_type)
    {
    case STAFF_AIR:
        special_damage =
            resist_adjust_damage(defender, BEAM_ELECTRICITY, staff_damage(sk));

        if (special_damage)
        {

            string msg;
            if (defender->is_player())
                msg = localise("You are electrocuted");
            else
                msg = localise("%s is electrocuted", defender->name(DESC_THE));

            special_damage_message =
                add_attack_strength_punct(msg, special_damage, false);
            special_damage_flavour = BEAM_ELECTRICITY;
        }

        break;

    case STAFF_COLD:
        special_damage =
            resist_adjust_damage(defender, BEAM_COLD, staff_damage(sk));

        if (special_damage)
        {
            special_damage_message =
                make_any_2_actors_message(attacker, defender,
                                          "freeze", "",
                                          attack_strength_punctuation(special_damage));
            special_damage_flavour = BEAM_COLD;
        }
        break;

    case STAFF_EARTH:
        special_damage = staff_damage(sk) * 4 / 3;
        special_damage = apply_defender_ac(special_damage, 0, ac_type::triple);

        if (special_damage > 0)
        {
            special_damage_message =
                make_any_2_actors_message(attacker, defender,
                                          "shatter", "",
                                          attack_strength_punctuation(special_damage));
        }
        break;

    case STAFF_FIRE:
        special_damage =
            resist_adjust_damage(defender, BEAM_FIRE, staff_damage(sk));

        if (special_damage)
        {
            special_damage_message =
                make_any_2_actors_message(attacker, defender,
                                          "burn", "",
                                          attack_strength_punctuation(special_damage));

            special_damage_flavour = BEAM_FIRE;

            if (defender->is_player())
                maybe_melt_player_enchantments(BEAM_FIRE, special_damage);
        }
        break;

    case STAFF_POISON:
        special_damage =
            resist_adjust_damage(defender, BEAM_POISON, staff_damage(sk));

        if (special_damage)
        {
            special_damage_message =
                make_any_2_actors_message(attacker, defender,
                                          "envenom", "",
                                          attack_strength_punctuation(special_damage));

            special_damage_flavour = BEAM_POISON;
        }
        break;

    case STAFF_DEATH:
        special_damage =
            resist_adjust_damage(defender, BEAM_NEG, staff_damage(sk));

        if (special_damage)
        {
            string msg;
            if (defender->is_player())
                msg = localise("You writhe in agony");
            else
                msg = localise("%s writhes in agony", defender->name(DESC_THE));

            special_damage_message =
                add_attack_strength_punct(msg, special_damage, false);

            attacker->god_conduct(DID_EVIL, 4);
        }
        break;

    case STAFF_CONJURATION:
        special_damage = staff_damage(sk);
        special_damage = apply_defender_ac(special_damage);

        if (special_damage > 0)
        {
            special_damage_message =
                make_any_2_actors_message(attacker, defender,
                                          "blast", "",
                                          attack_strength_punctuation(special_damage));
        }
        break;

#if TAG_MAJOR_VERSION == 34
    case STAFF_SUMMONING:
    case STAFF_POWER:
    case STAFF_ENCHANTMENT:
    case STAFF_ENERGY:
    case STAFF_WIZARDRY:
#endif
        break;

    default:
        die("Invalid staff type: %d", weapon->sub_type);
    }

    if (special_damage || special_damage_flavour)
    {
        dprf(DIAG_COMBAT, "Staff damage to %s: %d, flavour: %d",
             defender->name(DESC_THE).c_str(),
             special_damage, special_damage_flavour);

        if (needs_message && !special_damage_message.empty())
            mpr_nolocalise(special_damage_message);

        inflict_damage(special_damage, special_damage_flavour);
        if (special_damage > 0)
        {
            defender->expose_to_element(special_damage_flavour, 2);
            // XXX: this is messy, but poisoning from the staff of poison
            // should happen after damage.
            if (defender->alive() && special_damage_flavour == BEAM_POISON)
                defender->poison(attacker, 2);
        }
    }

    return true;
}

int melee_attack::calc_to_hit(bool random)
{
    int mhit = attack::calc_to_hit(random);
    if (mhit == AUTOMATIC_HIT)
        return AUTOMATIC_HIT;

    return mhit;
}

int melee_attack::post_roll_to_hit_modifiers(int mhit, bool random)
{
    int modifiers = attack::post_roll_to_hit_modifiers(mhit, random);

    // Just trying to touch is easier than trying to damage.
    if (you.duration[DUR_CONFUSING_TOUCH])
        modifiers += maybe_random_div(you.dex(), 2, random);

    // Rolling charges feel bad when they miss, so make them miss less often.
    if (roll_dist > 0)
        modifiers += 5; // matching UC form to-hit bonuses

    if (attacker->is_player() && !weapon && get_form()->unarmed_hit_bonus)
    {
        // TODO: Review this later (transformations getting extra hit
        // almost across the board seems bad) - Cryp71c
        modifiers += UC_FORM_TO_HIT_BONUS;
    }

    return modifiers;
}

void melee_attack::player_stab_check()
{
    if (!is_projected)
        attack::player_stab_check();
}

/**
 * Can we get a good stab with this weapon?
 */
bool melee_attack::player_good_stab()
{
    return wpn_skill == SK_SHORT_BLADES
           || you.get_mutation_level(MUT_PAWS)
           || player_equip_unrand(UNRAND_HOOD_ASSASSIN)
              && (!weapon || is_melee_weapon(*weapon));
}

bool melee_attack::is_reach_attack()
{
    int dist = (attack_position - defender->pos()).rdist();
    return dist > 1;
}

/* Select the attack message for attacker
 *
 * If klown, select randomly from klown_attack, otherwise check for any special
 * case attack verbs (tentacles or door/fountain-mimics) and if all else fails,
 * select an attack verb from attack_types based on the ENUM value of attk_type.
 *
 * Returns (attack message)
 */
string melee_attack::mons_attack_message()
{
    static const char *klown_attack[][2] =
    {
        {"%s hits you", "%s hits %s"},
        {"%s pokes you", "%s pokes %s"},
        {"%s prods you", "%s prods %s"},
        {"%s flogs you", "%s flogs %s"},
        {"%s pounds you", "%s pounds %s"},
        {"%s slaps you", "%s slaps %s"},
        {"%s tickles you", "%s tickles %s"},
        {"%s defenestrates you", "%s defenestrates %s"},
        {"%s sucker-punches you", "%s sucker-punches %s"},
        {"%s elbows you", "%s elbows %s"},
        {"%s pinches you", "%s pinches %s"},
        {"%s strangle-hugs you", "%s strangle-hugs %s"},
        {"%s squeezes you", "%s squeezes %s"},
        {"%s teases you", "%s teases %s"},
        {"%s eye-gouges you", "%s eye-gouges %s"},
        {"%s karate-kicks you", "%s karate-kicks %s"},
        {"%s headlocks you", "%s headlocks %s"},
        {"%s wrestles you", "%s wrestles %s"},
        {"%s trip-wires you", "%s trip-wires %s"},
        {"%s kneecaps you", "%s kneecaps %s"},
        {"%s flogs you", "%s flogs %s"},
    };

    bool on_you = defender->is_player();

    if (attacker->type == MONS_KILLER_KLOWN && attk_type == AT_HIT)
        return RANDOM_ELEMENT(klown_attack)[on_you ? 0 : 1];

    //XXX: then why give them it in the first place?
    if (attk_type == AT_TENTACLE_SLAP && mons_is_tentacle(attacker->type))
        return on_you ? "%s slaps you" : "%s slaps %s";

    switch (attk_type)
    {
        case AT_BITE: return on_you ? "%s bites you" : "%s bites %s";
#if TAG_MAJOR_VERSION == 34
        case AT_REACH_STING: // deliberate fall-through
#endif
        case AT_STING: return on_you ? "%s stings you" : "%s stings %s";
        case AT_SPORE: return on_you ? "%s releases spores at you"
                                     : "%s releases spores at %s";
        case AT_TOUCH: return on_you ? "%s touches you" : "%s touches %s";
        case AT_ENGULF: return on_you ? "%s engulfs you" : "%s engulfs %s";
        case AT_CLAW: return on_you ? "%s claws you" : "%s claws %s";
        case AT_PECK: return on_you ? "%s pecks you" : "%s pecks %s";
        case AT_HEADBUTT: return on_you ? "%s headbutts you" : "%s headbutts %s";
        case AT_PUNCH: return on_you ? "%s punches you" : "%s punches %s";
        case AT_KICK: return on_you ? "%s kicks you" : "%s kicks %s";
        case AT_TENTACLE_SLAP: return on_you ? "%s tentacle-slaps you"
                                             : "%s tentacle-slaps %s";
        case AT_TAIL_SLAP: return on_you ? "%s tail-slaps you" : "%s tail-slaps %s";
        case AT_GORE: return on_you ? "%s gores you" : "%s gores %s";
        case AT_CONSTRICT: return on_you ? "%s constricts you" : "%s constricts %s";
        case AT_TRAMPLE: return on_you ? "%s tramples you" : "%s tramples %s";
        case AT_TRUNK_SLAP: return on_you ? "%s trunk-slaps you"
                                          : "%s trunk-slaps %s";
#if TAG_MAJOR_VERSION == 34
        case AT_SNAP: return on_you ? "%s snaps at you" : "%s snaps at %s";
        case AT_SPLASH: return on_you ? "%s splashes you" : "%s splashes %s";
#endif
        case AT_POUNCE: return on_you ? "%s pounces on you" : "%s pounces on %s";
        default: return on_you ? "%s hits you" : "%s hits %s";
    }
}

string melee_attack::mons_attack_desc()
{
    bool seen = you.can_see(*attacker);
    bool on_you = defender->is_player();

    string ret;
    if (seen && is_reach_attack())
    {
        ASSERT(can_reach());
#if TAG_MAJOR_VERSION == 34
        if (attk_type == AT_REACH_STING)
        {
            ret = on_you ? "%s stings you from afar"
                         : "%s stings %s from afar";
        }
        else
#endif
        {
            ret = on_you ? "%s hits you from afar"
                         : "%s hits %s from afar";
        }
    }
    else
        ret = mons_attack_message();

    ret = localise(ret, atk_name(DESC_THE), def_name(DESC_THE));

    if (weapon && !mons_class_is_animated_weapon(attacker->type))
        ret += localise(" with %s", weapon->name(DESC_A));

    return ret;
}

void melee_attack::announce_hit()
{
    if (!needs_message || attk_flavour == AF_CRUSH)
        return;

    if (attacker->is_monster())
        mpr_nolocalise(mons_attack_desc());
    else
    {
        string msg = localise(player_attack_message(damage_done),
                              defender_name(false));
        msg += debug_damage_number(); // empty in non-debug build
        attack_strength_message(msg, damage_done, false);
    }
}

// Returns if the target was actually poisoned by this attack
bool melee_attack::mons_do_poison()
{
    int amount = 1;

    if (attk_flavour == AF_POISON_STRONG)
    {
        amount = random_range(attacker->get_hit_dice() * 11 / 3,
                              attacker->get_hit_dice() * 13 / 2);
    }
    else
    {
        amount = random_range(attacker->get_hit_dice() * 2,
                              attacker->get_hit_dice() * 4);
    }

    if (attacker->as_monster()->has_ench(ENCH_CONCENTRATE_VENOM))
    {
        return curare_actor(attacker, defender, 2, "concentrated venom",
                            attacker->name(DESC_PLAIN));
    }

    if (!defender->poison(attacker, amount))
        return false;

    if (needs_message)
    {
        if (defender->is_player())
            mprf("%s poisons you!", atk_name(DESC_THE).c_str());
        else
        {
            mprf("%s poisons %s!", atk_name(DESC_THE).c_str(),
                 defender_name(true).c_str());
        }
    }

    return true;
}

void melee_attack::mons_do_napalm()
{
    if (defender->res_sticky_flame())
        return;

    if (one_chance_in(3))
    {
        if (needs_message)
        {
            string msg;
            if (defender->is_player())
                msg = localise("You are covered in liquid flames");
            else
            {
                msg = localise("%s is covered in liquid flames",
                               defender_name(false));
            }
            attack_strength_message(msg, special_damage, false);
        }

        if (defender->is_player())
            napalm_player(random2avg(7, 3) + 1, atk_name(DESC_A));
        else
        {
            napalm_monster(
                defender->as_monster(),
                attacker,
                min(4, 1 + random2(attacker->get_hit_dice())/2));
        }
    }
}

static void _print_resist_messages(actor* defender, int base_damage,
                                   beam_type flavour)
{
    // check_your_resists is used for the player case to get additional
    // effects such as Xom amusement, melting of icy effects, etc.
    // mons_adjust_flavoured is used for the monster case to get all of the
    // special message handling ("The ice beast melts!") correct.
    // XXX: there must be a nicer way to do this, especially because we're
    // basically calculating the damage twice in the case where messages
    // are needed.
    if (defender->is_player())
        (void)check_your_resists(base_damage, flavour, "");
    else
    {
        bolt beam;
        beam.flavour = flavour;
        (void)mons_adjust_flavoured(defender->as_monster(),
                                    beam,
                                    base_damage,
                                    true);
    }
}

bool melee_attack::mons_attack_effects()
{
    // may have died earlier, due to e.g. pain bond
    // we could continue with the rest of their attack, but it's a minefield
    // of potential crashes. so, let's not.
    if (attacker->is_monster() && invalid_monster(attacker->as_monster()))
        return false;

    // Monsters attacking themselves don't get attack flavour.
    // The message sequences look too weird. Also, stealing
    // attacks aren't handled until after the damage msg. Also,
    // no attack flavours for dead defenders
    if (attacker != defender && defender->alive())
    {
        mons_apply_attack_flavour();

        if (needs_message && !special_damage_message.empty())
            mpr(special_damage_message);

        if (special_damage > 0)
        {
            inflict_damage(special_damage, special_damage_flavour);
            special_damage = 0;
            special_damage_message.clear();
            special_damage_flavour = BEAM_NONE;
        }
    }

    if (defender->is_player())
        practise_being_hit();

    // A tentacle may have banished its own parent/sibling and thus itself.
    if (!attacker->alive())
        return false;

    // consider_decapitation() returns true if the defender was killed
    // by the decapitation, in which case we should stop the rest of the
    // attack, too.
    if (consider_decapitation(damage_done,
                              attacker->damage_type(attack_number)))
    {
        return false;
    }

    if (attacker != defender && attk_flavour == AF_TRAMPLE)
        do_knockback();

    special_damage = 0;
    special_damage_message.clear();
    special_damage_flavour = BEAM_NONE;

    // Defender banished. Bail since the defender is still alive in the
    // Abyss.
    if (defender->is_banished())
        return false;

    if (!defender->alive())
        return attacker->alive();

    // Bail if the monster is attacking itself without a weapon, since
    // intrinsic monster attack flavours aren't applied for self-attacks.
    if (attacker == defender && !weapon)
        return false;

    return true;
}

void melee_attack::mons_apply_attack_flavour()
{
    // Most of this is from BWR 4.1.2.

    attack_flavour flavour = attk_flavour;
    if (flavour == AF_CHAOTIC)
        flavour = random_chaos_attack_flavour();

    const int base_damage = flavour_damage(flavour, attacker->get_hit_dice());

    // Note that if damage_done == 0 then this code won't be reached
    // unless the flavour is in flavour_triggers_damageless.
    switch (flavour)
    {
    default:
        // Just to trigger special melee damage effects for regular attacks
        // (e.g. Qazlal's elemental adaptation).
        defender->expose_to_element(BEAM_MISSILE, 2);
        break;

    case AF_MUTATE:
        if (one_chance_in(4))
        {
            defender->malmutate(you.can_see(*attacker) ?
                apostrophise(attacker->name(DESC_PLAIN)) + " mutagenic touch" :
                "mutagenic touch");
        }
        break;

    case AF_POISON:
    case AF_POISON_STRONG:
    case AF_REACH_STING:
        if (one_chance_in(3))
            mons_do_poison();
        break;

    case AF_FIRE:
        special_damage =
            resist_adjust_damage(defender,
                                 BEAM_FIRE,
                                 base_damage);
        special_damage_flavour = BEAM_FIRE;

        if (needs_message && base_damage)
        {
            string msg;
            if (defender->is_player())
                msg = localise("You are engulfed in flames");
            else
            {
                msg = localise("%s is engulfed in flames",
                               defender_name(false));
            }

            attack_strength_message(msg, special_damage, false);

            _print_resist_messages(defender, base_damage, BEAM_FIRE);
        }

        defender->expose_to_element(BEAM_FIRE, 2);
        break;

    case AF_COLD:
        special_damage =
            resist_adjust_damage(defender,
                                 BEAM_COLD,
                                 base_damage);
        special_damage_flavour = BEAM_COLD;

        if (needs_message && base_damage)
        {
            do_3rd_person_message(attacker, defender, "%s freezes %s",
                                  attack_strength_punctuation(special_damage));

            _print_resist_messages(defender, base_damage, BEAM_COLD);
        }

        defender->expose_to_element(BEAM_COLD, 2);
        break;

    case AF_ELEC:
        special_damage =
            resist_adjust_damage(defender,
                                 BEAM_ELECTRICITY,
                                 base_damage);
        special_damage_flavour = BEAM_ELECTRICITY;

        if (needs_message && base_damage)
        {
            do_3rd_person_message(attacker, defender, "%s shocks %s",
                                  attack_strength_punctuation(special_damage));

            _print_resist_messages(defender, base_damage, BEAM_ELECTRICITY);
        }

        dprf(DIAG_COMBAT, "Shock damage: %d", special_damage);
        defender->expose_to_element(BEAM_ELECTRICITY, 2);
        break;

        // Combines drain speed and vampiric.
    case AF_SCARAB:
        if (x_chance_in_y(3, 5))
            drain_defender_speed();

        // deliberate fall-through
    case AF_VAMPIRIC:
        if (!actor_is_susceptible_to_vampirism(*defender))
            break;

        if (defender->stat_hp() < defender->stat_maxhp())
        {
            int healed = resist_adjust_damage(defender, BEAM_NEG,
                                              1 + random2(damage_done));
            if (healed)
            {
                attacker->heal(healed);
                if (needs_message)
                {
                    string msg;
                    if (defender->is_player())
                    {
                        mprf("%s draws strength from your injuries!",
                             attacker->name(DESC_THE).c_str());
                    }
                    else
                    {
                        mprf("%s draws strength from %s injuries!",
                             attacker->name(DESC_THE).c_str(),
                             def_name(DESC_ITS).c_str());
                    }

                }
            }
        }
        break;

    case AF_DRAIN_STR:
    case AF_DRAIN_INT:
    case AF_DRAIN_DEX:
        if (one_chance_in(20) || one_chance_in(3))
        {
            stat_type drained_stat = (flavour == AF_DRAIN_STR ? STAT_STR :
                                      flavour == AF_DRAIN_INT ? STAT_INT
                                                              : STAT_DEX);
            defender->drain_stat(drained_stat, 1);
        }
        break;

    case AF_BLINK:
        // blinking can kill, delay the call
        if (one_chance_in(3))
            blink_fineff::schedule(attacker);
        break;

    case AF_BLINK_WITH:
        if (coinflip())
            blink_fineff::schedule(attacker, defender);
        break;

    case AF_CONFUSE:
        if (attk_type == AT_SPORE)
        {
            if (defender->is_unbreathing())
                break;

            monster *attkmon = attacker->as_monster();
            attkmon->set_hit_dice(attkmon->get_experience_level() - 1);
            if (attkmon->get_experience_level() <= 0)
                attacker->as_monster()->suicide();

            if (defender_visible)
            {
                string msg;
                if (defender->is_player())
                    msg = localise("You are engulfed in a cloud of spores!");
                else
                {
                    msg = localise("%s is engulfed in a cloud of spores!",
                                   defender->name(DESC_THE));
                }
                mpr_nolocalise(msg);
            }
        }

        if (one_chance_in(3))
        {
            defender->confuse(attacker,
                              1 + random2(3+attacker->get_hit_dice()));
        }
        break;

    case AF_DRAIN:
        if (coinflip())
            drain_defender();
        break;

    case AF_POISON_PARALYSE:
    {
        // Doesn't affect the poison-immune.
        if (defender->is_player() && you.duration[DUR_DIVINE_STAMINA] > 0)
        {
            mpr("Your divine stamina protects you from poison!");
            break;
        }
        else if (defender->res_poison() >= 3)
            break;

        // Same frequency as AF_POISON and AF_POISON_STRONG.
        if (one_chance_in(3))
        {
            int dmg = random_range(attacker->get_hit_dice() * 3 / 2,
                                   attacker->get_hit_dice() * 5 / 2);
            defender->poison(attacker, dmg);
        }

        // Try to apply either paralysis or slowing, with the normal 2/3
        // chance to resist with rPois.
        if (one_chance_in(6))
        {
            if (defender->res_poison() <= 0 || one_chance_in(3))
                defender->paralyse(attacker, roll_dice(1, 3));
        }
        else if (defender->res_poison() <= 0 || one_chance_in(3))
            defender->slow_down(attacker, roll_dice(1, 3));

        break;
    }

    case AF_REACH_TONGUE:
    case AF_ACID:
        defender->splash_with_acid(attacker, 3);
        break;

    case AF_CORRODE:
        defender->corrode_equipment(atk_name(DESC_THE).c_str());
        break;

    case AF_DISTORT:
        distortion_affects_defender();
        break;

    case AF_RAGE:
        if (!one_chance_in(3) || !defender->can_go_berserk())
            break;

        if (needs_message)
        {
            if (defender->is_player())
            {
                string fmt = "%s infuriates you!";
                mprf(fmt.c_str(),
                     defender_name(true).c_str());
            }
            else
            {
                string fmt = "%s infuriates %s!";
                mprf(fmt.c_str(),
                     atk_name(DESC_THE).c_str(),
                     defender_name(true).c_str());
            }
        }

        defender->go_berserk(false);
        break;

    case AF_STICKY_FLAME:
        mons_do_napalm();
        break;

    case AF_CHAOTIC:
        chaos_affects_defender();
        break;

    case AF_STEAL:
        // Ignore monsters, for now.
        if (!defender->is_player())
            break;

        attacker->as_monster()->steal_item_from_player();
        break;

    case AF_HOLY:
        if (defender->holy_wrath_susceptible())
            special_damage = attk_damage * 0.75;

        if (needs_message && special_damage)
        {
            do_3rd_person_message(attacker, defender, "%s sears %s",
                                  attack_strength_punctuation(special_damage));
        }
        break;

    case AF_ANTIMAGIC:
        antimagic_affects_defender(attacker->get_hit_dice() * 12);

        if (mons_genus(attacker->type) == MONS_VINE_STALKER
            && attacker->is_monster())
        {
            const bool spell_user = defender->antimagic_susceptible();

            if (you.can_see(*attacker) || you.can_see(*defender))
            {
                if (defender->is_player() && spell_user)
                {
                    mprf("%s drains your magic.",
                         attacker->name(DESC_THE).c_str());
                }
                else if (defender->is_player())
                {
                    mprf("%s drains your power.",
                         attacker->name(DESC_THE).c_str());
                }
                else if (spell_user)
                {
                    mprf("%s drains %s magic.",
                         attacker->name(DESC_THE).c_str(),
                         defender->name(DESC_ITS).c_str());
                }
                else
                {
                    mprf("%s drains %s power.",
                         attacker->name(DESC_THE).c_str(),
                         defender->name(DESC_ITS).c_str());
                }
            }

            monster* vine = attacker->as_monster();
            if (vine->has_ench(ENCH_ANTIMAGIC)
                && (defender->is_player()
                    || (!defender->as_monster()->is_summoned()
                        && !mons_is_firewood(*defender->as_monster()))))
            {
                mon_enchant me = vine->get_ench(ENCH_ANTIMAGIC);
                vine->lose_ench_duration(me, random2(damage_done) + 1);
                simple_monster_message(*attacker->as_monster(),
                                       spell_user
                                       ? "%s looks very invigorated."
                                       : "%s looks invigorated.");
            }
        }
        break;

    case AF_PAIN:
        pain_affects_defender();
        break;

    case AF_ENSNARE:
        if (one_chance_in(3))
            ensnare(defender);
        break;

    case AF_CRUSH:
        if (needs_message)
            do_3rd_person_message(attacker, defender, "%s grabs %s.");
        attacker->start_constricting(*defender);
        // if you got grabbed, interrupt stair climb and passwall
        if (defender->is_player())
            stop_delay(true);
        break;

    case AF_ENGULF:
        if (x_chance_in_y(2, 3)
            && attacker->can_constrict(defender, true, true))
        {
            const bool watery = attacker->type != MONS_QUICKSILVER_OOZE;
            if (defender->is_player() && !you.duration[DUR_WATER_HOLD])
            {
                you.duration[DUR_WATER_HOLD] = 10;
                you.props["water_holder"].get_int() = attacker->as_monster()->mid;
                you.props["water_hold_substance"].get_string() = watery ? "the water" : "the ooze";
            }
            else if (defender->is_monster()
                     && !defender->as_monster()->has_ench(ENCH_WATER_HOLD))
            {
                defender->as_monster()->add_ench(mon_enchant(ENCH_WATER_HOLD, 1,
                                                             attacker, 1));
            }
            else
                return; //Didn't apply effect; no message

            if (needs_message)
            {
                if (watery)
                {
                    do_3rd_person_message(attacker, defender,
                                          "%s engulfs %s in water!");
                }
                else
                    do_3rd_person_message(attacker, defender, "%s engulfs %s!");
            }
        }

        defender->expose_to_element(BEAM_WATER, 0);
        break;

    case AF_PURE_FIRE:
        if (attacker->type == MONS_FIRE_VORTEX)
            attacker->as_monster()->suicide(-10);

        special_damage = defender->apply_ac(base_damage, 0, ac_type::half);
        special_damage = resist_adjust_damage(defender,
                                              BEAM_FIRE,
                                              special_damage);

        if (needs_message && special_damage)
        {
            do_any_2_actors_message(attacker, defender,
                                    "burn", "", "!");
            _print_resist_messages(defender, special_damage, BEAM_FIRE);
        }

        defender->expose_to_element(BEAM_FIRE, 2);
        break;

    case AF_DRAIN_SPEED:
        if (x_chance_in_y(3, 5))
            drain_defender_speed();
        break;

    case AF_VULN:
        if (one_chance_in(3))
        {
            bool visible_effect = false;
            if (defender->is_player())
            {
                if (!you.duration[DUR_LOWERED_WL])
                    visible_effect = true;
                you.increase_duration(DUR_LOWERED_WL, 20 + random2(20), 40);
            }
            else
            {
                // Halving the WL of targets with infinite wills has no effect
                if (defender->as_monster()->willpower() == WILL_INVULN)
                    break;
                if (!defender->as_monster()->has_ench(ENCH_LOWERED_WL))
                    visible_effect = true;
                mon_enchant lowered_wl(ENCH_LOWERED_WL, 1, attacker,
                                       (20 + random2(20)) * BASELINE_DELAY);
                defender->as_monster()->add_ench(lowered_wl);
            }

            if (needs_message && visible_effect)
            {
                mprf("%s willpower is stripped away!",
                     def_name(DESC_ITS).c_str());
            }
        }
        break;

    case AF_SHADOWSTAB:
        attacker->as_monster()->del_ench(ENCH_INVIS, true);
        break;

    case AF_DROWN:
        if (attacker->type == MONS_DROWNED_SOUL)
            attacker->as_monster()->suicide(-1000);

        if (defender->res_water_drowning() <= 0)
        {
            special_damage = attacker->get_hit_dice() * 3 / 4
                            + random2(attacker->get_hit_dice() * 3 / 4);
            special_damage_flavour = BEAM_WATER;
            kill_type = KILLED_BY_WATER;

            if (needs_message)
            {
                do_3rd_person_message(attacker, defender, "%s drowns %s",
                                      attack_strength_punctuation(special_damage));
            }
        }
        break;

    case AF_WEAKNESS:
        if (coinflip())
            defender->weaken(attacker, 12);
        break;
    }
}

void melee_attack::do_passive_freeze()
{
    if (you.has_mutation(MUT_PASSIVE_FREEZE)
        && attacker->alive()
        && adjacent(you.pos(), attacker->as_monster()->pos()))
    {
        bolt beam;
        beam.flavour = BEAM_COLD;
        beam.thrower = KILL_YOU;

        monster* mon = attacker->as_monster();

        const int orig_hurted = random2(11);
        int hurted = mons_adjust_flavoured(mon, beam, orig_hurted);

        if (!hurted)
            return;

        simple_monster_message(*mon, " is very cold.");

        mon->hurt(&you, hurted);

        if (mon->alive())
        {
            mon->expose_to_element(BEAM_COLD, orig_hurted);
            print_wounds(*mon);
        }
    }
}

void melee_attack::mons_do_eyeball_confusion()
{
    if (you.has_mutation(MUT_EYEBALLS)
        && attacker->alive()
        && adjacent(you.pos(), attacker->as_monster()->pos())
        && x_chance_in_y(you.get_mutation_level(MUT_EYEBALLS), 20))
    {
        const int ench_pow = you.get_mutation_level(MUT_EYEBALLS) * 30;
        monster* mon = attacker->as_monster();

        if (mon->check_willpower(ench_pow) <= 0)
        {
            mprf("The eyeballs on your body gaze at %s.",
                 mon->name(DESC_THE).c_str());

            if (!mon->clarity())
            {
                mon->add_ench(mon_enchant(ENCH_CONFUSION, 0, &you,
                                          30 + random2(100)));
            }
        }
    }
}

void melee_attack::mons_do_tendril_disarm()
{
    monster* mon = attacker->as_monster();
    // some rounding errors here, but not significant
    const int adj_mon_hd = mon->is_fighter() ? mon->get_hit_dice() * 3 / 2
                                             : mon->get_hit_dice();

    if (you.get_mutation_level(MUT_TENDRILS)
        && one_chance_in(5)
        && (random2(you.dex()) > adj_mon_hd
            || random2(you.strength()) > adj_mon_hd))
    {
        item_def* mons_wpn = mon->disarm();
        if (mons_wpn)
        {
            mprf("Your tendrils lash around %s %s and pull it to the ground!",
                 apostrophise(mon->name(DESC_THE)).c_str(),
                 mons_wpn->name(DESC_PLAIN).c_str());
        }
    }
}

void melee_attack::do_spines()
{
    // Monsters only get struck on their first attack per round
    if (attacker->is_monster() && effective_attack_number > 0)
        return;

    if (defender->is_player())
    {
        const int mut = you.get_mutation_level(MUT_SPINY);

        if (mut && attacker->alive() && coinflip())
        {
            int dmg = random_range(mut,
                div_rand_round(you.experience_level * 2, 3) + mut * 3);
            int hurt = attacker->apply_ac(dmg);

            dprf(DIAG_COMBAT, "Spiny: dmg = %d hurt = %d", dmg, hurt);

            if (hurt <= 0)
                return;

            simple_monster_message(*attacker->as_monster(),
                                   " is struck by your spines.");

            attacker->hurt(&you, hurt);
        }
    }
    else if (defender->as_monster()->is_spiny())
    {
        // Thorn hunters can attack their own brambles without injury
        if (defender->type == MONS_BRIAR_PATCH
            && attacker->type == MONS_THORN_HUNTER
            // Dithmenos' shadow can't take damage, don't spam.
            || attacker->type == MONS_PLAYER_SHADOW)
        {
            return;
        }

        if (attacker->alive() && one_chance_in(3))
        {
            int dmg = roll_dice(5, 4);
            int hurt = attacker->apply_ac(dmg);
            dprf(DIAG_COMBAT, "Spiny: dmg = %d hurt = %d", dmg, hurt);

            if (hurt <= 0)
                return;
            if (you.can_see(*defender) || attacker->is_player())
            {
                string owner = apostrophise(defender->name(DESC_THE));
                if (attacker->is_player())
                {
                    if (defender->type == MONS_BRIAR_PATCH)
                        mprf("You are struck by %s thorns.", owner.c_str());
                    else
                        mprf("You are struck by %s spines.", owner.c_str());
                }
                else
                {
                    if (defender->type == MONS_BRIAR_PATCH)
                    {
                        mprf("%s is struck by %s thorns.",
                             attacker->name(DESC_THE).c_str(), owner.c_str());
                    }
                    else
                        mprf("%s is struck by %s spines.",
                             attacker->name(DESC_THE).c_str(), owner.c_str());
                }
            }
            attacker->hurt(defender, hurt, BEAM_MISSILE, KILLED_BY_SPINES);
        }
    }
}

void melee_attack::emit_foul_stench()
{
    monster* mon = attacker->as_monster();

    if (you.has_mutation(MUT_FOUL_STENCH)
        && attacker->alive()
        && adjacent(you.pos(), mon->pos()))
    {
        const int mut = you.get_mutation_level(MUT_FOUL_STENCH);

        if (damage_done > 0 && x_chance_in_y(mut * 3 - 1, 20)
            && !cell_is_solid(mon->pos())
            && !cloud_at(mon->pos()))
        {
            mpr("You emit a cloud of foul miasma!");
            place_cloud(CLOUD_MIASMA, mon->pos(), 5 + random2(6), &you);
        }
    }
}

void melee_attack::do_minotaur_retaliation()
{
    if (!defender->is_player())
    {
        // monsters have no STR or DEX
        if (x_chance_in_y(2, 5))
        {
            int hurt = attacker->apply_ac(random2(21));
            if (you.see_cell(defender->pos()))
            {
                const string defname = defender->name(DESC_THE);
                mprf("%s furiously retaliates!", defname.c_str());
                if (hurt <= 0)
                {
                    if (attacker->is_player())
                    {
                        mprf("%s headbutts you but does no damage.",
                             defname.c_str());
                    }
                    else
                    {
                        mprf("%s headbutts %s but does no damage.",
                             defname.c_str(),
                             attacker->name(DESC_THE).c_str());
                    }
                }
                else
                {
                    string msg;
                    if (attacker->is_player())
                    {
                        msg = localise("%s headbutts you",
                                       attacker->name(DESC_THE));
                    }
                    else
                    {
                        msg = localise("%s headbutts %s", defname,
                                       attacker->name(DESC_THE));
                    }
                    attack_strength_message(msg, hurt, false);
                }
            }
            if (hurt > 0)
            {
                attacker->hurt(defender, hurt, BEAM_MISSILE,
                               KILLED_BY_HEADBUTT);
            }
        }
        return;
    }

    if (!form_keeps_mutations())
    {
        // You are in a non-minotaur form.
        return;
    }
    // This will usually be 2, but could be 3 if the player mutated more.
    const int mut = you.get_mutation_level(MUT_HORNS);

    if (5 * you.strength() + 7 * you.dex() > random2(600))
    {
        // Use the same damage formula as a regular headbutt.
        int dmg = 5 + mut * 3;
        dmg = player_stat_modify_damage(dmg);
        dmg = random2(dmg);
        dmg = player_apply_fighting_skill(dmg, true);
        dmg = player_apply_misc_modifiers(dmg);
        dmg = player_apply_slaying_bonuses(dmg, true);
        dmg = player_apply_final_multipliers(dmg);
        int hurt = attacker->apply_ac(dmg);

        mpr("You furiously retaliate!");
        dprf(DIAG_COMBAT, "Retaliation: dmg = %d hurt = %d", dmg, hurt);
        if (hurt <= 0)
        {
            mprf("You headbutt %s but do no damage.",
                 attacker->name(DESC_THE).c_str());
            return;
        }
        else
        {
            string msg = localise("You headbutt %s", attacker->name(DESC_THE));
            attack_strength_message(msg, hurt, false);

            attacker->hurt(&you, hurt);
        }
    }
}

/** For UNRAND_STARLIGHT's dazzle effect, only against monsters.
 */
void melee_attack::do_starlight()
{
    static const vector<string> dazzle_msgs = {
        "@The_monster@ is blinded by the light from your cloak!",
        "@The_monster@ is temporarily struck blind!",
        "@The_monster@'s sight is seared by the starlight!",
        "@The_monster@'s vision is obscured by starry radiance!",
    };

    if (one_chance_in(5) && dazzle_monster(attacker->as_monster(), 100))
    {
        string msg = *random_iterator(dazzle_msgs);
        msg = do_mon_str_replacements(msg, *attacker->as_monster(), S_SILENT);
        mpr(msg);
    }
}


/**
 * Launch a long blade counterattack against the attacker. No sanity checks;
 * caller beware!
 *
 * XXX: might be wrong for deep elf blademasters with a long blade in only
 * one hand
 */
void melee_attack::riposte()
{
    if (you.see_cell(defender->pos()))
    {
        if (defender->is_player())
            mpr("You riposte.");
        else
            mprf("%s ripostes.", defender->name(DESC_THE).c_str());
    }
    melee_attack attck(defender, attacker, 0, effective_attack_number + 1);
    attck.is_riposte = true;
    attck.attack();
}

bool melee_attack::do_knockback(bool trample)
{
    if (defender->is_stationary())
        return false; // don't even print a message

    if (attacker->cannot_move())
        return false;

    const int size_diff =
        attacker->body_size(PSIZE_BODY) - defender->body_size(PSIZE_BODY);
    const coord_def old_pos = defender->pos();
    const coord_def new_pos = old_pos + old_pos - attack_position;

    if (!x_chance_in_y(size_diff + 3, 6)
        // need a valid tile
        || !defender->is_habitable_feat(env.grid(new_pos))
        // don't trample anywhere the attacker can't follow
        || !attacker->is_habitable_feat(env.grid(old_pos))
        // don't trample into a monster - or do we want to cause a chain
        // reaction here?
        || actor_at(new_pos)
        // Prevent trample/drown combo when flight is expiring
        || defender->is_player() && need_expiration_warning(new_pos)
        || defender->is_constricted())
    {
        if (needs_message)
        {
            if (defender->is_constricted())
            {
                if (defender->is_player())
                    mpr("You are held in place!");
                else
                    mprf("%s is held in place!",
                         defender_name(false).c_str());;
            }
            else
            {
                if (defender->is_player())
                    mpr("You hold your ground!");
                else
                {
                    // "hold his/her/its ground" is difficult to translate
                    // because grammatical gender can vary between languages
                    mprf("%s stands fast!", defender_name(false).c_str());
                }
            }
        }

        return false;
    }

    if (needs_message)
    {
        const bool can_stumble = !defender->airborne()
                                  && !defender->incapacitated();
        if (can_stumble)
        {
            if (defender->is_player())
                mpr("You stumble backwards!");
            else
                mprf("%s stumbles backwards!", defender->name(DESC_THE).c_str());
        }
        else
        {
            if (defender->is_player())
                mpr("You are shoved backwards!");
            else
                mprf("%s is shoved backwards!", defender->name(DESC_THE).c_str());
        }
    }

    // Schedule following _before_ actually trampling -- if the defender
    // is a player, a shaft trap will unload the level. If trampling will
    // somehow fail, move attempt will be ignored.
    if (trample)
        trample_follow_fineff::schedule(attacker, old_pos);

    if (defender->is_player())
    {
        move_player_to_grid(new_pos, false);
        // Interrupt stair travel and passwall.
        stop_delay(true);
    }
    else
        defender->move_to_pos(new_pos);

    return true;
}

/**
 * Find the list of targets to cleave after hitting the main target.
 */
void melee_attack::cleave_setup()
{
    // Don't cleave on a self-attack attack.
    if (attacker->pos() == defender->pos())
        return;

    // Allow Gyre & Gimble to 'cleave' when projected, but not other attacks.
    if (is_projected)
    {
        if (weapon && is_unrandom_artefact(*weapon, UNRAND_GYRE))
            cleave_targets.push_back(defender);
        return;
    }

    // We need to get the list of the remaining potential targets now because
    // if the main target dies, its position will be lost.
    get_cleave_targets(*attacker, defender->pos(), cleave_targets,
                       attack_number);
    // We're already attacking this guy.
    cleave_targets.pop_front();
}

// cleave damage modifier for additional attacks: 70% of base damage
int melee_attack::cleave_damage_mod(int dam)
{
    if (weapon && is_unrandom_artefact(*weapon, UNRAND_GYRE))
        return dam;
    return div_rand_round(dam * 7, 10);
}

// Martial strikes get modified by momentum and maneuver specific damage mods.
int melee_attack::martial_damage_mod(int dam)
{
    if (wu_jian_has_momentum(wu_jian_attack))
        dam = div_rand_round(dam * 14, 10);

    if (wu_jian_attack == WU_JIAN_ATTACK_LUNGE)
        dam = div_rand_round(dam * 12, 10);

    if (wu_jian_attack == WU_JIAN_ATTACK_WHIRLWIND)
        dam = div_rand_round(dam * 8, 10);

    return dam;
}

void melee_attack::chaos_affect_actor(actor *victim)
{
    ASSERT(victim); // XXX: change to actor &victim
    melee_attack attk(victim, victim);
    attk.weapon = nullptr;
    attk.fake_chaos_attack = true;
    attk.chaos_affects_defender();
    if (!attk.special_damage_message.empty()
        && you.can_see(*victim))
    {
        mpr(attk.special_damage_message);
    }
}

/**
 * Does the player get to use the given aux attack during this melee attack?
 *
 * Partially random.
 *
 * @param atk   The type of aux attack being considered.
 * @return      Whether the player may use the given aux attack.
 */
bool melee_attack::_extra_aux_attack(unarmed_attack_type atk)
{
    if (atk != UNAT_CONSTRICT
        && you.strength() + you.dex() <= random2(50))
    {
        return false;
    }

    if (wu_jian_attack != WU_JIAN_ATTACK_NONE
        && !x_chance_in_y(1, wu_jian_number_of_targets))
    {
       // Reduces aux chance proportionally to number of
       // enemies attacked with a martial attack
       return false;
    }

    switch (atk)
    {
    case UNAT_CONSTRICT:
        return you.get_mutation_level(MUT_CONSTRICTING_TAIL) >= 2
                || you.has_mutation(MUT_TENTACLE_ARMS)
                    && you.has_usable_tentacle();

    case UNAT_KICK:
        return you.has_usable_hooves()
               || you.has_usable_talons()
               || you.get_mutation_level(MUT_TENTACLE_SPIKE);

    case UNAT_PECK:
        return you.get_mutation_level(MUT_BEAK) && !one_chance_in(3);

    case UNAT_HEADBUTT:
        return you.get_mutation_level(MUT_HORNS) && !one_chance_in(3);

    case UNAT_TAILSLAP:
        return you.has_tail()
            // constricting tails are too slow to slap
            && !you.has_mutation(MUT_CONSTRICTING_TAIL)
            && coinflip();

    case UNAT_PSEUDOPODS:
        return you.has_usable_pseudopods() && !one_chance_in(3);

    case UNAT_TENTACLES:
        return you.has_usable_tentacles() && !one_chance_in(3);

    case UNAT_BITE:
        return you.get_mutation_level(MUT_ANTIMAGIC_BITE)
               || (you.has_usable_fangs()
                   || you.get_mutation_level(MUT_ACIDIC_BITE))
                   && x_chance_in_y(2, 5);

    case UNAT_PUNCH:
        return player_gets_aux_punch();

    default:
        return false;
    }
}

// TODO: Potentially move this, may or may not belong here (may not
// even belong as its own function, could be integrated with the general
// to-hit method
// Returns the to-hit for your extra unarmed attacks.
// DOES NOT do the final roll (i.e., random2(your_to_hit)).
int melee_attack::calc_your_to_hit_unarmed()
{
    int your_to_hit;

    your_to_hit = 1300
                + you.dex() * 75
                + you.skill(SK_FIGHTING, 30);
    your_to_hit /= 100;

    your_to_hit -= 5 * you.inaccuracy();

    if (you.get_mutation_level(MUT_EYEBALLS))
        your_to_hit += 2 * you.get_mutation_level(MUT_EYEBALLS) + 1;

    if (you.duration[DUR_VERTIGO])
        your_to_hit -= 5;

    if (you.confused())
        your_to_hit -= 5;

    your_to_hit += slaying_bonus();

    return your_to_hit;
}

bool melee_attack::using_weapon() const
{
    return weapon && is_melee_weapon(*weapon);
}

int melee_attack::weapon_damage()
{
    if (!using_weapon())
        return 0;

    return property(*weapon, PWPN_DAMAGE);
}

int melee_attack::calc_mon_to_hit_base()
{
    const bool fighter = attacker->is_monster()
                         && attacker->as_monster()->is_fighter();
    return mon_to_hit_base(attacker->get_hit_dice(), fighter, false);
}

/**
 * Add modifiers to the base damage.
 * Currently only relevant for monsters.
 */
int melee_attack::apply_damage_modifiers(int damage)
{
    ASSERT(attacker->is_monster());
    monster *as_mon = attacker->as_monster();

    // Berserk/mighted monsters get bonus damage.
    if (as_mon->has_ench(ENCH_MIGHT) || as_mon->has_ench(ENCH_BERSERK))
        damage = damage * 3 / 2;

    if (as_mon->has_ench(ENCH_IDEALISED))
        damage *= 2; // !

    if (as_mon->has_ench(ENCH_WEAK))
        damage = damage * 2 / 3;

    // If the defender is asleep, the attacker gets a stab.
    if (defender && (defender->asleep()
                     || (attk_flavour == AF_SHADOWSTAB
                         &&!defender->can_see(*attacker))))
    {
        damage = damage * 5 / 2;
        dprf(DIAG_COMBAT, "Stab damage vs %s: %d",
             defender->name(DESC_PLAIN).c_str(),
             damage);
    }

    if (cleaving)
        damage = cleave_damage_mod(damage);

    return damage;
}

int melee_attack::calc_damage()
{
    // Constriction deals damage over time, not when grabbing.
    if (attk_flavour == AF_CRUSH)
        return 0;

    return attack::calc_damage();
}

/* TODO: This code is only used from melee_attack methods, but perhaps it
 * should be ambigufied and moved to the actor class
 * Should life protection protect from this?
 *
 * Should eventually remove in favour of player/monster symmetry
 *
 * Called when stabbing and for bite attacks.
 *
 * Returns true if blood was drawn.
 */
bool melee_attack::_player_vampire_draws_blood(const monster* mon, const int damage,
                                               bool needs_bite_msg)
{
    ASSERT(you.has_mutation(MUT_VAMPIRISM));

    if (!_vamp_wants_blood_from_monster(mon) ||
        (!adjacent(defender->pos(), attack_position) && needs_bite_msg))
    {
        return false;
    }

    // Now print message, need biting unless already done (never for bat form!)
    if (needs_bite_msg && you.form != transformation::bat)
    {
        mprf("You bite %s, and draw blood!",
             mon->name(DESC_THE, true).c_str());
    }
    else
    {
        mprf("You draw %s blood!",
             apostrophise(mon->name(DESC_THE, true)).c_str());
    }

    // Regain hp.
    if (you.hp < you.hp_max)
    {
        int heal = 2 + random2(damage);
        heal += random2(damage);
        if (heal > you.experience_level)
            heal = you.experience_level;

        if (heal > 0 && !you.duration[DUR_DEATHS_DOOR])
        {
            inc_hp(heal);
            canned_msg(MSG_GAIN_HEALTH);
        }
    }

    return true;
}

bool melee_attack::apply_damage_brand(const char *what)
{
    // Staff damage overrides any brands
    return apply_staff_damage() || attack::apply_damage_brand(what);
}

bool melee_attack::_vamp_wants_blood_from_monster(const monster* mon)
{
    return you.has_mutation(MUT_VAMPIRISM)
           && !you.vampire_alive
           && actor_is_susceptible_to_vampirism(*mon)
           && mons_has_blood(mon->type);
}
