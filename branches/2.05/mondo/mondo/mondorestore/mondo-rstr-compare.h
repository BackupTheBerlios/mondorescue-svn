/* mondo-rstr-compare.h
 * $Id: mondo-rstr-compare.h,v 1.1 2004/06/10 15:29:13 hugo Exp $
 */


int compare_to_CD(struct s_bkpinfo*);
int compare_to_cdstream(struct s_bkpinfo*);
int compare_to_tape(struct s_bkpinfo*);

int 
compare_mode( struct s_bkpinfo *bkpinfo ,
	      struct mountlist_itself *mountlist, 
	      struct raidlist_itself *raidlist);
