/* Interface to "cvs edit", "cvs watch on", and related features

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

int watch_on(int argc, char **argv);
int watch_off(int argc, char **argv);

/* Check to see if any notifications are sitting around in need of being
   sent.  These are the notifications stored in CVSADM_NOTIFY (edit,unedit);
   commit calls notify_do directly.  */
void client_notify_check(char *repository, char *update_dir);

/* Issue a notification for file FILENAME.  TYPE is 'E' for edit, 'U'
   for unedit, and 'C' for commit.  WHO is the user currently running.
   For TYPE 'E' WATCHES is zero or more of E,U,C, in that order, to specify
   what kinds of temporary watches to set.  */
int notify_do (int type, const char *filename, const char *who, const char *date, const char *hostname, const char *pathname, const char *watches, const char *repository, const char *tag, const char *flags, const char *bugid, const char *message);

/* Take note of the fact that FILE is up to date (this munges CVS/Base;
   processing of CVS/Entries is done separately).  */
void mark_up_to_date(const char *file);

