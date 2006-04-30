/* X-specific.c


- subroutines which do display-type things
  and use the Qt library to do them

10/04
- took out the newt-related subroutines

09/12
- created
*/


#include "my-stuff.h"
#include "mondostructures.h"
#include "X-specific.h"
#include "libmondo-string-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-tools-EXT.h"
#include "libmondo-fork-EXT.h"
#include "libmondo-gui-EXT.h"
#include "lib-common-externs.h"




char err_log_lines[NOOF_ERR_LINES][MAX_STR_LEN],
	g_blurb_str_1[MAX_STR_LEN] = "", g_blurb_str_2[MAX_STR_LEN] =
	"", g_blurb_str_3[MAX_STR_LEN] = "";


long g_isoform_starttime;
int g_isoform_old_progress = -1;
char g_isoform_header_str[MAX_STR_LEN];
int g_mysterious_dot_counter;

int g_currentY = 3;				/* purpose */
int g_current_media_number;

long g_maximum_progress = 999;	/* purpose */
long g_current_progress = -999;	/* purpose */
long g_start_time = 0;			/* purpose */
bool g_text_mode = TRUE;


extern pid_t g_mastermind_pid;
