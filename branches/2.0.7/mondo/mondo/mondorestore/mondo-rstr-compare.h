/* mondo-rstr-compare.h
 * $Id$
 */


int compare_to_CD(struct s_bkpinfo *);
int compare_to_cdstream(struct s_bkpinfo *);
int compare_to_tape(struct s_bkpinfo *);

int
compare_mode(struct s_bkpinfo *bkpinfo,
			 struct mountlist_itself *mountlist,
			 struct raidlist_itself *raidlist);
