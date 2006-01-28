/* structs.h 


06/16
- added CLIENT_RCFILE
- added .bodyAux to msg record structure

06/14
- added SERVER_COMDEV and CLIENT_COMDEV

06/11
- added busy to s_registered_client_record structure
- added compare_ok, compare_fail

05/27
- added pthread.h

05/11
- added file transfer flags to type t_msg

*/



#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>


#define log_it(x,y) { log_it_SUB(g_logfile,x,y); }
#define CLIENT_RCFILE "/root/.monitas-client.rc"
#define CLIENT_COMDEV "/var/spool/monitas/client-input.dev"
#define SERVER_COMDEV "/var/spool/monitas/server-input.dev"
#define SERVER_STATUS_FILE "/var/spool/monitas/server-status.txt"
#define MAX_STR_LEN 510
#define MAX_PENDING 8 /* silently limited to 5 by BSD, says a Linux manual */
#define MAX_CLIENTS 100
#define MSG_BODY_SIZE 200 /* 512 doesn't work; don't ask me why */
#define XFER_BUF_SIZE 16384 /* for sending archives between client and server */
                            /* NB: 32768 doesn't work; don't ask me why */
typedef enum { false=0, true=1} bool;
typedef enum { unused=0, login, logout, ping, pong, login_ok, logout_ok, 
                login_fail, logout_fail, backup_ok, backup_fail,
                restore_ok, restore_fail, compare_ok, compare_fail,
		trigger_backup, trigger_restore, trigger_compare,
                begin_stream, end_stream, user_req, progress_rpt } t_msg;
typedef enum { debug, info, warn, error, fatal } t_loglevel;


struct s_client2server_msg_record /* msg from client to server */
{
  t_msg msg_type;
  long magic;
  int port; /* which port to contact client on */  
  char body[MSG_BODY_SIZE];
  char bodyAux[MSG_BODY_SIZE];
};



struct s_server2client_msg_record /* msg from client */
{
  t_msg msg_type;
  long magic; /* if pong/ack then it equals the magic# of the packet to which it's replying; else, 0 */
  int dummy;
  char body[MSG_BODY_SIZE];
  char bodyAux[MSG_BODY_SIZE];
};



struct s_registered_client_record
{
  char hostname_pretty[128]; /* specified by client in login packet */
  char ipaddr[20]; /* xxx.yyy.zzz.nnn of client */
  int port;
  bool busy; /* backing up / restoring / comparing */
  char last_progress_rpt[MAX_STR_LEN+1];
};



struct s_clientlist
{
  int items;
  struct s_registered_client_record el[MAX_CLIENTS];
};


