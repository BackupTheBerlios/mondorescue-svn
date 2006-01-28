/* client.c

CLIENT


FIXME
- fork child process ('daemon', almost) to interact with server
- parent process will interact with keyboard ('logout'=quit)
- if trigger_backup msg arrives then act accordingly
- remember, the client is a 'server' from the minute it logs in
  until the minute it logs out of the server: the client listens
  on a port between 8800 and 8899, waiting for incoming msgs
  from server; all the forked process has to do is take over the
  task of watching that port and act on the incoming data
- when trigger_backup is received then send file along connection

05/19
- added tmsg_to_string()
- forked port-watcher to receive triggers from server in bkgd

05/11
- clarified structures & their names
- improved login/logout OK/fail feedback

05/08
- did some housecleaning
- added comments; removed strcpy()'s
- replaced silly exit()'s with return()'s

*/


#include "structs.h"



/* global vars */



/* prototypes */

int find_and_bind_free_server_port(struct sockaddr_in*, int*);
long increment_magic_number(void);
int login_to_server(char*,char*);
int logout_of_server(char*);
int send_msg_to_server(struct s_client2server_msg_record*, char*);
int send_ping_to_server(char*,char*);
char *tmsg_to_string(t_msg);
void watch_port_for_triggers_from_server(struct sockaddr_in *, int, int);

/* subroutines */

int find_and_bind_free_server_port(struct sockaddr_in *sin, int *p_s)
/*
Purpose:Find a free port on the server. Bind to it, so that
	whichever subroutine called me can then send data
	to the server.
Params: sin - server's IP address in a structure
	p_s - [return] file descriptor of port binding
Return: result (0=success, nonzero=failure)
*/
{
  int server_port;

  for(server_port = 8700; server_port < 8710; server_port++)
    {
      sin->sin_port = htons(server_port);
      if ((*p_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
	  perror("socket");
	  return(1);
	}
      if (connect(*p_s, (struct sockaddr*)sin, sizeof(struct sockaddr_in)) < 0)
	{
	  printf("Not connecting at %d\r", server_port);
	  continue;
	}
      return(server_port);
    }
  fprintf(stderr, "Cannot find free server port\nIs server actually running?\n");
  return(1);
}



long increment_magic_number()
/*
Purpose:Increment the magic number which is attached to
	each message sent from client to server, to make
	the packet unique.
Params:	none
Return:	magic number
*/
{
  static unsigned long magic=1;
  magic=(magic % 999999999) + 1;
  return(magic);
}



int login_to_server(char*hostname, char*servername)
/*
Purpose:Ask server to log me (client) in.
Params: hostname - client's hostname (not IP address
	necessarily but it should resolve to it)
	servername - server's hostname
Return: result (-1=failure, N=client's port #)
NB:	The client's port # is chosen at random by
	send_msg_to_server and returned to me.
*/
{
  struct s_client2server_msg_record orig_rec;
  int i;

  orig_rec.msg_type = login;
  strncpy(orig_rec.body, hostname, sizeof(orig_rec.body));
  for(i=0; i<3; i++)
    {
      if (send_msg_to_server(&orig_rec, servername) > 0)
        { return(orig_rec.port); }
      printf("Attempt #%d to login failed. Retrying...\n", i+1);
    }
  printf("Failed to login\n");
  return(-1);
}



int logout_of_server(char*servername)
/*
Purpose:Instruct server to log client out.
Params: servername - hostname of server
Return: result (0=success; nonzero=failure)
*/
{
  struct s_client2server_msg_record orig_rec;

  orig_rec.msg_type = logout;
  strncpy(orig_rec.body, "Bye bye!", sizeof(orig_rec.body));
  if (send_msg_to_server(&orig_rec, servername) <= 0)
    { return(1); }
  else
    { return(0); }
}



// FIXME -
// Are the threads trampling on one another's st_sClient, st_new_sClient's?
// I don't see how. After all, there's only one thread... :-)
int send_msg_to_server(struct s_client2server_msg_record *rec, char *servername)
/*
Purpose:Send message to server - a login/logout/ping request
	or perhaps a request for data to be restored.
Params: rec - the message to be sent to server
	servername - the hostname of server
Return: result (-1=failure, >0=success)
*/
{
  struct hostent *hp;
  struct sockaddr_in sin;
  static struct sockaddr_in st_sinClient; /* client port; connected by login; closed by logout */
  static int st_sClient; /* client connection */
  static int st_new_sClient;
  int server_port, s, len;
  static int  st_client_port=0; /* client port; set by login */
  struct s_server2client_msg_record incoming_rec;
  struct pollfd ufds;
  static t_pid st_pid=-1;

/* If logging out then kill the trigger-watcher before trying;
   otherwise, the trigger-watcher will probably catch the
   'logout_ok' packet and go nutty on us :-)
*/
  if (rec->msg_type == logout)
    {
      sprintf(tmp, "kill %d &> /dev/null", st_pid);
	system(tmp);
//      kill(st_pid);
	st_pid = -1;
    }

/* st_sinClient, st_new_sClient and st_sClient contain info
   relating to the permanent socket which this subroutine
   opens at login-time so that it can listen for messages
   from the server. This subroutine doesn't watch the
   socket all the time (yet) but it leaves it open until
   logout-time, for neatness' sake.
*/
  if ((hp = gethostbyname(servername)) == NULL) { fprintf(stderr, "%s: unknown host\n", servername); return(1); }
  if (st_client_port > 0 && rec->msg_type == login) { fprintf(stderr, "Already logged in. Why try again?\n"); return(1); }
  if (st_client_port <= 0 && rec->msg_type != login) { fprintf(stderr, "Don't try to send msgs before logging in.\n"); return(1); }
  /* open client port if login */

  if (rec->msg_type == login)
    {
      printf("Logging in\n");
      st_client_port = 8800 + rand()%100; // FIXME: retry if I can't use this port
      memset((void*)&st_sinClient, 0, sizeof(st_sinClient));
      st_sinClient.sin_family = AF_INET;
      st_sinClient.sin_addr.s_addr = INADDR_ANY;
      st_sinClient.sin_port = htons(st_client_port);
      if ((st_sClient = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) { fprintf(stderr, "Unable to open socket on port #%d\n", st_client_port); return(1); }
      if (bind(st_sClient, (struct sockaddr*)&st_sinClient, sizeof(st_sinClient)) < 0) { fprintf(stderr, "Cannot bind %d - %s\n", st_client_port, strerror(errno)); return(2); }
      if (listen(st_sClient, MAX_PENDING) < 0) { fprintf(stderr, "Cannot setup listen (%d) - %s\n", st_client_port, strerror(errno)); return(3); }
    }

  /* send msg to server */
  rec->port = st_client_port;
  memset((void*)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  memcpy((void*)&sin.sin_addr, hp->h_addr, hp->h_length);
  server_port = find_and_bind_free_server_port(&sin, &s);
  if (server_port<=0) { fprintf(stderr, "Cannot find free server port\nIs server running?\n"); return(1); }
  rec->magic = increment_magic_number();
  send (s, (char*)rec, sizeof(struct s_client2server_msg_record), 0);
  close(s);

  /* wait for ack/pong/feedback */
  ufds.fd = st_sClient;
  ufds.events = POLLIN|POLLPRI;
  poll(&ufds, 1, 1000);
  if (!ufds.revents) { fprintf(stderr,"Failed to poll\n"); return(-1); }
  len = sizeof(st_sinClient);
  if ((st_new_sClient = accept(st_sClient, (struct sockaddr*)&st_sinClient, &len)) < 0) { fprintf(stderr,"[child] Cannot accept\n"); return(1); }
  if ((len = recv(st_new_sClient, (char*)&incoming_rec, sizeof(incoming_rec), MSG_DONTWAIT)) <= 0) { fprintf(stderr, "[child] Cannot recv\n"); return(2); }

  printf("Received %s - %s\n", tmsg_to_string(incoming_rec.msg_Type, incoming_rec.body);
  if (rec->msg_type != login && incoming_rec.msg_type == login_ok) { fprintf(stderr, "WTF? login_ok but I wasn't logging in\n"); }
  if (rec->msg_type != logout&& incoming_rec.msg_type == logout_ok){ fprintf(stderr, "WTF? logout_ok but I wasn't logging out\n"); }
// FIXME - should I close() after accepting and recving? Or should I wait until logging out?

  /* close client port if logout */
  if (rec->msg_type == logout)
    {
      close(st_new_sClient);
      st_new_sClient = st_sClient = st_client_port = 0;
    }

  if (incoming_rec.msg_type == fail) { return(-1); }

/* fork the process which watches in the background for pings/requests/etc. */
  if (rec->msg_type == login)
    {
      st_pid = fork();
      switch(st_pid)
        {
          case -1:
            fprintf(stderr, "Failed to fork\n"); exit(1);
          case 0: /* child */
            watch_port_for_triggers_from_server(&st_sinClient, st_new_sClient, st_sClient);
            exit(0);
          default:/* parent */
            return(server_port);
        }
    }
}



int send_ping_to_server(char*msg, char*servername)
/*
Purpose:Send a 'ping' to server. Wait for 'pong!'.
Params: msg - string to send ("Hello world"?)
	servername - server's hostname
Return: result (0=success, nonzero=failure)
*/
{
  struct s_client2server_msg_record orig_rec;
  orig_rec.msg_type = ping;
  strncpy(orig_rec.body, msg, sizeof(orig_rec.body));
  if (send_msg_to_server(&orig_rec, servername) <= 0)
    { return(1); }
  else
    { return(0); }
}



char *tmsg_to_string(t_msg msg_type)
/*
Purpose:Return a string/name of the msg of type msg_type
Params: msg_type - type of message (enum typedef)
Return: pointer to static string {name of msg)
*/
{
  static char sz_msg[MAX_STR_LEN];
  switch(msg_type)
    {
      case unused : strcpy(sz_msg, "unused"); break;
      case login  : strcpy(sz_msg, "login"); break;
      case logout : strcpy(sz_msg, "logout"); break;
      case login_ok:strcpy(sz_msg, "login_ok"); break;
      case logout_ok:strcpy(sz_msg, "logout_ok"); break;
      case ping   : strcpy(sz_msg, "ping"); break;
      case pong   : strcpy(sz_msg, "pong"); break;
      case fail   : strcpy(sz_msg, "fail"); break;
      case trigger_backup: strcpy(sz_msg, "trigger_backup"); break;
      case trigger_restore: strcpy(sz_msg, "trigger_restore"); break;
      case begin_stream: strcpy(sz_msg, "begin_stream"); break;
      case end_stream: strcpy(sz_msg, "end_stream"); break;
      default: strcpy(sz_msg, "(Fix tmsg_to_string please)"); break;
    }
  return(sz_msg);
}


void watch_port_for_triggers_from_server(struct sockaddr_in *sin, int new_s, int s)
/*
Purpose:Watch client port for incoming trigger
Params: sin - record containing info on the port/socket
        new_s - file descriptor of _open_ client socket
        s - file descriptor of client socket (not the 'open' side of it)
NB:     I don't really know why new_s and s are what they are or how or why.
Return: none
{
  struct pollfd ufds;
  int len;

  for(;;)
    {
  /* wait for ack/pong/feedback */
      ufds.fd = s;
      ufds.events = POLLIN|POLLPRI;
      poll(&ufds, 1, 1000);
      if (!ufds.revents) { continue; } /* wait again */
      len = sizeof(struct sockaddr_in);
      if ((new_s = accept(s, (struct sockaddr*)sin, &len)) < 0) { fprintf(stderr,"[child] Cannot accept\n"); return(1); }
      if ((len = recv(new_s, (char*)&incoming_rec, sizeof(incoming_rec), MSG_DONTWAIT)) <= 0) { fprintf(stderr, "[child] Cannot recv\n"); return(2); }
      printf("Received %s - %s\n", tmsg_to_string(incoming_rec.msg_Type, incoming_rec.body);
// FIXME - should I close() after accepting and recving?
    }
}



/* main */

int main(int argc, char*argv[])
/*
Purpose: main subroutine
Parameters: none
Return: result (0=success, nonzero=failure)
*/
{
  int client_port;
  char hostname[MAX_STR_LEN+1], msg[MAX_STR_LEN+1], servername[MAX_STR_LEN+1];
  bool done;

  srandom(time(NULL));
  /* FIXME - add Ctrl-C / sigterm trapping */
  /* FIXME - run login/logout code as a fork and share the logged in/logged out status flag */

  if (argc == 2)
    {
      strncpy(servername, argv[1], MAX_STR_LEN);
      servername[MAX_STR_LEN]='\0';
    }
  else
    {
      fprintf(stderr, "client <server addr>\n");
      exit(1);
    }
  gethostname(hostname, sizeof(hostname));
  printf("Logging onto server as client '%s'\n", hostname);
  client_port = login_to_server(hostname, servername);
  if (client_port <= 0) { fprintf(stderr, "Unable to login to server. Aborting\n"); exit(1); }
  for(done=false; !done; )
    {
      printf("MsgToSend: ");
      fgets(msg, sizeof(msg), stdin);
      if (!strncmp(msg, "logout", 6)) { done=true; }
      else
	{
	  if (send_ping_to_server(msg, servername))
	    { fprintf(stderr, "Error while waiting for response to ping\n"); exit(1); }
	}
    }
  printf("Logging out of server\n");
  if (logout_of_server(servername))
    { fprintf(stderr, "Warning - failed to logout of server.\n"); }
  exit(0);
}
/* end main() */



