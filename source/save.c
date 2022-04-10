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

static int    sv_write();
static bool   ver_lt(const int8u, const int8u, const int8u,
                     const int8u, const int8u, const int8u);
static bool   ver_ge(const int8u, const int8u, const int8u,
                     const int8u, const int8u, const int8u);
static void   wr_byte(const int8u);
static void   wr_short(const int16u);
static void   wr_long(const int32u);
static void   wr_bytes(int8u const *const, const int);
static void   wr_string(char const *const);
static void   wr_shorts(int16u const *const, const int);
static void   wr_item(inven_type const *const);
static void   wr_monster(monster_type const *const);
static int8u  rd_byte();
static int16u rd_short();
static int32u rd_long();
static void   rd_bytes(int8u *const, const int);
static void   rd_string(char *const);
static void   rd_shorts(int16u *const, const int);
static void   rd_item(inven_type *const);
static void   rd_monster(monster_type *const);

/* these are used for the save file, to avoid having to pass them to every
   procedure */
static FILE *fileptr;
static int8u xor_byte;
static int from_savefile;  /* can overwrite old savefile when save */
static int32u start_time;  /* time that play started */

/* This save package was brought to you by -JWT- and -RAK-
   and has been completely rewritten for UNIX by -JEW-
   and has been completely rewritten again by -CJS-
   and completely rewritten again! for portability by -JEW-
   and cleaned up / modernized for the 21st century by -BS-
*/

static int sv_write()
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

  /* write monster memory */
  for (i = 0; i < MAX_CREATURES; i++)
  {
    r_ptr = &c_recall[i];
    if (r_ptr->r_cmove || r_ptr->r_cdefense || r_ptr->r_kills ||
        r_ptr->r_spells || r_ptr->r_deaths || r_ptr->r_attacks[0] ||
        r_ptr->r_attacks[1] || r_ptr->r_attacks[2] || r_ptr->r_attacks[3])
    {
      wr_short((int16u)i);
      wr_long(r_ptr->r_cmove);
      wr_long(r_ptr->r_spells);
      wr_short(r_ptr->r_kills);
      wr_short(r_ptr->r_deaths);
      wr_short(r_ptr->r_cdefense);
      wr_byte(r_ptr->r_wake);
      wr_byte(r_ptr->r_ignore);
      wr_bytes(r_ptr->r_attacks, MAX_MON_NATTACK);
    }
  }
  wr_short((int16u)0xFFFF); /* sentinel to indicate no more monster info */

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

  wr_long(int32u_tmp);

  /* write player data */
  m_ptr = &py.misc;
  wr_string(m_ptr->name);
  wr_byte(m_ptr->male);
  wr_long((int32u)m_ptr->au);
  wr_long((int32u)m_ptr->max_exp);
  wr_long((int32u)m_ptr->exp);
  wr_short(m_ptr->exp_frac);
  wr_short(m_ptr->age);
  wr_short(m_ptr->ht);
  wr_short(m_ptr->wt);
  wr_short(m_ptr->lev);
  wr_short(m_ptr->max_dlv);
  wr_short((int16u)m_ptr->srh);
  wr_short((int16u)m_ptr->fos);
  wr_short((int16u)m_ptr->bth);
  wr_short((int16u)m_ptr->bthb);
  wr_short((int16u)m_ptr->mana);
  wr_short((int16u)m_ptr->mhp);
  wr_short((int16u)m_ptr->ptohit);
  wr_short((int16u)m_ptr->ptodam);
  wr_short((int16u)m_ptr->pac);
  wr_short((int16u)m_ptr->ptoac);
  wr_short((int16u)m_ptr->dis_th);
  wr_short((int16u)m_ptr->dis_td);
  wr_short((int16u)m_ptr->dis_ac);
  wr_short((int16u)m_ptr->dis_tac);
  wr_short((int16u)m_ptr->disarm);
  wr_short((int16u)m_ptr->save);
  wr_short((int16u)m_ptr->sc);
  wr_short((int16u)m_ptr->stl);
  wr_byte(m_ptr->pclass);
  wr_byte(m_ptr->prace);
  wr_byte(m_ptr->hitdie);
  wr_byte(m_ptr->expfact);
  wr_short((int16u)m_ptr->cmana);
  wr_short(m_ptr->cmana_frac);
  wr_short((int16u)m_ptr->chp);
  wr_short(m_ptr->chp_frac);
  for (i = 0; i < 4; i++) wr_string (m_ptr->history[i]);

  s_ptr = &py.stats;
  wr_bytes(s_ptr->max_stat, 6);
  wr_bytes(s_ptr->cur_stat, 6);
  wr_shorts((int16u *)s_ptr->mod_stat, 6);
  wr_bytes(s_ptr->use_stat, 6);

  f_ptr = &py.flags;
  wr_long(f_ptr->status);
  wr_short((int16u)f_ptr->rest);
  wr_short((int16u)f_ptr->blind);
  wr_short((int16u)f_ptr->paralysis);
  wr_short((int16u)f_ptr->confused);
  wr_short((int16u)f_ptr->food);
  wr_short((int16u)f_ptr->food_digested);
  wr_short((int16u)f_ptr->protection);
  wr_short((int16u)f_ptr->speed);
  wr_short((int16u)f_ptr->fast);
  wr_short((int16u)f_ptr->slow);
  wr_short((int16u)f_ptr->afraid);
  wr_short((int16u)f_ptr->poisoned);
  wr_short((int16u)f_ptr->image);
  wr_short((int16u)f_ptr->protevil);
  wr_short((int16u)f_ptr->invuln);
  wr_short((int16u)f_ptr->hero);
  wr_short((int16u)f_ptr->shero);
  wr_short((int16u)f_ptr->blessed);
  wr_short((int16u)f_ptr->resist_heat);
  wr_short((int16u)f_ptr->resist_cold);
  wr_short((int16u)f_ptr->detect_inv);
  wr_short((int16u)f_ptr->word_recall);
  wr_short((int16u)f_ptr->see_infra);
  wr_short((int16u)f_ptr->tim_infra);
  wr_byte(f_ptr->see_inv);
  wr_byte(f_ptr->teleport);
  wr_byte(f_ptr->free_act);
  wr_byte(f_ptr->slow_digest);
  wr_byte(f_ptr->aggravate);
  wr_byte(f_ptr->fire_resist);
  wr_byte(f_ptr->cold_resist);
  wr_byte(f_ptr->acid_resist);
  wr_byte(f_ptr->regenerate);
  wr_byte(f_ptr->lght_resist);
  wr_byte(f_ptr->ffall);
  wr_byte(f_ptr->sustain_str);
  wr_byte(f_ptr->sustain_int);
  wr_byte(f_ptr->sustain_wis);
  wr_byte(f_ptr->sustain_con);
  wr_byte(f_ptr->sustain_dex);
  wr_byte(f_ptr->sustain_chr);
  wr_byte(f_ptr->confuse_monster);
  wr_byte(f_ptr->new_spells);

  /* write game state */
  wr_short((int16u)missile_ctr);
  wr_long((int32u)turn);
  wr_short((int16u)inven_ctr);
  for (i = 0; i < inven_ctr; i++) wr_item(&inventory[i]);
  for (i = INVEN_WIELD; i < INVEN_ARRAY_SIZE; i++) wr_item(&inventory[i]);
  wr_short((int16u)inven_weight);
  wr_short((int16u)equip_ctr);
  wr_long(spell_learned);
  wr_long(spell_worked);
  wr_long(spell_forgotten);
  wr_bytes(spell_order, 32);
  wr_bytes(object_ident, OBJECT_IDENT_SIZE);
  wr_long(randes_seed);
  wr_long(town_seed);
  wr_short((int16u)last_msg);
  for (i = 0; i < MAX_SAVE_MSG; i++) wr_string(old_msg[i]);

  /* this indicates 'cheating' if it is a one */
  wr_short((int16u)panic_save);
  wr_short((int16u)total_winner);
  wr_short((int16u)noscore);
  wr_shorts(player_hp, MAX_PLAYER_LEVEL);

  /* write town store data */
  for (i = 0; i < MAX_STORES; i++)
  {
    st_ptr = &store[i];
    wr_long((int32u)st_ptr->store_open);
    wr_short((int16u)st_ptr->insult_cur);
    wr_byte(st_ptr->owner);
    wr_byte(st_ptr->store_ctr);
    wr_short(st_ptr->good_buy);
    wr_short(st_ptr->bad_buy);
    for (j = 0; j < st_ptr->store_ctr; j++)
    {
      wr_long((int32u)st_ptr->store_inven[j].scost);
      wr_item(&st_ptr->store_inven[j].sitem);
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
  wr_long(int32u_tmp);

  /* starting with 5.2, put died_from string in savefile */
  wr_string(died_from);

  /* starting with 5.2.2, put the max_score in the savefile */
  wr_long((int32u)total_points());

  /* starting with 5.2.2, put the birth_date in the savefile */
  wr_long((int32u)birth_date);

  /* only level specific info follows, this allows characters to be
     resurrected, the dungeon level info is not needed for a resurrection */
  if (death)
  {
    return (!ferror(fileptr) && fflush(fileptr) != EOF);
  }

  /* write dungeon data */
  wr_short((int16u)dun_level);
  wr_short((int16u)char_row);
  wr_short((int16u)char_col);
  wr_short((int16u)mon_tot_mult);
  wr_short((int16u)cur_height);
  wr_short((int16u)cur_width);
  wr_short((int16u)max_panel_rows);
  wr_short((int16u)max_panel_cols);

  for (i = 0; i < MAX_HEIGHT; i++)
  {
    for (j = 0; j < MAX_WIDTH; j++)
    {
      c_ptr = &cave[i][j];
      if (!(c_ptr->cptr)) continue;
      wr_byte((int8u)i);
      wr_byte((int8u)j);
      wr_byte(c_ptr->cptr);
    }
  }
  wr_byte((int8u)0xFF); /* marks end of cptr info */
  for (i = 0; i < MAX_HEIGHT; i++)
  {
    for (j = 0; j < MAX_WIDTH; j++)
    {
      c_ptr = &cave[i][j];
      if (!(c_ptr->tptr)) continue;
      wr_byte((int8u)i);
      wr_byte((int8u)j);
      wr_byte(c_ptr->tptr);
    }
  }
  wr_byte((int8u)0xFF); /* marks end of tptr info */
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
        wr_byte((int8u)count);
        wr_byte(prev_char);
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
  wr_byte((int8u)count);
  wr_byte(prev_char);

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
  wr_short((int16u)tcptr);
  for (i = MIN_TRIX; i < tcptr; i++) wr_item(&t_list[i]);
  wr_short((int16u)mfptr);
  for (i = MIN_MONIX; i < mfptr; i++) wr_monster(&m_list[i]);

  return (!ferror(fileptr) && fflush(fileptr) != EOF);
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
  fileptr = NULL;                   /* Do not assume it has been init'ed */
  if (access(fnam, F_OK) == -1 ||
      from_savefile ||
      (wizard && get_check("Save file already exists. Overwrite?")))
  {
    fileptr = fopen(savefile, "wb+");
  }

  if (fileptr != NULL)
  {
    file_created = true;

    xor_byte = 0;
    wr_byte((int8u)CUR_VERSION_MAJ);
    xor_byte = 0;
    wr_byte((int8u)CUR_VERSION_MIN);
    xor_byte = 0;
    wr_byte((int8u)PATCH_LEVEL);
    xor_byte = 0;
    char_tmp = randint(256) - 1;
    wr_byte(char_tmp);
    /* Note that xor_byte is now equal to char_tmp */

    ok = sv_write();
    if (fclose(fileptr) == EOF) ok = false;
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

    fileptr = fopen(savefile, "rb");
    if (fileptr == NULL) goto error;

    prt("Restoring Memory...", 0, 0);
    put_qio();

    xor_byte = 0;
    version_maj = rd_byte();
    xor_byte = 0;
    version_min = rd_byte();
    xor_byte = 0;
    patch_level = rd_byte();
    xor_byte = 0;
    xor_byte = rd_byte();

    /* COMPAT support savefiles from 5.0.14 to 5.0.17 */
    /* support savefiles from 5.1.0 to present */
    if (ver_lt(version_maj, version_min, patch_level, 5, 0, 14))
    {
      prt("Sorry. This savefile is from an incompatible version of umoria/hzmoria.",
          2, 0);
      goto error;
    }

    int16u_tmp = rd_short();
    while (int16u_tmp != 0xFFFF)
    {
      if (int16u_tmp >= MAX_CREATURES) goto error;
      r_ptr = &c_recall[int16u_tmp];
      r_ptr->r_cmove    = rd_long();
      r_ptr->r_spells   = rd_long();
      r_ptr->r_kills    = rd_short();
      r_ptr->r_deaths   = rd_short();
      r_ptr->r_cdefense = rd_short();
      r_ptr->r_wake     = rd_byte();
      r_ptr->r_ignore   = rd_byte();
      rd_bytes(r_ptr->r_attacks, MAX_MON_NATTACK);
      int16u_tmp        = rd_short();
    }

    /* for save files before 5.2.2, read and ignore log_index (sic) */
    if (ver_lt(version_maj, version_min, patch_level, 5, 2, 2))
    {
      int16u_tmp = rd_short();
    }
    int32u_tmp = rd_long();

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
        msg_print ("Sorry, this character is retired from moria.");
        msg_print ("You can not resurrect a retired character.");
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
      rd_string(m_ptr->name);
      m_ptr->male       = rd_byte();
      m_ptr->au         = (int32)rd_long();
      m_ptr->max_exp    = (int32)rd_long();
      m_ptr->exp        = (int32)rd_long();
      m_ptr->exp_frac   = rd_short();
      m_ptr->age        = rd_short();
      m_ptr->ht         = rd_short();
      m_ptr->wt         = rd_short();
      m_ptr->lev        = rd_short();
      m_ptr->max_dlv    = rd_short();
      m_ptr->srh        = (int16)rd_short();
      m_ptr->fos        = (int16)rd_short();
      m_ptr->bth        = (int16)rd_short();
      m_ptr->bthb       = (int16)rd_short();
      m_ptr->mana       = (int16)rd_short();
      m_ptr->mhp        = (int16)rd_short();
      m_ptr->ptohit     = (int16)rd_short();
      m_ptr->ptodam     = (int16)rd_short();
      m_ptr->pac        = (int16)rd_short();
      m_ptr->ptoac      = (int16)rd_short();
      m_ptr->dis_th     = (int16)rd_short();
      m_ptr->dis_td     = (int16)rd_short();
      m_ptr->dis_ac     = (int16)rd_short();
      m_ptr->dis_tac    = (int16)rd_short();
      m_ptr->disarm     = (int16)rd_short();
      m_ptr->save       = (int16)rd_short();
      m_ptr->sc         = (int16)rd_short();
      m_ptr->stl        = (int16)rd_short();
      m_ptr->pclass     = rd_byte();
      m_ptr->prace      = rd_byte();
      m_ptr->hitdie     = rd_byte();
      m_ptr->expfact    = rd_byte();
      m_ptr->cmana      = (int16)rd_short();
      m_ptr->cmana_frac = rd_short();
      m_ptr->chp        = (int16)rd_short();
      m_ptr->chp_frac   = rd_short();
      for (i = 0; i < 4; i++) rd_string(m_ptr->history[i]);

      s_ptr = &py.stats;
      rd_bytes(s_ptr->max_stat, 6);
      rd_bytes(s_ptr->cur_stat, 6);
      rd_shorts((int16u *)s_ptr->mod_stat, 6);
      rd_bytes(s_ptr->use_stat, 6);

      f_ptr = &py.flags;
      f_ptr->status          = rd_long();
      f_ptr->rest            = (int16)rd_short();
      f_ptr->blind           = (int16)rd_short();
      f_ptr->paralysis       = (int16)rd_short();
      f_ptr->confused        = (int16)rd_short();
      f_ptr->food            = (int16)rd_short();
      f_ptr->food_digested   = (int16)rd_short();
      f_ptr->protection      = (int16)rd_short();
      f_ptr->speed           = (int16)rd_short();
      f_ptr->fast            = (int16)rd_short();
      f_ptr->slow            = (int16)rd_short();
      f_ptr->afraid          = (int16)rd_short();
      f_ptr->poisoned        = (int16)rd_short();
      f_ptr->image           = (int16)rd_short();
      f_ptr->protevil        = (int16)rd_short();
      f_ptr->invuln          = (int16)rd_short();
      f_ptr->hero            = (int16)rd_short();
      f_ptr->shero           = (int16)rd_short();
      f_ptr->blessed         = (int16)rd_short();
      f_ptr->resist_heat     = (int16)rd_short();
      f_ptr->resist_cold     = (int16)rd_short();
      f_ptr->detect_inv      = (int16)rd_short();
      f_ptr->word_recall     = (int16)rd_short();
      f_ptr->see_infra       = (int16)rd_short();
      f_ptr->tim_infra       = (int16)rd_short();
      f_ptr->see_inv         = rd_byte();
      f_ptr->teleport        = rd_byte();
      f_ptr->free_act        = rd_byte();
      f_ptr->slow_digest     = rd_byte();
      f_ptr->aggravate       = rd_byte();
      f_ptr->fire_resist     = rd_byte();
      f_ptr->cold_resist     = rd_byte();
      f_ptr->acid_resist     = rd_byte();
      f_ptr->regenerate      = rd_byte();
      f_ptr->lght_resist     = rd_byte();
      f_ptr->ffall           = rd_byte();
      f_ptr->sustain_str     = rd_byte();
      f_ptr->sustain_int     = rd_byte();
      f_ptr->sustain_wis     = rd_byte();
      f_ptr->sustain_con     = rd_byte();
      f_ptr->sustain_dex     = rd_byte();
      f_ptr->sustain_chr     = rd_byte();
      f_ptr->confuse_monster = rd_byte();
      f_ptr->new_spells      = rd_byte();

      missile_ctr     = (int16)rd_short();
      turn            = (int32)rd_long();
      inven_ctr       = (int16)rd_short();
      if (inven_ctr > INVEN_WIELD) goto error;
      for (i = 0; i < inven_ctr; i++) rd_item(&inventory[i]);
      for (i = INVEN_WIELD; i < INVEN_ARRAY_SIZE; i++)
        rd_item(&inventory[i]);
      inven_weight    = (int16)rd_short();
      equip_ctr       = (int16)rd_short();
      spell_learned   = rd_long();
      spell_worked    = rd_long();
      spell_forgotten = rd_long();
      rd_bytes(spell_order, 32);
      rd_bytes(object_ident, OBJECT_IDENT_SIZE);
      randes_seed     = rd_long();
      town_seed       = rd_long();
      last_msg        = (int16)rd_short();
      for (i = 0; i < MAX_SAVE_MSG; i++) rd_string(old_msg[i]);

      int16u_tmp   = rd_short();
      panic_save   = !!int16u_tmp;
      total_winner = (int16)rd_short();
      noscore      = (int16)rd_short();
      rd_shorts(player_hp, MAX_PLAYER_LEVEL);

      if (ver_ge(version_maj, version_min, patch_level, 5, 1, 3))
      {
        for (i = 0; i < MAX_STORES; i++)
        {
          st_ptr = &store[i];
          st_ptr->store_open = (int32)rd_long();
          st_ptr->insult_cur = (int16)rd_short();
          st_ptr->owner      = rd_byte();
          st_ptr->store_ctr  = rd_byte();
          st_ptr->good_buy   = rd_short();
          st_ptr->bad_buy    = rd_short();
          if (st_ptr->store_ctr > STORE_INVEN_MAX) goto error;
          for (j = 0; j < st_ptr->store_ctr; j++)
          {
            st_ptr->store_inven[j].scost = (int32)rd_long();
            rd_item(&st_ptr->store_inven[j].sitem);
          }
        }
      }

      /* read the time that the file was saved */
      if (ver_ge(version_maj, version_min, patch_level, 5, 1, 3))
        time_saved = rd_long();

      if (ver_ge(version_maj, version_min, patch_level, 5, 2, 0))
        rd_string(died_from);

      max_score = 0;
      if (ver_ge(version_maj, version_min, patch_level, 5, 2, 2))
        max_score = (int32)rd_long();

      birth_date = time(NULL);
      if (ver_ge(version_maj, version_min, patch_level, 5, 2, 2))
        birth_date = (int32)rd_long();
    } /* end if not dead */

    /* if end of file or dead */
    if ((c = getc(fileptr)) == EOF || dead_save)
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

    if (ungetc(c, fileptr) == EOF) goto error;

    prt("Restoring Character...", 0, 0);
    put_qio();

    /* only level specific info should follow, not present for dead
        characters */

    dun_level      = (int16)rd_short();
    char_row       = (int16)rd_short();
    char_col       = (int16)rd_short();
    mon_tot_mult   = (int16)rd_short();
    cur_height     = (int16)rd_short();
    cur_width      = (int16)rd_short();
    max_panel_rows = (int16)rd_short();
    max_panel_cols = (int16)rd_short();

    /* read in the creature ptr info */
    for (char_tmp = rd_byte(); char_tmp != 0xFF; char_tmp = rd_byte())
    {
      ychar    = char_tmp;
      xchar    = rd_byte();
      char_tmp = rd_byte();
      if (xchar > MAX_WIDTH || ychar > MAX_HEIGHT) goto error;
      cave[ychar][xchar].cptr = char_tmp;
    }
    /* read in the treasure ptr info */
    for (char_tmp = rd_byte(); char_tmp != 0xFF; char_tmp = rd_byte())
    {
      ychar    = char_tmp;
      xchar    = rd_byte();
      char_tmp = rd_byte();
      if (xchar > MAX_WIDTH || ychar > MAX_HEIGHT) goto error;
      cave[ychar][xchar].tptr = char_tmp;
    }
    /* read in the rest of the cave info */
    c_ptr = &cave[0][0];
    for (total_count = 0;
         total_count < MAX_HEIGHT*MAX_WIDTH;
         total_count += count)
    {
      count    = rd_byte();
      char_tmp = rd_byte();
      for (i = 0; i < count; ++i)
      {
        if (c_ptr >= &cave[MAX_HEIGHT][0]) goto error;
        c_ptr->fval = char_tmp & 0xF;
        c_ptr->lr = (char_tmp >> 4) & 0x1;
        c_ptr->fm = (char_tmp >> 5) & 0x1;
        c_ptr->pl = (char_tmp >> 6) & 0x1;
        c_ptr->tl = (char_tmp >> 7) & 0x1;
        c_ptr++;
      }
    }

    tcptr = (int16)rd_short();
    if (tcptr > MAX_TALLOC) goto error;
    for (i = MIN_TRIX; i < tcptr; i++) rd_item(&t_list[i]);
    mfptr = (int16)rd_short();
    if (mfptr > MAX_MALLOC) goto error;
    for (i = MIN_MONIX; i < mfptr; i++) rd_monster(&m_list[i]);

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
        st_ptr->store_open = (int32)rd_long();
        st_ptr->insult_cur = (int16)rd_short();
        st_ptr->owner      = rd_byte();
        st_ptr->store_ctr  = rd_byte();
        st_ptr->good_buy   = rd_short();
        st_ptr->bad_buy    = rd_short();
        if (st_ptr->store_ctr > STORE_INVEN_MAX) goto error;
        for (j = 0; j < st_ptr->store_ctr; j++)
        {
          st_ptr->store_inven[j].scost = (int32)rd_long();
          rd_item(&st_ptr->store_inven[j].sitem);
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
      time_saved = rd_long();
    }

    if (ferror(fileptr)) goto error;

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
    if (fileptr != NULL && fclose(fileptr) < 0) ok = false;

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

static void wr_byte(const int8u c)
{
  xor_byte ^= c;
  putc((int)xor_byte, fileptr);
}

static void wr_short(const int16u s)
{
  /* legacy code copy-pasted xor_byte and putc() calls, but I think calling
     wr_byte() instead is more readable and maintainable -BS- */
  wr_byte( s       & 0xFF);
  wr_byte((s >> 8) & 0xFF);
}

static void wr_long(const int32u l)
{
  wr_byte( l        & 0xFF);
  wr_byte((l >>  8) & 0xFF);
  wr_byte((l >> 16) & 0xFF);
  wr_byte((l >> 24) & 0xFF);
}

static void wr_bytes(int8u const *const c, const int count)
{
  int i;

  if (c == NULL) return;
  /* legacy code walked a pointer in parallel with the loop counter, but
     this is more readable. It's not going to be a noticeable performance
     difference in the 21st century -BS- */
  for (i = 0; i < count; ++i) wr_byte(c[i]);
}

static void wr_string(char const *const str)
{
  char const *cptr;

  if (str == NULL) return;
  for (cptr = str; *cptr != '\0'; ++cptr) wr_byte(*cptr);
  /* write the NULL terminator too */
  wr_byte('\0');
}

static void wr_shorts(int16u const *const s, const int count)
{
  int i;

  if (s == NULL) return;
  for (i = 0; i < count; ++i) wr_short(s[i]);
}

static void wr_item(inven_type const *const item)
{
  if (item == NULL) return;
  wr_short(item->index);
  wr_byte(item->name2);
  wr_string(item->inscrip);
  wr_long(item->flags);
  wr_byte(item->tval);
  wr_byte(item->tchar);
  wr_short((int16u)item->p1);
  wr_long((int32u)item->cost);
  wr_byte(item->subval);
  wr_byte(item->number);
  wr_short(item->weight);
  wr_short((int16u)item->tohit);
  wr_short((int16u)item->todam);
  wr_short((int16u)item->ac);
  wr_short((int16u)item->toac);
  wr_bytes(item->damage, 2);
  wr_byte(item->level);
  wr_byte(item->ident);
}

static void wr_monster(monster_type const *const mon)
{
  if (mon == NULL) return;
  wr_short((int16u)mon->hp);
  wr_short((int16u)mon->csleep);
  wr_short((int16u)mon->cspeed);
  wr_short(mon->mptr);
  wr_byte(mon->fy);
  wr_byte(mon->fx);
  wr_byte(mon->cdis);
  wr_byte(mon->ml);
  wr_byte(mon->stunned);
  wr_byte(mon->confused);
}

static int8u rd_byte()
{
  const int8u xor_old = xor_byte;
  xor_byte = getc(fileptr) & 0xFF;
  return xor_byte ^ xor_old;
}

static int16u rd_short()
{
  return ((int16u)rd_byte()
       | ((int16u)rd_byte() << 8)
  );
}

static int32u rd_long()
{
  return ((int32u)rd_byte()
       | ((int32u)rd_byte() <<  8)
       | ((int32u)rd_byte() << 16)
       | ((int32u)rd_byte() << 24)
  );
}

static void rd_bytes(int8u *const ch_ptr, const int count)
{
  register int i;
  register int8u *ptr;
  register int8u c;

  ptr = ch_ptr;
  for (i = 0; i < count; i++)
    {
      c = (getc(fileptr) & 0xFF);
      *ptr++ = c ^ xor_byte;
      xor_byte = c;
    }
}

static void rd_string(char *const str)
{
  register int8u c;
  char *s = str;

  do
  {
    c = (getc(fileptr) & 0xFF);
    *s = c ^ xor_byte;
    xor_byte = c;
  } while (*s++ != '\0');
}

static void rd_shorts(int16u *const ptr, const int count)
{
  register int i;
  register int16u *sptr;
  register int16u s;
  int8u c;

  sptr = ptr;
  for (i = 0; i < count; i++)
    {
      c = (getc(fileptr) & 0xFF);
      s = c ^ xor_byte;
      xor_byte = (getc(fileptr) & 0xFF);
      s |= (int16u)(c ^ xor_byte) << 8;
      *sptr++ = s;
    }
}

static void rd_item(inven_type *const item)
{
  item->index  = rd_short();
  item->name2  = rd_byte();
  rd_string(item->inscrip);
  item->flags  = rd_long();
  item->tval   = rd_byte();
  item->tchar  = rd_byte();
  item->p1     = (int16)rd_short();
  item->cost   = (int32)rd_long();
  item->subval = rd_byte();
  item->number = rd_byte();
  item->weight = rd_short();
  item->tohit  = (int16)rd_short();
  item->todam  = (int16)rd_short();
  item->ac     = (int16)rd_short();
  item->toac   = (int16)rd_short();
  rd_bytes(item->damage, 2);
  item->level  = rd_byte();
  item->ident  = rd_byte();
}

static void rd_monster(monster_type *const mon)
{
  mon->hp       = (int16)rd_short();
  mon->csleep   = (int16)rd_short();
  mon->cspeed   = (int16)rd_short();
  mon->mptr     = rd_short();
  mon->fy       = rd_byte();
  mon->fx       = rd_byte();
  mon->cdis     = rd_byte();
  mon->ml       = rd_byte();
  mon->stunned  = rd_byte();
  mon->confused = rd_byte();
}

/* functions called from death.c to implement the score file */

/* set the local fileptr to the scorefile fileptr */
void set_fileptr(FILE *file)
{
  fileptr = file;
}

void wr_highscore(high_scores const *const score)
{
  if (score == NULL) return;

  /* Save the encryption byte for robustness.  */
  wr_byte(xor_byte);

  wr_long((int32u)score->points);
  wr_long((int32u)score->birth_date);
  wr_short((int16u)score->uid);
  wr_short((int16u)score->mhp);
  wr_short((int16u)score->chp);
  wr_byte(score->dun_level);
  wr_byte(score->lev);
  wr_byte(score->max_dlv);
  wr_byte(score->sex);
  wr_byte(score->race);
  wr_byte(score->class);
  wr_bytes((int8u *)score->name, PLAYER_NAME_SIZE);
  wr_bytes((int8u *)score->died_from, 25);
}

void rd_highscore(high_scores *const score)
{
  /* Read the encryption byte.  */
  xor_byte = rd_byte();

  score->points     = (int32)rd_long();
  score->birth_date = (int32)rd_long();
  score->uid        = (int16)rd_short();
  score->mhp        = (int16)rd_short();
  score->chp        = (int16)rd_short();
  score->dun_level  = rd_byte();
  score->lev        = rd_byte();
  score->max_dlv    = rd_byte();
  score->sex        = rd_byte();
  score->race       = rd_byte();
  score->class      = rd_byte();
  rd_bytes((int8u *)score->name, PLAYER_NAME_SIZE);
  rd_bytes((int8u *)score->died_from, 25);
}
