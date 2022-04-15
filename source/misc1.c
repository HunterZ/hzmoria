/* source/misc1.c: misc utility and initialization code, magic objects code

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

#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

/* holds the previous rnd state */
static int32u old_seed;

/* gets a new random seed for the random number generator */
void init_seeds(const int32u seed)
{
  time_t clock_var = (seed == 0 ? time(NULL) : seed);
  randes_seed = (int32u)clock_var;

  clock_var += 8762;
  town_seed = (int32)clock_var;

  clock_var += 113452L;
  set_rnd_seed(clock_var);
  /* make it a little more random */
  for (clock_var = randint(100); clock_var >= 0; clock_var--) rnd();
}

/* change to different random number generator state */
void set_seed(const int32u seed)
{
  old_seed = get_rnd_seed();

  /* want reproducible state here */
  set_rnd_seed(seed);
}

/* restore the normal random generator state */
void reset_seed()
{
  set_rnd_seed(old_seed);
}

/* Check the day-time strings to see if open -RAK- */
bool check_time()
{
#ifdef MORIA_HOU
  long clock_var;
  struct tm *tp;

  clock_var = time((time_t *)0);
  tp = localtime(&clock_var);
  return (days[tp->tm_wday][tp->tm_hour+4] == 'X');
#else
  return true;
#endif
}

/* Generates a random integer x where 1<=X<=MAXVAL -RAK- */
int randint(const int maxval)
{
  return ((int)(rnd() % maxval) + 1);
}

/* Generates a random integer number of NORMAL distribution -RAK-*/
int randnor(const int mean, const int stand)
{
  int tmp, offset, low, iindex, high;
#if 0
  /* alternate randnor code, slower but much smaller since no table */
  /* 2 per 1,000,000 will be > 4*SD, max is 5*SD */
  tmp = damroll(8, 99);   /* mean 400, SD 81 */
  tmp = (tmp - 400) * stand / 81;
  return tmp + mean;
#endif
  tmp = randint(MAX_SHORT);

  /* off scale, assign random value between 4 and 5 times SD */
  if (tmp == MAX_SHORT)
  {
    offset = 4 * stand + randint(stand);
    /* one half are negative */
    if (randint(2) == 1) offset = -offset;
    return mean + offset;
  }

  /* binary search normal normal_table to get index that matches tmp */
  /* this takes up to 8 iterations */
  low    = 0;
  iindex = NORMAL_TABLE_SIZE >> 1;
  high   = NORMAL_TABLE_SIZE;
  while ((normal_table[iindex] != tmp) && (high != (low + 1)))
  {
    if (normal_table[iindex] > tmp)
    {
      high = iindex;
      iindex = low + ((iindex - low) >> 1);
    }
    else
    {
      low = iindex;
      iindex = iindex + ((high - iindex) >> 1);
    }
  }

  /* might end up one below target, check that here */
  if (normal_table[iindex] < tmp) ++iindex;

  /* normal_table is based on SD of 64, so adjust the index value here,
     round the half way case up */
  offset = ((stand * iindex) + (NORMAL_TABLE_SD >> 1)) / NORMAL_TABLE_SD;

  /* one half should be negative */
  if (randint(2) == 1) offset = -offset;

  return mean + offset;
}

/* Returns position of first set bit and clears that bit -RAK- */
int bit_pos(int32u *const test)
{
  int i;
  int32u mask = 0x1;

  for (i = 0; i < sizeof(*test)*8; i++)
  {
    if (*test & mask)
    {
      *test &= ~mask;
      return(i);
    }
    mask <<= 1;
  }

  /* no one bits found */
  return(-1);
}

/* Checks a co-ordinate for in bounds status -RAK-
   This is in regards to the current dungeon level -BS- */
bool in_bounds(const int y, const int x)
{
  return ((y > 0) && (y < cur_height - 1)
       && (x > 0) && (x < cur_width  - 1));
}

/* Calculates current boundaries -RAK- */
void panel_bounds()
{
  panel_row_min = panel_row     * (SCREEN_HEIGHT / 2);
  panel_row_max = panel_row_min + SCREEN_HEIGHT - 1;
  panel_row_prt = panel_row_min - 1;
  panel_col_min = panel_col     * (SCREEN_WIDTH / 2);
  panel_col_max = panel_col_min + SCREEN_WIDTH - 1;
  panel_col_prt = panel_col_min - 13;
}

/* Given an row (y) and col (x), this routine detects when a move off the
   screen has occurred and figures new borders. Force forcses the panel
   bounds to be recalculated, useful for 'W'here. -RAK- */
bool get_panel(const int y, const int x, const int force)
{
  int prow, pcol;
  bool panel;

  prow = panel_row;
  pcol = panel_col;
  if (force || (y < panel_row_min + 2) || (y > panel_row_max - 2))
  {
    prow = ((y - SCREEN_HEIGHT / 4) / (SCREEN_HEIGHT / 2));
    if (prow > max_panel_rows)
    {
      prow = max_panel_rows;
    }
    else if (prow < 0)
    {
      prow = 0;
    }
  }
  if (force || (x < panel_col_min + 3) || (x > panel_col_max - 3))
  {
    pcol = ((x - SCREEN_WIDTH / 4) / (SCREEN_WIDTH / 2));
    if (pcol > max_panel_cols)
    {
      pcol = max_panel_cols;
    }
    else if (pcol < 0)
    {
      pcol = 0;
    }
  }
  panel = ((prow != panel_row) || (pcol != panel_col));
  if (panel)
  {
    panel_row = prow;
    panel_col = pcol;
    panel_bounds();
    /* stop movement if any */
    if (find_bound) end_find();
  }
  return panel;
}

/* Tests a given point to see if it is within the screen boundaries
   -RAK- */
bool panel_contains(const int y, const int x)
{
  return ((y >= panel_row_min) && (y <= panel_row_max)
       && (x >= panel_col_min) && (x <= panel_col_max));
}

/* Distance between two points -RAK- */
int distance(const int y1, const int x1, const int y2, const int x2)
{
  int dy, dx;

  dy = y1 - y2;
  if (dy < 0) dy = -dy;
  dx = x1 - x2;
  if (dx < 0) dx = -dx;

  return ((((dy + dx) << 1) - (dy > dx ? dx : dy)) >> 1);
}

/* Checks points north, south, east, and west for a wall -RAK-
   note that y,x is always in_bounds(), i.e. 0 < y < cur_height-1, and
   0 < x < cur_width-1 */
int next_to_walls(const int y, const int x)
{
  int i = 0;

  if (cave[y-1][x].fval >= MIN_CAVE_WALL) ++i;
  if (cave[y+1][x].fval >= MIN_CAVE_WALL) ++i;
  if (cave[y][x-1].fval >= MIN_CAVE_WALL) ++i;
  if (cave[y][x+1].fval >= MIN_CAVE_WALL) ++i;

  return i;
}

/* Checks all adjacent spots for corridors -RAK-
   note that y, x is always in_bounds(), hence no need to check that
   j, k are in_bounds(), even if they are 0 or cur_x-1 is still works */
int next_to_corr(const int y, const int x)
{
  int k, j, i;
  cave_type *c_ptr;

  i = 0;
  for (j = y - 1; j <= (y + 1); j++)
  {
    for (k = x - 1; k <= (x + 1); k++)
    {
      c_ptr = &cave[j][k];
      /* should fail if there is already a door present */
      if (c_ptr->fval == CORR_FLOOR
          && (c_ptr->tptr == 0 || t_list[c_ptr->tptr].tval < TV_MIN_DOORS))
      {
        i++;
      }
    }
  }

  return i;
}

/* generates damage for 2d6 style dice rolls */
int damroll(const int num, const int sides)
{
  int i, sum = 0;
  for (i = 0; i < num; i++) sum += randint(sides);
  return sum;
}

int pdamroll(int8u const *const array)
{
  return damroll((int)array[0], (int)array[1]);
}

/* A simple, fast, integer-based line-of-sight algorithm.  By Joseph Hall,
   4116 Brewster Drive, Raleigh NC 27606.  Email to jnh@ecemwl.ncsu.edu.

   Returns true if a line of sight can be traced from x0, y0 to x1, y1.

   The LOS begins at the center of the tile [x0, y0] and ends at
   the center of the tile [x1, y1].  If los() is to return true, all of
   the tiles this line passes through must be transparent, WITH THE
   EXCEPTIONS of the starting and ending tiles.

   We don't consider the line to be "passing through" a tile if
   it only passes across one corner of that tile.

   Because this function uses (short) ints for all calculations, overflow
   may occur if deltaX and deltaY exceed 90. */
bool los(int fromY, int fromX, int toY, int toX)
{
  int tmp, deltaX, deltaY;

  deltaX = toX - fromX;
  deltaY = toY - fromY;

  /* Adjacent? */
  if ((deltaX < 2) && (deltaX > -2) && (deltaY < 2) && (deltaY > -2))
    return true;

  /* Handle the cases where deltaX or deltaY == 0. */
  if (deltaX == 0)
  {
    int p_y;  /* y position -- loop variable  */
    if (deltaY < 0)
    {
      tmp = fromY;
      fromY = toY;
      toY = tmp;
    }
    for (p_y = fromY + 1; p_y < toY; p_y++)
    {
      if (cave[p_y][fromX].fval >= MIN_CLOSED_SPACE) return false;
    }
    return true;
  }
  if (deltaY == 0)
  {
    int px;  /* x position -- loop variable  */
    if (deltaX < 0)
    {
      tmp = fromX;
      fromX = toX;
      toX = tmp;
    }
    for (px = fromX + 1; px < toX; px++)
    {
      if (cave[fromY][px].fval >= MIN_CLOSED_SPACE) return false;
    }
    return true;
  }

  /* Now, we've eliminated all the degenerate cases.
     In the computations below, dy (or dx) and m are multiplied by a
     scale factor, scale = abs(deltaX * deltaY * 2), so that we can use
     integer arithmetic. */
  {
    int px;     /* x position              */
    int p_y;    /* y position              */
    int scale2; /* above scale factor / 2  */
    int scale;  /* above scale factor      */
    int xSign;  /* sign of deltaX          */
    int ySign;  /* sign of deltaY          */
    int m;      /* slope or 1/slope of LOS */

    scale2 = abs(deltaX * deltaY);
    scale  = scale2 << 1;
    xSign  = (deltaX < 0) ? -1 : 1;
    ySign  = (deltaY < 0) ? -1 : 1;

    /* Travel from one end of the line to the other, oriented along
       the longer axis. */
    if (abs(deltaX) >= abs(deltaY))
    {
      int dy; /* "fractional" y position */
      /* We start at the border between the first and second tiles,
         where the y offset = .5 * slope.  Remember the scale
         factor.  We have:

           m = deltaY / deltaX * 2 * (deltaY * deltaX)
             = 2 * deltaY * deltaY. */
      dy = deltaY * deltaY;
      m  = dy << 1;
      px = fromX + xSign;

      /* Consider the special case where slope == 1. */
      if (dy == scale2)
      {
        p_y = fromY + ySign;
        dy -= scale;
      }
      else
      {
        p_y = fromY;
      }

      while (toX - px)
      {
        if (cave[p_y][px].fval >= MIN_CLOSED_SPACE) return false;

        dy += m;
        if (dy < scale2)
        {
          px += xSign;
        }
        else if (dy > scale2)
        {
          p_y += ySign;
          if (cave[p_y][px].fval >= MIN_CLOSED_SPACE) return false;
          px += xSign;
          dy -= scale;
        }
        else
        {
          /* This is the case, dy == scale2, where the LOS
             exactly meets the corner of a tile. */
          px  += xSign;
          p_y += ySign;
          dy  -= scale;
        }
      }
      return true;
    }

    int dx; /* "fractional" x position */
    dx = deltaX * deltaX;
    m = dx << 1;

    p_y = fromY + ySign;
    if (dx == scale2)
    {
      px = fromX + xSign;
      dx -= scale;
    }
    else
    {
      px = fromX;
    }

    while (toY - p_y)
    {
      if (cave[p_y][px].fval >= MIN_CLOSED_SPACE) return false;
      dx += m;
      if (dx < scale2)
      {
        p_y += ySign;
      }
      else if (dx > scale2)
      {
        px += xSign;
        if (cave[p_y][px].fval >= MIN_CLOSED_SPACE) return false;
        p_y += ySign;
        dx -= scale;
      }
      else
      {
        px += xSign;
        p_y += ySign;
        dx -= scale;
      }
    }
  }
  return true;
}

/* Returns symbol for given row, column -RAK- */
unsigned char loc_symbol(const int y, const int x)
{
  cave_type *cave_ptr = &cave[y][x];
  /* Murphy, it's you... */
  if ((cave_ptr->cptr == 1) && (!find_flag || find_prself))
    return '@';
  /* player is blind - return blank */
  if (py.flags.status & PY_BLIND)
    return ' ';
  /* player is hallucinating - return random */
  if ((py.flags.image > 0) && (randint (12) == 1))
    return randint(95) + 31;
  /* it's a monster - return associated character */
  if ((cave_ptr->cptr > 1) && (m_list[cave_ptr->cptr].ml))
    return c_list[m_list[cave_ptr->cptr].mptr].cchar;
  /* unlit floor or hidden object - return blank */
  if (!cave_ptr->pl && !cave_ptr->tl && !cave_ptr->fm)
    return ' ';
  /* visible object - return associated character */
  if ((cave_ptr->tptr != 0)
      && (t_list[cave_ptr->tptr].tval != TV_INVIS_TRAP))
    return t_list[cave_ptr->tptr].tchar;
  /* lit floor - return a dot */
  if (cave_ptr->fval <= MAX_CAVE_FLOOR)
  {
#ifdef MSDOS
    return floorsym;
#else
    return '.';
#endif
  }
  /* wall - return a square shape */
  if (cave_ptr->fval == GRANITE_WALL || cave_ptr->fval == BOUNDARY_WALL
      || highlight_seams == false)
  {
#ifdef MSDOS
    return wallsym;
#else
    return '#';
#endif
  }
  /* Originally set highlight bit, but that is not portable, now use
     the percent sign instead. */
  return '%';
}

/* Tests a spot for light or field mark status -RAK- */
bool test_light(const int y, const int x)
{
  cave_type *cave_ptr = &cave[y][x];
  return (cave_ptr->pl || cave_ptr->tl || cave_ptr->fm);
}

/* Prints the map of the dungeon -RAK-
   (the local view, not the zoomed-out map -BS-)

   Currently uses erase_line() to clear each line before redrawing, instead
   of just printing spaces in the blank spots - this is probably an
   optimization for old terminals, which may have been able to clear the
   line with less data via a control code -BS- */
void prt_map()
{
  int i, j, k;
  unsigned char tmp_char;

  k = 0;
  for (i = panel_row_min; i <= panel_row_max; i++)  /* Top to bottom */
  {
    k++;
    erase_line(k, 13);
    for (j = panel_col_min; j <= panel_col_max; j++)  /* Left to right */
    {
      tmp_char = loc_symbol(i, j);
      if (tmp_char != ' ') print(tmp_char, i, j);
    }
  }
}

/* Compact monsters -RAK-
   Returns true if any monsters were deleted, false if could not delete any
   monsters. */
bool compact_monsters()
{
  int i;
  int cur_dis = 66;
  bool delete_any = false;
  monster_type *mon_ptr;

  msg_print("Compacting monsters...");

  while (!delete_any)
  {
    for (i = mfptr - 1; i >= MIN_MONIX; i--)
    {
      mon_ptr = &m_list[i];
      if ((cur_dis >= mon_ptr->cdis) || (randint(3) != 1)) continue;
      /* Never compact away the Balrog!! */
      if (c_list[mon_ptr->mptr].cmove & CM_WIN) continue;
      /* in case this is called from within creatures(), this is a
          horrible hack, the m_list/creatures() code needs to be
          rewritten */
      if (hack_monptr < i)
      {
        delete_monster(i);
        delete_any = true;
        continue;
      }
      /* fix1_delete_monster() does not decrement mfptr, so
          don't set delete_any if this was called */
      fix1_delete_monster(i);
    }
    if (!delete_any)
    {
      cur_dis -= 6;
      /* Can't delete any monsters, return failure.  */
      if (cur_dis < 0) return false;
    }
  }

  return true;
}

/* Add to the players food time -RAK- */
void add_food(const int num)
{
  struct flags *p_ptr;
  int extra, penalty;

  p_ptr = &py.flags;
  if (p_ptr->food < 0) p_ptr->food = 0;
  p_ptr->food += num;
  if (p_ptr->food > PLAYER_FOOD_MAX)
  {
    msg_print("You are bloated from overeating.");

    /* Calculate how much of num is responsible for the bloating.
       Give the player food credit for 1/50, and slow him for that many
       turns also. */
    extra = p_ptr->food - PLAYER_FOOD_MAX;
    if (extra > num) extra = num;
    penalty = extra / 50;

    p_ptr->slow += penalty;
    p_ptr->food = penalty + ((extra == num) ?
      p_ptr->food - num : PLAYER_FOOD_MAX);
  }
  else if (p_ptr->food > PLAYER_FOOD_FULL)
  {
    msg_print("You are full.");
  }
}

/* Returns a pointer to next free space, or -1 if could not allocate a
   monster. -RAK- */
int popm()
{
  return (mfptr == MAX_MALLOC && !compact_monsters()) ? -1 : mfptr++;
}

/* Gives Max hit points -RAK- */
int max_hp(int8u const *const array)
{
  return(array[0] * array[1]);
}

/* Places a monster at given location -RAK- */
bool place_monster(const int y, const int x, const int z, const int slp)
{
  int cur_pos = popm();
  monster_type *mon_ptr = (cur_pos == -1) ? NULL : &m_list[cur_pos];
  creature_type *ctl_ptr;

  if (mon_ptr == NULL) return false;
  ctl_ptr = &c_list[z];

  mon_ptr->fy      = y;
  mon_ptr->fx      = x;
  mon_ptr->mptr    = z;
  mon_ptr->hp      = (ctl_ptr->cdefense & CD_MAX_HP) ?
    max_hp(ctl_ptr->hd) : pdamroll(ctl_ptr->hd);
  /* the c_list speed value is 10 greater, so that it can be a int8u */
  mon_ptr->cspeed  = ctl_ptr->speed - 10 + py.flags.speed;
  mon_ptr->stunned = 0;
  mon_ptr->cdis    = distance(char_row, char_col, y, x);
  mon_ptr->ml      = false;
  cave[y][x].cptr  = cur_pos;
  mon_ptr->csleep  = (!slp || ctl_ptr->sleep == 0) ?
    0 : (ctl_ptr->sleep * 2) + randint(ctl_ptr->sleep * 10);

  return true;
}

/* Places a monster at given location -RAK- */
void place_win_monster()
{
  int y, x, cur_pos;
  monster_type *mon_ptr;
  creature_type *ctl_ptr;

  if (total_winner) return;

  cur_pos = popm();
  /* Check for case where could not allocate space for the win monster,
     this should never happen. */
  if (cur_pos == -1) abort();
  mon_ptr = &m_list[cur_pos];
  ctl_ptr = &c_list[mon_ptr->mptr];
  do
  {
    y = randint(cur_height - 2);
    x = randint(cur_width  - 2);
  } while ((cave[y][x].fval >= MIN_CLOSED_SPACE)
        || (cave[y][x].cptr != 0)
        || (cave[y][x].tptr != 0)
        || (distance(y,x,char_row, char_col) <= MAX_SIGHT));
  mon_ptr->fy      = y;
  mon_ptr->fx      = x;
  mon_ptr->mptr    = randint(WIN_MON_TOT) - 1 + m_level[MAX_MONS_LEVEL];
  mon_ptr->hp      = (ctl_ptr->cdefense & CD_MAX_HP) ?
    max_hp(ctl_ptr->hd) : pdamroll(ctl_ptr->hd);
  /* the c_list speed value is 10 greater, so that it can be a int8u */
  mon_ptr->cspeed  = ctl_ptr->speed - 10 + py.flags.speed;
  mon_ptr->stunned = 0;
  mon_ptr->cdis    = distance(char_row, char_col,y,x);
  cave[y][x].cptr  = cur_pos;
  mon_ptr->csleep  = 0;
}

/* Return a monster suitable to be placed at a given level. This makes high
   level monsters (up to the given level) slightly more common than low
   level monsters at any given level. -CJS- */
int get_mons_num(int level)
{
  int i, j, num;

  if (level == 0)
  {
    return (randint(m_level[0]) - 1);
  }

  if (level > MAX_MONS_LEVEL) level = MAX_MONS_LEVEL;

  if (randint(MON_NASTY) == 1)
  {
    i = randnor(0, 4);
    level += abs(i) + 1;
    if (level > MAX_MONS_LEVEL) level = MAX_MONS_LEVEL;
  }
  else
  {
    /* This code has been added to make it slightly more likely to get
        the higher level monsters. Originally a uniform distribution over
        all monsters of level less than or equal to the dungeon level.
        This distribution makes a level n monster occur approx 2/n% of the
        time on level n, and 1/n*n% are 1st level. */
    num = m_level[level] - m_level[0];
    i = randint(num) - 1;
    j = randint(num) - 1;
    if (j > i) i = j;
    level = c_list[i + m_level[0]].level;
  }

  return randint(m_level[level] - m_level[level - 1]) - 1
          + m_level[level - 1];
}

/* Allocates a random monster -RAK- */
void alloc_monster(const int num, const int dis, int slp)
{
  int y, x, i, l;

  for (i = 0; i < num; i++)
  {
    do
    {
      y = randint(cur_height - 2);
      x = randint(cur_width  - 2);
    }
    while (cave[y][x].fval >= MIN_CLOSED_SPACE || (cave[y][x].cptr != 0)
           || (distance(y, x, char_row, char_col) <= dis));

    l = get_mons_num (dun_level);
    /* Dragons are always created sleeping here, so as to give the player a
       sporting chance. */
    if (c_list[l].cchar == 'd' || c_list[l].cchar == 'D') slp = true;
    /* Place_monster() should always return true here. It does not matter
       if it fails though. */
    place_monster(y, x, l, slp);
  }
}

/* Places creature adjacent to given location -RAK- */
bool summon_monster(int *y, int *x, const int slp)
{
  int i, j, k, l;
  cave_type *cave_ptr;

  l = get_mons_num(dun_level + MON_SUMMON_ADJ);
  for (i = 0; i <= 9; ++i)
  {
    j = *y - 2 + randint(3);
    k = *x - 2 + randint(3);
    if (!in_bounds(j, k)) continue;
    cave_ptr = &cave[j][k];
    if (cave_ptr->fval <= MAX_OPEN_SPACE && cave_ptr->cptr == 0)
    {
      /* Place_monster() should always return true here. */
      if (!place_monster(j, k, l, slp)) return false;
      *y = j;
      *x = k;
      return true;
    }
  }

  return false;
}

/* Places undead adjacent to given location -RAK- */
bool summon_undead(int *y, int *x)
{
  int i = 0, j, k, l = m_level[MAX_MONS_LEVEL], m, ctr;
  cave_type *cave_ptr;

  /* pick a random creature up to l-1, then check up to 20 creatures from
     there to l (inclusive). Break if an undead creature is found, else try
     again with a new random creature -BS- */
  while (l != 0)
  {
    ctr = 0;
    for (m = randint(l) - 1; m <= l; ++m)
    {
      if (c_list[m].cdefense & CD_UNDEAD)
      {
        l = 0;
        break;
      }
      if (++ctr > 19) break;
    }
  }

  for (i = 0; i <= 9; ++i)
  {
    j = *y - 2 + randint(3);
    k = *x - 2 + randint(3);
    if (!in_bounds(j, k)) continue;
    cave_ptr = &cave[j][k];
    if (cave_ptr->fval <= MAX_OPEN_SPACE && (cave_ptr->cptr == 0))
    {
      /* Place_monster() should always return true here. */
      if (!place_monster(j, k, m, false)) return false;
      *y = j;
      *x = k;
      return true;
    }
  }

  return false;
}

/* If too many objects on floor level, delete some of them -RAK-
   Currently deletes only one item, and tries to do it outside of the
   player's view -BS- */
static void compact_objects()
{
  int i, j, ctr = 0, cur_dis = 66, chance;
  cave_type *cave_ptr;

  msg_print("Compacting objects...");

  while (ctr <= 0)
  {
    /* scan entire dungeon */
    for (i = 0; i < cur_height; i++)
    {
      for (j = 0; j < cur_width; j++)
      {
        cave_ptr = &cave[i][j];
        /* skip cell if it's empty or within cur_dis exclusion radius */
        if (cave_ptr->tptr == 0
            || distance(i, j, char_row, char_col) <= cur_dis) continue;
        /* assign percent chance to delete based on cell contents */
        switch(t_list[cave_ptr->tptr].tval)
        {
          case TV_INVIS_TRAP:
          case TV_RUBBLE:
          case TV_OPEN_DOOR:
          case TV_CLOSED_DOOR:
            chance = 5;
            break;
          case TV_VIS_TRAP:
            chance = 15;
            break;
          case TV_UP_STAIR:
          case TV_DOWN_STAIR:
          case TV_STORE_DOOR:
            /* don't delete these */
            continue;
          case TV_SECRET_DOOR:
            chance = 3;
            break;
          default:
            chance = 10;
        }
        /* if percentile roll succeeds, delete and count object */
        if (randint (100) <= chance)
        {
          delete_object(i, j);
          ctr++;
        }
      }
    }
    /* nothing deleted yet; shrink exclusion radius */
    if (ctr == 0) cur_dis -= 6;
  }
  /* if exclusion radius shrank to within possible player view distance,
     local view needs to be redrawn in case something in it was deleted */
  if (cur_dis < 66) prt_map();
}

/* Gives pointer to next free space -RAK- */
int popt()
{
  if (tcptr == MAX_TALLOC) compact_objects();
  return tcptr++;
}

/* Pushes a record back onto free space list. delete_object() should always
   be called instead, unless the object in question is not in the dungeon,
   e.g. in store1.c and files.c -RAK- */
void pusht(const int8u x)
{
  int i, j;

  if (x != tcptr - 1)
  {
    t_list[x] = t_list[tcptr - 1];
    /* must change the tptr in the cave of the object just moved */
    for (i = 0; i < cur_height; i++)
    {
      for (j = 0; j < cur_width; j++)
      {
        if (cave[i][j].tptr == tcptr - 1) cave[i][j].tptr = x;
      }
    }
  }
  tcptr--;
  invcopy(&t_list[tcptr], OBJ_NOTHING);
}

/* Boolean : is object enchanted -RAK-
   (determined via random chance -BS-) */
bool magik(const int chance)
{
  return (randint(100) <= chance);
}

/* Enchant a bonus based on degree desired -RAK- */
int m_bonus(const int base, const int max_std, const int level)
{
  int x, stand_dev, tmp;

  stand_dev = (OBJ_STD_ADJ * level / 100) + OBJ_STD_MIN;
  /* Check for level > max_std since that may have generated an overflow. */
  if (stand_dev > max_std || level > max_std) stand_dev = max_std;
  /* abs may be a macro, don't call it with randnor as a parameter */
  tmp = randnor(0, stand_dev);
  x = (abs(tmp) / 10) + base;
  return (x < base ? base : x);
}
