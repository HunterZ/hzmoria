/* source/death.c: code executed when player dies

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
#include "externs.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef SYS_V
#include <sys/types.h>
#endif

#ifdef MSDOS
#include <io.h>
# ifdef __MINGW32__
#  include <unistd.h> /* sleep() */
# endif
#else /* Linux etc. */
# include <pwd.h>
#endif

static void date(char *);
static char *center_string(char *, char const *const);
static void print_tomb(void);
static bool similar_score(high_scores const *const,
                          high_scores const *const);
static void kingly(void);

static void date(char *day)
{
  const time_t clockvar = time(NULL);
  char const *const tmp = ctime(&clockvar);
  /* Legacy code wrote a null terminator to the ctime() result, but this
     seems bad. Use strncpy() instead, then write null to 'day' -BS-
  tmp[10] = '\0';
  strcpy(day, tmp);
  */
  strncpy(day, tmp, 10);
  day[10] = '\0';
}

/* Centers a string within a 31 character string  -JWT-  */
static char *center_string(char *centered_str, char const *const in_str)
{
  const int i = strlen(in_str);
  const int j = 15 - (i / 2);

  sprintf(centered_str, "%*s%s%*s", j, "", in_str, 31 - i - j, "");
  return centered_str;
}

#if !defined(__CYGWIN__) && !defined(MSDOS)

#include <sys/stat.h>
#include <errno.h>

/* The following code is provided especially for systems which  -CJS-
   have no flock system call. It has never been tested.  */

/* DEBIAN_LINUX defined because fcntlbits.h defines EX and SH the       -RJW-
 * other way.  The comment below indicates that they're not
 * distinguished anyways, so this should be harmless, and this does
 * seem to be the prevailing order (c.f. IRIX 6.5) but just in case,
 * they've been ifdef'ed. */

#ifdef DEBIAN_LINUX
  #define LOCK_SH 1
  #define LOCK_EX 2
#else
  #define LOCK_EX 1
  #define LOCK_SH 2
#endif
#define LOCK_NB 4
#define LOCK_UN 8

/* An flock HACK.  LOCK_SH and LOCK_EX are not distinguished.  DO NOT release
   a lock which you failed to set!  ALWAYS release a lock you set! */
static int flock(const int f, const int l)
{
  struct stat sbuf;
  char lockname[80];

  if (fstat (f, &sbuf) < 0) return -1;
  sprintf(lockname, "/tmp/moria.%ld", (long)sbuf.st_ino);
  if (l & LOCK_UN) return unlink(lockname);

  while (open(lockname, O_WRONLY|O_CREAT|O_EXCL, 0644) < 0)
  {
    if (errno != EEXIST) return -1;
    if (stat(lockname, &sbuf) < 0) return -1;
    /* Locks which last more than 10 seconds get deleted. */
    if (time(NULL) - sbuf.st_mtime > 10)
    {
      if (unlink(lockname) < 0) return -1;
    }
    else if (l & LOCK_NB)
    {
      return -1;
    }
    else
    {
      sleep(1);
    }
  }
  return 0;
}
#endif

void display_scores(const bool player_only)
{
  register int i, rank;
  high_scores score;
  char input;
  char string[100];
  int8u version_maj, version_min, patch_level;
#if defined(unix)
  int16 player_uid;
#endif

#if defined(MSDOS)
  if ((highscore_fp = fopen(MORIA_TOP, "rb")) == NULL)
  {
    sprintf (string, "Error opening score file \"%s\"\n", MORIA_TOP);
    msg_print(string);
    msg_print(CNIL);
    return;
  }
#endif

  fseek(highscore_fp, 0L, SEEK_SET);

  /* Read version numbers from the score file, and check for validity. */
  version_maj = getc(highscore_fp);
  version_min = getc(highscore_fp);
  patch_level = getc(highscore_fp);
  if (feof(highscore_fp))
  {
    /* An empty score file. */
  }
  /* Support score files from 5.2.2 to present.
     i.e. close and abort if scores file is older than 5.2.2, or newer than
     current game version -BS- */
  else if (ver_lt(version_maj, version_min, patch_level, 5, 2, 2)
        || ver_lt(CUR_VERSION_MAJ, CUR_VERSION_MIN, PATCH_LEVEL,
                  version_min, version_maj, patch_level))
  {
    msg_print(
      "Sorry. This scorefile is from an incompatible version of hzmoria.");
    msg_print(CNIL);
#if defined(MSDOS)
    fclose(highscore_fp);
#endif
    return;
  }

#ifdef unix
  player_uid = getuid();
#endif

  rank = 1;
  rd_highscore(highscore_fp, &score);
  while (!feof(highscore_fp))
  {
    clear_screen();
    /* Put twenty scores on each page, on lines 2 through 21. */
    for (i = 1; i < 21 && !feof(highscore_fp);
         rd_highscore(highscore_fp, &score))
    {
#ifdef unix
      /* Only show the entry if player_only false, or if the entry belongs
         to the current player. */
      if (!player_only || score.uid == player_uid)
#endif
      {
        sprintf(string,
                "%-4d%8ld %-19.19s %c %-10.10s %-7.7s%3d %-22.22s",
                rank, score.points, score.name, score.sex,
                race[score.race].trace, class[score.class].title,
                score.lev, score.died_from);
        prt(string, ++i, 0);
      }
      rank++;
    }
    prt(
      "Rank  Points Name              Sex Race       Class  Lvl Killed By",
      0, 0);
      erase_line (1, 0);
      prt("[Press any key to continue.]", 23, 23);
      input = inkey();
      if (input == ESCAPE) break;
  }
#if defined(MSDOS)
  fclose(highscore_fp);
#endif
}

bool duplicate_character()
{
  /* Only check for duplicate characters under unix */
#ifdef unix
  high_scores score;
  int8u version_maj, version_min, patch_level;
  int16 player_uid;

  fseek(highscore_fp, 0L, SEEK_SET);

  /* Read version numbers from the score file, and check for validity.  */
  version_maj = getc(highscore_fp);
  version_min = getc(highscore_fp);
  patch_level = getc(highscore_fp);
  /* Support score files from 5.2.2 to present. */
  if (feof(highscore_fp)) return false; /* An empty score file. */
  if (ver_lt(version_maj, version_min, patch_level, 5, 2, 2))
  {
    msg_print(
      "Sorry. This scorefile is from an incompatible version of hzmoria.");
    msg_print (CNIL);
    return false;
  }

  /* set the static fileptr in save.c to the highscore file pointer */
  set_fileptr(highscore_fp);

  player_uid = getuid();

  for (rd_highscore(&score); !feof(highscore_fp); rd_highscore(&score))
  {
    if (score.uid == player_uid && score.birth_date == birth_date
        && score.class == py.misc.pclass && score.race == py.misc.prace
        && score.sex == (py.misc.male ? 'M' : 'F')
        && strcmp (score.died_from, "(saved)")) return true;
  }
#endif /* unix end */
  return false;
}

/* Prints the gravestone of the character -RAK- */
static void print_tomb()
{
  vtype str, tmp_str;
  int i;
  char day[11];
  char *p;

  clear_screen();
  put_buffer("_______________________", 1, 15);
  put_buffer("/", 2, 14);
  put_buffer("\\         ___", 2, 38);
  put_buffer("/", 3, 13);
  put_buffer("\\ ___   /   \\      ___", 3, 39);
  put_buffer("/            RIP            \\   \\  :   :     /   \\", 4, 12);
  put_buffer("/", 5, 11);
  put_buffer("\\  : _;,,,;_    :   :", 5, 41);
  sprintf(str, "/%s\\,;_          _;,,,;_",
          center_string(tmp_str, py.misc.name));
  put_buffer(str, 6, 10);
  put_buffer("|               the               |   ___", 7, 9);
  p = total_winner ? "Magnificent" : title_string();
  sprintf(str, "| %s |  /   \\", center_string(tmp_str, p));
  put_buffer(str, 8, 9);
  put_buffer("|", 9, 9);
  put_buffer("|  :   :", 9, 43);
  p = total_winner ? (py.misc.male ? "*King*" : "*Queen*")
                   : class[py.misc.pclass].title;
  sprintf(str,"| %s | _;,,,;_   ____", center_string(tmp_str, p));
  put_buffer(str, 10, 9);
  sprintf(str, "Level : %d", (int) py.misc.lev);
  sprintf(str,"| %s |          /    \\", center_string(tmp_str, str));
  put_buffer(str, 11, 9);
  sprintf(str, "%ld Exp", py.misc.exp);
  sprintf(str,"| %s |          :    :", center_string(tmp_str, str));
  put_buffer(str, 12, 9);
  sprintf(str, "%ld Au", py.misc.au);
  sprintf(str,"| %s |          :    :", center_string(tmp_str, str));
  put_buffer(str, 13, 9);
  sprintf(str, "Died on Level : %d", dun_level);
  sprintf(str,"| %s |         _;,,,,;_", center_string(tmp_str, str));
  put_buffer(str, 14, 9);
  put_buffer("|            killed by            |", 15, 9);
  p = died_from;
  i = strlen(p);
  p[i] = '.';  /* add a trailing period */
  p[i+1] = '\0';
  sprintf(str, "| %s |", center_string(tmp_str, p));
  put_buffer(str, 16, 9);
  p[i] = '\0';  /* strip off the period */
  date(day);
  sprintf(str, "| %s |", center_string(tmp_str, day));
  put_buffer(str, 17, 9);
  put_buffer("*|   *     *     *    *   *     *  | *", 18, 8);
  put_buffer(
    "________)/\\\\_)_/___(\\/___(//_\\)/_\\//__\\\\(/_|_)_______", 19, 0);

  do
  {
    flush();
    put_buffer(
      "(ESC to abort, return to print on screen, or file name)", 23, 0);
    put_buffer("Character record?", 22, 0);
    /* return on ESC */
    if (!get_string(str, 22, 18, 60)) return;
    for (i = 0; i < INVEN_ARRAY_SIZE; i++)
    {
      known1(&inventory[i]);
      known2(&inventory[i]);
    }
    calc_bonuses();
    /* print to screen and return on enter (empty string) */
    if (!str[0])
    {
      clear_screen();
      display_char();
      put_buffer("Type ESC to skip the inventory:", 23, 0);
      if (inkey() == ESCAPE) return;
      clear_screen();
      msg_print("You are using:");
      show_equip(true, 0);
      msg_print(CNIL);
      msg_print("You are carrying:");
      clear_from(1);
      show_inven(0, inven_ctr - 1, true, 0, CNIL);
      msg_print(CNIL);
      return;
    }
  } while (!file_character(str));
}

/* Calculates the total number of points earned -JWT- */
int32 total_points()
{
  int32 total;
  int i;

  /* basic scoring algorithm appears to be as follows:
      1 point per max XP earned
      100 points per maximum dungeon level reached
      1 point per 100 gold carried
      50 points per current dungeon level at time of death
      1 point per gold value of each carried item?
      -BS-
  */
  total = py.misc.max_exp
        + (py.misc.max_dlv * 100)
        + (py.misc.au / 100)
        + (dun_level * 50);
  for (i = 0; i < INVEN_ARRAY_SIZE; i++)
    total += item_value(&inventory[i]);

  /* Don't ever let the score decrease from one save to the next. */
  if (total < max_score) total = max_score;

  return total;
}

/* returns true if scores new_entry should not be added to scores file
   based on similarity to old_entry -BS- */
static bool similar_score(high_scores const *const new_entry,
                          high_scores const *const old_entry)
{
  /* under unix, only allow one sex/race/class combo per person. On
      single user systems, allow any number of entries, but try to prevent
      multiple entries per character by checking for case when
      birthdate/sex/race/class are the same, and died_from of scorefile
      entry is "(saved)" */
  /* unix: same user ID */
  const bool same_owner =
    (new_entry->uid != 0 && new_entry->uid == old_entry->uid);
  /* non-unix: died_from is "(saved)" */
  const bool saved =
    (new_entry->uid == 0 && !strcmp(old_entry->died_from, "(saved)"));
  /* non-unix: same birthdate */
  const bool same_birthdate =
    (new_entry->birth_date == old_entry->birth_date);
  /* all environments: same unique attributes */
  const bool same_combo =
    (
         new_entry->sex   == old_entry->sex
      && new_entry->race  == old_entry->race
      && new_entry->class == old_entry->class
    );

  return (same_owner || (saved && same_birthdate)) && same_combo;
}

/* Enters a players name on the top twenty list -JWT- */
static void highscores()
{
  high_scores old_entry, new_entry, entry;
  int i;
  char *tmp;
  int8u version_maj = 0, version_min = 0, patch_level = 0;
  long curpos;
#ifdef MSDOS
  char string[100];
#endif

  clear_screen();

  if (noscore) return;

  if (panic_save)
  {
    msg_print("Sorry, scores for games restored from panic save files " \
              "are not saved.");
    return;
  }

  new_entry.points     = total_points();
  new_entry.birth_date = birth_date;
#ifdef unix
  new_entry.uid        = getuid();
#else
  new_entry.uid        = 0;
#endif
  new_entry.mhp        = py.misc.mhp;
  new_entry.chp        = py.misc.chp;
  new_entry.dun_level  = dun_level;
  new_entry.lev        = py.misc.lev;
  new_entry.max_dlv    = py.misc.max_dlv;
  new_entry.sex        = (py.misc.male ? 'M' : 'F');
  new_entry.race       = py.misc.prace;
  new_entry.class      = py.misc.pclass;
  /* strcpy() is safe here because both fields are the same size -BS- */
  strcpy(new_entry.name, py.misc.name);
  /* fast forward past "a/an" and any spaces in 'died_from' -BS- */
  tmp = died_from;
  if ('a' == *tmp)
  {
    if ('n' == *(++tmp))
    {
      tmp++;
    }
    while (isspace(*tmp))
    {
      tmp++;
    }
  }
  /* use strncpy() because dest buffer is smaller than src one -BS- */
  strncpy(new_entry.died_from, tmp, DIED_FROM_SIZE - 1);
  new_entry.died_from[DIED_FROM_SIZE - 1] = '\0';

  /* First, get a lock on the high score file so no-one else tries to write
     to it while we are using it. Nn PCs only one process can have the file
     open at a time, so we just open it here */
#ifdef MSDOS
  if ((highscore_fp = fopen(MORIA_TOP, "rb+")) == NULL)
  {
    sprintf(string, "Error opening score file \"%s\"\n", MORIA_TOP);
    msg_print(string);
    msg_print(CNIL);
    return;
  }
#else
  if (0 != flock((int)fileno(highscore_fp), LOCK_EX))
  {
    msg_print("Error locking score file");
    msg_print(CNIL);
    return;
  }
#endif

  /* Search file to find where to insert this character, if uid != 0 and
     find same uid/sex/race/class combo then exit without saving this score
     Also, seek to the beginning of the file just to be safe. */
  fseek(highscore_fp, 0L, SEEK_SET);

  /* Read version numbers from the score file, and check for validity. */
  version_maj = getc(highscore_fp);
  version_min = getc(highscore_fp);
  patch_level = getc(highscore_fp);
  /* If this is a new scorefile, it should be empty. Write the current
     version numbers to the score file. */
  if (feof(highscore_fp))
  {
    /* Seek to the beginning of the file just to be safe. */
    fseek(highscore_fp, 0L, SEEK_SET);

    putc(CUR_VERSION_MAJ, highscore_fp);
    putc(CUR_VERSION_MIN, highscore_fp);
    putc(PATCH_LEVEL,     highscore_fp);

    /* must fseek() before can change read/write mode */
    fseek(highscore_fp, 0L, SEEK_CUR);
  }
  /* Support score files from 5.2.2 to present.
     i.e. close and abort if scores file is older than 5.2.2, or newer than
     current game version -BS- */
  else if (ver_lt(version_maj, version_min, patch_level, 5, 2, 2)
        || ver_lt(CUR_VERSION_MAJ, CUR_VERSION_MIN, PATCH_LEVEL,
                  version_min, version_maj, patch_level))
  {
    /* No need to print a message, a subsequent call to display_scores()
       will print a message. */
#ifdef MSDOS
    fclose(highscore_fp);
#endif
    return;
  }

  /* scan existing score file entries until either EOF is reached, or an
     existing score is found that is matched/exceeded by new score. Abort
     if higher score for similar character exists, or if score file is full
     of higher-scoring characters -BS- */
  i = 0;
  curpos = ftell(highscore_fp);
  for (rd_highscore(highscore_fp, &old_entry);
       !feof(highscore_fp) && (new_entry.points < old_entry.points);
       rd_highscore(highscore_fp, &old_entry))
  {
    /* abort if similar character with higher score already recorded */
    if (similar_score(&new_entry, &old_entry))
    {
#ifdef MSDOS
      fclose(highscore_fp);
#endif
      return;
    }
    /* only allow SCOREFILE_SIZE scores in the score file */
    if (++i >= SCOREFILE_SIZE)
    {
#ifdef MSDOS
      fclose(highscore_fp);
#endif
      return;
    }
    curpos = ftell(highscore_fp);
  }

  if (feof(highscore_fp))
  {
    /* reached end of file with space left; simply append new entry */
    fseek(highscore_fp, curpos, SEEK_SET);
    wr_highscore(highscore_fp, &new_entry);
  }
  else
  {
    /* Write new_entry into old_entry's slot, then push the rest of the
       entries in the file down a slot. This is done by rewinding to the
       start of the last-read slot, overwriting it with cached data, then
       reading the next slot's data into the cache, then repeating.
       Optimization: If an entry similar to new_entry is encountered, abort
       after overwriting it to effectively bump it off the scoreboard -BS-
    */
    entry = new_entry;
    while (!feof(highscore_fp))
    {
      /* old_entry already contains the to-be-overwritten slot's data, so
         rewind the file I/O stream to that slot's start location -BS- */
      fseek(highscore_fp,
            -(long)(sizeof(high_scores) + sizeof(char)),
            SEEK_CUR);
      /* write 'entry' to current location, overwriting whatever was there
         previously. This also advances the file pointer one record -BS- */
      wr_highscore(highscore_fp, &entry);
      /* if similar character similar to new_entry was just overwritten,
         just abort to effectively replace its scoreboard entry */
      if (similar_score(&new_entry, &old_entry)) break;
      /* save off the most recently-read record for writing into the next
         slot -BS- */
      entry = old_entry;
      /* must fseek() before switching from writing to reading */
      fseek(highscore_fp, 0L, SEEK_CUR);
      curpos = ftell(highscore_fp);
      /* read into 'old_entry' the record currently in the slot *after* the
         one which was just overwritten -BS- */
      rd_highscore(highscore_fp, &old_entry);
    }
    /* if all slots read, overwrite last one with last cached data */
    if (feof(highscore_fp))
    {
      fseek(highscore_fp, curpos, SEEK_SET);
      wr_highscore(highscore_fp, &entry);
    }
  }

#ifdef MSDOS
  fclose(highscore_fp);
#else
  flock((int)fileno(highscore_fp), LOCK_UN);
#endif
}

/* Change the player into a sovereign! -RAK- */
static void kingly()
{
  register struct misc *p_ptr;
  register char *p;

  /* Change the character attributes. */
  dun_level = 0;
  strcpy(died_from, "Ripe Old Age");
  p_ptr = &py.misc;
  restore_level();
  p_ptr->lev     += MAX_PLAYER_LEVEL;
  p_ptr->au      += 250000L;
  p_ptr->max_exp += 5000000L;
  p_ptr->exp      = p_ptr->max_exp;

  /* Let the player know that they did good. */
  clear_screen();
  put_buffer("#", 1, 34);
  put_buffer("#####", 2, 32);
  put_buffer("#", 3, 34);
  put_buffer(",,,  $$$  ,,,", 4, 28);
  put_buffer(",,=$   \"$$$$$\"   $=,,", 5, 24);
  put_buffer(",$$        $$$        $$,", 6, 22);
  put_buffer("*>         <*>         <*", 7, 22);
  put_buffer("$$         $$$         $$", 8, 22);
  put_buffer("\"$$        $$$        $$\"", 9, 22);
  put_buffer("\"$$       $$$       $$\"", 10, 23);
  p = "*#########*#########*";
  put_buffer(p, 11, 24);
  put_buffer(p, 12, 24);
  put_buffer("Veni, Vidi, Vici!", 15, 26);
  put_buffer("I came, I saw, I conquered!", 16, 21);
  if (p_ptr->male)
    put_buffer("All Hail the Mighty King!", 17, 22);
  else
    put_buffer("All Hail the Mighty Queen!", 17, 22);
  flush();
  pause_line(23);
}

/* Handles the gravestone end top-twenty routines -RAK- */
void exit_game()
{
  /* What happens upon dying. -RAK- */
  msg_print(CNIL);
  flush();     /* flush all input */
  nosignals(); /* Can't interrupt or suspend. */
  /* If the game has been saved, then save sets turn back to -1, which
     inhibits the printing of the tomb. */
  if (turn >= 0)
  {
    if (total_winner) kingly();
    print_tomb();
  }
  /* Save the memory at least. */
  if (character_generated && !character_saved) save_char();
  /* add score to scorefile if applicable */
  if (character_generated)
  {
    /* Clear character_saved, strange thing to do, but it prevents inkey()
       from recursively calling exit_game() when an eof has been detected
       on stdin. */
    character_saved = false;
    highscores();
    display_scores(true);
  }
  erase_line(23, 0);
  restore_term();
  exit(0);
}
