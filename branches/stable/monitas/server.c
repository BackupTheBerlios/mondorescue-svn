/* server.c



06/19
- fixed bugs in mondoarchive compare code

06/16
- every 15 seconds, update /var/spool/monitas/server-status.txt file
- added handle_progress_rpt()
- modify restore_path() to allow user to restore to another path

06/14
- added "2> /dev/null" to call to find /var/spool/monitas
- improved backup_client(), compare_client(), restore_client()
- pipe logs to logfile, not stdout

06/11
- moved register_pid(), set_signals(), termination_in_progress() to common.c
- commented code a bit
- improved backup_client(), compare_client(), restore_client()

06/10
- improved backup_client()
- discover whether mondoarchive returned error (ask client)
  and act accordingly (i.e. drop archive)

06/09
- added /var/spool/monitas/input.dev FIFO to
  let external programs instruct me to backup
  or restore a given computer
- added signal trapping

06/07
- rename transmit_file... and move
  to common.c, as transmit_file_to_socket()
- rename receive_file... and move to
  common.c, as receive_file_from_socket()

05/27
- save bkps to /var/spool/monitas/123.456.789.012/[time]

05/23
- added elementary saving/storing of incoming data
  from client backing itself up

05/21
- added backup_client, compare_client, restore_client
  templates for triggers
 
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




/*-----------------------------------------------------------*/



#include "structs.h"
//#define LOG_THESE_AND_HIGHER debug



#define NOOF_THREADS 10
#define LOGFILE "/var/log/monitas-server.log"
#define log_it(x,y) { log_it_SUB(g_logfile,x,y); }


/* externs */

extern bool does_file_exist(char*);
extern char *call_program_and_get_last_line_of_output(char*);
extern int call_program_and_log_output(char*);
extern int create_and_watch_fifo_for_commands(char*);
extern bool does_file_exist(char*);
extern void log_it_SUB(char*, t_loglevel, char *);
extern int make_hole_for_file (char *);
extern int read_block_from_fd(int socket_fd, char*buf, int len_to_read);
extern int receive_file_from_socket(FILE*, int);
extern void register_pid(pid_t, char*);
extern void set_signals(bool);
extern void termination_in_progress(int);
extern char *tmsg_to_string(t_msg);
extern int transmit_file_to_socket(FILE*,int);



/* global vars */

struct s_clientlist g_clientlist; /* FIXME - lock during login/logout, using mutexes */
char g_command_fifo[MAX_STR_LEN+1]; // Device which server will monitor for incoming commands
pthread_t g_threadinfo[NOOF_THREADS]; // One thread per port, to watch for requests from clients
char g_logfile[MAX_STR_LEN+1] = "/var/log/monitas-server.log";
char g_server_status_file[MAX_STR_LEN+1];



/* prototypes */

int backup_client(char*, int, char*);
int compare_client(char*, int, char*);
int find_client_in_clientlist(char *);
int forcibly_logout_all_clients(void);
int forcibly_logout_client(int);
void* generate_server_status_file_regularly(void*);
int handle_incoming_message(int, struct sockaddr_in *, struct s_client2server_msg_record *);
int handle_login_request(int, struct s_client2server_msg_record *, char *);
int handle_logout_request(int, struct s_client2server_msg_record *, char *);
int handle_ping_request(int, struct s_client2server_msg_record *, char *);
int handle_progress_rpt(int, struct s_client2server_msg_record *, char *);
int handle_user_request(int, struct s_client2server_msg_record *, char *);
int process_incoming_command(char*);
int read_block_from_fd(int, char*, int);
int restore_client(char*, int, char*, char*);
int send_msg_to_client(struct s_server2client_msg_record *, char *, int, int*);
void start_threads_to_watch_ports_for_requests(void);
void terminate_daemon(int);
char *tmsg_to_string(t_msg);
void* watch_port_for_requests_from_clients(void*);



/*-----------------------------------------------------------*/



int backup_client(char*ipaddr, int port, char*clientpath)
/*
Purpose:Backup the path of a specific client. Receive
the archives. Store them locally (on me, the server).
Params:	clientno - client# in g_clientlist[]
        clientpath - client's path to be backed up
Return: result (0=success, nonzero=failure)
*/
{
  struct s_server2client_msg_record rec_to_client;
  int res=0, socket_fd, noof_archives, i, len;
  char tmp[MAX_STR_LEN+1], outfile[MAX_STR_LEN+1];
  FILE*fout;

  sprintf(tmp, "%s - backup of %s commencing", ipaddr, clientpath);
  log_it(info, tmp);
  sprintf(outfile, "/var/spool/monitas/%s/%s.dat", ipaddr, call_program_and_get_last_line_of_output("date +%s"));
  if (does_file_exist(outfile)) { log_it(error, "Backup storage location exists already. That should be impossible."); return(1); }
  if (make_hole_for_file(outfile))
    { res++; log_it(error, "Cannot write archive to spool dir"); }
  else if (!(fout=fopen(outfile, "w")))
    { res++; log_it(fatal, "Failed to openout temp data file"); }
  else
    {
      sprintf(tmp, "Backing up %s - archive=%s", ipaddr, outfile);
      log_it(debug, tmp);
      rec_to_client.msg_type = trigger_backup;
      strncpy(rec_to_client.body, clientpath, sizeof(rec_to_client.body));
      if (send_msg_to_client(&rec_to_client, ipaddr, port, &socket_fd))
        { log_it(error, "backup_client - failed to send msg to client"); return(1); }
      res += receive_file_from_socket(fout, socket_fd);
      len=read(socket_fd, (char*)&i, sizeof(i));
      if (!len) { res++; log_it(error, "Client hasn't told me the result of its call to mondoarchive"); }
      else if (len!=sizeof(i)) { res++; log_it(error, "Client didn't sent _entire_ result of its call to mondoarchive"); }
      else if (i) { res++; log_it(error, "Client said, mondoarchive returned an error."); }
      else { log_it(debug, "Client said, mondoarchive returned OK"); }
      fclose(fout);
      close(socket_fd);
    }
/* Shuffle older backups off the mortal coil. Leave maximum of 4 backup files in /var/spool/monitas/[ipaddr] */
  sprintf(tmp, "find /var/spool/monitas/%s -type f 2> /dev/null | grep -n \"\" | tail -n1 | cut -d':' -f1", ipaddr);
  noof_archives = atoi(call_program_and_get_last_line_of_output(tmp));
  i = noof_archives - 3;
  if (i>0)
    {
      sprintf(tmp, "rm -f `find /var/spool/monitas/%s -type f | sort | head -n%d`", ipaddr, i);
      call_program_and_log_output(tmp);
    }
/* Return success/failure value */
  if (res>0)
    {
      sprintf(tmp, "%s - error(s) occurred while backing up %s", ipaddr, clientpath);
      log_it(error, tmp);
      rec_to_client.msg_type = backup_fail;
      sprintf(rec_to_client.body, "Failed to backup %s", clientpath);
      log_it(debug, rec_to_client.body);
      unlink(outfile);
    }
  else
    {
      sprintf(tmp, "%s - backed up %s ok", ipaddr, clientpath);
      log_it(info, tmp);
      rec_to_client.msg_type = backup_ok;
      sprintf(rec_to_client.body, "%s - backed up ok", clientpath);
      log_it(debug, rec_to_client.body);
    }
  if (send_msg_to_client(&rec_to_client, ipaddr, port, &socket_fd))
    {
      res++;
      sprintf(tmp, "Unable to notify %s of backup success/failure", ipaddr);
      log_it(error, tmp);
      i = find_client_in_clientlist(ipaddr);
      if (i>=0) { forcibly_logout_client(i); }
      log_it(info, "I'm assuming the backup was bad because the client cannot be reached.");
      unlink(outfile);
    }
  return(res);
}



/*-----------------------------------------------------------*/



int compare_client(char*ipaddr, int port, char*clientpath)
/*
Purpose:Compare the path of a specific client. Transmit
the archives from my (the server's) local storage loc.
Params:	clientno - client# in g_clientlist[]
        clientpath - client's path to be compared
Return: result (0=success, nonzero=failure)
*/
{
  struct s_server2client_msg_record rec_to_client;
  int res=0, socket_fd, len, i;
  char tmp[MAX_STR_LEN+1], infile[MAX_STR_LEN+1];
  FILE*fin;

  sprintf(tmp, "%s - comparison of %s commencing", ipaddr, clientpath);
  log_it(info, tmp);
// FIXME - don't assume the latest backup contains the files we want ;)
  sprintf(tmp, "find /var/spool/monitas/%s -type f | sort | tail -n1", ipaddr);
  strcpy(infile, call_program_and_get_last_line_of_output(tmp));
  sprintf(tmp, "Comparing to data file '%s'", infile);
  log_it(debug, tmp);
  if (!does_file_exist(infile)) { log_it(error, "Backup not found. That should be impossible."); return(1); }
  sprintf(tmp, "Comparing %s - archive=%s", ipaddr, infile);
  log_it(debug, tmp);
  rec_to_client.msg_type = trigger_compare;
  strncpy(rec_to_client.body, clientpath, sizeof(rec_to_client.body));
  if (send_msg_to_client(&rec_to_client, ipaddr, port, &socket_fd))
    { log_it(error, "compare_client - failed to send msg to client"); return(1); }
  if (!(fin=fopen(infile, "r")))
    { log_it(fatal, "Failed to openin temp data file"); }
  res += transmit_file_to_socket(fin, socket_fd);
  len=read(socket_fd, (char*)&i, sizeof(i));
  if (!len) { res++; log_it(error, "Client hasn't told me the result of its call to mondoarchive"); }
  else if (len!=sizeof(i)) { res++; log_it(error, "Client didn't sent _entire_ result of its call to mondoarchive"); }
  else if (i) { res++; log_it(error, "Client said, mondoarchive returned an error."); }
  else { log_it(debug, "Client said, mondoarchive returned OK"); }
  fclose(fin);
  close(socket_fd);
  if (res>0)
    {
      sprintf(tmp, "%s - error(s) occurred while comparing %s", ipaddr, clientpath);
      log_it(error, tmp);
      rec_to_client.msg_type = compare_fail;
      sprintf(rec_to_client.body, "Failed to compare %s", clientpath);
      log_it(debug, rec_to_client.body);
    }
  else
    {
      sprintf(tmp, "%s - compared %s ok", ipaddr, clientpath);
      log_it(info, tmp);
      rec_to_client.msg_type = compare_ok;
      sprintf(rec_to_client.body, "%s - compared ok", clientpath);
      log_it(debug, rec_to_client.body);
    }
  if (send_msg_to_client(&rec_to_client, ipaddr, port, &socket_fd))
    {
      sprintf(tmp, "Unable to notify %s of compare success/failure", ipaddr);
      log_it(error, tmp);
      i = find_client_in_clientlist(ipaddr);
      if (i>=0) { forcibly_logout_client(i); }
    }
  return(res);
}



/*-----------------------------------------------------------*/




int find_client_in_clientlist(char *clientIP)
/*
Purpose:Find a client in the clientlist (list of logged-in
	clients).
Params:	clientIP - IP address of client
Return: result (<0=not found, 0+=found in element N)
*/
{
  int i;
  char tmp[MAX_STR_LEN+1];

  for(i = 0; i < g_clientlist.items; i++)
    {
      if (!strcmp(clientIP, g_clientlist.el[i].ipaddr))
	{ return(i); }
      sprintf(tmp, "find_client_in_clientlist: Compared %s to clientlist[%d]=%s; failed\n", clientIP, i, g_clientlist.el[i].ipaddr);
      log_it(debug, tmp);
    }
  return(-1);
}



/*-----------------------------------------------------------*/



int forcibly_logout_all_clients()
/*
Purpose: Tell all clients to disconnect, right now.
Params:  None
Returns: 0=success
*/
{
// FIXME - lock g_clientlist for duration of this function
  while(g_clientlist.items>0)
    {
      forcibly_logout_client(0);
    }
  return(0);
}



/*-----------------------------------------------------------*/



int forcibly_logout_client(int clientno)
/*
Purpose: Logout specific client(#) by force.
Params:  Client# in g_clientlist[] array.
Returns: 0=success, nonzero=failure to get other end to hear me;)
NB:      The client was definitely removed from our login table.
         If the client got the message, return 0; else, nonzero.
*/
{
  struct s_server2client_msg_record rec_to_client;
  char tmp[MAX_STR_LEN+1];
  int res=0;

  sprintf(tmp, "Forcibly logging %s out", g_clientlist.el[clientno].ipaddr);
  log_it(info, tmp);
  rec_to_client.msg_type = logout_ok; /* to confirm logout */
  strcpy(rec_to_client.body, "Server is shutting down. You are forced to logout");
  res=send_msg_to_client(&rec_to_client, g_clientlist.el[clientno].ipaddr, g_clientlist.el[clientno].port, NULL);
  if (--g_clientlist.items > 0)
    {
      sprintf(tmp, "Moving clientlist[%d] to clientlist[%d]", clientno, g_clientlist.items);
      log_it(debug, tmp);
      sprintf(tmp, "Was ipaddr=%s; now is ipaddr=", g_clientlist.el[clientno].ipaddr);
      memcpy((void*)&g_clientlist.el[clientno], (void*)&g_clientlist.el[g_clientlist.items], sizeof(struct s_registered_client_record));
      strcat(tmp, g_clientlist.el[clientno].ipaddr);
      log_it(debug, tmp);
    }
  return(res);
}



/*-----------------------------------------------------------*/


void* generate_server_status_file_regularly(void*inp)
{
  int i;
  FILE*fout;

  strncpy(g_server_status_file, (char*)inp, MAX_STR_LEN);
  for(;;)
    {
      if ((fout = fopen(g_server_status_file, "w")))
        {
// FIXME - lock g_clientlist
          for(i=0; i<g_clientlist.items; i++)
            {
              fprintf(fout, "%s [%s] : %s\n", g_clientlist.el[i].ipaddr, g_clientlist.el[i].hostname_pretty, g_clientlist.el[i].last_progress_rpt);
            }
          fclose(fout);
        }
      sleep(1);
    }
  exit(0);
}



/*-----------------------------------------------------------*/



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
  char clientIP[MAX_STR_LEN+1], tmp[MAX_STR_LEN+1];
  unsigned char *ptr;
  int res=0;

  //  echo_ipaddr_to_screen(&sin->sin_addr);
  ptr = (unsigned char*)(&sin->sin_addr);
  sprintf(clientIP, "%d.%d.%d.%d", ptr[0], ptr[1], ptr[2], ptr[3]);
  sprintf(tmp, "clientIP = %s", clientIP);
  log_it(debug, tmp);
  sprintf(tmp, "%s message from %s [%s] (port %d)", tmsg_to_string(rec->msg_type), clientIP, rec->body, rec->port);
  log_it(debug, tmp);
  switch(rec->msg_type)
    {
    case login:
      res=handle_login_request(skt, rec, clientIP);
      break;
    case ping:
      res=handle_ping_request(skt, rec, clientIP);
      break;
    case progress_rpt:
      res=handle_progress_rpt(skt, rec, clientIP);
      break;
    case logout:
      res=handle_logout_request(skt, rec, clientIP);
      break;
    case user_req:
      res=handle_user_request(skt, rec, clientIP);
      break;
    default:
      log_it(error, "...How do I handle it?");
    }
  return(res);
}



/*-----------------------------------------------------------*/



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
  char tmp[MAX_STR_LEN+1];

//FIXME - lock g_clientlist[]
  clientno = find_client_in_clientlist(clientIP);
  if (clientno>=0)
    {
      rec_to_client.msg_type = login_fail;
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
      g_clientlist.el[clientno].busy = false;
      g_clientlist.items ++;
      sprintf(tmp, "Login request from %s ACCEPTED", clientIP);
      log_it(info, tmp);
      strcpy(g_clientlist.el[clientno].last_progress_rpt, "Logged in");
    }
  send_msg_to_client(&rec_to_client, clientIP, rec_from_client->port, NULL);
  return(0);
}



/*-----------------------------------------------------------*/



int handle_logout_request(int skt, struct s_client2server_msg_record *rec_from_client, char *clientIP)
/*
Purpose:Handle a logout request which has just been received
	from client.
Params:	skt - client's port to talk to
	rec_from_client - logout rq record received from client
	clientIP - client's IP address, in string
Return: result (0=success, nonzero=failure)
*/
{
  struct s_server2client_msg_record rec_to_client;
  int i, res=0;
  char tmp[MAX_STR_LEN+1];

  i = find_client_in_clientlist(clientIP);
  if (i<0)
    {
      sprintf(rec_to_client.body, "Client is not logged in yet. How can I log him out?");
      log_it(error, rec_to_client.body);
      rec_to_client.msg_type = logout_fail;
      res=1;
    }
  else if (g_clientlist.el[i].busy)
    {
      sprintf(rec_to_client.body, "Client is working. I shouldn't log him out.");
      log_it(error, rec_to_client.body);
      rec_to_client.msg_type = logout_fail;
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
  send_msg_to_client(&rec_to_client, clientIP, rec_from_client->port, NULL);
  return(res);
}



/*-----------------------------------------------------------*/



int handle_ping_request(int skt, struct s_client2server_msg_record *rec_from_client, char *clientIP)
/*
Purpose:Handle a ping request which has just been received
	from client.
Params:	skt - client's port to talk to
	rec_from_client - ping record received from client
	clientIP - client's IP address, in string
Return: result (0=success, nonzero=failure)
*/
{
  struct s_server2client_msg_record rec_to_client;
  int i;
  char tmp[MAX_STR_LEN+1];

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
      send_msg_to_client(&rec_to_client, clientIP, rec_from_client->port, NULL);
      log_it(debug, rec_to_client.body);
    }
  return(0);
}



/*-----------------------------------------------------------*/



int handle_progress_rpt(int skt, struct s_client2server_msg_record *rec_from_client, char *clientIP)
/*
Purpose:Handle a progress_rpt which has just been received
	from client.
Params:	skt - client's port to talk to
	rec_from_client - user record received from client
	clientIP - client's IP address, in string
Return: result (0=success, nonzero=failure)
*/
{
//  struct s_server2client_msg_record rec_to_client;
  int i, res=0;
  char tmp[MAX_STR_LEN+1];

  i = find_client_in_clientlist(clientIP);
  if (i < 0)
    {
      sprintf(tmp, "Hey, %s isn't logged in. I'm not going to deal with his progress_rpt.", clientIP);
      log_it(error, tmp);
      res++;
    }
  else
    {
      strcpy(g_clientlist.el[i].last_progress_rpt, rec_from_client->body);
    }
  return(res);
}



/*-----------------------------------------------------------*/



int handle_user_request(int skt, struct s_client2server_msg_record *rec_from_client, char *clientIP)
/*
Purpose:Handle a user request which has just been received
	from client.
Params:	skt - client's port to talk to
	rec_from_client - user record received from client
	clientIP - client's IP address, in string
Return: result (0=success, nonzero=failure)
*/
{
//  struct s_server2client_msg_record rec_to_client;
  int i, res=0;
  char tmp[MAX_STR_LEN+1], command[MAX_STR_LEN+1], first_half[MAX_STR_LEN+1], second_half[MAX_STR_LEN+1], *p;

  i = find_client_in_clientlist(clientIP);
  if (i < 0)
    {
      sprintf(tmp, "Hey, %s isn't logged in. I'm not going to deal with his request.", clientIP);
      log_it(error, tmp);
      res++;
    }
  else
    {
      strcpy(first_half, rec_from_client->body);
      p = strchr(first_half, ' ');
      if (!p) { second_half[0]='\0'; } else { strcpy(second_half, p); *p='\0'; }
      sprintf(command, "echo \"%s %s%s\" > %s", first_half, clientIP, second_half, SERVER_COMDEV);
      log_it(debug, command);
      i = system(command);
      if (i) { res++; log_it(error, "Failed to echo command to FIFO"); }
    }
  return(res);
}



/*-----------------------------------------------------------*/



int process_incoming_command(char*incoming)
/*
Purpose:Process incoming command, presumably
        read from FIFO and sent there by sysadm/user.
Params:	incoming - raw command string itself
Return: result (0=success; nonzero=failure)
*/
{
int res=0, port;
int clientno;
char tmp[MAX_STR_LEN+1];
int pos;
char command[MAX_STR_LEN+1], ipaddr[MAX_STR_LEN+1],
	path[MAX_STR_LEN+1], aux[MAX_STR_LEN+1];

//  sprintf(tmp, "incoming = '%s'", incoming);
//  log_it(debug, tmp);
          pos=0;
          sscanf(incoming, "%s %s %s", command, ipaddr, path);
          if (!strcmp(command, "restore"))
            { sscanf(incoming, "%s %s %s %s", command, ipaddr, path, aux); }
          else
            { aux[0] = '\0'; }

//          for(i=0; i<strlen(command); i++) { command[i]=command[i]|0x60; }
          sprintf(tmp, "cmd=%s ipaddr=%s path=%s", command, ipaddr, path);
          log_it(debug, tmp);
          sprintf(tmp, "%s of %s on %s <-- command received", command, path, ipaddr);
          log_it(info, tmp);
          if ((clientno = find_client_in_clientlist(ipaddr)) < 0)
            {
              sprintf(tmp, "%s not found in clientlist; so, %s failed.", ipaddr, command);
              log_it(error, tmp);
            }
          else if (g_clientlist.el[clientno].busy == true)
            {
              sprintf(tmp, "%s is busy; so, %s failed.", ipaddr, command);
              log_it(error, tmp);
            }
          else
            {
              g_clientlist.el[clientno].busy = true;
              port = g_clientlist.el[clientno].port;
              if (!strcmp(command, "backup"))
                { res = backup_client(ipaddr, port, path); }
              else if (!strcmp(command, "compare"))
                { res = compare_client(ipaddr, port, path); }
              else if (!strcmp(command, "restore"))
                { res = restore_client(ipaddr, port, path, aux); }
              else
                {
                  sprintf(tmp, "%s - cannot '%s'. Command unknown.", ipaddr, command);
                  log_it(error, tmp);
                  res=1;
                }
              g_clientlist.el[clientno].busy = false;
            }
  return(res);
}



/*-----------------------------------------------------------*/



int restore_client(char*ipaddr, int port, char*clientpath, char*auxpath)
/*
Purpose:Restore the path of a specific client. Transmit
the archives from my (the server's) local storage loc.
Params:	clientno - client# in g_clientlist[]
        clientpath - client's path to be restored
Return: result (0=success, nonzero=failure)
*/
{
  struct s_server2client_msg_record rec_to_client;
  int res=0, socket_fd, len, i;
  char tmp[MAX_STR_LEN+1], infile[MAX_STR_LEN+1];
  FILE*fin;

  sprintf(tmp, "%s - restoration of %s commencing", ipaddr, clientpath);
  log_it(info, tmp);
// FIXME - don't assume the latest backup contains the files we want ;)
  sprintf(tmp, "find /var/spool/monitas/%s -type f | sort | tail -n1", ipaddr);
  strcpy(infile, call_program_and_get_last_line_of_output(tmp));
  sprintf(tmp, "Restoring from data file '%s'", infile);
  log_it(debug, tmp);
  if (!does_file_exist(infile)) { log_it(error, "Backup not found. That should be impossible."); return(1); }
  sprintf(tmp, "Restoring %s - archive=%s", ipaddr, infile);
  log_it(debug, tmp);
  rec_to_client.msg_type = trigger_restore;
  strncpy(rec_to_client.body, clientpath, sizeof(rec_to_client.body));
  strncpy(rec_to_client.bodyAux, auxpath, sizeof(rec_to_client.bodyAux));
  if (send_msg_to_client(&rec_to_client, ipaddr, port, &socket_fd))
    { log_it(error, "restore_client - failed to send msg to client"); return(1); }
  if (!(fin=fopen(infile, "r")))
    { log_it(fatal, "Failed to openin temp data file"); }
  res += transmit_file_to_socket(fin, socket_fd);

  len=read(socket_fd, (char*)&i, sizeof(i));
  if (!len) { res++; log_it(error, "Client hasn't told me the result of its call to mondorestore"); }
  else if (len!=sizeof(i)) { res++; log_it(error, "Client didn't sent _entire_ result of its call to mondorestore"); }
  else if (i) { res++; log_it(error, "Client said, mondorestore returned an error."); }
  else { log_it(debug, "Client said, mondorestore returned OK"); }

  fclose(fin);
  close(socket_fd);
  if (res>0)
    {
      sprintf(tmp, "%s - error(s) occurred while restoring %s", ipaddr, clientpath);
      log_it(error, tmp);
      rec_to_client.msg_type = restore_fail;
      sprintf(rec_to_client.body, "Failed to restore %s", clientpath);
      log_it(debug, rec_to_client.body);
    }
  else
    {
      sprintf(tmp, "%s - restored %s ok", ipaddr, clientpath);
      log_it(info, tmp);
      rec_to_client.msg_type = restore_ok;
      sprintf(rec_to_client.body, "%s - restored ok", clientpath);
      log_it(debug, rec_to_client.body);
    }
  if (send_msg_to_client(&rec_to_client, ipaddr, port, &socket_fd))
    {
      sprintf(tmp, "Unable to notify %s of restore success/failure", ipaddr);
      log_it(error, tmp);
      i = find_client_in_clientlist(ipaddr);
      if (i>=0) { forcibly_logout_client(i); }
    }
  return(res);
}


/*-----------------------------------------------------------*/



int send_msg_to_client(struct s_server2client_msg_record *rec, char *clientIP, int port, int *psocket)
/*
Purpose:Send a message from server to client.
A 'message' could be a response to a login/logout/ping
request or perhaps a 'trigger'. (A trigger is a message
from server to client intended to initiate a backup
or similar activity.)
Params:	rec - record containing the data to be sent to client
clientIP - the xxx.yyy.zzz.aaa IP address of client
port - the client's port to send data to
psocket - returns the socket's file descriptor, left
open by me; or, if psocket==NULL, then socket is closed
by me before I return.
Return: result (0=success, nonzero=failure)
*/
{
struct hostent *hp;
struct sockaddr_in sin;
int  s;
char tmp[MAX_STR_LEN+1];
if ((hp = gethostbyname(clientIP)) == NULL)
{
sprintf(tmp, "send_msg_to_client: %s: unknown host", clientIP);
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
if (psocket) { *psocket=s; } else { close(s); }
sprintf(tmp, "Sent %s msg [%s] to %s (port %d)", tmsg_to_string(rec->msg_type), rec->body, clientIP, port);
log_it(debug, tmp);
return(0);
}



/*-----------------------------------------------------------*/



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
  char tmp[MAX_STR_LEN+1];

  g_clientlist.items = 0;
  for(i=0; i<NOOF_THREADS; i++)
    {
      port = 8700+i;
      sprintf(tmp,"%d", port);
      res = pthread_create(&(g_threadinfo[i]), NULL, watch_port_for_requests_from_clients, (void*)tmp);
      if (res != 0)
        {
	  perror("Thread creation failed");
        }
      usleep(50000);
    }
  sprintf(tmp, "Now monitoring ports %d thru %d for requests from clients.", 8700, 8700+NOOF_THREADS-1);
  log_it(info, tmp);
}



/*-----------------------------------------------------------*/



void terminate_daemon(int sig)
/*
Purpose: Shut down the server in response to interrupt.
Params:  Signal received.
Returns: None
*/
{
  char command[MAX_STR_LEN+1];
  static bool logged_out_everyone=false;

  set_signals(false); // termination in progress
  log_it(info, "Abort signal caught by interrupt handler");
  if (!logged_out_everyone)
    {
// FIXME - lock the var w/mutex
      logged_out_everyone=true;
      forcibly_logout_all_clients();
    }
/*
  for(i=0; i<NOOF_THREADS; i--)
    {
      sprintf(tmp, "Terminating thread #%d", i);
      log_it(debug, tmp);
      pthread_join(g_threadinfo[i], NULL);
    }
*/
  forcibly_logout_all_clients();
  sprintf(command, "rm -f %s", g_command_fifo);
  call_program_and_log_output(command);
//  chmod(g_command_fifo, 0);
  unlink(g_command_fifo);
  unlink(g_server_status_file);
  register_pid(0, "server");
  log_it(info, "---------- Monitas (server) has terminated ----------");
  exit(0);
}



/*-----------------------------------------------------------*/



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
  char buf[MAX_STR_LEN+1], tmp[MAX_STR_LEN+1];
  int len, s, new_s;
  struct s_client2server_msg_record rec;

  watch_port = atoi((char*)sz_watchport);
//  sprintf(tmp, "watch_port_for_requests_from_clients(%d) - starting", watch_port); log_it(debug, tmp);
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
  sprintf(tmp, "Bound port #%d OK", watch_port);
  log_it(debug, tmp);
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



/*-----------------------------------------------------------*/



int main(int argc, char*argv[])
/*
Purpose: main subroutine
Parameters: none
Return: result (0=success, nonzero=failure)
*/
{
  pthread_t server_status_thread;

  log_it(info, "---------- Monitas (server) by Hugo Rabson ----------");
  register_pid(getpid(), "server");
  set_signals(true);
  start_threads_to_watch_ports_for_requests();
  pthread_create(&server_status_thread, NULL, generate_server_status_file_regularly, (void*)SERVER_STATUS_FILE);
  create_and_watch_fifo_for_commands(SERVER_COMDEV);
  log_it(warn, "Execution should never reach this point");
  exit(0);
}
/* end main() */


