/* source/save.c: save and restore games and monster memory info

   Copyright (C) 1989-2008 James E. Wilson, Robert A. Koeneke,
                           David J. Grabiner

   This file is part of Umoria.

   Umoria is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Umoria is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Umoria.  If not, see <http://www.gnu.org/licenses/>. */

#include "config.h"
#include "types.h"

#ifdef _MSC_VER
  #include  <io.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* This must be included after fcntl.h, which has a prototype for `open'
   on some systems.  Otherwise, the `open' prototype conflicts with the
   `topen' declaration.  */
#include "externs.h"

static bool   sv_write(FILE *);
static bool   ver_lt(const int8u, const int8u, const int8u,
                     const int8u, const int8u, const int8u);
static bool   ver_ge(const int8u, const int8u, const int8u,
                     const int8u, const int8u, const int8u);
static void   wr_int8u(FILE *, const int8u);
static void   wr_int16u(FILE *, const int16u);
static void   wr_int32u(FILE *, const int32u);
static void   wrn_int8u(FILE *, int8u const *const, const int);
static void   wr_string(FILE *, char const *const);
static void   wrn_int16u(FILE *, int16u const *const, const int);
static void   wr_item(FILE *, inven_type const *const);
static void   wr_monster(FILE *, monster_type const *const);
static int8u  rd_int8u(FILE *);
static int16u rd_int16u(FILE *);
static int32u rd_int32u(FILE *);
static void   rdn_int8u(FILE *, int8u *const, const int);
static void   rd_string(FILE *, char *const);
static void   rdn_int16u(FILE *, int16u *const, const int);
static void   rd_item(FILE *, inven_type *const);
static void   rd_monster(FILE *, monster_type *const);

/* these are used for the save file, to avoid having to pass them to every
   procedure */
static int8u xor_byte;
static int from_savefile;  /* can overwrite old savefile when save */
static int32u start_time;  /* time that play started */

/* This save package was brought to you by -JWT- and -RAK-
   and has been completely rewritten for UNIX by -JEW-
   and has been completely rewritten again by -CJS-
   and completely rewritten again! for portability by -JEW-
   and cleaned up / modernized for the 21st century by -BS-
*/

static bool sv_write(FILE* file_ptr)
{
  int32u int32u_tmp;
  register int i, j;
  int count;
  int8u char_tmp, prev_char;
  register cave_type *c_ptr;
  register recall_type *r_ptr;
  struct stats *s_ptr;
  register struct flags *f_ptr;
  store_type *st_ptr;
  struct misc *m_ptr;
#ifdef MSDOS
  int8u *tchar_ptr;
#endif

  if (file_ptr == NULL) return false;

  /* write monster memory */
  for (i = 0; i < MAX_CREATURES; i++)
  {
    r_ptr = &c_recall[i];
    if (r_ptr->r_cmove || r_ptr->r_cdefense || r_ptr->r_kills ||
        r_ptr->r_spells || r_ptr->r_deaths || r_ptr->r_attacks[0] ||
        r_ptr->r_attacks[1] || r_ptr->r_attacks[2] || r_ptr->r_attacks[3])
    {
      wr_int16u(file_ptr, (int16u)i);
      wr_int32u(file_ptr, r_ptr->r_cmove);
      wr_int32u(file_ptr, r_ptr->r_spells);
      wr_int16u(file_ptr, r_ptr->r_kills);
      wr_int16u(file_ptr, r_ptr->r_deaths);
      wr_int16u(file_ptr, r_ptr->r_cdefense);
      wr_int8u (file_ptr, r_ptr->r_wake);
      wr_int8u (file_ptr, r_ptr->r_ignore);
      wrn_int8u(file_ptr, r_ptr->r_attacks, MAX_MON_NATTACK);
    }
  }
  wr_int16u(file_ptr, (int16u)0xFFFF); /* sentinel to indicate no more monster info */

  /* write in-game settings and other flags */
  /* clear the death flag when creating a HANGUP save file, so that player
     can see tombstone when restart */
  if (eof_flag) death = false;

  int32u_tmp = 0;
  if (find_cut)            int32u_tmp |= SV_FIND_CUT;
  if (find_examine)        int32u_tmp |= SV_FIND_EXAMINE;
  if (find_prself)         int32u_tmp |= SV_FIND_PRSELF;
  if (find_bound)          int32u_tmp |= SV_FIND_BOUND;
  if (prompt_carry_flag)   int32u_tmp |= SV_PROMPT_CARRY_FLAG;
  if (rogue_like_commands) int32u_tmp |= SV_ROGUE_LIKE_COMMANDS;
  if (show_weight_flag)    int32u_tmp |= SV_SHOW_WEIGHT_FLAG;
  if (highlight_seams)     int32u_tmp |= SV_HIGHLIGHT_SEAMS;
  if (find_ignore_doors)   int32u_tmp |= SV_FIND_IGNORE_DOORS;
  if (sound_beep_flag)     int32u_tmp |= SV_SOUND_BEEP_FLAG;
  if (display_counts)      int32u_tmp |= SV_DISPLAY_COUNTS;
  if (total_winner)        int32u_tmp |= SV_TOTAL_WINNER;
  if (death)               int32u_tmp |= SV_DEATH;

  wr_int32u(file_ptr, int32u_tmp);

  /* write player data */
  m_ptr = &py.misc;
  wr_string(file_ptr, m_ptr->name);
  wr_int8u (file_ptr, m_ptr->male);
  wr_int32u(file_ptr, (int32u)m_ptr->au);
  wr_int32u(file_ptr, (int32u)m_ptr->max_exp);
  wr_int32u(file_ptr, (int32u)m_ptr->exp);
  wr_int16u(file_ptr, m_ptr->exp_frac);
  wr_int16u(file_ptr, m_ptr->age);
  wr_int16u(file_ptr, m_ptr->ht);
  wr_int16u(file_ptr, m_ptr->wt);
  wr_int16u(file_ptr, m_ptr->lev);
  wr_int16u(file_ptr, m_ptr->max_dlv);
  wr_int16u(file_ptr, (int16u)m_ptr->srh);
  wr_int16u(file_ptr, (int16u)m_ptr->fos);
  wr_int16u(file_ptr, (int16u)m_ptr->bth);
  wr_int16u(file_ptr, (int16u)m_ptr->bthb);
  wr_int16u(file_ptr, (int16u)m_ptr->mana);
  wr_int16u(file_ptr, (int16u)m_ptr->mhp);
  wr_int16u(file_ptr, (int16u)m_ptr->ptohit);
  wr_int16u(file_ptr, (int16u)m_ptr->ptodam);
  wr_int16u(file_ptr, (int16u)m_ptr->pac);
  wr_int16u(file_ptr, (int16u)m_ptr->ptoac);
  wr_int16u(file_ptr, (int16u)m_ptr->dis_th);
  wr_int16u(file_ptr, (int16u)m_ptr->dis_td);
  wr_int16u(file_ptr, (int16u)m_ptr->dis_ac);
  wr_int16u(file_ptr, (int16u)m_ptr->dis_tac);
  wr_int16u(file_ptr, (int16u)m_ptr->disarm);
  wr_int16u(file_ptr, (int16u)m_ptr->save);
  wr_int16u(file_ptr, (int16u)m_ptr->sc);
  wr_int16u(file_ptr, (int16u)m_ptr->stl);
  wr_int8u (file_ptr, m_ptr->pclass);
  wr_int8u (file_ptr, m_ptr->prace);
  wr_int8u (file_ptr, m_ptr->hitdie);
  wr_int8u (file_ptr, m_ptr->expfact);
  wr_int16u(file_ptr, (int16u)m_ptr->cmana);
  wr_int16u(file_ptr, m_ptr->cmana_frac);
  wr_int16u(file_ptr, (int16u)m_ptr->chp);
  wr_int16u(file_ptr, m_ptr->chp_frac);
  for (i = 0; i < 4; i++) wr_string(file_ptr, m_ptr->history[i]);

  s_ptr = &py.stats;
  wrn_int8u (file_ptr, s_ptr->max_stat, 6);
  wrn_int8u (file_ptr, s_ptr->cur_stat, 6);
  wrn_int16u(file_ptr, (int16u *)s_ptr->mod_stat, 6);
  wrn_int8u (file_ptr, s_ptr->use_stat, 6);

  f_ptr = &py.flags;
  wr_int32u(file_ptr, f_ptr->status);
  wr_int16u(file_ptr, (int16u)f_ptr->rest);
  wr_int16u(file_ptr, (int16u)f_ptr->blind);
  wr_int16u(file_ptr, (int16u)f_ptr->paralysis);
  wr_int16u(file_ptr, (int16u)f_ptr->confused);
  wr_int16u(file_ptr, (int16u)f_ptr->food);
  wr_int16u(file_ptr, (int16u)f_ptr->food_digested);
  wr_int16u(file_ptr, (int16u)f_ptr->protection);
  wr_int16u(file_ptr, (int16u)f_ptr->speed);
  wr_int16u(file_ptr, (int16u)f_ptr->fast);
  wr_int16u(file_ptr, (int16u)f_ptr->slow);
  wr_int16u(file_ptr, (int16u)f_ptr->afraid);
  wr_int16u(file_ptr, (int16u)f_ptr->poisoned);
  wr_int16u(file_ptr, (int16u)f_ptr->image);
  wr_int16u(file_ptr, (int16u)f_ptr->protevil);
  wr_int16u(file_ptr, (int16u)f_ptr->invuln);
  wr_int16u(file_ptr, (int16u)f_ptr->hero);
  wr_int16u(file_ptr, (int16u)f_ptr->shero);
  wr_int16u(file_ptr, (int16u)f_ptr->blessed);
  wr_int16u(file_ptr, (int16u)f_ptr->resist_heat);
  wr_int16u(file_ptr, (int16u)f_ptr->resist_cold);
  wr_int16u(file_ptr, (int16u)f_ptr->detect_inv);
  wr_int16u(file_ptr, (int16u)f_ptr->word_recall);
  wr_int16u(file_ptr, (int16u)f_ptr->see_infra);
  wr_int16u(file_ptr, (int16u)f_ptr->tim_infra);
  wr_int8u (file_ptr, f_ptr->see_inv);
  wr_int8u (file_ptr, f_ptr->teleport);
  wr_int8u (file_ptr, f_ptr->free_act);
  wr_int8u (file_ptr, f_ptr->slow_digest);
  wr_int8u (file_ptr, f_ptr->aggravate);
  wr_int8u (file_ptr, f_ptr->fire_resist);
  wr_int8u (file_ptr, f_ptr->cold_resist);
  wr_int8u (file_ptr, f_ptr->acid_resist);
  wr_int8u (file_ptr, f_ptr->regenerate);
  wr_int8u (file_ptr, f_ptr->lght_resist);
  wr_int8u (file_ptr, f_ptr->ffall);
  wr_int8u (file_ptr, f_ptr->sustain_str);
  wr_int8u (file_ptr, f_ptr->sustain_int);
  wr_int8u (file_ptr, f_ptr->sustain_wis);
  wr_int8u (file_ptr, f_ptr->sustain_con);
  wr_int8u (file_ptr, f_ptr->sustain_dex);
  wr_int8u (file_ptr, f_ptr->sustain_chr);
  wr_int8u (file_ptr, f_ptr->confuse_monster);
  wr_int8u (file_ptr, f_ptr->new_spells);

  /* write game state */
  wr_int16u(file_ptr, (int16u)missile_ctr);
  wr_int32u(file_ptr, (int32u)turn);
  wr_int16u(file_ptr, (int16u)inven_ctr);
  for (i = 0; i < inven_ctr; i++) wr_item(file_ptr, &inventory[i]);
  for (i = INVEN_WIELD; i < INVEN_ARRAY_SIZE; i++)
    wr_item(file_ptr, &inventory[i]);
  wr_int16u(file_ptr, (int16u)inven_weight);
  wr_int16u(file_ptr, (int16u)equip_ctr);
  wr_int32u(file_ptr, spell_learned);
  wr_int32u(file_ptr, spell_worked);
  wr_int32u(file_ptr, spell_forgotten);
  wrn_int8u(file_ptr, spell_order, 32);
  wrn_int8u(file_ptr, object_ident, OBJECT_IDENT_SIZE);
  wr_int32u(file_ptr, randes_seed);
  wr_int32u(file_ptr, town_seed);
  wr_int16u(file_ptr, (int16u)last_msg);
  for (i = 0; i < MAX_SAVE_MSG; i++) wr_string(file_ptr, old_msg[i]);

  /* this indicates 'cheating' if it is a one */
  wr_int16u (file_ptr, (int16u)panic_save);
  wr_int16u (file_ptr, (int16u)total_winner);
  wr_int16u (file_ptr, (int16u)noscore);
  wrn_int16u(file_ptr, player_hp, MAX_PLAYER_LEVEL);

  /* write town store data */
  for (i = 0; i < MAX_STORES; i++)
  {
    st_ptr = &store[i];
    wr_int32u(file_ptr, (int32u)st_ptr->store_open);
    wr_int16u(file_ptr, (int16u)st_ptr->insult_cur);
    wr_int8u (file_ptr, st_ptr->owner);
    wr_int8u (file_ptr, st_ptr->store_ctr);
    wr_int16u(file_ptr, st_ptr->good_buy);
    wr_int16u(file_ptr, st_ptr->bad_buy);
    for (j = 0; j < st_ptr->store_ctr; j++)
    {
      wr_int32u(file_ptr, (int32u)st_ptr->store_inven[j].scost);
      wr_item(file_ptr, &st_ptr->store_inven[j].sitem);
    }
  }

  /* save the current time in the savefile */
  int32u_tmp = time(NULL);
  if (int32u_tmp < start_time)
  {
    /* someone is messing with the clock!
       assume that we have been playing for 1 day */
    int32u_tmp = start_time + 86400L;
  }
  wr_int32u(file_ptr, int32u_tmp);

  /* starting with 5.2, put died_from string in savefile */
  wr_string(file_ptr, died_from);

  /* starting with 5.2.2, put the max_score in the savefile */
  wr_int32u(file_ptr, (int32u)total_points());

  /* starting with 5.2.2, put the birth_date in the savefile */
  wr_int32u(file_ptr, (int32u)birth_date);

  /* only level specific info follows, this allows characters to be
     resurrected, the dungeon level info is not needed for a resurrection */
  if (death)
  {
    return (!ferror(file_ptr) && fflush(file_ptr) != EOF);
  }

  /* write dungeon data */
  wr_int16u(file_ptr, (int16u)dun_level);
  wr_int16u(file_ptr, (int16u)char_row);
  wr_int16u(file_ptr, (int16u)char_col);
  wr_int16u(file_ptr, (int16u)mon_tot_mult);
  wr_int16u(file_ptr, (int16u)cur_height);
  wr_int16u(file_ptr, (int16u)cur_width);
  wr_int16u(file_ptr, (int16u)max_panel_rows);
  wr_int16u(file_ptr, (int16u)max_panel_cols);

  for (i = 0; i < MAX_HEIGHT; i++)
  {
    for (j = 0; j < MAX_WIDTH; j++)
    {
      c_ptr = &cave[i][j];
      if (!(c_ptr->cptr)) continue;
      wr_int8u(file_ptr, (int8u)i);
      wr_int8u(file_ptr, (int8u)j);
      wr_int8u(file_ptr, c_ptr->cptr);
    }
  }
  wr_int8u(file_ptr, (int8u)0xFF); /* marks end of cptr info */
  for (i = 0; i < MAX_HEIGHT; i++)
  {
    for (j = 0; j < MAX_WIDTH; j++)
    {
      c_ptr = &cave[i][j];
      if (!(c_ptr->tptr)) continue;
      wr_int8u(file_ptr, (int8u)i);
      wr_int8u(file_ptr, (int8u)j);
      wr_int8u(file_ptr, c_ptr->tptr);
    }
  }
  wr_int8u(file_ptr, (int8u)0xFF); /* marks end of tptr info */
  /* must set counter to zero, note that code may write out two bytes
     unnecessarily */
  count = 0;
  prev_char = 0;
  for (i = 0; i < MAX_HEIGHT; i++)
  {
    for (j = 0; j < MAX_WIDTH; j++)
    {
      c_ptr = &cave[i][j];
      char_tmp = c_ptr->fval
              | (c_ptr->lr << 4)
              | (c_ptr->fm << 5)
              | (c_ptr->pl << 6)
              | (c_ptr->tl << 7);
      if (char_tmp != prev_char || count == MAX_UCHAR)
      {
        wr_int8u(file_ptr, (int8u)count);
        wr_int8u(file_ptr, prev_char);
        prev_char = char_tmp;
        count = 1;
      }
      else
      {
        count++;
      }
    }
  }
  /* save last entry */
  wr_int8u(file_ptr, (int8u)count);
  wr_int8u(file_ptr, prev_char);

#ifdef MSDOS
  /* must change graphics symbols for walls and floors back to default chars,
     this is necessary so that if the user changes the graphics line, the
     program will be able change all existing walls and floors to the new
     symbol */
  /* Or if the user moves the savefile from one machine to another, we
     must have a consistent representation here.  */
  for (i = MIN_TRIX; i < tcptr; i++)
  {
    tchar_ptr = &(t_list[i].tchar);
    if (*tchar_ptr == wallsym) *tchar_ptr = '#';
  }
#endif
  wr_int16u(file_ptr, (int16u)tcptr);
  for (i = MIN_TRIX; i < tcptr; i++) wr_item(file_ptr, &t_list[i]);
  wr_int16u(file_ptr, (int16u)mfptr);
  for (i = MIN_MONIX; i < mfptr; i++) wr_monster(file_ptr, &m_list[i]);

  return (!ferror(file_ptr) && fflush(file_ptr) != EOF);
}

int save_char()
{
  vtype temp;
  /* attempt to save until success or player abort */
  while (!_save_char(savefile))
  {
    sprintf(temp, "Save file '%s' create/overwrite failed.", savefile);
    msg_print(temp);
    /* this used to let the user try to delete the file, but that's
       potentially dangerous, and they can just tab out to do it in modern
       operating systems. The code was also a bit mind-bending. That's all
       been removed, and the previously undocumented option to retry
       without rename is now explicitly mentioned to the user. -BS- */
    prt("New filename [none=retry/ESC=fail]:", 0, 0);
    if (!get_string(temp, 0, 35, 41)) return false;
    if (temp[0]) strcpy(savefile, temp);
    sprintf(temp, "Okay, retrying save with file '%s'...", savefile);
    prt(temp, 0, 0);
  }
  return true;
}

int _save_char(char *fnam)
{
  FILE* file_ptr = NULL;
  vtype temp;
  bool ok = false, file_created = false;
  int8u char_tmp;

  if (character_saved) return true; /* Nothing to save. */

  nosignals();
  put_qio();
  disturb(1, 0);                    /* Turn off resting and searching. */
  change_speed(-pack_heavy);        /* Fix the speed */
  pack_heavy = 0;

  /* legacy implementation used POSIX-heavy calls to do the following:
     - attempt to create new file, failing if already exists
     - if already exists, and either from_savefile is true or
       wizard && get_check("Save file already exists. Overwrite?") returns
       true, set read+write and attempt to open read+write+truncate
     - if file created/truncated, close POSIX file descriptor and attempt to
       open an ANSI read-write file handle

     new implementation accomplishes the same thing, but with less code and
     no messy POSIX open() calls:
     - if does not exist, OR it does AND it's okay to overwrite, try to
       fopen() as wb+ (truncate + read/write)  -BS- */
  if (access(fnam, F_OK) == -1 ||
      from_savefile ||
      (wizard && get_check("Save file already exists. Overwrite?")))
  {
    file_ptr = fopen(savefile, "wb+");
  }

  if (file_ptr != NULL)
  {
    file_created = true;

    xor_byte = 0;
    wr_int8u(file_ptr, (int8u)CUR_VERSION_MAJ);
    xor_byte = 0;
    wr_int8u(file_ptr, (int8u)CUR_VERSION_MIN);
    xor_byte = 0;
    wr_int8u(file_ptr, (int8u)PATCH_LEVEL);
    xor_byte = 0;
    char_tmp = randint(256) - 1;
    wr_int8u(file_ptr, char_tmp);
    /* Note that xor_byte is now equal to char_tmp */

    ok = sv_write(file_ptr);
    if (fclose(file_ptr) == EOF) ok = false;
  }

  if (!ok)
  {
    /* only attempt to remove the file if we created/truncated it -BS- */
    if (file_created) unlink(fnam);
    signals();
    if (file_created) sprintf(temp, "Error writing to file %s", fnam);
    else              sprintf(temp, "Can't create new file %s", fnam);
    msg_print(temp);
    return false;
  }

  character_saved = 1;
  turn = -1;
  signals();

  return true;
}

/* true if left version is less/older than right version */
static bool ver_lt(
  const int8u lmaj, const int8u lmin, const int8u lpat,
  const int8u rmaj, const int8u rmin, const int8u rpat
)
{
  if (lmaj < rmaj) return true;  /* major version older */
  if (lmaj > rmaj) return false; /* major version newer */
  /* major version equal; check minor version */
  if (lmin < rmin) return true;  /* minor version older */
  if (lmin > rmin) return false; /* minor version newer */
  /* minor version equal; check patch level */
  if (lpat < rpat) return true;  /* patch level older */
  /* patch level same or newer */
  return false;
}

/* true if left version is greater than or equal to right version
   i.e. left is same or newer than right */
static bool ver_ge(
  const int8u lmaj, const int8u lmin, const int8u lpat,
  const int8u rmaj, const int8u rmin, const int8u rpat
)
{
  /* this is literally the opposite of ver_lt, so just use that
     it's still worth having this function for readability */
  return !ver_lt(lmaj, lmin, lpat,
                 rmaj, rmin, rpat);
}

/* Certain checks are ommitted for the wizard. -CJS- */
int get_char(int *generate)
{
  FILE* file_ptr = NULL;
  register int i, j;
  int c, ok, total_count;
  int32u int32u_tmp, age, time_saved;
  vtype temp;
  int16u int16u_tmp;
  register cave_type *c_ptr;
  register recall_type *r_ptr;
  struct misc *m_ptr;
  struct stats *s_ptr;
  register struct flags *f_ptr;
  store_type *st_ptr;
  int8u char_tmp, ychar, xchar, count;
  int8u version_maj, version_min, patch_level;
  bool dead_save, winner_save;
#ifdef MSDOS
  int8u *tchar_ptr;
#endif

  nosignals();
  *generate = true;

  if (access(savefile, F_OK) == -1)
  {
    signals();
    msg_print("Savefile does not exist.");
    return false;  /* Don't bother with messages here. File absent. */
  }

  clear_screen();

  sprintf(temp, "Savefile %s present. Attempting restore.", savefile);
  put_buffer(temp, 23, 0);

  if (turn >= 0)
  {
    msg_print("IMPOSSIBLE! Attempt to restore while still alive!");
  }
  else
  {
    turn = -1;
    ok = true;

    file_ptr = fopen(savefile, "rb");
    if (file_ptr == NULL) goto error;

    prt("Restoring Memory...", 0, 0);
    put_qio();

    xor_byte = 0;
    version_maj = rd_int8u(file_ptr);
    xor_byte = 0;
    version_min = rd_int8u(file_ptr);
    xor_byte = 0;
    patch_level = rd_int8u(file_ptr);
    xor_byte = 0;
    xor_byte = rd_int8u(file_ptr);

    /* COMPAT support savefiles from 5.0.14 to 5.0.17 */
    /* support savefiles from 5.1.0 to present */
    if (ver_lt(version_maj, version_min, patch_level, 5, 0, 14))
    {
      prt("Sorry. This savefile is from an incompatible version of umoria/hzmoria.",
          2, 0);
      goto error;
    }

    int16u_tmp = rd_int16u(file_ptr);
    while (int16u_tmp != 0xFFFF)
    {
      if (int16u_tmp >= MAX_CREATURES) goto error;
      r_ptr = &c_recall[int16u_tmp];
      r_ptr->r_cmove    = rd_int32u(file_ptr);
      r_ptr->r_spells   = rd_int32u(file_ptr);
      r_ptr->r_kills    = rd_int16u(file_ptr);
      r_ptr->r_deaths   = rd_int16u(file_ptr);
      r_ptr->r_cdefense = rd_int16u(file_ptr);
      r_ptr->r_wake     = rd_int8u (file_ptr);
      r_ptr->r_ignore   = rd_int8u (file_ptr);
      rdn_int8u(file_ptr, r_ptr->r_attacks, MAX_MON_NATTACK);
      int16u_tmp        = rd_int16u(file_ptr);
    }

    /* for save files before 5.2.2, read and ignore log_index (sic) */
    if (ver_lt(version_maj, version_min, patch_level, 5, 2, 2))
    {
      int16u_tmp = rd_int16u(file_ptr);
    }
    int32u_tmp = rd_int32u(file_ptr);

    find_cut            = !!(int32u_tmp & SV_FIND_CUT);
    find_examine        = !!(int32u_tmp & SV_FIND_EXAMINE);
    find_prself         = !!(int32u_tmp & SV_FIND_PRSELF);
    find_bound          = !!(int32u_tmp & SV_FIND_BOUND);
    prompt_carry_flag   = !!(int32u_tmp & SV_PROMPT_CARRY_FLAG);
    rogue_like_commands = !!(int32u_tmp & SV_ROGUE_LIKE_COMMANDS);
    show_weight_flag    = !!(int32u_tmp & SV_SHOW_WEIGHT_FLAG);
    highlight_seams     = !!(int32u_tmp & SV_HIGHLIGHT_SEAMS);
    find_ignore_doors   = !!(int32u_tmp & SV_FIND_IGNORE_DOORS);
    /* save files before 5.2.2 don't have sound_beep_flag, set it on
       for compatibility */
    sound_beep_flag     = !!(int32u_tmp & SV_SOUND_BEEP_FLAG) ||
                          ver_lt(version_maj, version_min, patch_level,
                                 5, 2, 2);
    /* save files before 5.2.2 don't have display_counts, set it on
       for compatibility */
    display_counts      = !!(int32u_tmp & SV_DISPLAY_COUNTS) ||
                          ver_lt(version_maj, version_min, patch_level,
                                 5, 2, 2);
    dead_save           = !!(int32u_tmp & SV_DEATH);
    winner_save         = !!(int32u_tmp & SV_TOTAL_WINNER);

    if (to_be_wizard)
    {
      /* Don't allow resurrection of total_winner characters. It causes
        problems because the character level is out of the allowed range. */
      if (winner_save)
      {
        msg_print("Sorry, this character is retired from moria.");
        msg_print("You can not resurrect a retired character.");
      }
      else if (dead_save && get_check("Resurrect a dead character?"))
      {
        dead_save = false;
      }
    }
    /* if alive, read character data */
    if (!dead_save)
    {
      m_ptr = &py.misc;
      rd_string(file_ptr, m_ptr->name);
      m_ptr->male       = rd_int8u(file_ptr);
      m_ptr->au         = (int32)rd_int32u(file_ptr);
      m_ptr->max_exp    = (int32)rd_int32u(file_ptr);
      m_ptr->exp        = (int32)rd_int32u(file_ptr);
      m_ptr->exp_frac   = rd_int16u(file_ptr);
      m_ptr->age        = rd_int16u(file_ptr);
      m_ptr->ht         = rd_int16u(file_ptr);
      m_ptr->wt         = rd_int16u(file_ptr);
      m_ptr->lev        = rd_int16u(file_ptr);
      m_ptr->max_dlv    = rd_int16u(file_ptr);
      m_ptr->srh        = (int16)rd_int16u(file_ptr);
      m_ptr->fos        = (int16)rd_int16u(file_ptr);
      m_ptr->bth        = (int16)rd_int16u(file_ptr);
      m_ptr->bthb       = (int16)rd_int16u(file_ptr);
      m_ptr->mana       = (int16)rd_int16u(file_ptr);
      m_ptr->mhp        = (int16)rd_int16u(file_ptr);
      m_ptr->ptohit     = (int16)rd_int16u(file_ptr);
      m_ptr->ptodam     = (int16)rd_int16u(file_ptr);
      m_ptr->pac        = (int16)rd_int16u(file_ptr);
      m_ptr->ptoac      = (int16)rd_int16u(file_ptr);
      m_ptr->dis_th     = (int16)rd_int16u(file_ptr);
      m_ptr->dis_td     = (int16)rd_int16u(file_ptr);
      m_ptr->dis_ac     = (int16)rd_int16u(file_ptr);
      m_ptr->dis_tac    = (int16)rd_int16u(file_ptr);
      m_ptr->disarm     = (int16)rd_int16u(file_ptr);
      m_ptr->save       = (int16)rd_int16u(file_ptr);
      m_ptr->sc         = (int16)rd_int16u(file_ptr);
      m_ptr->stl        = (int16)rd_int16u(file_ptr);
      m_ptr->pclass     = rd_int8u(file_ptr);
      m_ptr->prace      = rd_int8u(file_ptr);
      m_ptr->hitdie     = rd_int8u(file_ptr);
      m_ptr->expfact    = rd_int8u(file_ptr);
      m_ptr->cmana      = (int16)rd_int16u(file_ptr);
      m_ptr->cmana_frac = rd_int16u(file_ptr);
      m_ptr->chp        = (int16)rd_int16u(file_ptr);
      m_ptr->chp_frac   = rd_int16u(file_ptr);
      for (i = 0; i < 4; i++) rd_string(file_ptr, m_ptr->history[i]);

      s_ptr = &py.stats;
      rdn_int8u (file_ptr, s_ptr->max_stat, 6);
      rdn_int8u (file_ptr, s_ptr->cur_stat, 6);
      rdn_int16u(file_ptr, (int16u *)s_ptr->mod_stat, 6);
      rdn_int8u (file_ptr, s_ptr->use_stat, 6);

      f_ptr = &py.flags;
      f_ptr->status          = rd_int32u(file_ptr);
      f_ptr->rest            = (int16)rd_int16u(file_ptr);
      f_ptr->blind           = (int16)rd_int16u(file_ptr);
      f_ptr->paralysis       = (int16)rd_int16u(file_ptr);
      f_ptr->confused        = (int16)rd_int16u(file_ptr);
      f_ptr->food            = (int16)rd_int16u(file_ptr);
      f_ptr->food_digested   = (int16)rd_int16u(file_ptr);
      f_ptr->protection      = (int16)rd_int16u(file_ptr);
      f_ptr->speed           = (int16)rd_int16u(file_ptr);
      f_ptr->fast            = (int16)rd_int16u(file_ptr);
      f_ptr->slow            = (int16)rd_int16u(file_ptr);
      f_ptr->afraid          = (int16)rd_int16u(file_ptr);
      f_ptr->poisoned        = (int16)rd_int16u(file_ptr);
      f_ptr->image           = (int16)rd_int16u(file_ptr);
      f_ptr->protevil        = (int16)rd_int16u(file_ptr);
      f_ptr->invuln          = (int16)rd_int16u(file_ptr);
      f_ptr->hero            = (int16)rd_int16u(file_ptr);
      f_ptr->shero           = (int16)rd_int16u(file_ptr);
      f_ptr->blessed         = (int16)rd_int16u(file_ptr);
      f_ptr->resist_heat     = (int16)rd_int16u(file_ptr);
      f_ptr->resist_cold     = (int16)rd_int16u(file_ptr);
      f_ptr->detect_inv      = (int16)rd_int16u(file_ptr);
      f_ptr->word_recall     = (int16)rd_int16u(file_ptr);
      f_ptr->see_infra       = (int16)rd_int16u(file_ptr);
      f_ptr->tim_infra       = (int16)rd_int16u(file_ptr);
      f_ptr->see_inv         = rd_int8u(file_ptr);
      f_ptr->teleport        = rd_int8u(file_ptr);
      f_ptr->free_act        = rd_int8u(file_ptr);
      f_ptr->slow_digest     = rd_int8u(file_ptr);
      f_ptr->aggravate       = rd_int8u(file_ptr);
      f_ptr->fire_resist     = rd_int8u(file_ptr);
      f_ptr->cold_resist     = rd_int8u(file_ptr);
      f_ptr->acid_resist     = rd_int8u(file_ptr);
      f_ptr->regenerate      = rd_int8u(file_ptr);
      f_ptr->lght_resist     = rd_int8u(file_ptr);
      f_ptr->ffall           = rd_int8u(file_ptr);
      f_ptr->sustain_str     = rd_int8u(file_ptr);
      f_ptr->sustain_int     = rd_int8u(file_ptr);
      f_ptr->sustain_wis     = rd_int8u(file_ptr);
      f_ptr->sustain_con     = rd_int8u(file_ptr);
      f_ptr->sustain_dex     = rd_int8u(file_ptr);
      f_ptr->sustain_chr     = rd_int8u(file_ptr);
      f_ptr->confuse_monster = rd_int8u(file_ptr);
      f_ptr->new_spells      = rd_int8u(file_ptr);

      missile_ctr     = (int16)rd_int16u(file_ptr);
      turn            = (int32)rd_int32u(file_ptr);
      inven_ctr       = (int16)rd_int16u(file_ptr);
      if (inven_ctr > INVEN_WIELD) goto error;
      for (i = 0; i < inven_ctr; i++) rd_item(file_ptr, &inventory[i]);
      for (i = INVEN_WIELD; i < INVEN_ARRAY_SIZE; i++)
        rd_item(file_ptr, &inventory[i]);
      inven_weight    = (int16)rd_int16u(file_ptr);
      equip_ctr       = (int16)rd_int16u(file_ptr);
      spell_learned   = rd_int32u(file_ptr);
      spell_worked    = rd_int32u(file_ptr);
      spell_forgotten = rd_int32u(file_ptr);
      rdn_int8u(file_ptr, spell_order, 32);
      rdn_int8u(file_ptr, object_ident, OBJECT_IDENT_SIZE);
      randes_seed     = rd_int32u(file_ptr);
      town_seed       = rd_int32u(file_ptr);
      last_msg        = (int16)rd_int16u(file_ptr);
      for (i = 0; i < MAX_SAVE_MSG; i++) rd_string(file_ptr, old_msg[i]);

      int16u_tmp   = rd_int16u(file_ptr);
      panic_save   = !!int16u_tmp;
      total_winner = (int16)rd_int16u(file_ptr);
      noscore      = (int16)rd_int16u(file_ptr);
      rdn_int16u(file_ptr, player_hp, MAX_PLAYER_LEVEL);

      if (ver_ge(version_maj, version_min, patch_level, 5, 1, 3))
      {
        for (i = 0; i < MAX_STORES; i++)
        {
          st_ptr = &store[i];
          st_ptr->store_open = (int32)rd_int32u(file_ptr);
          st_ptr->insult_cur = (int16)rd_int16u(file_ptr);
          st_ptr->owner      = rd_int8u (file_ptr);
          st_ptr->store_ctr  = rd_int8u (file_ptr);
          st_ptr->good_buy   = rd_int16u(file_ptr);
          st_ptr->bad_buy    = rd_int16u(file_ptr);
          if (st_ptr->store_ctr > STORE_INVEN_MAX) goto error;
          for (j = 0; j < st_ptr->store_ctr; j++)
          {
            st_ptr->store_inven[j].scost = (int32)rd_int32u(file_ptr);
            rd_item(file_ptr, &st_ptr->store_inven[j].sitem);
          }
        }
      }

      /* read the time that the file was saved */
      if (ver_ge(version_maj, version_min, patch_level, 5, 1, 3))
        time_saved = rd_int32u(file_ptr);

      if (ver_ge(version_maj, version_min, patch_level, 5, 2, 0))
        rd_string(file_ptr, died_from);

      max_score = 0;
      if (ver_ge(version_maj, version_min, patch_level, 5, 2, 2))
        max_score = (int32)rd_int32u(file_ptr);

      birth_date = time(NULL);
      if (ver_ge(version_maj, version_min, patch_level, 5, 2, 2))
        birth_date = (int32)rd_int32u(file_ptr);
    } /* end if not dead */

    /* if no more data, or loaded a dead character */
    if ((c = getc(file_ptr)) == EOF || dead_save)
    {
      if (dead_save)
      {
        /* Make sure that this message is seen, since it is a bit
           more interesting than the other messages.  */
        msg_print("Restoring Memory of a departed spirit...");
        turn = -1;
      }
      else
      {
        if (!to_be_wizard || turn < 0) goto error;
        prt("Attempting a resurrection!", 0, 0);
        if (py.misc.chp < 0)
        {
          py.misc.chp = 0;
          py.misc.chp_frac = 0;
        }
        /* don't let them starve to death immediately */
        if (py.flags.food < 0) py.flags.food = 0;
        /* don't let them die of poison again immediately */
        if (py.flags.poisoned > 1) py.flags.poisoned = 1;
        dun_level = 0; /* Resurrect on the town level. */
        character_generated = 1;
        /* set noscore to indicate a resurrection, and don't enter
           wizard mode */
        to_be_wizard = false;
        noscore |= 0x1;
      }
      put_qio();
      goto closefiles;
    } /* end if EOF or dead */

    /* push character consumed by EOF check back into file read stream */
    if (ungetc(c, file_ptr) == EOF) goto error;

    prt("Restoring Character...", 0, 0);
    put_qio();

    /* only level specific info should follow, not present for dead
        characters */

    dun_level      = (int16)rd_int16u(file_ptr);
    char_row       = (int16)rd_int16u(file_ptr);
    char_col       = (int16)rd_int16u(file_ptr);
    mon_tot_mult   = (int16)rd_int16u(file_ptr);
    cur_height     = (int16)rd_int16u(file_ptr);
    cur_width      = (int16)rd_int16u(file_ptr);
    max_panel_rows = (int16)rd_int16u(file_ptr);
    max_panel_cols = (int16)rd_int16u(file_ptr);

    /* read in the creature ptr info */
    for (char_tmp = rd_int8u(file_ptr);
         char_tmp != 0xFF;
         char_tmp = rd_int8u(file_ptr))
    {
      ychar    = char_tmp;
      xchar    = rd_int8u(file_ptr);
      char_tmp = rd_int8u(file_ptr);
      if (xchar > MAX_WIDTH || ychar > MAX_HEIGHT) goto error;
      cave[ychar][xchar].cptr = char_tmp;
    }
    /* read in the treasure ptr info */
    for (char_tmp = rd_int8u(file_ptr);
         char_tmp != 0xFF;
         char_tmp = rd_int8u(file_ptr))
    {
      ychar    = char_tmp;
      xchar    = rd_int8u(file_ptr);
      char_tmp = rd_int8u(file_ptr);
      if (xchar > MAX_WIDTH || ychar > MAX_HEIGHT) goto error;
      cave[ychar][xchar].tptr = char_tmp;
    }
    /* read in the rest of the cave info */
    c_ptr = &cave[0][0];
    for (total_count = 0;
         total_count < MAX_HEIGHT*MAX_WIDTH;
         total_count += count)
    {
      count    = rd_int8u(file_ptr);
      char_tmp = rd_int8u(file_ptr);
      for (i = 0; i < count; ++i)
      {
        if (c_ptr >= &cave[MAX_HEIGHT][0]) goto error;
        c_ptr->fval =  char_tmp       & 0xF; /* bits 0-3 */
        c_ptr->lr   = (char_tmp >> 4) & 0x1; /* bit 4 */
        c_ptr->fm   = (char_tmp >> 5) & 0x1; /* bit 5 */
        c_ptr->pl   = (char_tmp >> 6) & 0x1; /* bit 6 */
        c_ptr->tl   = (char_tmp >> 7) & 0x1; /* bit 7 */
        c_ptr++;
      }
    }

    tcptr = (int16)rd_int16u(file_ptr);
    if (tcptr > MAX_TALLOC) goto error;
    for (i = MIN_TRIX; i < tcptr; i++) rd_item(file_ptr, &t_list[i]);
    mfptr = (int16)rd_int16u(file_ptr);
    if (mfptr > MAX_MALLOC) goto error;
    for (i = MIN_MONIX; i < mfptr; i++) rd_monster(file_ptr, &m_list[i]);

#ifdef MSDOS
    /* change walls and floors to graphic symbols */
    for (i = MIN_TRIX; i < tcptr; i++)
    {
      tchar_ptr = &(t_list[i].tchar);
    if (*tchar_ptr == '#') *tchar_ptr = wallsym;
    }
#endif

    *generate = false;  /* We have restored a cave - no need to generate. */

    if (ver_lt(version_maj, version_min, patch_level, 5, 1, 3))
    {
      for (i = 0; i < MAX_STORES; i++)
      {
        st_ptr = &store[i];
        st_ptr->store_open = (int32)rd_int32u(file_ptr);
        st_ptr->insult_cur = (int16)rd_int16u(file_ptr);
        st_ptr->owner      = rd_int8u (file_ptr);
        st_ptr->store_ctr  = rd_int8u (file_ptr);
        st_ptr->good_buy   = rd_int16u(file_ptr);
        st_ptr->bad_buy    = rd_int16u(file_ptr);
        if (st_ptr->store_ctr > STORE_INVEN_MAX) goto error;
        for (j = 0; j < st_ptr->store_ctr; j++)
        {
          st_ptr->store_inven[j].scost = (int32)rd_int32u(file_ptr);
          rd_item(file_ptr, &st_ptr->store_inven[j].sitem);
        }
      }
    }

    /* read the time that the file was saved (old versions) */
    if (ver_lt(version_maj, version_min, patch_level, 5, 0, 16))
    {
      time_saved = 0; /* no time in file, clear to zero */
    }
    else if (ver_lt(version_maj, version_min, patch_level, 5, 1, 3))
    {
      time_saved = rd_int32u(file_ptr);
    }

    if (ferror(file_ptr)) goto error;

    if (turn < 0)
error:
      ok = false;  /* Assume bad data. */
    else
    {
      /* don't overwrite the killed by string if character is dead */
      if (py.misc.chp >= 0) strcpy(died_from, "(alive and well)");
      character_generated = 1;
    }

closefiles:
    if (file_ptr != NULL && fclose(file_ptr) < 0) ok = false;

    if (!ok)
    {
      msg_print("Error during reading of file.");
    }
    else
    {
      /* let the user overwrite the old savefile when save/quit */
      from_savefile = 1;

      signals();

      if (panic_save)
      {
        sprintf(temp, "This game is from a panic save. " \
                      "Score will not be added to scoreboard.");
        msg_print(temp);
      }
      else if ((!noscore & 0x04) && duplicate_character())
      {
        sprintf (temp, "This character is already on the " \
                       "scoreboard; it will not be scored again.");
        msg_print(temp);
        noscore |= 0x4;
      }

      if (turn >= 0)
      {  /* Only if a full restoration. */
        weapon_heavy = false;
        pack_heavy = 0;
        check_strength();

        /* rotate store inventory, depending on how old the save file */
        /* is foreach day old (rounded up), call store_maint */
        /* calculate age in seconds */
        start_time = time(NULL);
        /* check for reasonable values of time here ... */
        age = (start_time < time_saved) ? 0 : start_time - time_saved;
        age = (age + 43200L) / 86400L;  /* age in days, rounded */
        if (age > 10) age = 10; /* in case savefile is very old */
        for (i = 0; i < age; i++) store_maint();
      }

      if (noscore)
        msg_print("This save file cannot be used to get on the score board.");

      if (version_maj != CUR_VERSION_MAJ || version_min != CUR_VERSION_MIN)
      {
        sprintf(temp,
          "Save file version %d.%d %s on game version %d.%d.",
          version_maj, version_min,
          version_min <= CUR_VERSION_MIN ? "accepted" : "risky",
          CUR_VERSION_MAJ, CUR_VERSION_MIN);
        msg_print(temp);
      }

      /* return true if full restoration, or false if only restored options
         and monster memory */
      return (turn >= 0);
    }
  }
  turn = -1;
  prt("Please try again without that savefile.", 1, 0);
  signals();
  exit_game();

  return false;  /* not reached, unless on mac */
}

static void wr_int8u(FILE *file_ptr, const int8u c)
{
  if (file_ptr == NULL) return;
  xor_byte ^= c;
  putc((int)xor_byte, file_ptr);
}

static void wr_int16u(FILE *file_ptr, const int16u s)
{
  /* legacy code copy-pasted xor_byte and putc() calls, but I think calling
     wr_int8u() instead is more readable and maintainable -BS- */
  wr_int8u(file_ptr,  s       & 0xFF);
  wr_int8u(file_ptr, (s >> 8) & 0xFF);
}

static void wr_int32u(FILE *file_ptr, const int32u l)
{
  wr_int8u(file_ptr,  l        & 0xFF);
  wr_int8u(file_ptr, (l >>  8) & 0xFF);
  wr_int8u(file_ptr, (l >> 16) & 0xFF);
  wr_int8u(file_ptr, (l >> 24) & 0xFF);
}

static void wrn_int8u(FILE *file_ptr, int8u const *const c, const int count)
{
  int i;

  if (c == NULL) return;
  /* legacy code walked a pointer in parallel with the loop counter, but
     this is more readable. It's not going to be a noticeable performance
     difference in the 21st century -BS- */
  for (i = 0; i < count; ++i) wr_int8u(file_ptr, c[i]);
}

static void wr_string(FILE *file_ptr, char const *const str)
{
  char const *cptr;

  if (str == NULL) return;
  for (cptr = str; *cptr != '\0'; ++cptr) wr_int8u(file_ptr, *cptr);
  /* write the NULL terminator too */
  wr_int8u(file_ptr, '\0');
}

static void wrn_int16u(FILE *file_ptr, int16u const *const s, const int count)
{
  int i;

  if (s == NULL) return;
  for (i = 0; i < count; ++i) wr_int16u(file_ptr, s[i]);
}

static void wr_item(FILE *file_ptr, inven_type const *const item)
{
  if (item == NULL) return;
  wr_int16u(file_ptr, item->index);
  wr_int8u (file_ptr, item->name2);
  wr_string(file_ptr, item->inscrip);
  wr_int32u(file_ptr, item->flags);
  wr_int8u (file_ptr, item->tval);
  wr_int8u (file_ptr, item->tchar);
  wr_int16u(file_ptr, (int16u)item->p1);
  wr_int32u(file_ptr, (int32u)item->cost);
  wr_int8u (file_ptr, item->subval);
  wr_int8u (file_ptr, item->number);
  wr_int16u(file_ptr, item->weight);
  wr_int16u(file_ptr, (int16u)item->tohit);
  wr_int16u(file_ptr, (int16u)item->todam);
  wr_int16u(file_ptr, (int16u)item->ac);
  wr_int16u(file_ptr, (int16u)item->toac);
  wrn_int8u(file_ptr, item->damage, 2);
  wr_int8u (file_ptr, item->level);
  wr_int8u (file_ptr, item->ident);
}

static void wr_monster(FILE *file_ptr, monster_type const *const mon)
{
  if (mon == NULL) return;
  wr_int16u(file_ptr, (int16u)mon->hp);
  wr_int16u(file_ptr, (int16u)mon->csleep);
  wr_int16u(file_ptr, (int16u)mon->cspeed);
  wr_int16u(file_ptr, mon->mptr);
  wr_int8u (file_ptr, mon->fy);
  wr_int8u (file_ptr, mon->fx);
  wr_int8u (file_ptr, mon->cdis);
  wr_int8u (file_ptr, mon->ml);
  wr_int8u (file_ptr, mon->stunned);
  wr_int8u (file_ptr, mon->confused);
}

static int8u rd_int8u(FILE *file_ptr)
{
  const int8u xor_old = xor_byte;
  if (file_ptr == NULL) return 0;
  xor_byte = getc(file_ptr) & 0xFF;
  return xor_byte ^ xor_old;
}

static int16u rd_int16u(FILE *file_ptr)
{
  return ((int16u)rd_int8u(file_ptr)
       | ((int16u)rd_int8u(file_ptr) << 8)
  );
}

static int32u rd_int32u(FILE *file_ptr)
{
  return ((int32u)rd_int8u(file_ptr)
       | ((int32u)rd_int8u(file_ptr) <<  8)
       | ((int32u)rd_int8u(file_ptr) << 16)
       | ((int32u)rd_int8u(file_ptr) << 24)
  );
}

static void rdn_int8u(FILE *file_ptr, int8u *const ch_ptr, const int count)
{
  int i;
  if (ch_ptr == NULL) return;
  for (i = 0; i < count; ++i) ch_ptr[i] = rd_int8u(file_ptr);
}

static void rd_string(FILE *file_ptr, char *const str)
{
  /* pointer to current data destination */
  char *s = str;
  if (str == NULL) return;
  /* this is a bit mind-bending due to use of post-increment, but the basic
     idea is to read bytes into incrementing memory locations until we've
     read and copied a null terminator -BS- */
  do *s = (char)rd_int8u(file_ptr); while (*s++ != '\0');
}

static void rdn_int16u(FILE *file_ptr, int16u *const ptr, const int count)
{
  int i;
  if (ptr == NULL) return;
  for (i = 0; i < count; ++i) ptr[i] = rd_int16u(file_ptr);
}

static void rd_item(FILE *file_ptr, inven_type *const item)
{
  item->index  = rd_int16u(file_ptr);
  item->name2  = rd_int8u(file_ptr);
  rd_string(file_ptr, item->inscrip);
  item->flags  = rd_int32u(file_ptr);
  item->tval   = rd_int8u(file_ptr);
  item->tchar  = rd_int8u(file_ptr);
  item->p1     = (int16)rd_int16u(file_ptr);
  item->cost   = (int32)rd_int32u(file_ptr);
  item->subval = rd_int8u(file_ptr);
  item->number = rd_int8u(file_ptr);
  item->weight = rd_int16u(file_ptr);
  item->tohit  = (int16)rd_int16u(file_ptr);
  item->todam  = (int16)rd_int16u(file_ptr);
  item->ac     = (int16)rd_int16u(file_ptr);
  item->toac   = (int16)rd_int16u(file_ptr);
  rdn_int8u(file_ptr, item->damage, 2);
  item->level  = rd_int8u(file_ptr);
  item->ident  = rd_int8u(file_ptr);
}

static void rd_monster(FILE *file_ptr, monster_type *const mon)
{
  mon->hp       = (int16)rd_int16u(file_ptr);
  mon->csleep   = (int16)rd_int16u(file_ptr);
  mon->cspeed   = (int16)rd_int16u(file_ptr);
  mon->mptr     = rd_int16u(file_ptr);
  mon->fy       = rd_int8u(file_ptr);
  mon->fx       = rd_int8u(file_ptr);
  mon->cdis     = rd_int8u(file_ptr);
  mon->ml       = rd_int8u(file_ptr);
  mon->stunned  = rd_int8u(file_ptr);
  mon->confused = rd_int8u(file_ptr);
}

/* functions called from death.c to implement the score file */

void wr_highscore(FILE* file_ptr, high_scores const *const score)
{
  if (score == NULL) return;

  /* Save the encryption byte for robustness.  */
  wr_int8u (file_ptr, xor_byte);

  wr_int32u(file_ptr, (int32u)score->points);
  wr_int32u(file_ptr, (int32u)score->birth_date);
  wr_int16u(file_ptr, (int16u)score->uid);
  wr_int16u(file_ptr, (int16u)score->mhp);
  wr_int16u(file_ptr, (int16u)score->chp);
  wr_int8u (file_ptr, score->dun_level);
  wr_int8u (file_ptr, score->lev);
  wr_int8u (file_ptr, score->max_dlv);
  wr_int8u (file_ptr, score->sex);
  wr_int8u (file_ptr, score->race);
  wr_int8u (file_ptr, score->class);
  wrn_int8u(file_ptr, (int8u *)score->name, PLAYER_NAME_SIZE);
  wrn_int8u(file_ptr, (int8u *)score->died_from, 25);
}

void rd_highscore(FILE* file_ptr, high_scores *const score)
{
  /* Read the encryption byte.  */
  xor_byte = rd_int8u(file_ptr);

  score->points     = (int32)rd_int32u(file_ptr);
  score->birth_date = (int32)rd_int32u(file_ptr);
  score->uid        = (int16)rd_int16u(file_ptr);
  score->mhp        = (int16)rd_int16u(file_ptr);
  score->chp        = (int16)rd_int16u(file_ptr);
  score->dun_level  = rd_int8u(file_ptr);
  score->lev        = rd_int8u(file_ptr);
  score->max_dlv    = rd_int8u(file_ptr);
  score->sex        = rd_int8u(file_ptr);
  score->race       = rd_int8u(file_ptr);
  score->class      = rd_int8u(file_ptr);
  rdn_int8u(file_ptr, (int8u *)score->name, PLAYER_NAME_SIZE);
  rdn_int8u(file_ptr, (int8u *)score->died_from, 25);
}
