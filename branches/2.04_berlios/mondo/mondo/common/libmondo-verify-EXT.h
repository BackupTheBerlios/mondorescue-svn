/* libmondo-verify-EXT.h */


extern int verify_cd_image (struct s_bkpinfo *);
extern int verify_a_tarball (struct s_bkpinfo *, char *);
extern int verify_an_afioball_from_CD (struct s_bkpinfo *, char *);
extern int verify_an_afioball_from_tape (struct s_bkpinfo *, char *, long long);
extern int verify_a_biggiefile_from_tape (struct s_bkpinfo *, char *, long long);
int verify_afioballs_from_CD (struct s_bkpinfo *);
extern int verify_afioballs_from_tape (struct s_bkpinfo *);
extern int verify_biggiefiles_from_tape (struct s_bkpinfo *);
extern int verify_tape_backups (struct s_bkpinfo *);
extern char *vfy_tball_fname (struct s_bkpinfo *, char *, int);


