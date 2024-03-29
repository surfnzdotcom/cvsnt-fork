/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * Create Administration.
 * 
 * Creates a CVS administration directory based on the argument repository; the
 * "Entries" file is prefilled from the "initrecord" argument.
 */

#include "cvs.h"

/* update_dir includes dir as its last component.

   Return value is 0 for success, or 1 if we printed a warning message.
   Note that many errors are still fatal; particularly for unlikely errors
   a fatal error is probably better than a warning which might be missed
   or after which CVS might do something non-useful.  If WARN is zero, then
   don't print warnings; all errors are fatal then.  */

int Create_Admin (const char *dir, const char *update_dir, const char *repository, const char *tag,
				  const char *date, int nonbranch, int warn)
{
	FILE *fout;
    char *cp;
	char *reposcopy;
	char *tmp;

	TRACE(1,"Create_Admin (%s, %s, %s, %s, %s, %d, %d)",
		 PATCH_NULL(dir), PATCH_NULL(update_dir), PATCH_NULL(repository), tag ? tag : "",
		 date ? date : "", nonbranch, warn);

    if (noexec)
		return 0;

    tmp = (char*)xmalloc (strlen (dir) + 100);
    if (dir != NULL)
		sprintf (tmp, "%s/%s", dir, CVSADM);
    else
		strcpy (tmp, CVSADM);
    if (isfile (tmp))
		error (1, 0, "there is a version in %s already", update_dir);

    if (CVS_MKDIR (tmp, 0777) < 0)
    {
	/* We want to print out the entire update_dir, since a lot of
	   our code calls this function with dir == "." or dir ==
	   NULL.  I hope that gives enough information in cases like
	   absolute pathnames; printing out xgetwd or something would
	   be way too verbose in the common cases.  */

	if (warn)
	{
	    /* The reason that this is a warning, rather than silently
	       just skipping creating the directory, is that we don't want
	       CVS's behavior to vary subtly based on factors (like directory
	       permissions) which are not made clear to the user.  With
	       the warning at least we let them know what is going on.  */
	    error (0, errno, "warning: cannot make directory %s in %s",
		   CVSADM, update_dir);
	    xfree (tmp);
	    return 1;
	}
	else
	    error (1, errno, "cannot make directory %s in %s",
		   CVSADM, update_dir);
    }
#ifdef _WIN32
	wnt_hide_file(tmp);
#endif

    /* record the current cvs root for later use */
	Create_Root (dir, current_parsed_root->original);
    if (dir != NULL)
	(void) sprintf (tmp, "%s/%s", dir, CVSADM_REP);
    else
	(void) strcpy (tmp, CVSADM_REP);
    fout = CVS_FOPEN (tmp, "w+");
    if (fout == NULL)
    {
	if (update_dir[0] == '\0')
	    error (1, errno, "cannot open %s", tmp);
	else
	    error (1, errno, "cannot open %s/%s", update_dir, CVSADM_REP);
    }
    reposcopy = xstrdup (repository);
    Sanitize_Repository_Name (&reposcopy);

    /* The top level of the repository is a special case -- we need to
       write it with an extra dot at the end.  This trailing `.' stuff
       rubs me the wrong way -- on the other hand, I don't want to
       spend the time making sure all of the code can handle it if we
       don't do it. */

    if (strcmp (reposcopy, current_parsed_root->directory) == 0)
    {
	reposcopy = (char*)xrealloc (reposcopy, strlen (reposcopy) + 3);
	strcat (reposcopy, "/.");
    }

    cp = reposcopy;

#ifdef RELATIVE_REPOS
    /*
     * If the Repository file is to hold a relative path, try to strip off
     * the leading CVSroot argument.
     */
    {
    char *path = (char*)xmalloc (strlen (current_parsed_root->directory) + 2);

    (void) sprintf (path, "%s/", current_parsed_root->directory);
    if (fnncmp (cp, path, strlen (path)) == 0) 
		cp += strlen (path);
    xfree (path);
    }
#endif

    if (fprintf (fout, "%s\n", cp) < 0)
    {
	if (update_dir[0] == '\0')
	    error (1, errno, "write to %s failed", tmp);
	else
	    error (1, errno, "write to %s/%s failed", update_dir, CVSADM_REP);
    }
    if (fclose (fout) == EOF)
    {
	if (update_dir[0] == '\0')
	    error (1, errno, "cannot close %s", tmp);
	else
	    error (1, errno, "cannot close %s/%s", update_dir, CVSADM_REP);
    }

    /* now, do the Entries files */
	/* CVS/Entries */
    if (dir != NULL)
	(void) sprintf (tmp, "%s/%s", dir, CVSADM_ENT);
    else
	(void) strcpy (tmp, CVSADM_ENT);
    fout = CVS_FOPEN (tmp, "w+");
    if (fout == NULL)
    {
	if (update_dir[0] == '\0')
	    error (1, errno, "cannot open %s", tmp);
	else
	    error (1, errno, "cannot open %s/%s", update_dir, CVSADM_ENT);
    }
    if (fclose (fout) == EOF)
    {
	if (update_dir[0] == '\0')
	    error (1, errno, "cannot close %s", tmp);
	else
	    error (1, errno, "cannot close %s/%s", update_dir, CVSADM_ENT);
    }

	/* CVS/Entries.Extra */
    if (dir != NULL)
	(void) sprintf (tmp, "%s/%s", dir, CVSADM_ENTEXT);
    else
	(void) strcpy (tmp, CVSADM_ENT);
    fout = CVS_FOPEN (tmp, "w+");
    if (fout == NULL)
    {
	if (update_dir[0] == '\0')
	    error (1, errno, "cannot open %s", tmp);
	else
	    error (1, errno, "cannot open %s/%s", update_dir, CVSADM_ENTEXT);
    }
    if (fclose (fout) == EOF)
    {
	if (update_dir[0] == '\0')
	    error (1, errno, "cannot close %s", tmp);
	else
	    error (1, errno, "cannot close %s/%s", update_dir, CVSADM_ENTEXT);
    }

    /* Create a new CVS/Tag file */
    WriteTag (dir, tag, date, nonbranch, update_dir, repository, NULL);

    xfree (reposcopy);
    xfree (tmp);
    return 0;
}
