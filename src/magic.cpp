/*! \page License
   KQ is Copyright (C) 2002 by Josh Bolduc

   This file is part of KQ... a freeware RPG.

   KQ is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   KQ is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with KQ; see the file COPYING.  If not, write to
   the Free Software Foundation,
       675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <stdio.h>
#include <string.h>

#include "combat.h"
#include "draw.h"
#include "effects.h"
#include "enemyc.h"
#include "fade.h"
#include "heroc.h"
#include "itemdefs.h"
#include "itemmenu.h"
#include "kq.h"
#include "magic.h"
#include "menu.h"
#include "res.h"
#include "setup.h"
#include "ssprites.h"
#include "structs.h"


/*! \file
 * \brief Magic spells
 *
 * \author JB
 * \date ????????
 */


/*  Internal functions  */
static void beffect_all_enemies(size_t, size_t);
static void beffect_one_enemy(size_t, size_t, size_t);
static void cure_oneall_allies(size_t, int, size_t);
static void damage_all_enemies(size_t, size_t);
static void damage_oneall_enemies(size_t, int, size_t);
static void geffect_all_allies(size_t, size_t);
static void geffect_one_ally(size_t, size_t);
static void heal_one_ally(size_t, size_t, size_t);
static void set_timed_sts_effect(size_t, int);
static void special_spells(size_t, size_t);
static void spell_damage(size_t, int, size_t, size_t);


/*! \brief Adjust character's HP
 *
 * I put this is just to make things nice and neat.
 *
 * \param   fighter_index Index of character
 * \param   amt Amount to adjust
 */
void adjust_hp(size_t fighter_index, int amt)
{
    fighter[fighter_index].hp += amt;
    if (fighter[fighter_index].hp > fighter[fighter_index].mhp)
    {
        fighter[fighter_index].hp = fighter[fighter_index].mhp;
    }
    if (fighter[fighter_index].hp < 0)
    {
        fighter[fighter_index].hp = 0;
    }
}



/*! \brief Adjust character's MP
 *
 * I put this is just to make things nice and neat.
 *
 * \param   fighter_index Index of character
 * \param   amt Amount to adjust
 */
void adjust_mp(size_t fighter_index, int amt)
{
    fighter[fighter_index].mp += amt;
    if (fighter[fighter_index].mp > fighter[fighter_index].mmp)
    {
        fighter[fighter_index].mp = fighter[fighter_index].mmp;
    }
    if (fighter[fighter_index].mp < 0)
    {
        fighter[fighter_index].mp = 0;
    }
}



/*! \brief Bad effects on all targets
 *
 * These are 'bad' effect spells that affect all enemy targets.
 *
 * \param   caster_fighter_index Caster
 * \param   spell_number Spell number
 */
static void beffect_all_enemies(size_t caster_fighter_index, size_t spell_number)
{
    size_t end_fighter_index, start_fighter_index, fighter_index;
    int sp_hit;

    if (caster_fighter_index < PSIZE)
    {
        start_fighter_index = PSIZE;
        end_fighter_index = num_enemies;
    }
    else
    {
        start_fighter_index = 0;
        end_fighter_index = numchrs;
    }
    for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
    {
        ta[fighter_index] = NODISPLAY;
    }
    sp_hit = magic[spell_number].hit;
    switch (spell_number)
    {
        case M_SLOW:
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
            {
                if (res_throw(fighter_index, magic[spell_number].elem) == 0
                        && non_dmg_save(fighter_index, sp_hit) == 0
                        && fighter[fighter_index].sts[S_STONE] == 0)
                {
                    if (fighter[fighter_index].sts[S_TIME] == 2)
                    {
                        fighter[fighter_index].sts[S_TIME] = 0;
                    }
                    else
                    {
                        if (fighter[fighter_index].sts[S_TIME] == 0)
                        {
                            fighter[fighter_index].sts[S_TIME] = 1;
                            ta[fighter_index] = NODISPLAY;
                        }
                        else
                        {
                            ta[fighter_index] = MISS;
                        }
                    }
                }
                else
                {
                    ta[fighter_index] = MISS;
                }
            }
            break;
        case M_VISION:
            vspell = 1;
            do_transition(TRANS_FADE_OUT, 2);
            battle_render(0, 0, 0);
            blit2screen(0, 0);
            do_transition(TRANS_FADE_IN, 2);
            break;
        case M_MALISON:
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
            {
                if (non_dmg_save(fighter_index, sp_hit) == 0
                    && fighter[fighter_index].sts[S_MALISON] == 0
                    && fighter[fighter_index].sts[S_STONE] == 0)
                {
                    fighter[fighter_index].sts[S_MALISON] = 2;
                    ta[fighter_index] = NODISPLAY;
                }
                else
                {
                    ta[fighter_index] = MISS;
                }
            }
            break;
        case M_SLEEPALL:
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
            {
                if (res_throw(fighter_index, magic[spell_number].elem) == 0
                    && non_dmg_save(fighter_index, sp_hit) == 0
                    && fighter[fighter_index].sts[S_SLEEP] == 0
                    && fighter[fighter_index].sts[S_STONE] == 0)
                {
                    fighter[fighter_index].sts[S_SLEEP] = rand() % 2 + 4;
                    ta[fighter_index] = NODISPLAY;
                }
                else
                {
                    ta[fighter_index] = MISS;
                }
            }
            break;
    }
}



/*! \brief Bad effects on one target
 *
 * This function handles 'bad' effect spells that have a single target.
 *
 * \param   caster_fighter_index Caster
 * \param   target_fighter_index Target
 * \param   spell_number Spell number
 */
static void beffect_one_enemy(size_t caster_fighter_index, size_t target_fighter_index, size_t spell_number)
{
    int r, a = 0, sp_hit;
    size_t stats_index;

    ta[target_fighter_index] = NODISPLAY;
    if (fighter[target_fighter_index].sts[S_STONE] > 0)
    {
        ta[target_fighter_index] = MISS;
        return;
    }
    if (res_throw(target_fighter_index, magic[spell_number].elem) == 1)
    {
        ta[target_fighter_index] = MISS;
        return;
    }
    sp_hit = magic[spell_number].hit;
    switch (spell_number)
    {
        case M_BLIND:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0 && fighter[target_fighter_index].sts[S_BLIND] == 0)
            {
                fighter[target_fighter_index].sts[S_BLIND] = 1;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_CONFUSE:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0 && fighter[target_fighter_index].sts[S_CHARM] == 0)
            {
                fighter[target_fighter_index].sts[S_CHARM] = rand() % 3 + 3;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_STONE:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0)
            {
                for (stats_index = 0; stats_index < 24; stats_index++)
                {
                    if (stats_index != S_DEAD)
                    {
                        fighter[target_fighter_index].sts[stats_index] = 0;
                    }
                }
                fighter[target_fighter_index].sts[S_STONE] = rand() % 3 + 3;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_DIFFUSE:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0)
            {
                r = 0;
                if (fighter[target_fighter_index].sts[S_RESIST] > 0)
                {
                    fighter[target_fighter_index].sts[S_RESIST] = 0;
                    r++;
                }
                if (fighter[target_fighter_index].sts[S_TIME] > 1)
                {
                    fighter[target_fighter_index].sts[S_TIME] = 0;
                    r++;
                }
                if (fighter[target_fighter_index].sts[S_SHIELD] > 0)
                {
                    fighter[target_fighter_index].sts[S_SHIELD] = 0;
                    r++;
                }
                if (fighter[target_fighter_index].sts[S_BLESS] > 0)
                {
                    fighter[target_fighter_index].sts[S_BLESS] = 0;
                    r++;
                }
                if (fighter[target_fighter_index].sts[S_STRENGTH] > 0)
                {
                    fighter[target_fighter_index].sts[S_STRENGTH] = 0;
                    r++;
                }
                if (r == 0)
                {
                    ta[target_fighter_index] = MISS;
                }
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_HOLD:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0 && fighter[target_fighter_index].sts[S_STOP] == 0)
            {
                fighter[target_fighter_index].sts[S_STOP] = rand() % 3 + 2;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_SILENCE:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0 && fighter[target_fighter_index].sts[S_MUTE] == 0)
            {
                fighter[target_fighter_index].sts[S_MUTE] = 1;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_SLEEP:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0 && fighter[target_fighter_index].sts[S_SLEEP] == 0)
            {
                fighter[target_fighter_index].sts[S_SLEEP] = rand() % 2 + 4;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_ABSORB:
            spell_damage(caster_fighter_index, spell_number, target_fighter_index, 1);
            r = ta[target_fighter_index];
            if (non_dmg_save(target_fighter_index, sp_hit) == 1)
            {
                r = r / 2;
            }
            if (fighter[target_fighter_index].mp < abs(r))
            {
                r = 0 - fighter[target_fighter_index].mp;
            }
            ta[target_fighter_index] = r;
            ta[caster_fighter_index] = 0 - r;
            break;
        case M_DRAIN:
            spell_damage(caster_fighter_index, spell_number, target_fighter_index, 1);
            r = ta[target_fighter_index];
            if (non_dmg_save(target_fighter_index, sp_hit) == 1)
            {
                r = r / 2;
            }
            if (fighter[target_fighter_index].unl > 0)
            {
                if (fighter[caster_fighter_index].hp < abs(r))
                {
                    r = 0 - fighter[caster_fighter_index].hp;
                }
                ta[target_fighter_index] = 0 - r;
                ta[caster_fighter_index] = r;
            }
            else
            {
                if (fighter[target_fighter_index].hp < abs(r))
                {
                    r = 0 - fighter[target_fighter_index].hp;
                }
                ta[target_fighter_index] = r;
                ta[caster_fighter_index] = 0 - r;
            }
            break;
        case M_DOOM:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0)
            {
                a = fighter[target_fighter_index].hp * 3 / 4;
                if (a < 1)
                {
                    a = 1;
                }
                if (fighter[target_fighter_index].hp - a < 1)
                {
                    a = fighter[target_fighter_index].hp - 1;
                }
                ta[target_fighter_index] = 0 - a;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_DEATH:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0)
            {
                a = fighter[target_fighter_index].hp;
                ta[target_fighter_index] = 0 - a;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_NAUSEA:
            if (non_dmg_save(target_fighter_index, sp_hit) == 0 && fighter[target_fighter_index].sts[S_MALISON] == 0)
            {
                fighter[target_fighter_index].sts[S_MALISON] = 1;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
    }
}



/*! \brief Use imbued item like spell
 *
 * This is used to invoke items inbued with a spell
 *
 * \param   fighter_index Attacker
 * \param   target_item Item for imbued spell
 * \param   sag_int_value Value for SAG and INT when casting imbued
 * \param   tgt Target (defender)
 */
void cast_imbued_spell(size_t fighter_index, int target_item, int sag_int_value, int tgt)
{
    int temp_int = fighter[fighter_index].stats[A_INT];
    int temp_sag = fighter[fighter_index].stats[A_SAG];
    int temp_aur = fighter[fighter_index].stats[A_AUR];
    int temp_spi = fighter[fighter_index].stats[A_SPI];

    fighter[fighter_index].stats[A_INT] = sag_int_value;
    fighter[fighter_index].stats[A_SAG] = sag_int_value;
    fighter[fighter_index].stats[A_AUR] = 100;
    fighter[fighter_index].stats[A_SPI] = 100;
    fighter[fighter_index].csmem = target_item;
    fighter[fighter_index].ctmem = tgt;
    if (tgt == TGT_CASTER)
    {
        fighter[fighter_index].ctmem = fighter_index;
        cast_spell(fighter_index, 1);
    }
    else
    {
        combat_spell(fighter_index, 1);
    }

    fighter[fighter_index].stats[A_INT] = temp_int;
    fighter[fighter_index].stats[A_SAG] = temp_sag;
    fighter[fighter_index].stats[A_AUR] = temp_aur;
    fighter[fighter_index].stats[A_SPI] = temp_spi;
}



/*! \brief Cast a spell
 *
 * Generic function called from camp or combat to cast a spell
 *
 * \param   caster_fighter_index Index of caster
 * \param   is_item 0 if regular spell, 1 if item (no MP used)
 * \returns 1 if spell cast/used successfully, 0 otherwise
 */
int cast_spell(size_t caster_fighter_index, int is_item)
{
    int spell_number = fighter[caster_fighter_index].csmem;
    int tgt = fighter[caster_fighter_index].ctmem;
    int c;

    if (is_item == 0)
    {
        c = mp_needed(caster_fighter_index, spell_number);
        if (c < 1)
        {
            c = 1;
        }
        fighter[caster_fighter_index].mp -= c;
        /*
            check for spell failure - only applies to spells that
            don't have a hit% or do damage
         */

        /*  DS IDEA: move this code to the function non_dmg_save() */
        if (magic[spell_number].dmg == 0 && magic[spell_number].bon == 0
                && magic[spell_number].hit == 0)
        {
            if (rand() % 100 + 1 >
                    fighter[caster_fighter_index].stats[A_AUR + magic[spell_number].stat])
            {

                /*  DS: The spell fail, so set ta[target] to MISS */
                if (tgt != SEL_ALL_ALLIES)
                {
                    ta[tgt] = MISS;
                }
                else
                {
                    size_t i, end_fighter_index, start_fighter_index;

                    if (caster_fighter_index < PSIZE)
                    {
                        end_fighter_index = numchrs;
                        start_fighter_index = 0;
                    }
                    else
                    {
                        end_fighter_index = num_enemies;
                        start_fighter_index = PSIZE;
                    }
                    for (i = start_fighter_index; i < end_fighter_index; i++)
                    {
                        ta[i] = MISS;
                    }
                }

                return 0;
            }
        }
    }
    /* call the appropriate spell effect function */
    switch (magic[spell_number].icon)
    {
        case 40:
        case 41:
            special_spells(caster_fighter_index, spell_number);
            break;
        case 45:
            cure_oneall_allies(caster_fighter_index, tgt, spell_number);
            break;
        case 46:
            heal_one_ally(caster_fighter_index, tgt, spell_number);
            break;
        case 47:
        case 42:
            if (magic[spell_number].tgt == TGT_ALLY_ONE)
            {
                geffect_one_ally(tgt, spell_number);
            }
            else
            {
                geffect_all_allies(caster_fighter_index, spell_number);
            }
            break;
        case 48:
        case 43:
            if (magic[spell_number].tgt == TGT_ENEMY_ONE)
            {
                beffect_one_enemy(caster_fighter_index, tgt, spell_number);
            }
            else
            {
                beffect_all_enemies(caster_fighter_index, spell_number);
            }
            break;
        case 49:
        case 44:
            if (magic[spell_number].tgt == TGT_ENEMY_ALL)
            {
                damage_all_enemies(caster_fighter_index, spell_number);
            }
            else
            {
                damage_oneall_enemies(caster_fighter_index, tgt, spell_number);
            }
            break;
    }
    return 1;
}



/*! \brief Call spells for combat
 *
 * This function just calls the right magic routine based on the spell's
 * type and target.  This function also displays the caster and spell
 * effects.
 *
 * \param   caster_fighter_index Index of caster
 * \param   is_item 0 if regular spell, 1 if item (no MP used)
 * \returns 1 if spell cast/used successfully, 0 otherwise
 */
int combat_spell(size_t caster_fighter_index, int is_item)
{
    int b, tgt, spell_number, tall = 0, ss = 0;
    size_t fighter_index;
    size_t start_fighter_index;
    size_t end_fighter_index = 1;

    spell_number = fighter[caster_fighter_index].csmem;
    if (magic[spell_number].tgt == TGT_NONE)
    {
        return 0;
    }
    tgt = fighter[caster_fighter_index].ctmem;
    end_fighter_index = 1;
    start_fighter_index = tgt;
    if (magic[spell_number].tgt >= TGT_ALLY_ONE && magic[spell_number].tgt <= TGT_ALLY_ALL)
    {
        if (tgt == SEL_ALL_ALLIES)
        {
            tall = 1;
            if (caster_fighter_index < PSIZE)
            {
                end_fighter_index = numchrs;
                start_fighter_index = 0;
            }
            else
            {
                end_fighter_index = num_enemies;
                start_fighter_index = PSIZE;
            }
        }
    }
    else
    {
        if (tgt == SEL_ALL_ENEMIES)
        {
            tall = 1;
            if (caster_fighter_index < PSIZE)
            {
                end_fighter_index = num_enemies;
                start_fighter_index = PSIZE;
            }
            else
            {
                end_fighter_index = numchrs;
                start_fighter_index = 0;
            }
        }
    }
    strcpy(attack_string, magic[spell_number].name);
    if (is_item == 0)
    {
        draw_castersprite(caster_fighter_index, eff[magic[spell_number].eff].kolor);
    }
    if (magic[spell_number].dmg > 0)
    {
        ss = 1;
    }
    if (spell_number == M_LIFE || spell_number == M_FULLLIFE)
    {
        deadeffect = 1;
    }
    if (spell_number == M_TREMOR || spell_number == M_EARTHQUAKE)
    {
        if (start_fighter_index == 0)
        {
            draw_hugesprite(start_fighter_index, 80, 126, magic[spell_number].eff, 1);
        }
        else
        {
            draw_hugesprite(start_fighter_index, 80, 66, magic[spell_number].eff, 1);
        }
    }
    else
    {
        if (spell_number == M_FLOOD || spell_number == M_TSUNAMI)
        {
            if (start_fighter_index == 0)
            {
                draw_hugesprite(start_fighter_index, 80, 108, magic[spell_number].eff, 1);
            }
            else
            {
                draw_hugesprite(start_fighter_index, 80, 56, magic[spell_number].eff, 1);
            }
        }
        else
        {
            if (spell_number != M_VISION && spell_number != M_WARP)
            {
                draw_spellsprite(start_fighter_index, tall, magic[spell_number].eff, ss);
            }
        }
    }

    if (spell_number == M_LIFE || spell_number == M_FULLLIFE)
    {
        deadeffect = 0;
    }

    cast_spell(caster_fighter_index, is_item);

    if (spell_number == M_ABSORB || spell_number == M_DRAIN)
    {
        if (spell_number == M_ABSORB)
        {
            if (ta[tgt] <= 0)
            {
                display_amount(tgt, FRED, 0);
                adjust_mp(tgt, ta[tgt]);
                display_amount(caster_fighter_index, FGREEN, 0);
                adjust_mp(caster_fighter_index, ta[caster_fighter_index]);
            }
            else
            {
                display_amount(caster_fighter_index, FRED, 0);
                adjust_mp(caster_fighter_index, ta[caster_fighter_index]);
                display_amount(tgt, FGREEN, 0);
                adjust_mp(tgt, ta[tgt]);
            }
        }
        else
        {
            if (ta[tgt] <= 0)
            {
                display_amount(tgt, FNORMAL, 0);
                adjust_hp(tgt, ta[tgt]);
                display_amount(caster_fighter_index, FYELLOW, 0);
                adjust_hp(caster_fighter_index, ta[caster_fighter_index]);
            }
            else
            {
                display_amount(caster_fighter_index, FNORMAL, 0);
                adjust_hp(caster_fighter_index, ta[caster_fighter_index]);
                display_amount(tgt, FYELLOW, 0);
                adjust_hp(tgt, ta[tgt]);
            }
        }
    }
    else
    {
        if (ss == 0)
        {
            b = 0;
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
            {
                if (ta[fighter_index] == MISS)
                {
                    b++;
                }
            }
            if (b > 0)
            {
                display_amount(start_fighter_index, FNORMAL, tall);
            }
        }
        else
        {
            display_amount(start_fighter_index, FDECIDE, tall);
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
            {
                adjust_hp(fighter_index, ta[fighter_index]);
            }
        }
    }
    b = 0;
    for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
    {
        if (fighter[fighter_index].sts[S_DEAD] == 0 && fighter[fighter_index].hp <= 0)
        {
            fkill(fighter_index);
            ta[fighter_index] = 1;
            b++;
        }
        else
        {
            ta[fighter_index] = 0;
        }
    }
    if (b > 0)
    {
        death_animation(start_fighter_index, tall);
    }

    return 1;
}



/*! \brief Healing spell handler
 *
 * This function only handles healing spells (one or all allied targets).
 *
 * \param   caster_fighter_index Caster
 * \param   tgt Target
 * \param   spell_number Spell number
 */
static void cure_oneall_allies(size_t caster_fighter_index, int tgt, size_t spell_number)
{
    int a = 0, b = 0, z = 0, spwr;
    size_t fighter_index;
    size_t end_fighter_index, start_fighter_index;

    if (tgt == SEL_ALL_ALLIES)
    {
        if (caster_fighter_index < PSIZE)
        {
            end_fighter_index = numchrs;
            start_fighter_index = 0;
        }
        else
        {
            end_fighter_index = num_enemies;
            start_fighter_index = PSIZE;
        }
    }
    else
    {
        start_fighter_index = tgt;
        end_fighter_index = 1;
    }
    spwr =
        magic[spell_number].dmg +
        (fighter[caster_fighter_index].stats[A_INT + magic[spell_number].stat] *
         magic[spell_number].bon / 100);
    if (spwr < DMG_RND_MIN * 5)
    {
        b = rand() % DMG_RND_MIN + spwr;
    }
    else
    {
        b = rand() % (spwr / 5) + spwr;
    }
    a = fighter[caster_fighter_index].stats[A_AUR + magic[spell_number].stat];
    b = b * a / 100;
    if (b < 1)
    {
        b = 1;
    }
    for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
    {
        if (fighter[fighter_index].sts[S_STONE] == 0 && fighter[fighter_index].sts[S_DEAD] == 0)
        {
            z++;
        }
    }
    if (z == 0)
    {
        klog(_("... the hell, how can there be nobody to cure?"));
        return;
    }
    if (tgt == SEL_ALL_ALLIES)
    {
        b = b / z;
    }

    /*
        If you wonder why I do this separate like this, it's just for looks.
        This way, it displays the amounts on screen, then adds the hp after
        the visual effect has taken place... it just looks nicer that way.
     */
    for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
    {
        if (fighter[fighter_index].sts[S_STONE] == 0 && fighter[fighter_index].sts[S_DEAD] == 0)
        {
            ta[fighter_index] = b;
            ta[fighter_index] = do_shell_check(fighter_index, ta[fighter_index]);
        }
    }
}



/*! \brief Damage effects on all targets
 *
 * These are damage spells that affect the entire enemy party.
 *
 * \param   caster_fighter_index Caster
 * \param   spell_number Spell number
 */
static void damage_all_enemies(size_t caster_fighter_index, size_t spell_number)
{
    size_t end_fighter_index, start_fighter_index;

    if (caster_fighter_index < PSIZE)
    {
        start_fighter_index = PSIZE;
        end_fighter_index = num_enemies;
    }
    else
    {
        start_fighter_index = 0;
        end_fighter_index = numchrs;
    }
    spell_damage(caster_fighter_index, spell_number, start_fighter_index, end_fighter_index);
}



/*! \brief Damage effects on one or all enemies
 *
 * These are damage spells that affect the one or all of the enemy's party.
 *
 * \param   caster_fighter_index Caster
 * \param   tgt Traget
 * \param   spell_number Spell number
 */
static void damage_oneall_enemies(size_t caster_fighter_index, int tgt, size_t spell_number)
{
    size_t end_fighter_index, start_fighter_index;

    if (tgt == SEL_ALL_ENEMIES)
    {
        if (caster_fighter_index < PSIZE)
        {
            end_fighter_index = num_enemies;
            start_fighter_index = PSIZE;
        }
        else
        {
            end_fighter_index = numchrs;
            start_fighter_index = 0;
        }
    }
    else
    {
        start_fighter_index = tgt;
        end_fighter_index = 1;
    }
    spell_damage(caster_fighter_index, spell_number, start_fighter_index, end_fighter_index);
}



/*! \brief Check if character is protected by shell.
 *
 * This just checks to see if the target has a shell protecting him/her.
 *
 * \param   tgt Target
 * \param   amt Amount of damage to ricochet off shield
 * \returns the amount of damage that gets through to target
 */
int do_shell_check(int tgt, int amt)
{
    int a = 0;

    if (fighter[tgt].sts[S_RESIST] == 0)
    {
        return amt;
    }
    if (fighter[tgt].sts[S_RESIST] == 1)
    {
        a = amt * 75 / 100;
    }
    if (fighter[tgt].sts[S_RESIST] == 2)
    {
        a = amt * 5 / 10;
    }
    return a;
}



/*! \brief Check if character is protected by shield.
 *
 * This just checks to see if the target has a shield protecting him/her.
 *
 * \param   tgt Target
 * \param   amt Amount of damage to ricochet off shield
 * \returns the amount of damage that gets through to target
 */
int do_shield_check(int tgt, int amt)
{
    int a = 0;

    if (fighter[tgt].sts[S_SHIELD] == 0)
    {
        return amt;
    }
    if (fighter[tgt].sts[S_SHIELD] == 1)
    {
        a = amt * 75 / 100;
    }
    if (fighter[tgt].sts[S_SHIELD] == 2)
    {
        a = amt * 666 / 1000;
    }
    return a;
}



/*! \brief Good effects on all allies
 *
 * These are 'good' effect spells that affect all allied targets.
 *
 * \param   caster_fighter_index Caster
 * \param   spell_number Spell Number
 */
static void geffect_all_allies(size_t caster_fighter_index, size_t spell_number)
{
    int fighter_hp;
    size_t fighter_index = 0;
    size_t end_fighter_index, start_fighter_index;

    if (caster_fighter_index < PSIZE)
    {
        end_fighter_index = numchrs;
        start_fighter_index = 0;
    }
    else
    {
        end_fighter_index = num_enemies;
        start_fighter_index = PSIZE;
    }
    if (rand() % 100 + 1 > fighter[caster_fighter_index].stats[A_AUR + magic[spell_number].stat])
    {
        for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
        {
            ta[fighter_index] = MISS;
        }
        return;
    }
    switch (spell_number)
    {
        case M_BLESS:
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
            {
                if (fighter[fighter_index].sts[S_BLESS] < 3)
                {
                    fighter_hp = fighter[fighter_index].mhp / 10;
                    if (fighter_hp < 10)
                    {
                        fighter_hp = 10;
                    }
                    fighter[fighter_index].hp += fighter_hp;
                    fighter[fighter_index].mhp += fighter_hp;
                    fighter[fighter_index].sts[S_BLESS]++;
                    ta[fighter_index] = NODISPLAY;
                }
                else
                {
                    ta[fighter_index] = MISS;
                }
            }
            break;
        case M_SHIELDALL:
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
            {
                if (fighter[fighter_index].sts[S_SHIELD] < 2)
                {
                    fighter[fighter_index].sts[S_SHIELD] = 2;
                    ta[fighter_index] = NODISPLAY;
                }
                else
                {
                    ta[fighter_index] = MISS;
                }
            }
            break;
        case M_DIVINEGUARD:
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
            {
                if (fighter[fighter_index].sts[S_SHIELD] < 2 || fighter[fighter_index].sts[S_RESIST] < 2)
                {
                    if (fighter[fighter_index].sts[S_SHIELD] < 2)
                    {
                        fighter[fighter_index].sts[S_SHIELD] = 2;
                    }
                    if (fighter[fighter_index].sts[S_RESIST] < 2)
                    {
                        fighter[fighter_index].sts[S_RESIST] = 2;
                    }
                }
                else
                {
                    ta[fighter_index] = MISS;
                }
            }
            break;
        case M_QUICKEN:
            for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
            {
                if (fighter[fighter_index].sts[S_TIME] != 2 && fighter[fighter_index].sts[S_STONE] == 0)
                {
                    if (fighter[fighter_index].sts[S_TIME] == 1)
                    {
                        fighter[fighter_index].sts[S_TIME] = 0;
                    }
                    else
                    {
                        fighter[fighter_index].sts[S_TIME] = 2;
                        ta[fighter_index] = NODISPLAY;
                    }
                }
                else
                {
                    ta[fighter_index] = MISS;
                }
            }
            break;
    }
}



/*! \brief Good effects on one ally
 *
 * These are 'good' effect spells that affect a single allied target.
 *
 * \param   target_fighter_index Target
 * \param   spell_number Spell number
 */
static void geffect_one_ally(size_t target_fighter_index, size_t spell_number)
{
    /* Validate the target_fighter_index parameter */
    if (target_fighter_index >= NUM_FIGHTERS)
    {
        program_death(_("Invalid target parameter in geffect_one_ally"));
    }

    switch (spell_number)
    {
        case M_TRUEAIM:
            if (fighter[target_fighter_index].sts[S_TRUESHOT] == 0)
            {
                fighter[target_fighter_index].sts[S_TRUESHOT] = 1;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_THROUGH:
            if (fighter[target_fighter_index].sts[S_ETHER] == 0)
            {
                fighter[target_fighter_index].sts[S_ETHER] = 3;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_REGENERATE:
            if (fighter[target_fighter_index].sts[S_REGEN] == 0)
            {
                set_timed_sts_effect(target_fighter_index, S_REGEN);
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_HOLYMIGHT:
            if (fighter[target_fighter_index].sts[S_STRENGTH] < 2)
            {
                fighter[target_fighter_index].sts[S_STRENGTH]++;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_SHELL:
            if (fighter[target_fighter_index].sts[S_RESIST] == 0)
            {
                fighter[target_fighter_index].sts[S_RESIST] = 1;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_WALL:
            if (fighter[target_fighter_index].sts[S_RESIST] != 2)
            {
                fighter[target_fighter_index].sts[S_RESIST] = 2;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_SHIELD:
            if (fighter[target_fighter_index].sts[S_SHIELD] == 0)
            {
                fighter[target_fighter_index].sts[S_SHIELD] = 1;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_HASTEN:
            if (fighter[target_fighter_index].sts[S_TIME] != 2)
            {
                if (fighter[target_fighter_index].sts[S_TIME] == 1)
                {
                    fighter[target_fighter_index].sts[S_TIME] = 0;
                }
                else
                {
                    fighter[target_fighter_index].sts[S_TIME] = 2;
                }
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
    }
}



/*! \brief Heal only one ally
 *
 * This is for a special category of spells which are beneficial, but
 * not really effect spells or curative spells.
 *
 * \param   caster_fighter_index Caster
 * \param   target_fighter_index Target
 * \param   spell_number Spell number
 */
static void heal_one_ally(size_t caster_fighter_index, size_t target_fighter_index, size_t spell_number)
{
    size_t stat_index;

    /*  DS: Because these lines, sometimes when you cast restore or others */
    /*      spells, the spell don't work correctly. In cast_spell() this */
    /*      is tested, so don't need to test again. */
#if 0
    if (rand() % 100 + 1 > fighter[caster_fighter_index].stats[A_AUR + magic[spell_number].stat])
    {
        ta[target_fighter_index] = MISS;
        return;
    }
#endif

    /*  DS: Now the 'caster_fighter_index' argument isn't used, so I'm doing this: */
    caster_fighter_index = caster_fighter_index;
    switch (spell_number)
    {
        case M_RESTORE:
            if (fighter[target_fighter_index].sts[S_DEAD] == 0)
            {
                fighter[target_fighter_index].sts[S_POISON] = 0;
                fighter[target_fighter_index].sts[S_BLIND] = 0;
            }
            break;
        case M_RECOVERY:
            if (fighter[target_fighter_index].sts[S_DEAD] == 0)
            {
                for (stat_index = 0; stat_index < 7; stat_index++)
                {
                    fighter[target_fighter_index].sts[stat_index] = 0;
                }
            }
            break;
        case M_LIFE:
            if (fighter[target_fighter_index].sts[S_DEAD] == 1)
            {
                for (stat_index = 0; stat_index < 24; stat_index++)
                {
                    fighter[target_fighter_index].sts[stat_index] = 0;
                }
                fighter[target_fighter_index].hp = 1;
                fighter[target_fighter_index].aframe = 0;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
        case M_FULLLIFE:
            if (fighter[target_fighter_index].sts[S_DEAD] == 1)
            {
                for (stat_index = 0; stat_index < 24; stat_index++)
                {
                    fighter[target_fighter_index].sts[stat_index] = 0;
                }
                fighter[target_fighter_index].hp = fighter[target_fighter_index].mhp;
                fighter[target_fighter_index].aframe = 0;
            }
            else
            {
                ta[target_fighter_index] = MISS;
            }
            break;
    }
}



/*! \brief Returns MP needed for a spell
 *
 * This returns the amount of mp needed to cast a spell.  This
 * function was created to allow for different mp consumption rates.
 * \note this is the only place that mrp is used.
 *
 * \param   fighter_index Index of caster
 * \param   spell_number Spell number
 * \returns needed MP or 0 if insufficient MP
 */
int mp_needed(size_t fighter_index, int spell_number)
{
    int amt;

    if (spell_number > 0)
    {
        amt = magic[spell_number].mpc * fighter[fighter_index].mrp / 100;
        if (amt < 1)
        {
            amt = 1;
        }
        return amt;
    }
    else
    {
        return 0;
    }
}



/*! \brief Status changes
 *
 * This used to be fancier... but now this is basically used
 * to test for status changes or other junk.
 *
 * \param   tgt Target
 * \param   per Damage percent inflicted (?)
 * \returns 0 if damage taken, 1 otherwise (or vise-versa?)
 */
int non_dmg_save(int tgt, int per)
{

    /*  RB TODO:  */
    tgt = tgt;
    if (per == 0)
    {
        return 1;
    }
    if (rand() % 100 < per)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}



/*! \brief Adjust the resistance to elements
 *
 * This adjusts the passed damage amount based on the target's
 * resistance to the passed element.  The adjusted value is
 * then returned.
 *
 * \param   target_fighter_index Target
 * \param   rune_index Rune/element
 * \param   amt Amount of resistence to given rune
 * \returns difference of resistance to damage given by rune
 */
int res_adjust(size_t target_fighter_index, size_t rune_index, int amt)
{
    int ad, b;
    s_fighter tf;
    char current_res;

    if (rune_index >= R_TOTAL_RES)
    {
        return amt;
    }
    ad = amt;
    tf = status_adjust(target_fighter_index);
    current_res = tf.res[rune_index];
    if (current_res < 0)
    {
        b = 10 + abs(current_res);
        ad = ad * b / 10;
    }
    else if (current_res > 10)
    {
        b = (current_res - 10) * ad;
        ad = -1 * (b / 10);
    }
    else if (current_res >= 1 && current_res <= 10)
    {
        ad -= ad * current_res / 10;
    }
    return ad;
}



/*! \brief See if resistance is effective
 *
 * This is a simple yes or no answer to an elemental/special
 * resistance check.
 *
 * \param   tgt Target
 * \param   rs Rune/spell used
 * \returns 0 if not resistant, 1 otherwise
 */
int res_throw(int tgt, int rs)
{
    s_fighter tf;

    if (rs > R_TIME || rs < R_EARTH)
    {
        return 0;
    }
    tf = status_adjust(tgt);
    if (tf.res[rs] < 1)
    {
        return 0;
    }
    if (tf.res[rs] >= 10)
    {
        return 1;
    }
    if (rand() % 10 < tf.res[rs])
    {
        return 1;
    }
    return 0;
}



/*! \brief Set counter for effects
 *
 * This is used to set things like poison and regen
 * which activate based on the combat timer.
 *
 * \param   fighter_index Index of character affected
 * \param   ss Which stat is being affected
 */
static void set_timed_sts_effect(size_t fighter_index, int ss)
{
    fighter[fighter_index].sts[ss] = rcount + 1;
}



/*! \brief Special damage on one or all enemies
 *
 * This is for skills and items that cause damage, but don't duplicate spells.
 * Essentially, this is only used for things where the user's magic power
 * doesn't affect the power of the effect.
 *
 * \param   caster_index: Caster
 * \param   spell_dmg: Damage that a spell does
 * \param   rune_type: Rune used
 * \param   target_index: Target
 * \param   split: Total damage, split among targets
 */
void special_damage_oneall_enemies(size_t caster_index, int spell_dmg, int rune_type, size_t target_index, int split)
{
    int b = 0, average_damage = 1, multiple_targets = 0, number_of_enemies = 0;
    size_t first_target, last_target;
    size_t fighter_index;

    if (target_index == SEL_ALL_ENEMIES)
    {
        if (caster_index < PSIZE)
        {
            /* Enemies are the monsters; you are attacking */
            first_target = PSIZE;
            last_target = num_enemies;
            for (fighter_index = PSIZE; fighter_index < PSIZE + num_enemies; fighter_index++)
            {
                if (fighter[fighter_index].sts[S_DEAD] == 0)
                {
                    number_of_enemies++;
                }
            }
        }
        else
        {
            /* Enemies are your party members; monsters are attacking */
            first_target = 0;
            last_target = numchrs;
            for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
            {
                if (fighter[fighter_index].sts[S_DEAD] == 0)
                {
                    number_of_enemies++;
                }
            }
        }
        multiple_targets = 1;
    }
    else
    {
        first_target = target_index;
        number_of_enemies = 1;
        last_target = 1;
    }

    if (number_of_enemies == 0)
    {
        return;
    }

    if (spell_dmg < DMG_RND_MIN * 5)
    {
        average_damage = rand() % DMG_RND_MIN + spell_dmg;
    }
    else
    {
        average_damage = rand() % (spell_dmg / 5) + spell_dmg;
    }

    if (number_of_enemies > 1 && split == 0)
    {
        average_damage = average_damage / number_of_enemies;
    }

    for (fighter_index = first_target; fighter_index < first_target + last_target; fighter_index++)
    {
        if (fighter[fighter_index].sts[S_DEAD] == 0 && fighter[fighter_index].mhp > 0)
        {
            tempd = status_adjust(fighter_index);
            b = do_shell_check(fighter_index, average_damage);
            b -= tempd.stats[A_MAG];
            if (b < 0)
            {
                b = 0;
            }
            b = res_adjust(fighter_index, rune_type, b);
            if (fighter[fighter_index].sts[S_STONE] > 0
                && rune_type != R_BLACK
                && rune_type != R_WHITE
                && rune_type != R_EARTH
                && rune_type != R_WATER)
            {
                b /= 10;
            }
            ta[fighter_index] = 0 - b;
            if (b < 0 && rune_type == R_POISON)
            {
                if (!res_throw(fighter_index, rune_type) && !non_dmg_save(fighter_index, 75))
                {
                    set_timed_sts_effect(fighter_index, S_POISON);
                }
            }
            if (ta[fighter_index] != 0)
            {
                fighter[fighter_index].sts[S_SLEEP] = 0;
            }
        }
        else
        {
            ta[fighter_index] = 0;
        }
    }
    display_amount(first_target, FDECIDE, multiple_targets);
    for (fighter_index = first_target; fighter_index < first_target + last_target; fighter_index++)
    {
        if (ta[fighter_index] != MISS)
        {
            adjust_hp(fighter_index, ta[fighter_index]);
        }
    }
    b = 0;
    for (fighter_index = first_target; fighter_index < first_target + last_target; fighter_index++)
    {
        if (fighter[fighter_index].sts[S_DEAD] == 0 && fighter[fighter_index].hp <= 0)
        {
            fkill(fighter_index);
            ta[fighter_index] = 1;
            b++;
        }
        else
        {
            ta[fighter_index] = 0;
        }
    }
    if (b > 0)
    {
        death_animation(first_target, multiple_targets);
    }
}



/*! \brief Special spell handling
 *
 * Special spells like warp and vision.
 *
 * \param   caster_fighter_index Index of Caster
 * \param   spell_number Index of spell
 */
static void special_spells(size_t caster_fighter_index, size_t spell_number)
{
    if (caster_fighter_index >= PSIZE)
    {
        sprintf(strbuf, _("Enemy %d tried to cast %s?!"), (int)caster_fighter_index, magic[spell_number].name);
        klog(strbuf);
    }
    switch (spell_number)
    {
        case M_VISION:
            do_transition(TRANS_FADE_OUT, 2);
            vspell = 1;
            battle_render(0, 0, 0);
            blit2screen(0, 0);
            do_transition(TRANS_FADE_IN, 2);
            break;
        case M_WARP:
            if (in_combat == 1)
            {
                do_transition(TRANS_FADE_OUT, 2);
                revert_equipstats();
                drawmap();
                blit2screen(xofs, yofs);
                do_transition(TRANS_FADE_IN, 2);
                combatend = 2;
            }
            else
            {
                if (curmap == "main")
                {
                    /* TT: I would like to have a check here: if the player casts Warp,
                     * the player can select WHERE to warp to, instead of just to the
                     * house, etc.
                     */

                    change_mapm("town4", "warp", 0, 0);
                }
                else
                {
                    change_map("main", g_map.warpx, g_map.warpy, g_map.warpx, g_map.warpy);
                }
            }
            break;
        case M_REPULSE:
            save_spells[P_REPULSE] = 150;
            break;
    }
}



/*! \brief Damage done from spells
 *
 * This function does all of the damage calculating for damage
 * spells, and fills the ta[] array with the damage amounts.
 *
 * \param   caster_fighter_index Caster
 * \param   spell_number Spell number
 * \param   start_fighter_index Starting target
 * \param   end_fighter_index Ending target
 */
static void spell_damage(size_t caster_fighter_index, int spell_number, size_t start_fighter_index, size_t end_fighter_index)
{
    int a = 0, b = 0, ad = 0, rt = 0, ne = 0;
    size_t fighter_index = 0;

    if (end_fighter_index > 1)
    {
        if (caster_fighter_index < PSIZE)
        {
            for (fighter_index = PSIZE; fighter_index < PSIZE + num_enemies; fighter_index++)
            {
                if (fighter[fighter_index].sts[S_DEAD] == 0)
                {
                    ne++;
                }
            }
        }
        else
        {
            for (fighter_index = 0; fighter_index < numchrs; fighter_index++)
            {
                if (fighter[fighter_index].sts[S_DEAD] == 0)
                {
                    ne++;
                }
            }
        }
    }
    else
    {
        ne = 1;
    }
    if (ne == 0)
    {
        return;
    }
    rt = magic[spell_number].elem;
    ad =
        magic[spell_number].dmg +
        (fighter[caster_fighter_index].stats[A_INT + magic[spell_number].stat] *
         magic[spell_number].bon / 100);
    if (ad < DMG_RND_MIN * 5)
    {
        ad += rand() % DMG_RND_MIN;
    }
    else
    {
        ad += rand() % (ad / 5);
    }
    if (ad < 1)
    {
        ad = 1;
    }
    a = fighter[caster_fighter_index].stats[A_AUR + magic[spell_number].stat];
    ad = ad * a / 100;
    if (ad < 0)
    {
        ad = 0;
    }
    if (ne > 1 && magic[spell_number].tgt != TGT_ENEMY_ALL)
    {
        ad = ad / ne;
    }
    for (fighter_index = start_fighter_index; fighter_index < start_fighter_index + end_fighter_index; fighter_index++)
    {
        if (fighter[fighter_index].sts[S_DEAD] == 0 && fighter[fighter_index].mhp > 0)
        {
            tempd = status_adjust(fighter_index);
            b = do_shell_check(fighter_index, ad);
            b -= tempd.stats[A_MAG];
            if (b < 0)
            {
                b = 0;
            }
            b = res_adjust(fighter_index, rt, b);
            if (fighter[fighter_index].sts[S_STONE] > 0
                && rt != R_BLACK
                && rt != R_WHITE
                && rt != R_EARTH
                && rt != R_WATER)
            {
                b = b / 10;
            }
            ta[fighter_index] = 0 - b;
            if (b < 0 && rt == R_POISON)
            {
                if (!res_throw(fighter_index, rt) && !non_dmg_save(fighter_index, magic[spell_number].hit))
                {
                    set_timed_sts_effect(fighter_index, S_POISON);
                }
            }
            if (ta[fighter_index] != 0)
            {
                fighter[fighter_index].sts[S_SLEEP] = 0;
            }
        }
        else
        {
            ta[fighter_index] = 0;
        }
    }
}



/*! \brief Adjusts stats with spells
 *
 * This adjusts a fighter's stats by applying the effects of
 * status-affecting spells.
 *
 * \returns a struct by value (PH: a good thing???)
 */
s_fighter status_adjust(size_t fighter_index)
{
    s_fighter tf;

    tf = fighter[fighter_index];
    if (tf.sts[S_STRENGTH] > 0)
    {
        tf.stats[A_ATT] += tf.stats[A_STR] * tf.sts[S_STRENGTH] * 50 / 100;
    }
    if (tf.sts[S_MALISON] == 1)
    {
        tf.stats[A_HIT] = tf.stats[A_HIT] * 75 / 100;
        tf.stats[A_EVD] = tf.stats[A_EVD] * 75 / 100;
    }
    if (tf.sts[S_MALISON] == 2)
    {
        tf.stats[A_HIT] = tf.stats[A_HIT] * 50 / 100;
        tf.stats[A_EVD] = tf.stats[A_EVD] * 50 / 100;
    }
    if (tf.sts[S_BLESS] > 0)
    {
        tf.stats[A_HIT] += tf.sts[S_BLESS] * 25;
        tf.stats[A_EVD] += tf.sts[S_BLESS] * 10;
    }
    if (tf.sts[S_TIME] == 1)
    {
        tf.stats[A_SPD] = tf.stats[A_SPD] * 5 / 10;
        tf.stats[A_HIT] = tf.stats[A_HIT] * 75 / 100;
        tf.stats[A_EVD] = tf.stats[A_EVD] * 75 / 100;
    }
    if (tf.sts[S_TIME] == 2)
    {
        tf.stats[A_SPD] = tf.stats[A_SPD] * 15 / 10;
        tf.stats[A_HIT] = tf.stats[A_HIT] * 15 / 10;
        tf.stats[A_EVD] = tf.stats[A_EVD] * 15 / 10;
    }
    if (tf.sts[S_TIME] == 3)
    {
        tf.stats[A_SPD] = tf.stats[A_SPD] * 2;
        tf.stats[A_HIT] = tf.stats[A_HIT] * 2;
        tf.stats[A_EVD] = tf.stats[A_EVD] * 2;
    }
    if (tf.sts[S_BLIND] > 0)
    {
        tf.stats[A_HIT] /= 4;
        if (tf.stats[A_HIT] < 1)
        {
            tf.stats[A_HIT] = 1;
        }
        tf.stats[A_EVD] /= 4;
        if (tf.stats[A_EVD] < 1)
        {
            tf.stats[A_EVD] = 1;
        }
    }
    if (tf.sts[S_SLEEP] > 0 || tf.sts[S_STOP] > 0)
    {
        tf.stats[A_EVD] = 0;
    }
    if (tf.sts[S_STONE] > 0)
    {
        tf.stats[A_DEF] *= 2;
        tf.stats[A_EVD] = 0;
    }
    return tf;
}

