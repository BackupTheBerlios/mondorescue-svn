/* server.c



SERVER


FIXME
- when sysadm requests it:-
  - connect to relevant port of client
  - send trigger_backup msg
  - receive file(s)
  - close client's port

05/20
- changed 'clientname' to 'clientIP'
 
05/11
- clarified structures & their names
- improved login/logout OK/fail feedback

05/10
- made main loop multithreaded (replaced multiple forks)
- thanks to the move from forks to threads, clientlist
  is now shared between processes automatically
- replaced printf()'s and fprintf()'s with log_it() function
  and levels debug/info/warn/error/fatal

05/08
- got down to some housecleaning
- added comments; removed strcpy()'s
- replaced silly exit()'s with return()'s

*/




#include "structs.h"

#define LOG_THESE_AND_HIGHER debug   /* debug, info, warn, error, fatal */
#define NOOF_THREADS 10




/* global vars */

struct s_clientlist g_clientlist; /* FIXME - lock during login/logout, using mutexes */




/* prototypes */

int backup_client(int,char*);
int find_client_in_clientlist(char *);
int handle_incoming_message(int, struct sockaddr_in *, struct s_client2server_msg_record *);
int handle_login_request(int, struct s_client2server_msg_record *, char *);
int handle_logout_request(int, struct s_client2server_msg_record *, char *);
int handle_ping_request(int, struct s_client2server_msg_record *, char *);
void log_it(t_loglevel, char *);
int restore_client(int,char*);
int send_msg_to_client(struct s_server2client_msg_record *, char *, int);
void start_threads_to_watch_ports_for_requests(void);
void interact_with_sysadm(void);
char *tmsg_to_string(t_msg);
void* watch_port_for_requests_from_clients(void*);



/* subroutines */

int backup_client(int clientno, char*path)
/*
Purpose:Backup the path of a specific client. Receive
	the archives. Store them locally (on me, the server).
Params:	clientno - client# in g_clientlist[]
	path - client's path to be backed up
Return: result (0=success, nonzero=failure)
*/
{
  struct s_server2client_msg_record rec_to_client;
  int i, res=0;
  char tmp[MAX_STR_LEN];

  sprintf(tmp, "Client# = %d", clientno);
  log_it(debug, tmp);
  if (clientno >= g_clientlist.items)
    { log_it(fatal, "backup_client: clientno>=g_clientlist.items"); }
  sprintf(tmp, "Backing up %s - path=%s", g_clientlist.el[clientno].ipaddr, path);
  log_it(info, tmp);
  rec_to_client.msg_type = trigger_backup;
  strncpy(rec_to_client.body, path, sizeof(rec_to_client.body));
  if (send_msg_to_client(&rec_to_client, g_clientlist.el[clientno].ipaddr, g_clientlist.el[clientno].port))
    { log_it(error, "backup_client - failed to send msg to client"); return(1); }

  if (res)
    {
      sprintf(tmp, "Error(s) occurred while backing up %s", g_clientlist.el[clientno].ipaddr);
      log_it(error, tmp);
      return(1);
    }
  else
    {
      sprintf(tmp, "Backed up %s OK", g_clientlist.el[clientno].ipaddr);
      log_it(error, tmp);
      return(0);
    }
}



int find_client_in_clientlist(char *clientIP)
/*
Purpose:Find a client in the clientlist (list of logged-in
	clients).
Params:	clientIP - IP address of client
Return: result (<0=not found, 0+=found in element N)
*/
{
  int i;
  char tmp[MAX_STR_LEN];

  for(i = 0; i < g_clientlist.items; i++)
    {
      if (!strcmp(clientIP, g_clientlist.el[i].ipaddr))
	{ return(i); }
      sprintf(tmp, "find_client_in_clientlist: Compared %s to clientlist[%d]=%s; failed\n", clientIP, i, g_clientlist.el[i].ipaddr);
      log_it(debug, tmp);
    }
  return(-1);
}



int handle_incoming_message(int skt, struct sockaddr_in *sin, struct s_client2server_msg_record *rec)
/*
Purpose:Process message which has just arrived from client.
	A 'message' could be a login/logout request or a ping.
Params:	skt - client's port to respond to
	sin - client's IP address, in sockaddr_in structure
	rec - data received from client
Return: result (0=success, nonzero=failure)
*/
{
  char clientIP[MAX_STR_LEN], *ptr, tmp[MAX_STR_LEN];
  int res=0;

  //  echo_ipaddr_to_screen(&sin->sin_addr);
  ptr = (unsigned char*)(&sin->sin_addr);
  sprintf(clientIP, "%d.%d.%d.%d", ptr[0], ptr[1], ptr[2], ptr[3]);
  sprintf(tmp, "clientIP = %s", clientIP);
  log_it(debug, tmp);
  switch(rec->msg_type)
    {
    case login:
      res=handle_login_request(skt, rec, clientIP);
      break;
    case ping:
      res=handle_ping_request(skt, rec, clientIP);
      break;
    case logout:
      res=handle_logout_request(skt, rec, clientIP);
      break;
    default:
      sprintf(tmp, "%s request from %s [%s] (port %d) - how do I handle it?", tmsg_to_string(rec->msg_type), clientIP, rec->body, rec->port);
      log_it(error, tmp);
    }
  return(res);
}



int handle_login_request(int skt, struct s_client2server_msg_record *rec_from_client, char *clientIP)
/*
Purpose:Handle a login request which has just been received
	from client.
Params:	skt - client's port to talk to
	rec_from_client - login rq record received from client
	clientIP - client's IP address, in string
Return: result (0=success, nonzero=failure)
*/
{
  struct s_server2client_msg_record rec_to_client;
  int clientno;
  char tmp[MAX_STR_LEN];

  sprintf(tmp, "Login request from %s [%s] (port %d)", clientIP, rec_from_client->body, rec_from_client->port);
  log_it(info, tmp);
  clientno = find_client_in_clientlist(clientIP);
  if (clientno>=0)
    {
      rec_to_client.msg_type = fail;
      sprintf(rec_to_client.body, "Sorry, you're already logged in!");
      sprintf(tmp, "Ignoring login rq from %s: he's already logged in.", clientIP);
      log_it(error, tmp);
/* FIXME - ping client (which will have a child watching for incoming
packets by now - you didn't forget to do that, did you? :)) - to find out
if client is still running. If it's not then say OK, forget it, I'll kill
that old connection and log you in anew. If it _is_ then say hey, you're
already logged in; either you're an idiot or you're a hacker. */
    }
  else
    {
      rec_to_client.msg_type = login_ok; /* to confirm login */
      sprintf(rec_to_client.body, "Thanks for logging in.");
      clientno = g_clientlist.items;
      strncpy(g_clientlist.el[clientno].hostname_pretty, rec_from_client->body, sizeof(g_clientlist.el[clientno].hostname_pretty));
      strncpy(g_clientlist.el[clientno].ipaddr, clientIP, sizeof(g_clientlist.el[clientno].ipaddr));
      g_clientlist.el[clientno].port = rec_from_client->port;
      g_clientlist.items ++;
      sprintf(tmp, "Login request from %s ACCEPTED", clientIP);
      log_it(info, tmp);
    }
  send_msg_to_client(&rec_to_client, clientIP, rec_from_client->port);
  return(0);
}



int handle_logout_request(int skt, struct s_client2server_msg_record *rec_from_client, char *clientIP)
/*
Purpose:Handle a logout request which has just been received
	from client.
Params:	skt - client's port to talk to
	rec_from_client - login rq record received from client
	clientIP - client's IP address, in string
Return: result (0=success, nonzero=failure)
*/
{
  struct s_server2client_msg_record rec_to_client;
  int i, res=0;
  char tmp[MAX_STR_LEN];

  sprintf(tmp, "Logout request from %s [%s] (port %d)", clientIP, rec_from_client->body, rec_from_client->port);
  log_it(info, tmp);
  i = find_client_in_clientlist(clientIP);
  if (i<0)
    {
      sprintf(rec_to_client.body, "Client is not logged in yet. How can I log him out?");
      log_it(error, rec_to_client.body);
      rec_to_client.msg_type = fail;
      res=1;
    }
  else
    {
      sprintf(rec_to_client.body, "Removed client#%d from login table. Thanks for logging out.", i);
      for(; i<g_clientlist.items; i++)
        {
	  memcpy((char*)(&g_clientlist.el[i]), (char*)(&g_clientlist.el[i+1]), sizeof(struct s_registered_client_record));
	}
      strncpy(g_clientlist.el[i].hostname_pretty, "WTF? Someone teach Hugo to handle pointers properly, please!", sizeof(g_clientlist.el[i].hostname_pretty));
      g_clientlist.items--;
      rec_to_client.msg_type = logout_ok; /* to confirm logout */
      sprintf(tmp, "Logout request from %s ACCEPTED", clientIP);
      log_it(info, tmp);
    }
  send_msg_to_client(&rec_to_client, clientIP, rec_from_client->port);
  return(res);
}



int handle_ping_request(int skt, struct s_client2server_msg_record *rec_from_client, char *clientIP)
/*
Purpose:Handle a ping request which has just been received
	from client.
Params:	skt - client's port to talk to
	rec_from_client - login rq record received from client
	clientIP - client's IP address, in string
Return: result (0=success, nonzero=failure)
*/
{
  struct s_server2client_msg_record rec_to_client;
  int i;
  char tmp[MAX_STR_LEN];

  sprintf(tmp, "ping request from port %d --- %s", rec_from_client->port, rec_from_client->body);
  log_it(info, tmp);
  i = find_client_in_clientlist(clientIP);
  if (i < 0)
    {
      sprintf(tmp, "Hey, %s isn't logged in. I'm not going to pong him.", clientIP);
      log_it(error, tmp);
    }
  else
    {
      rec_to_client.msg_type = pong; /* reply to ping */
      sprintf(rec_to_client.body, "Hey, I'm replying to client#%d's ping. Pong! (re: %s", i, rec_from_client->body);
      send_msg_to_client(&rec_to_client, clientIP, rec_from_client->port);
      log_it(debug, rec_to_client.body);
    }
  return(0);
}



void interact_with_sysadm()
/*
Purpose:Tell sysadm about users logged in, etc. in
	response to keypresses (1=users, ENTER=exit)
Params: none
Return: none
NB:	Called by main()
*/
{
  char tmp[MAX_STR_LEN];
  int i;
  bool done;

  for(done=false; !done; )
    {
      printf("Type 1 to list clients\n");
      printf("Type 2 to backup a client\n");
      printf("Type 3 to restore a client\n");
      printf("Type ENTER to quit.\n");
      fgets(tmp, sizeof(tmp), stdin);
      if (!strcmp(tmp, "1\n"))
	{
	  printf("g_clientlist.items is %d\n", g_clientlist.items);
	  printf("List of %d clients:-\n", g_clientlist.items);
	  printf("\tClient# Port#  IP address       Client name\n");
	  for(i=0; i<g_clientlist.items; i++)
	    {
	      printf("\t%d\t%4d   %-16s %-20s \n", i, g_clientlist.el[i].port, g_clientlist.el[i].ipaddr, g_clientlist.el[i].hostname_pretty);
	    }
          printf("<End of list>\n\n");
        }
      else if (!strcmp(tmp, "2\n"))
        {
	  if (g_clientlist.items!=1) { printf("Forget it\n"); }
	  else
	    {
	      backup_client(0, "/usr/local");
	    }
	}
      else if (!strcmp(tmp, "3\n"))
        {
	  if (g_clientlist.items!=1) { printf("Forget it\n"); }
	  else
	    {
	      restore_client(0, "/usr/local");
	    }
	}
      else if (!strcmp(tmp, "\n"))
        {
          if (g_clientlist.items > 0)
            {
              printf("There are %d users logged in. Are you sure you want to quit? ", g_clientlist.items);
              fgets(tmp, sizeof(tmp), stdin);
              if (tmp[0]=='Y' || tmp[0]=='y')
                { done=true; }
            }
          else
            { done=true; }
          if (done)
            { log_it(info, "OK, you've chosen to shutdown server."); }
          else
            { log_it(info, "Shutdown canceled."); }
        }
    }
}



void log_it(t_loglevel level, char *sz_message)
{
  char sz_outstr[MAX_STR_LEN], sz_level[MAX_STR_LEN], sz_time[MAX_STR_LEN];
  time_t time_rec;

  time(&time_rec);
  switch(level)
    {
      case debug: strcpy(sz_level, "DEBUG"); break;
      case info:  strcpy(sz_level, "INFO "); break;
      case warn:  strcpy(sz_level, "WARN "); break;
      case error: strcpy(sz_level, "ERROR"); break;
      case fatal: strcpy(sz_level, "FATAL"); break;
      default:    strcpy(sz_level, "UNKWN"); break;
    }
  sprintf(sz_time, ctime(&time_rec));
  sz_time[strlen(sz_time)-6] = '\0';
  sprintf(sz_outstr, "%s %s: %s", sz_time+11, sz_level, sz_message);
  if ((int)level >= LOG_THESE_AND_HIGHER)
    {
      fprintf(stderr, "%s\n",sz_outstr);
    }
  if (level==fatal) { fprintf(stderr,"Aborting now."); exit(1); }
}




int restore_client(int clientno, char*path)
{
}


int send_msg_to_client(struct s_server2client_msg_record *rec, char *clientIP, int port)
/*
Purpose:Send a message from server to client.
	A 'message' could be a response to a login/logout/ping
	request or perhaps a 'trigger'. (A trigger is a message
	from server to client intended to initiate a backup
	or similar activity.)
Params:	rec - record containing the data to be sent to client
	clientIP - the xxx.yyy.zzz.aaa IP address of client
	port - the client's port to send data to
Return: result (0=success, nonzero=failure)
*/
{
  struct hostent *hp;
  struct sockaddr_in sin;
  int  s;
  char tmp[MAX_STR_LEN];

  if ((hp = gethostbyname(clientIP)) == NULL)
    {
      sprintf(tmp, "send_msg_to_client: %s: unknown host\n", clientIP);
      log_it(error, tmp);
      return(1);
    }
  memset((void*)&sin, 0, sizeof(sin));
  memcpy((void*)&sin.sin_addr, hp->h_addr, hp->h_length);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    { perror("socket"); log_it(error, "send_msg_to_client: SOCKET error"); return(1); }
  if (connect(s, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)) < 0)
    { sprintf(tmp, "Failed to connect to client %s on port %d", clientIP, port); log_it(error, tmp); return(1); }
  send(s, (char*)rec, sizeof(struct s_server2client_msg_record), 0);
  close(s);
  sprintf(tmp, "Sent %s msg to %s (port %d)", tmsg_to_string(rec->msg_type), clientIP, port);
  log_it(debug, tmp);
  return(0);
}



void start_threads_to_watch_ports_for_requests()
/*
Purpose:Start the threads that watch the 'incoming' ports
	for messages from clients.
Params: none
Return: none
NB:	Called by main()
*/
{
  int i, port, res;
  pthread_t threadinfo[NOOF_THREADS];
  char tmp[MAX_STR_LEN];

  g_clientlist.items = 0;
  for(i=0; i<NOOF_THREADS; i++)
    {
      port = 8700+i;
      sprintf(tmp,"%d", port);
      res = pthread_create(&(threadinfo[i]), NULL, watch_port_for_requests_from_clients, (void*)tmp);
      if (res != 0)
        {
	    perror("Thread creation failed");
        }
      usleep(80000);
    }
  sprintf(tmp, "Threads are now running.");
  log_it(info, tmp);
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



void* watch_port_for_requests_from_clients(void*sz_watchport)
/*
Purpose:Watch a port for incoming messages from clients.
	A 'message' could be a request to login/logout or
	a ping, or perhaps a request to backup/restore data.
Params: sz_watchport - the port to watch for incoming messages
	from clients
Return: result (0=success, nonzero=failure)
NB:	Function will return nonzero if error occurs during
	setup but will otherwise run forever, or until killed.
*/
{
  int watch_port;
  struct sockaddr_in sin;
  char buf[MAX_STR_LEN+1], tmp[MAX_STR_LEN];
  int len, s, new_s;
  struct s_client2server_msg_record rec;

  watch_port = atoi((char*)sz_watchport);
  sprintf(tmp, "watch_port_for_requests_from_clients(%d) - starting", watch_port);
  log_it(debug, tmp);
  memset((void*)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(watch_port);
  if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      sprintf(tmp, "Unable to open socket on port #%d", watch_port);
      log_it(error, tmp);
      return((void*)-1);
    }
  if (bind(s, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
      sprintf(tmp, "Cannot bind %d - %s\n", watch_port, strerror(errno));
      log_it(error, tmp);
      return((void*)-1);
    }
  if (listen(s, MAX_PENDING) < 0)
    {
      sprintf(tmp, "Cannot setup listen (%d) - %s\n", watch_port, strerror(errno));
      log_it(error, tmp);
      return((void*)-1);
    }
  /* service incoming connections */
  sprintf(tmp, "Opened socket on port #%d OK", watch_port);
  log_it(info, tmp);
  while(true)
    {
      len = sizeof(sin);
      if ((new_s = accept(s, (struct sockaddr *)&sin, (unsigned int*)&len)) < 0)
	{
	  sleep(1);
	  continue;
	}
      while ((len = recv(new_s, buf, sizeof(buf), 0)) > 0)
	{
	  if (len > MAX_STR_LEN) { len = MAX_STR_LEN; }
	  buf[len] = '\0';
	  memcpy((char*)&rec, buf, sizeof(rec));
	  handle_incoming_message(new_s, &sin, &rec);
	}
      close(new_s);
    }
  return(NULL);
}



int main(int argc, char*argv[])
/*
Purpose: main subroutine
Parameters: none
Return: result (0=success, nonzero=failure)
*/
{
  char tmp[MAX_STR_LEN];

/* FIXME - add Ctrl-C / sigterm trapping */

  start_threads_to_watch_ports_for_requests();
  interact_with_sysadm(); /* report on users logged in, etc.; return after a while... */
  if (g_clientlist.items != 0)
	{ fprintf(stderr, "Ruh-roh! Some users are still logged in. Oh well...\n"); }
/* FIXME - force clients to log off */
/* FIXME - send signal to threads, telling them to terminate */
  sprintf(tmp, "Done. Server is exiting now.");
  log_it(info, tmp);
  return(0);
}
/* end main() */


