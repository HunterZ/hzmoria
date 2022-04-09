/* source/signals.c: signal handlers

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

/* This signal package was brought to you by    -JEW-  */
/* Completely rewritten by        -CJS- */

/* To find out what system we're on.  */
#include "config.h"
#include "externs.h"

/* Since libc6, linux (Debian, at least) defaults to BSD signal().  This */
/* expects SYSV.  Thus, DEBIAN_LINUX uses the sysv_signal call, everyone */
/* else uses just signal.  RJW 00_0528 */
#ifdef DEBIAN_LINUX
#define MSIGNAL sysv_signal
#else
#define MSIGNAL signal
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h> /* exit() */
#include <string.h>

static int error_sig = -1;
static int signal_count = 0;

static void signal_handler(int sig)
{
  if (error_sig >= 0) /* Ignore all second signals. */
  {
    /* Be safe. We will die if persistent enough. */
    if(++signal_count > 10) MSIGNAL(sig, SIG_DFL);
    return;
  }
  error_sig = sig;

  /* Allow player to think twice. Wizard may force a core dump. */
  if (sig == SIGINT
#if !defined(MSDOS)
   || sig == SIGQUIT
#endif
  )
  {
    if (death)
    {
      MSIGNAL(sig, SIG_IGN); /* Can't quit after death. */
    }
    else if (!character_saved && character_generated)
    {
      if (!get_check("Really commit *Suicide*?"))
      {
        if (turn > 0) disturb(1, 0);
        erase_line(0, 0);
        put_qio();
        error_sig = -1;
        MSIGNAL(sig, signal_handler); /* Have to restore handler. */
        /* in case control-c typed during msg_print */
        if (wait_for_more) put_buffer(" -more-", MSG_LINE, 0);
        put_qio();
        return;    /* OK. We don't quit. */
      }
      strcpy(died_from, "Interrupting");
    }
    else
    {
      strcpy(died_from, "Abortion");
    }
    prt("Interrupt!", 0, 0);
    death = true;
    exit_game();
  }
  /* Die. */
  prt(
    "OH NO!!!!!!  A gruesome software bug LEAPS out at you. There is NO defense!",
    23, 0);
  if (!death && !character_saved && character_generated)
  {
    panic_save = true;
    prt("Your guardian angel is trying to save you.", 0, 0);
    sprintf(died_from,"(panic save %d)",sig);
    if (!save_char())
    {
      strcpy(died_from, "software bug");
      death = true;
      turn = -1;
    }
  }
  else
  {
    death = true;
    _save_char(savefile);  /* Quietly save the memory anyway. */
  }
  restore_term();
  exit(1);
}

void nosignals()
{
#ifdef SIGTSTP
  MSIGNAL(SIGTSTP, SIG_IGN);
#endif
  if (error_sig < 0) error_sig = 0;
}

void signals()
{
#ifdef SIGTSTP
  MSIGNAL(SIGTSTP, suspend);
#endif
  if (error_sig == 0) error_sig = -1;
}

void init_signals()
{
  /* legacy code used to be chock full of system-specific checks
     instead, just check for each non-ANSI signal being #defined
     -BS- */
  /* these are the ANSI-guaranteed signals */
  MSIGNAL(SIGINT,  signal_handler);
  MSIGNAL(SIGILL,  signal_handler);
  MSIGNAL(SIGFPE,  signal_handler);
  MSIGNAL(SIGSEGV, signal_handler);
  MSIGNAL(SIGTERM, signal_handler);
  MSIGNAL(SIGABRT, signal_handler);
  /* POSIX signals */
#ifdef SIGHUP
  /* Ignore HANGUP, and let the EOF code take care of this case. */
  MSIGNAL(SIGHUP,  SIG_IGN);
#endif
#ifdef SIGQUIT
  MSIGNAL(SIGQUIT, signal_handler);
#endif
#ifdef SIGTRAP
  MSIGNAL(SIGTRAP, signal_handler);
#endif
#ifdef SIGIOT
  MSIGNAL(SIGIOT,  signal_handler);
#endif
#ifdef SIGEMT  /* in BSD systems */
  MSIGNAL(SIGEMT,  signal_handler);
#endif
#ifdef SIGKILL
  MSIGNAL(SIGKILL, signal_handler);
#endif
#ifdef SIGBUS
  MSIGNAL(SIGBUS,  signal_handler);
#endif
#ifdef SIGSYS
  MSIGNAL(SIGSYS,  signal_handler);
#endif
#ifdef SIGPIPE
  MSIGNAL(SIGPIPE, signal_handler);
#endif
  /* weird stuff (remove?) */
#ifdef SIGXCPU  /* BSD */
  MSIGNAL(SIGXCPU, signal_handler);
#endif
#ifdef SIGPWR /* SYSV */
  MSIGNAL(SIGPWR, signal_handler);
#endif
}

void ignore_signals()
{
  MSIGNAL(SIGINT, SIG_IGN);
#ifdef SIGQUIT
  MSIGNAL(SIGQUIT, SIG_IGN);
#endif
}

void default_signals()
{
  MSIGNAL(SIGINT, SIG_DFL);
#ifdef SIGQUIT
  MSIGNAL(SIGQUIT, SIG_DFL);
#endif
}

void restore_signals()
{
  MSIGNAL(SIGINT, signal_handler);
#ifdef SIGQUIT
  MSIGNAL(SIGQUIT, signal_handler);
#endif
}
