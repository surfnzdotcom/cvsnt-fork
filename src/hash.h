#ifndef HASH__H
#define HASH__H
/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 */

/*
 * The number of buckets for the hash table contained in each list.  This
 * should probably be prime.
 */
#define HASHSIZE	151

/*
 * Types of nodes
 */
enum ntype
{
    NT_UNKNOWN, ntHEADER, ENTRIES, FILES, LIST, RCSNODE,
    RCSVERS, DIRS, UPDATE, LOCK, NDBMNODE, FILEATTR,
    VARIABLE, RCSFIELD, RCSCMPFLD
};
typedef enum ntype Ntype;

struct node
{
    Ntype type;
    struct node *next;
    struct node *prev;
    struct node *hashnext;
    struct node *hashprev;
    char *key;
    char *data;
    void (*delproc) (struct node *);
	bool nofree;
};
typedef struct node Node;

struct list_t
{
    Node *list;
    Node *hasharray[HASHSIZE];
    struct list_t *next;
	bool nofree;
};
typedef struct list_t List;

List *getlist();
Node *findnode (List *list, const char *key);
Node *findnode_fn(List *list, const char *key);
Node *getnode();
int insert_before (List *list, Node *marker, Node *p);
int addnode (List *list, Node *p);
int addnode_at_front (List *list, Node *p);
int walklist (List *list, int (*proc)(Node *, void *), void *closure);
int list_isempty (List *list);
void dellist (List **listp);
void delnode (Node *p);
void freenode (Node *p);
int freenodecache();
int freelistcache();
void sortlist (List *list, int (*comp)(const Node *, const Node *));
int fsortcmp (const Node *p, const Node *q);

#endif
