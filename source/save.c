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

/* For debugging the savefile code on systems with broken compilers.  */
#if 0
#define DEBUG(x)  x
#else
#define DEBUG(x)
#endif

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

DEBUG(static FILE *logfile);

static int sv_write();
static void wr_byte();
static void wr_short();
static void wr_long();
static void wr_bytes();
static void wr_string();
static void wr_shorts();
static void wr_item();
static void wr_monster();
static void rd_byte();
static void rd_short();
static void rd_long();
static void rd_bytes();
static void rd_string();
static void rd_shorts();
static void rd_item();
static void rd_monster();

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

  DEBUG(logfile = fopen("IO_LOG", "a"));
  DEBUG(fprintf (logfile, "Saving data to %s\n", savefile));
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
    DEBUG(fclose (logfile));
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

    DEBUG(logfile = fopen("IO_LOG", "a"));
    DEBUG(fprintf(logfile, "Reading data from %s\n", savefile));

    xor_byte = 0;
    rd_byte(&version_maj);
    xor_byte = 0;
    rd_byte(&version_min);
    xor_byte = 0;
    rd_byte(&patch_level);
    xor_byte = 0;
    rd_byte(&xor_byte);

    /* COMPAT support savefiles from 5.0.14 to 5.0.17 */
    /* support savefiles from 5.1.0 to present */
    if (ver_lt(version_maj, version_min, patch_level, 5, 0, 14))
    {
      prt("Sorry. This savefile is from an incompatible version of umoria/hzmoria.",
          2, 0);
      goto error;
    }

    rd_short(&int16u_tmp);
    while (int16u_tmp != 0xFFFF)
    {
      if (int16u_tmp >= MAX_CREATURES) goto error;
      r_ptr = &c_recall[int16u_tmp];
      rd_long(&r_ptr->r_cmove);
      rd_long(&r_ptr->r_spells);
      rd_short(&r_ptr->r_kills);
      rd_short(&r_ptr->r_deaths);
      rd_short(&r_ptr->r_cdefense);
      rd_byte(&r_ptr->r_wake);
      rd_byte(&r_ptr->r_ignore);
      rd_bytes(r_ptr->r_attacks, MAX_MON_NATTACK);
      rd_short(&int16u_tmp);
    }

    /* for save files before 5.2.2, read and ignore log_index (sic) */
    if (ver_lt(version_maj, version_min, patch_level, 5, 2, 2))
    {
      rd_short(&int16u_tmp);
    }
    rd_long(&int32u_tmp);

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
      rd_byte(&m_ptr->male);
      rd_long((int32u *)&m_ptr->au);
      rd_long((int32u *)&m_ptr->max_exp);
      rd_long((int32u *)&m_ptr->exp);
      rd_short(&m_ptr->exp_frac);
      rd_short(&m_ptr->age);
      rd_short(&m_ptr->ht);
      rd_short(&m_ptr->wt);
      rd_short(&m_ptr->lev);
      rd_short(&m_ptr->max_dlv);
      rd_short((int16u *)&m_ptr->srh);
      rd_short((int16u *)&m_ptr->fos);
      rd_short((int16u *)&m_ptr->bth);
      rd_short((int16u *)&m_ptr->bthb);
      rd_short((int16u *)&m_ptr->mana);
      rd_short((int16u *)&m_ptr->mhp);
      rd_short((int16u *)&m_ptr->ptohit);
      rd_short((int16u *)&m_ptr->ptodam);
      rd_short((int16u *)&m_ptr->pac);
      rd_short((int16u *)&m_ptr->ptoac);
      rd_short((int16u *)&m_ptr->dis_th);
      rd_short((int16u *)&m_ptr->dis_td);
      rd_short((int16u *)&m_ptr->dis_ac);
      rd_short((int16u *)&m_ptr->dis_tac);
      rd_short((int16u *)&m_ptr->disarm);
      rd_short((int16u *)&m_ptr->save);
      rd_short((int16u *)&m_ptr->sc);
      rd_short((int16u *)&m_ptr->stl);
      rd_byte(&m_ptr->pclass);
      rd_byte(&m_ptr->prace);
      rd_byte(&m_ptr->hitdie);
      rd_byte(&m_ptr->expfact);
      rd_short((int16u *)&m_ptr->cmana);
      rd_short(&m_ptr->cmana_frac);
      rd_short((int16u *)&m_ptr->chp);
      rd_short(&m_ptr->chp_frac);
      for (i = 0; i < 4; i++) rd_string(m_ptr->history[i]);

      s_ptr = &py.stats;
      rd_bytes(s_ptr->max_stat, 6);
      rd_bytes(s_ptr->cur_stat, 6);
      rd_shorts((int16u *)s_ptr->mod_stat, 6);
      rd_bytes(s_ptr->use_stat, 6);

      f_ptr = &py.flags;
      rd_long(&f_ptr->status);
      rd_short((int16u *)&f_ptr->rest);
      rd_short((int16u *)&f_ptr->blind);
      rd_short((int16u *)&f_ptr->paralysis);
      rd_short((int16u *)&f_ptr->confused);
      rd_short((int16u *)&f_ptr->food);
      rd_short((int16u *)&f_ptr->food_digested);
      rd_short((int16u *)&f_ptr->protection);
      rd_short((int16u *)&f_ptr->speed);
      rd_short((int16u *)&f_ptr->fast);
      rd_short((int16u *)&f_ptr->slow);
      rd_short((int16u *)&f_ptr->afraid);
      rd_short((int16u *)&f_ptr->poisoned);
      rd_short((int16u *)&f_ptr->image);
      rd_short((int16u *)&f_ptr->protevil);
      rd_short((int16u *)&f_ptr->invuln);
      rd_short((int16u *)&f_ptr->hero);
      rd_short((int16u *)&f_ptr->shero);
      rd_short((int16u *)&f_ptr->blessed);
      rd_short((int16u *)&f_ptr->resist_heat);
      rd_short((int16u *)&f_ptr->resist_cold);
      rd_short((int16u *)&f_ptr->detect_inv);
      rd_short((int16u *)&f_ptr->word_recall);
      rd_short((int16u *)&f_ptr->see_infra);
      rd_short((int16u *)&f_ptr->tim_infra);
      rd_byte(&f_ptr->see_inv);
      rd_byte(&f_ptr->teleport);
      rd_byte(&f_ptr->free_act);
      rd_byte(&f_ptr->slow_digest);
      rd_byte(&f_ptr->aggravate);
      rd_byte(&f_ptr->fire_resist);
      rd_byte(&f_ptr->cold_resist);
      rd_byte(&f_ptr->acid_resist);
      rd_byte(&f_ptr->regenerate);
      rd_byte(&f_ptr->lght_resist);
      rd_byte(&f_ptr->ffall);
      rd_byte(&f_ptr->sustain_str);
      rd_byte(&f_ptr->sustain_int);
      rd_byte(&f_ptr->sustain_wis);
      rd_byte(&f_ptr->sustain_con);
      rd_byte(&f_ptr->sustain_dex);
      rd_byte(&f_ptr->sustain_chr);
      rd_byte(&f_ptr->confuse_monster);
      rd_byte(&f_ptr->new_spells);

      rd_short((int16u *)&missile_ctr);
      rd_long((int32u *)&turn);
      rd_short((int16u *)&inven_ctr);
      if (inven_ctr > INVEN_WIELD) goto error;
      for (i = 0; i < inven_ctr; i++) rd_item(&inventory[i]);
      for (i = INVEN_WIELD; i < INVEN_ARRAY_SIZE; i++)
        rd_item(&inventory[i]);
      rd_short((int16u *)&inven_weight);
      rd_short((int16u *)&equip_ctr);
      rd_long(&spell_learned);
      rd_long(&spell_worked);
      rd_long(&spell_forgotten);
      rd_bytes(spell_order, 32);
      rd_bytes(object_ident, OBJECT_IDENT_SIZE);
      rd_long(&randes_seed);
      rd_long(&town_seed);
      rd_short((int16u *)&last_msg);
      for (i = 0; i < MAX_SAVE_MSG; i++) rd_string(old_msg[i]);

      rd_short((int16u *)&int16u_tmp);
      panic_save = !!int16u_tmp;
      rd_short((int16u *)&total_winner);
      rd_short((int16u *)&noscore);
      rd_shorts(player_hp, MAX_PLAYER_LEVEL);

      if (ver_ge(version_maj, version_min, patch_level, 5, 1, 3))
      {
        for (i = 0; i < MAX_STORES; i++)
        {
          st_ptr = &store[i];
          rd_long((int32u *)&st_ptr->store_open);
          rd_short((int16u *)&st_ptr->insult_cur);
          rd_byte(&st_ptr->owner);
          rd_byte(&st_ptr->store_ctr);
          rd_short(&st_ptr->good_buy);
          rd_short(&st_ptr->bad_buy);
          if (st_ptr->store_ctr > STORE_INVEN_MAX) goto error;
          for (j = 0; j < st_ptr->store_ctr; j++)
          {
            rd_long((int32u *)&st_ptr->store_inven[j].scost);
            rd_item(&st_ptr->store_inven[j].sitem);
          }
        }
      }

      /* read the time that the file was saved */
      if (ver_ge(version_maj, version_min, patch_level, 5, 1, 3))
        rd_long(&time_saved);

      if (ver_ge(version_maj, version_min, patch_level, 5, 2, 0))
        rd_string(died_from);

      max_score = 0;
      if (ver_ge(version_maj, version_min, patch_level, 5, 2, 2))
      {
        rd_long ((int32u *)&max_score);
      }

      birth_date = time((time_t *)0);
      if (ver_ge(version_maj, version_min, patch_level, 5, 2, 2))
        rd_long ((int32u *)&birth_date);
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

    rd_short((int16u *)&dun_level);
    rd_short((int16u *)&char_row);
    rd_short((int16u *)&char_col);
    rd_short((int16u *)&mon_tot_mult);
    rd_short((int16u *)&cur_height);
    rd_short((int16u *)&cur_width);
    rd_short((int16u *)&max_panel_rows);
    rd_short((int16u *)&max_panel_cols);

    /* read in the creature ptr info */
    rd_byte(&char_tmp);
    while (char_tmp != 0xFF)
    {
      ychar = char_tmp;
      rd_byte(&xchar);
      rd_byte(&char_tmp);
      if (xchar > MAX_WIDTH || ychar > MAX_HEIGHT) goto error;
      cave[ychar][xchar].cptr = char_tmp;
      rd_byte(&char_tmp);
    }
    /* read in the treasure ptr info */
    rd_byte(&char_tmp);
    while (char_tmp != 0xFF)
    {
      ychar = char_tmp;
      rd_byte(&xchar);
      rd_byte(&char_tmp);
      if (xchar > MAX_WIDTH || ychar > MAX_HEIGHT) goto error;
      cave[ychar][xchar].tptr = char_tmp;
      rd_byte(&char_tmp);
    }
    /* read in the rest of the cave info */
    c_ptr = &cave[0][0];
    for (total_count = 0;
         total_count < MAX_HEIGHT*MAX_WIDTH;
         total_count += count)
    {
      rd_byte(&count);
      rd_byte(&char_tmp);
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

    rd_short((int16u *)&tcptr);
    if (tcptr > MAX_TALLOC) goto error;
    for (i = MIN_TRIX; i < tcptr; i++) rd_item(&t_list[i]);
    rd_short((int16u *)&mfptr);
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
        rd_long((int32u *)&st_ptr->store_open);
        rd_short((int16u *)&st_ptr->insult_cur);
        rd_byte(&st_ptr->owner);
        rd_byte(&st_ptr->store_ctr);
        rd_short(&st_ptr->good_buy);
        rd_short(&st_ptr->bad_buy);
        if (st_ptr->store_ctr > STORE_INVEN_MAX) goto error;
        for (j = 0; j < st_ptr->store_ctr; j++)
        {
          rd_long((int32u *)&st_ptr->store_inven[j].scost);
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
      rd_long(&time_saved);
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
    DEBUG(fclose (logfile));

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
        start_time = time((time_t *)0);
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

static void wr_byte(c)
int8u c;
{
  xor_byte ^= c;
  (void) putc((int)xor_byte, fileptr);
  DEBUG(fprintf (logfile, "BYTE:  %02X = %d\n", (int) xor_byte, (int) c));
}

static void wr_short(s)
int16u s;
{
  xor_byte ^= (s & 0xFF);
  (void) putc((int)xor_byte, fileptr);
  DEBUG(fprintf (logfile, "SHORT: %02X", (int) xor_byte));
  xor_byte ^= ((s >> 8) & 0xFF);
  (void) putc((int)xor_byte, fileptr);
  DEBUG(fprintf (logfile, " %02X = %d\n", (int) xor_byte, (int) s));
}

static void wr_long(l)
register int32u l;
{
  xor_byte ^= (l & 0xFF);
  (void) putc((int)xor_byte, fileptr);
  DEBUG(fprintf (logfile, "LONG:  %02X", (int) xor_byte));
  xor_byte ^= ((l >> 8) & 0xFF);
  (void) putc((int)xor_byte, fileptr);
  DEBUG(fprintf (logfile, " %02X", (int) xor_byte));
  xor_byte ^= ((l >> 16) & 0xFF);
  (void) putc((int)xor_byte, fileptr);
  DEBUG(fprintf (logfile, " %02X", (int) xor_byte));
  xor_byte ^= ((l >> 24) & 0xFF);
  (void) putc((int)xor_byte, fileptr);
  DEBUG(fprintf (logfile, " %02X = %ld\n", (int) xor_byte, (long) l));
}

static void wr_bytes(c, count)
int8u *c;
register int count;
{
  register int i;
  register int8u *ptr;

  DEBUG(fprintf (logfile, "%d BYTES:", count));
  ptr = c;
  for (i = 0; i < count; i++)
    {
      xor_byte ^= *ptr++;
      (void) putc((int)xor_byte, fileptr);
      DEBUG(fprintf (logfile, "  %02X = %d", (int) xor_byte,
         (int) (ptr[-1])));
    }
  DEBUG(fprintf (logfile, "\n"));
}

static void wr_string(str)
register char *str;
{
  DEBUG(char *s = str);
  DEBUG(fprintf (logfile, "STRING:"));
  while (*str != '\0')
    {
      xor_byte ^= *str++;
      (void) putc((int)xor_byte, fileptr);
      DEBUG(fprintf (logfile, " %02X", (int) xor_byte));
    }
  xor_byte ^= *str;
  (void) putc((int)xor_byte, fileptr);
  DEBUG(fprintf (logfile, " %02X = \"%s\"\n", (int) xor_byte, s));
}

static void wr_shorts(s, count)
int16u *s;
register int count;
{
  register int i;
  register int16u *sptr;

  DEBUG(fprintf (logfile, "%d SHORTS:", count));
  sptr = s;
  for (i = 0; i < count; i++)
    {
      xor_byte ^= (*sptr & 0xFF);
      (void) putc((int)xor_byte, fileptr);
      DEBUG(fprintf (logfile, "  %02X", (int) xor_byte));
      xor_byte ^= ((*sptr++ >> 8) & 0xFF);
      (void) putc((int)xor_byte, fileptr);
      DEBUG(fprintf (logfile, " %02X = %d", (int) xor_byte, (int) sptr[-1]));
    }
  DEBUG(fprintf (logfile, "\n"));
}

static void wr_item(item)
register inven_type *item;
{
  DEBUG(fprintf (logfile, "ITEM:\n"));
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

static void wr_monster(mon)
register monster_type *mon;
{
  DEBUG(fprintf (logfile, "MONSTER:\n"));
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

static void rd_byte(ptr)
int8u *ptr;
{
  int8u c;

  c = getc(fileptr) & 0xFF;
  *ptr = c ^ xor_byte;
  xor_byte = c;
  DEBUG(fprintf (logfile, "BYTE:  %02X = %d\n", (int) c, (int) *ptr));
}

static void rd_short(ptr)
int16u *ptr;
{
  int8u c;
  int16u s;

  c = (getc(fileptr) & 0xFF);
  s = c ^ xor_byte;
  xor_byte = (getc(fileptr) & 0xFF);
  s |= (int16u)(c ^ xor_byte) << 8;
  *ptr = s;
  DEBUG(fprintf (logfile, "SHORT: %02X %02X = %d\n", (int) c, (int) xor_byte,\
     (int) s));
}

static void rd_long(ptr)
int32u *ptr;
{
  register int32u l;
  register int8u c;

  c = (getc(fileptr) & 0xFF);
  l = c ^ xor_byte;
  xor_byte = (getc(fileptr) & 0xFF);
  l |= (int32u)(c ^ xor_byte) << 8;
  DEBUG(fprintf (logfile, "LONG:  %02X %02X ", (int) c, (int) xor_byte));
  c = (getc(fileptr) & 0xFF);
  l |= (int32u)(c ^ xor_byte) << 16;
  xor_byte = (getc(fileptr) & 0xFF);
  l |= (int32u)(c ^ xor_byte) << 24;
  *ptr = l;
  DEBUG(fprintf (logfile, "%02X %02X = %ld\n", (int) c, (int) xor_byte,\
     (long) l));
}

static void rd_bytes(ch_ptr, count)
int8u *ch_ptr;
register int count;
{
  register int i;
  register int8u *ptr;
  register int8u c;

  DEBUG(fprintf (logfile, "%d BYTES:", count));
  ptr = ch_ptr;
  for (i = 0; i < count; i++)
    {
      c = (getc(fileptr) & 0xFF);
      *ptr++ = c ^ xor_byte;
      xor_byte = c;
      DEBUG(fprintf (logfile, "  %02X = %d", (int) c, (int) ptr[-1]));
    }
  DEBUG(fprintf (logfile, "\n"));
}

static void rd_string(str)
char *str;
{
  register int8u c;

  DEBUG(char *s = str);
  DEBUG(fprintf (logfile, "STRING: "));
  do
    {
      c = (getc(fileptr) & 0xFF);
      *str = c ^ xor_byte;
      xor_byte = c;
      DEBUG(fprintf (logfile, "%02X ", (int) c));
    }
  while (*str++ != '\0');
  DEBUG(fprintf (logfile, "= \"%s\"\n", s));
}

static void rd_shorts(ptr, count)
int16u *ptr;
register int count;
{
  register int i;
  register int16u *sptr;
  register int16u s;
  int8u c;

  DEBUG(fprintf (logfile, "%d SHORTS:", count));
  sptr = ptr;
  for (i = 0; i < count; i++)
    {
      c = (getc(fileptr) & 0xFF);
      s = c ^ xor_byte;
      xor_byte = (getc(fileptr) & 0xFF);
      s |= (int16u)(c ^ xor_byte) << 8;
      *sptr++ = s;
      DEBUG(fprintf (logfile, "  %02X %02X = %d", (int) c, (int) xor_byte,\
         (int) s));
    }
  DEBUG(fprintf (logfile, "\n"));
}

static void rd_item(item)
register inven_type *item;
{
  DEBUG(fprintf (logfile, "ITEM:\n"));
  rd_short(&item->index);
  rd_byte(&item->name2);
  rd_string(item->inscrip);
  rd_long(&item->flags);
  rd_byte(&item->tval);
  rd_byte(&item->tchar);
  rd_short((int16u *)&item->p1);
  rd_long((int32u *)&item->cost);
  rd_byte(&item->subval);
  rd_byte(&item->number);
  rd_short(&item->weight);
  rd_short((int16u *)&item->tohit);
  rd_short((int16u *)&item->todam);
  rd_short((int16u *)&item->ac);
  rd_short((int16u *)&item->toac);
  rd_bytes(item->damage, 2);
  rd_byte(&item->level);
  rd_byte(&item->ident);
}

static void rd_monster(mon)
register monster_type *mon;
{
  DEBUG(fprintf (logfile, "MONSTER:\n"));
  rd_short((int16u *)&mon->hp);
  rd_short((int16u *)&mon->csleep);
  rd_short((int16u *)&mon->cspeed);
  rd_short(&mon->mptr);
  rd_byte(&mon->fy);
  rd_byte(&mon->fx);
  rd_byte(&mon->cdis);
  rd_byte(&mon->ml);
  rd_byte(&mon->stunned);
  rd_byte(&mon->confused);
}

/* functions called from death.c to implement the score file */

/* set the local fileptr to the scorefile fileptr */
void set_fileptr(file)
FILE *file;
{
  fileptr = file;
}

void wr_highscore(score)
high_scores *score;
{
  DEBUG(logfile = fopen ("IO_LOG", "a"));
  DEBUG(fprintf (logfile, "Saving score:\n"));
  /* Save the encryption byte for robustness.  */
  wr_byte(xor_byte);

  wr_long((int32u) score->points);
  wr_long((int32u) score->birth_date);
  wr_short((int16u) score->uid);
  wr_short((int16u) score->mhp);
  wr_short((int16u) score->chp);
  wr_byte(score->dun_level);
  wr_byte(score->lev);
  wr_byte(score->max_dlv);
  wr_byte(score->sex);
  wr_byte(score->race);
  wr_byte(score->class);
  wr_bytes((int8u *)score->name, PLAYER_NAME_SIZE);
  wr_bytes((int8u *)score->died_from, 25);
  DEBUG(fclose (logfile));
}

void rd_highscore(score)
high_scores *score;
{
  DEBUG(logfile = fopen ("IO_LOG", "a"));
  DEBUG(fprintf (logfile, "Reading score:\n"));
  /* Read the encryption byte.  */
  rd_byte (&xor_byte);

  rd_long((int32u *)&score->points);
  rd_long((int32u *)&score->birth_date);
  rd_short((int16u *)&score->uid);
  rd_short((int16u *)&score->mhp);
  rd_short((int16u *)&score->chp);
  rd_byte(&score->dun_level);
  rd_byte(&score->lev);
  rd_byte(&score->max_dlv);
  rd_byte(&score->sex);
  rd_byte(&score->race);
  rd_byte(&score->class);
  rd_bytes((int8u *)score->name, PLAYER_NAME_SIZE);
  rd_bytes((int8u *)score->died_from, 25);
  DEBUG(fclose (logfile));
}
