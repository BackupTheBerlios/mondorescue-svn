/* client.c

CLIENT



FIXME
- perror() --- replace with log_it()



06/19
- fixed bugs in mondoarchive compare code
- added track_restore_task_progress()

06/16
- when calling mondoarchive in bkgd, see if it starts OK; if it
  doesn't then say so & return error

06/14
- added a FIFO to let user request backup/compare/restore
- send progress info to server - % done, etc. - when backing up
- pipe logs to logfile, not stdout

06/11
- added call to register_pid()
- commented code a bit
- implemented compare_archives and restore_archives()

06/10
- create function to call external executable in background
- create function to wait for it to terminate & to grab its result
- put them in common.c

05/27
- fixed watch_port_for_triggers()
- turned st_'s into a global and some locals
- fork in login thingy becomes a thread
- added bind_client_port()
- added accept_and_recv_thru_client_port()
- changed flag (recv) from DONTWAIT to 0 [no flag]

05/21
- added back_my_smitch_up(), compare_archives(), restore_archives()
- added log_it(); fixed fprintf(stderr,"") and printf() reporting
- parallelize/fork the 'watch for triggers from server' process
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
//#define LOG_THESE_AND_HIGHER debug
#define LOGFILE "/var/log/monitas-client.log"



/* global vars */

bool g_logged_in_currently=false, g_logging_out=false;
int g_sClient=-1, g_client_port=0; /* client port; set by login */
struct sockaddr_in g_sinClient; /* client port */
char g_server_name[MAX_STR_LEN+1];
pthread_t g_mondo_thread=0;
char g_command_fifo[MAX_STR_LEN+1];
char g_logfile[MAX_STR_LEN+1] = "/var/log/monitas-client.log";

/* externs */

extern char *call_program_and_get_last_line_of_output(char*);
extern int call_program_and_log_output(char*);
extern void call_program_in_background(pthread_t*, char*);
extern int create_and_watch_fifo_for_commands(char*);
extern bool does_file_exist(char*);
extern int get_bkgd_prog_result(pthread_t*);
extern void log_it_SUB(char*, t_loglevel level, char *sz_message);
extern bool program_still_running(char*);
extern int receive_file_from_socket(FILE*, int);
extern void register_pid(pid_t, char*);
extern char *tmsg_to_string(t_msg msg_type);
extern int transmit_file_to_socket(FILE*, int);
extern void register_pid(pid_t, char*);
extern void set_signals(bool);

/* prototypes */

int accept_and_recv_thru_client_port(int, int*, struct sockaddr_in*, char*, int);
int back_my_smitch_up(char*, int);
int bind_client_port(struct sockaddr_in*, int);
int compare_archives(char*, int);
int find_and_bind_free_server_port(struct sockaddr_in*, int*);
long increment_magic_number(void);
int login_to_server(char*,char*);
void logout_and_exit(char*);
int logout_of_server(char*);
int process_incoming_command(char*);
int restore_archives(char*, char*, int);
void restore_archives_SIGPIPE(int);
int send_final_progress_report(char*);
int send_msg_to_server(struct s_client2server_msg_record*, char*);
int send_ping_to_server(char*,char*);
int send_progress_rpt_to_server(char*, char*);
void terminate_daemon(int);
void *track_backup_task_progress(void*);
void *track_compare_task_progress(void*);
void *track_restore_task_progress(void*);
void *watch_port_for_triggers_from_server(void*);




/*-----------------------------------------------------------*/



int accept_and_recv_thru_client_port(int sClient, int *new_sClient, struct sockaddr_in*sinClient, char*incoming, int expected_length)
/*
Purpose:Run accept() and recv() to open port and receive
	message from server.
Params:	sClient - file descriptor of port
	new_sClient - [returned] file descriptor of the
	new connection to port which we open in this func
	sinClient - record about port
	expected_length - expected length of incoming block
Return: length of block received, or <0 if error
*/
{
      int len;

      len = sizeof(struct sockaddr_in);
      if ((*new_sClient = accept(sClient, (struct sockaddr*)sinClient, (unsigned int*)&len)) < 0) { log_it(error, "[child] Cannot accept"); return(-6); }
      if ((len = recv(*new_sClient, incoming, expected_length, /*MSG_DONTWAIT*/0)) <= 0) { log_it(error, "[child] Cannot recv"); return(-7); }
      return(len);
}



/*-----------------------------------------------------------*/



char *get_param_from_rcfile(char*fname, char*field)
{
  char command[MAX_STR_LEN+1], tmp[MAX_STR_LEN+1];
  static char sz_res[MAX_STR_LEN+1];

  sz_res[0]='\0';
  if (does_file_exist(fname))
    {
      sprintf(command, "cat %s | grep %s= | cut -d'=' -f2,3,4,5,6,7,8,9", fname, field);
      strcpy(tmp, call_program_and_get_last_line_of_output(command));
      strcpy(sz_res, tmp);
    }
  return(sz_res);
}



int back_my_smitch_up(char*msgbody, int socket_fd)
/*
Purpose:Backup archives to server.
Params: msgbody - char[MSG_BODY_SIZE] containing info
	about the archives to be created
	socket_fd - file descriptor to which to
	write the archives to server.
Return: result (0=success; nonzero=failure)
*/
{
  char tmp[MAX_STR_LEN+1], command[MAX_STR_LEN+1], tempdev[MAX_STR_LEN+1];
  char temporary_logfile[MAX_STR_LEN+1]; // where mondoarchive writes its stdout,stderr
  char mondoparams_str[MAX_STR_LEN+1];
  struct s_server2client_msg_record incoming_rec;
  int retval=0, len, new_sClient, res=0;
  FILE*fin;
  pthread_t progress_thread;

  sprintf(tmp, "Backup of %s commenced", msgbody);
  log_it(info, tmp);
  if (send_progress_rpt_to_server(g_server_name, tmp)) { log_it(error, "Unable to send 'yep, got this msg' progress_rpt to server"); }
  sprintf(temporary_logfile, "/tmp/monitas-client.templog.%d", (int)random()%32767);
  sprintf(tempdev, "/tmp/monitas-client.device.%d", (int)random()%32767);
  unlink(tempdev);
  if (mkfifo(tempdev, 700))
    {
      log_it(error, "Unable to create temporary data output fifo in preparation for the call to mondoarchive");
      return(1);
    }
  unlink(temporary_logfile);
  if (mkfifo(temporary_logfile, 700))
    {
      log_it(error, "Unable to create temporary logfile fifo in preparation for the call to mondoarchive");
      return(1);
    }
  strcpy(mondoparams_str, get_param_from_rcfile(CLIENT_RCFILE, "mondoarchive_params"));
  sprintf(tmp, "mondoarchive_params --> '%s'", mondoparams_str);
  log_it(debug, tmp);
  sprintf(command, "mondoarchive -Ou %s -d %s -I %s -F &> %s; rm -f %s %s", mondoparams_str, tempdev, msgbody, temporary_logfile, tempdev, temporary_logfile);
  call_program_in_background(&g_mondo_thread, command);
  sleep(10);
  if (!program_still_running(command))
    { res=1; log_it(error, "Unable to start mondoarchive. Please check /var/log/mondo-archive.log"); }
  else
    {
      if (pthread_create(&progress_thread, NULL, track_backup_task_progress, (void*)temporary_logfile))
        { log_it(error, "Cannot create pthread to track mondo task progress"); return(1); }
      log_it(debug, "Opening fopen() to tempdev");
      if (!(fin = fopen(tempdev, "r"))) { log_it(error, "Cannot open FIFO"); return(1); }
      log_it(debug, "fopen() OK");
      retval = transmit_file_to_socket(fin, socket_fd);
      fclose(fin);
      res = get_bkgd_prog_result(&g_mondo_thread);
      pthread_join(progress_thread, NULL);
    }
  if (res)
    { retval++; log_it(error, "Mondoarchive returned an error. Notifying server..."); }
  if (res)
    { retval++; log_it(error, "Mondoarchive returned an error."); }
  if (retval) { log_it(debug, "Errors have occurred. Notifing server..."); }
  else        { log_it(debug, "Everything is OK so far. Notifying server..."); }
  if (write(socket_fd, (char*)&retval, sizeof(retval))!=sizeof(retval)) {retval++;}
/* receive msg from server; did backup go OK at its end or not? */
  unlink(tempdev);
  unlink(temporary_logfile);
  log_it(debug, "Waiting for progress thread to join us");
  log_it(debug, "Groovy. Continuing..");
  len = accept_and_recv_thru_client_port(g_sClient, &new_sClient, &g_sinClient, (char*)&incoming_rec, sizeof(incoming_rec));
  if (len<0) { log_it(error, "After backup, unable to accept/recv thru client port"); return(-10); }
  sprintf(tmp, "After backup, I received %s - %s", tmsg_to_string(incoming_rec.msg_type), incoming_rec.body);
  log_it(debug, tmp);
  if (incoming_rec.msg_type == backup_fail)
    { retval++; log_it(error, "Server reported error(s) during backup, although client didn't."); }
  if (retval)
    {
      sprintf(tmp, "Backup of %s failed", msgbody);
      log_it(error, tmp);
      call_program_and_log_output("tail -n6 /var/log/mondo-archive.log");
    }
  else
    {
      sprintf(tmp, "Server agrees, backup of %s succeeded :-)", msgbody);
      log_it(info, tmp);
    }
  if (send_final_progress_report(tmp) < 0) { retval++; log_it(error, "Unable to send final progress_rpt to server"); }
  return(retval);
}



/*-----------------------------------------------------------*/



int bind_client_port(struct sockaddr_in *sinClient, int client_port)
/*
Purpose:Bind one of my ports so that I may open it later and
        write/read data to/from server with it.
Params: sinClient - record/structure relating to the socket
        client_port - port# to be bound
Return: socket handle, to be used by other subroutines, if success
        or <0 if failure
*/
{
      int sClient=-1;
      char tmp[MAX_STR_LEN+1];

      memset((void*)sinClient, 0, sizeof(struct sockaddr_in));
      sinClient->sin_family = AF_INET;
      sinClient->sin_addr.s_addr = INADDR_ANY;
      sinClient->sin_port = htons(client_port);
      if ((sClient = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
	  sprintf(tmp, "Unable to open socket on port #%d", g_client_port);
	  log_it(error, tmp);
          return(-1);
	}
      if (bind(sClient, (struct sockaddr*)sinClient, sizeof(struct sockaddr_in)) < 0)
	{
	  sprintf(tmp, "Cannot bind %d - %s", g_client_port, strerror(errno));
	  log_it(error, tmp);
	  return(-2);
	}
      if (listen(sClient, MAX_PENDING) < 0)
	{
	  sprintf(tmp, "Cannot setup listen (%d) - %sn", g_client_port, strerror(errno));
	  log_it(error, tmp);
	  return(-3);
	}
      log_it(debug, "Successfully bound client port.");
      return(sClient);
}



/*-----------------------------------------------------------*/



int compare_archives(char*msgbody, int socket_fd)
/*
Purpose:Compare archives, sent by server.
Params: msgbody - char[MSG_BODY_SIZE] containing info
about the archives to be compared
socket_fd - file descriptor from which to
read the archives sent by server to be compared.
Return: result (0=success; nonzero=failure)
*/
{
  char tmp[MAX_STR_LEN+1], command[MAX_STR_LEN+1], tempdev[MAX_STR_LEN+1], *p;
  char temporary_logfile[MAX_STR_LEN+1]; // where mondoarchive writes its stdout,stderr
  char mondoparams_str[MAX_STR_LEN+1];
  struct s_server2client_msg_record incoming_rec;
  int retval=0, len, new_sClient, res=0;
  FILE*fout, *fin;
  long diffs=0;
  pthread_t progress_thread;

  sprintf(tmp, "Comparison of %s commenced", msgbody);
  log_it(info, tmp);
  if (send_progress_rpt_to_server(g_server_name, tmp)) { log_it(error, "Unable to send 'yep, got this msg' progress_rpt to server"); }
  sprintf(temporary_logfile, "/tmp/monitas-client.templog.%d", (int)random()%32767);
  sprintf(tempdev, "/tmp/monitas-client.device.%d", (int)random()%32767);
  unlink(tempdev);
  if (mkfifo(tempdev, 700))
    {
      log_it(error, "Unable to create temporary fifo in preparation for the call to mondoarchive");
      return(1);
    }
  unlink(temporary_logfile);
  if (mkfifo(temporary_logfile, 700))
    {
      log_it(error, "Unable to create temporary logfile fifo in preparation for the call to mondoarchive");
      return(1);
    }
  strcpy(mondoparams_str, get_param_from_rcfile(CLIENT_RCFILE, "mondoarchive_params"));
  sprintf(tmp, "mondoarchive_params --> '%s'", mondoparams_str);
  log_it(debug, tmp);
  sprintf(command, "mondoarchive -Vu -F %s -d %s -I %s > %s"/*; rm -f %s %s"*/, mondoparams_str, tempdev, msgbody, temporary_logfile/*, tempdev, temporary_logfile*/);
  call_program_in_background(&g_mondo_thread, command);
  sleep(5);
  if (!program_still_running(command))
    { res=1; log_it(error, "Unable to start mondoarchive. Please check /var/log/mondo-archive.log"); }
  else
    {
      if (pthread_create(&progress_thread, NULL, track_compare_task_progress, (void*)temporary_logfile))
        { log_it(error, "Cannot create pthread to track mondo task progress"); return(1); }
      fout = fopen(tempdev, "w");
      log_it(debug, "Opened fopen() to tempdev");
      retval = receive_file_from_socket(fout, socket_fd);
      log_it(debug, "Calling get_bkgd_prog_result");
      fclose(fout);
      res = get_bkgd_prog_result(&g_mondo_thread);
      res = 0; // *shrug* Seems to help :)
      pthread_join(progress_thread, NULL);
    }
  if (res)
    { retval++; log_it(error, "Mondoarchive returned an error."); }
  if (retval) { log_it(debug, "Errors have occurred. Notifing server..."); }
  else        { log_it(debug, "Everything is OK so far. Notifying server..."); }
  if (write(socket_fd, (char*)&retval, sizeof(retval))!=sizeof(retval)) {retval++;}
/* receive msg from server; did comparison go OK at its end or not? */
  unlink(tempdev);
  unlink(temporary_logfile);
  len = accept_and_recv_thru_client_port(g_sClient, &new_sClient, &g_sinClient, (char*)&incoming_rec, sizeof(incoming_rec));
  if (len<0) { log_it(error, "After comparison, unable to accept/recv thru client port"); return(-10); }
  sprintf(tmp, "After comparison, I received %s - %s", tmsg_to_string(incoming_rec.msg_type), incoming_rec.body);
  log_it(debug, tmp);
  if (incoming_rec.msg_type == compare_fail)
    { retval++; log_it(error, "Server reported error(s) during comparison, although client didn't."); }
  if (retval)
    {
      sprintf(tmp, "Errors occurred during comparison of %s", msgbody);
      strcpy(command, call_program_and_get_last_line_of_output("tail -n20 /var/log/mondo-archive.log | grep /tmp/changed | head -n1"));
      p = strstr(command, "/tmp/changed");
      if (p)
        {
          strcat(command, " ");
          sprintf(tmp, "command = '%s'", command);
          log_it(debug, tmp);
          *(strchr(p, ' '))='\0';
          sprintf(tmp, "Opening list of changed files ('%s')", p);
          log_it(debug, tmp);
          log_it(info, "---Changed files---");
          if ((fin=fopen(p, "r")))
            { for(diffs=0; !feof(fin); diffs++) { fgets(tmp, MAX_STR_LEN, fin); if (strlen(tmp)>0) {tmp[strlen(tmp)-1]='\0';} log_it(info, tmp); } fclose(fin); }
          log_it(info, "----End of list----");
          sprintf(tmp, "%ld differences were found during comparison of %s", diffs, msgbody);
          log_it(warn, tmp);
          unlink(p);
        }
      else
        {
          sprintf(tmp, "Errors occurred during comparison of %s", msgbody);
          log_it(error, tmp);
          call_program_and_log_output("tail -n6 /var/log/mondo-archive.log");
          log_it(info, "Please check /var/log/mondo-archive.log for more information");
        }
    }
  else
    {
      sprintf(tmp, "Server agrees, comparison of %s succeeded :-)", msgbody);
      log_it(info, tmp);
    }
  if (send_final_progress_report(tmp)) { retval++; log_it(error, "Unable to send final progress_rpt to server"); }
  return(retval);
}



/*-----------------------------------------------------------*/



int find_and_bind_free_server_port(struct sockaddr_in *sin, int *p_s)
/*
Purpose:Find a free port on the server. Bind to it, so that
	whichever subroutine called me can then send data
	to the server.
Params: sin - server's IP address in a structure
	p_s - [return] file descriptor of port binding
Return: result (>0=success, -1=failure)
*/
{
  int server_port;
  char tmp[MAX_STR_LEN+1];

  for(server_port = 8700; server_port < 8710; server_port++)
    {
      sin->sin_port = htons(server_port);
      if ((*p_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
	  perror("socket");
	  return(-1);
	}
      if (connect(*p_s, (struct sockaddr*)sin, sizeof(struct sockaddr_in)) < 0)
	{
	  sprintf(tmp, "Not connecting at %d", server_port);
	  log_it(debug, tmp);
	  continue;
	}
      return(server_port);
    }
  return(-1);
}



/*-----------------------------------------------------------*/



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



/*-----------------------------------------------------------*/



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

  orig_rec.msg_type = login;
  strncpy(orig_rec.body, hostname, sizeof(orig_rec.body));
  if (send_msg_to_server(&orig_rec, servername) < 0)
    { return(-1); }
  else
    { return(orig_rec.port); }
}



/*-----------------------------------------------------------*/




void logout_and_exit(char*servername)
/*
Purpose:Logout of server. Terminate.
Params: servername - ip address of server
Return: none
*/
{
  if (g_logged_in_currently)
    {
      if (logout_of_server(servername))
        { log_it(warn, "Failed to logout of server."); }
    }
  call_program_and_log_output("rm -Rf /tmp/monitas-client.*");
  register_pid(0, "client");
//  chmod(g_command_fifo, 0);
  unlink(g_command_fifo);
  log_it(info, "---------- Monitas (client) has terminated ----------");
  exit(0);
}



/*-----------------------------------------------------------*/



int logout_of_server(char*servername)
/*
Purpose:Instruct server to log client out.
Params: servername - hostname of server
Return: result (0=success; nonzero=failure)
*/
{
  struct s_client2server_msg_record orig_rec;

  g_logging_out = true;
  log_it(debug, "Logging out of server");
  orig_rec.msg_type = logout;
  strncpy(orig_rec.body, "Bye bye!", sizeof(orig_rec.body));
  if (send_msg_to_server(&orig_rec, servername) < 0)
    { return(1); }
  else
    { return(0); }
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
  int res=0;
  char tmp[MAX_STR_LEN+1];
  int pos, i;
  char command[MAX_STR_LEN+1], path[MAX_STR_LEN+1], aux[MAX_STR_LEN+1];
  struct s_client2server_msg_record orig_rec;

  pos=0;
  sscanf(incoming, "%s %s", command, path);
  for(i=0; i<strlen(command); i++) { command[i]=command[i]|0x60; }
  if (!strcmp(command, "restore"))
    { sscanf(incoming, "%s %s %s", command, path, aux); }
  else
    { aux[0] = '\0'; }
  sprintf(tmp, "cmd=%s path=%s aux=%s", command, path, aux);
  log_it(debug, tmp);
  sprintf(tmp, "%s of %s [aux='%s'] <-- command received", command, path, aux);
  log_it(info, tmp);
  if (strcmp(command, "restore") && aux[0]!='\0')
    { log_it(warn, "Ignoring auxiliary parameter: it is superfluous."); }
  if (strcmp(command, "backup") && strcmp(command, "compare") && strcmp(command, "restore"))
    {
      sprintf(tmp, "%s - command unknown.", command);
      log_it(error, tmp);
      res=1;
    }
  else
    {
      sprintf(tmp, "'%s' sent to server as a formal request", incoming);
      orig_rec.msg_type = user_req;
      strncpy(orig_rec.body, incoming, sizeof(orig_rec.body));
      if (send_msg_to_server(&orig_rec, g_server_name) < 0)
        { res++; log_it(error, "Unable to send user req to server"); }
      else
        { log_it(debug, tmp); }
    }
  return(res);
}



/*-----------------------------------------------------------*/



int restore_archives(char*msgbody, char*msgbodyAux, int socket_fd)
/*
Purpose:Restore archives, sent by server.
Params: msgbody - char[MSG_BODY_SIZE] containing info
about the archives to be restored
socket_fd - file descriptor from which to
read the archives sent by server.
Return: result (0=success; nonzero=failure)
*/
{
  char tmp[MAX_STR_LEN+1], command[MAX_STR_LEN+1], tempdev[MAX_STR_LEN+1], *p;
  char temporary_logfile[MAX_STR_LEN+1]; // where mondorestore writes its stdout,stderr
  struct s_server2client_msg_record incoming_rec;
  int retval=0, len, new_sClient, res=0;
  FILE*fout, *fin;
  long diffs=0;
  pthread_t progress_thread;
  
  sprintf(tmp, "Restoration of %s commenced", msgbody);
  log_it(info, tmp);
  if (send_progress_rpt_to_server(g_server_name, tmp)) { log_it(error, "Unable to send 'yep, got this msg' progress_rpt to server"); }
  sprintf(temporary_logfile, "/tmp/monitas-client.templog.%d", (int)random()%32767);
  sprintf(tempdev, "/tmp/monitas-client.device.%d", (int)random()%32767);
  unlink(tempdev);
  if (mkfifo(tempdev, 700))
    {
      log_it(error, "Unable to create temporary fifo in preparation for the call to mondorestore");
      return(1);
    }
  unlink(temporary_logfile);
  if (mkfifo(temporary_logfile, 700))
    {
      log_it(error, "Unable to create temporary logfile in preparation for the call to mondorestore");
      return(1);
    }
  sprintf(command, "mondorestore --monitas-live %s %s %s &> %s", tempdev, msgbody, msgbodyAux, temporary_logfile);
//  sprintf(command, "cat %s > %s", tempdev, "/tmp/out.dat");
  call_program_in_background(&g_mondo_thread, command);
  sleep(5);
  if (!program_still_running(command))
    {
      res=1; log_it(error, "mondorestore could not be started. Please check /tmp/mondo-restore.log");
    }
  else
    {
      if (pthread_create(&progress_thread, NULL, track_restore_task_progress, (void*)temporary_logfile))
        { log_it(error, "Cannot create pthread to track mondo task progress"); return(1); }
      fout = fopen(tempdev, "w");
      log_it(debug, "Opened fopen() to tempdev");
      retval = receive_file_from_socket(fout, socket_fd);
      if (retval && !system("cat /tmp/mondo-restore.log | grep -i \"End of restore_live_from_monitas_server\" &> /dev/null"))
        {
          retval=0;
          log_it(debug, "SIGPIPE caught but that's OK, it was anticipated.");
          res=0;
        }
      log_it(debug, "Calling get_bkgd_prog_result");
      fclose(fout);
      res = get_bkgd_prog_result(&g_mondo_thread);
      pthread_join(progress_thread, NULL);
    }
  unlink(tempdev);
  unlink(temporary_logfile);
  if (res)
    { retval++; log_it(error, "mondorestore returned an error."); }
  if (retval) { log_it(debug, "Errors have occurred. Notifing server..."); }
  else        { log_it(debug, "Everything is OK so far. Notifying server..."); }
  sleep(1); // probably unnecessary
/* I do this thrice because mondorestore often causes a SIGPIPE, which means... I have to do this thrice :-) */
  if (write(socket_fd, (char*)&retval, sizeof(retval))!=sizeof(retval)) {log_it(debug, "Failed to write wtf-info"); retval++;}
  if (write(socket_fd, (char*)&retval, sizeof(retval))!=sizeof(retval)) {log_it(debug, "Failed to write wtf-info"); retval++;}
  if (write(socket_fd, (char*)&retval, sizeof(retval))!=sizeof(retval)) {log_it(debug, "Failed to write wtf-info"); retval++;}
/* receive msg from server; did restoration go OK at its end or not? */
  unlink(tempdev);
  unlink(temporary_logfile);
  len = accept_and_recv_thru_client_port(g_sClient, &new_sClient, &g_sinClient, (char*)&incoming_rec, sizeof(incoming_rec));
  if (len<0) { log_it(error, "After restoration, unable to accept/recv thru client port"); return(-10); }
  sprintf(tmp, "After restoration, I received %s - %s", tmsg_to_string(incoming_rec.msg_type), incoming_rec.body);
  log_it(debug, tmp);
  if (incoming_rec.msg_type == restore_fail)
    { retval++; log_it(error, "Server reported error(s) during restoration, although client didn't."); }
  if (retval)
    {
      sprintf(tmp, "Errors occurred during restoration of %s", msgbody);
      strcpy(command, call_program_and_get_last_line_of_output("tail -n20 /var/log/mondo-archive.log | grep /tmp/changed | head -n1"));
      p = strstr(command, "/tmp/changed");
      if (p)
        {
          strcat(command, " ");
          sprintf(tmp, "command = '%s'", command);
          log_it(debug, tmp);
          *(strchr(p, ' '))='\0';
          sprintf(tmp, "Opening list of changed files ('%s')", p);
          log_it(debug, tmp);
          log_it(info, "---Changed files---");
          if ((fin=fopen(p, "r")))
            { for(diffs=0; !feof(fin); diffs++) { fgets(tmp, MAX_STR_LEN, fin); if (strlen(tmp)>0) {tmp[strlen(tmp)-1]='\0';} log_it(info, tmp); } fclose(fin); }
          log_it(info, "----End of list----");
          sprintf(tmp, "%ld differences were found during restoration of %s", diffs, msgbody);
          log_it(warn, tmp);
        }
      else
        {
          sprintf(tmp, "Errors occurred during restoration of %s", msgbody);
          log_it(error, tmp);
          call_program_and_log_output("tail -n6 /var/log/mondo-archive.log");
          log_it(info, "Please check /var/log/mondo-archive.log for more information");
        }
    }
  else
    {
      sprintf(tmp, "Server agrees, restoration of %s succeeded :-)", msgbody);
      log_it(info, tmp);
    }
  if (send_final_progress_report(tmp)) { retval++; log_it(error, "Unable to send final progress_rpt to server"); }
  return(retval);
}



/*-----------------------------------------------------------*/



void *send_final_progress_report_SUB(void*inp)
{
  char message[MAX_STR_LEN+1];

  strncpy(message, (char*)inp, MAX_STR_LEN);
  sleep(10);
  send_progress_rpt_to_server(g_server_name, message);
  pthread_exit(NULL);
}



int send_final_progress_report(char*final_string)
{
  pthread_t thread;

  if (send_progress_rpt_to_server(g_server_name, final_string)) { return(1); }
  if (pthread_create(&thread, NULL, send_final_progress_report_SUB, (void*)"Idle")) { log_it(error, "Unable to create pthread"); return(1); }
  return(0);
}



/*-----------------------------------------------------------*/



int send_msg_to_server(struct s_client2server_msg_record *rec, char *servername)
/*
Purpose:Send message to server - a login/logout/ping request
        or perhaps a request for data to be restored.
Params: rec - the message to be sent to server
        servername - the hostname of server
Return: result (<0=failure, 0+=success)
*/
{
  struct hostent *hp;
  struct sockaddr_in sin;
  int server_port, new_sClient, s, len, res;
  struct s_server2client_msg_record incoming_rec;
  struct pollfd ufds;
  char tmp[MAX_STR_LEN+1];
  void *thread_result;
  pthread_t a_thread;

/* If logging out then kill the trigger-watcher before trying;
otherwise, the trigger-watcher will probably catch the
'logout_ok' packet and go nutty on us :-)
*/
  if ((hp = gethostbyname(servername)) == NULL)
    {
      sprintf(tmp, "%s: unknown host", servername);
      log_it(error, tmp);
      return(-1);
    }
  if (g_logged_in_currently && rec->msg_type == login) { log_it(error, "Already logged in. Why try again?"); return(-1); }
  if (!g_logged_in_currently && rec->msg_type != login)
    {
      log_it(fatal, "Server has forcibly logged you out.");
    } /* or you never logged in to begin which, which suggests the programmer screwed up */
  /* open client port if login */
  if (rec->msg_type == login)
    {
      log_it(info, "Trying to login");
      g_client_port = 8800 + rand()%100; // FIXME: retry if I can't use this port
      g_sClient = bind_client_port(&g_sinClient, g_client_port);
      if (g_sClient<0) { log_it(error, "Cannot bind client port"); return(-1); }
    }
  /* send msg to server */
  rec->port = g_client_port;
  memset((void*)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  memcpy((void*)&sin.sin_addr, hp->h_addr, hp->h_length);
  server_port = find_and_bind_free_server_port(&sin, &s);
  if (server_port<=0)
    {
      sprintf(tmp, "Cannot find+bind free server port. Is server running?");
      log_it(error, tmp);
      return(-3);
    }
  rec->magic = increment_magic_number();
  send (s, (char*)rec, sizeof(struct s_client2server_msg_record), 0);
  close(s);
  /* wait for ack/pong/feedback (if logging in/out) */
  if (rec->msg_type == login /* || rec->msg_type == logout */)
    {
      ufds.fd = g_sClient;
      ufds.events = POLLIN|POLLPRI;
      poll(&ufds, 1, 1000);
      if (!ufds.revents) { log_it(error, "Failed to poll"); return(-5); }
      len = accept_and_recv_thru_client_port(g_sClient, &new_sClient, &g_sinClient, (char*)&incoming_rec, sizeof(incoming_rec));
      if (len<0) { log_it(error, "Unable to accept/recv thru client port"); return(-10); }
      sprintf(tmp, "Received %s - %s", tmsg_to_string(incoming_rec.msg_type), incoming_rec.body);
      log_it(info, tmp);
      if (rec->msg_type != login && incoming_rec.msg_type == login_ok) { log_it(error, "WTF? login_ok but I wasn't logging in"); }
      if (rec->msg_type != logout&& incoming_rec.msg_type == logout_ok){ log_it(error, "WTF? logout_ok but I wasn't logging out"); }
      close(new_sClient);
      new_sClient=-1;
    }
  if (incoming_rec.msg_type == backup_fail) { log_it(error, "Failed, says server"); return(-8); }
  /* fork the process which watches in the background for pings/requests/etc. */
  if (rec->msg_type == login)
    {
      g_logged_in_currently = true;
      strcpy(tmp, "Hello world!");
      res = pthread_create(&a_thread, NULL, watch_port_for_triggers_from_server, (void*)tmp);
      log_it(debug, "Returning from login + call to pthread_create");
      return(server_port);
    }
  if (rec->msg_type == logout)
    {
      log_it(debug, "Calling pthread_join to reunite trigger-watcher and main loop");
      res = pthread_join(a_thread, &thread_result);
      if (res) { /*perror("Thread join failed");*/ log_it(debug, "Thread join failed in send_msg_to_server()"); }
      else { sprintf(tmp, "Thread join succeeded. Result: %s", (char*)thread_result); log_it(debug, tmp); }
    }
  return(0);
}



/*-----------------------------------------------------------*/



int send_ping_to_server(char*servername, char*msg)
/*
Purpose:Send a 'ping' to server.
Params: msg - string to send ("Hello world"?)
	servername - server's hostname
Return: result (0=success, nonzero=failure)
*/
{
  struct s_client2server_msg_record orig_rec;
  orig_rec.msg_type = ping;
  strncpy(orig_rec.body, msg, sizeof(orig_rec.body));
  if (send_msg_to_server(&orig_rec, servername) < 0)
    { return(1); }
  else
    { return(0); }
}



/*-----------------------------------------------------------*/



int send_progress_rpt_to_server(char*servername, char*msg)
/*
Purpose:Send a 'progress_rpt' string to server.
Params: msg - string to send ("Hello world"?)
	servername - server's hostname
Return: result (0=success, nonzero=failure)
*/
{
  struct s_client2server_msg_record orig_rec;
  orig_rec.msg_type = progress_rpt;
  strncpy(orig_rec.body, msg, sizeof(orig_rec.body));
  if (send_msg_to_server(&orig_rec, servername) < 0)
    { return(1); }
  else
    { return(0); }
}



/*-----------------------------------------------------------*/



void terminate_daemon(int sig)
/*
Purpose: Shut down the server in response to interrupt.
Params:  Signal received.
Returns: None
*/
{
  int res;

// FIXME - takes server 1-2 mins to realize I've aborted. I want that to be 5-10 seconds :)
  set_signals(false); // termination in progress
  log_it(info, "Abort signal caught by interrupt handler");
  call_program_and_log_output("kill `ps ax | grep mondo | grep -v \"ps ax\" | grep -v \"grep mondo\" | grep monitas | cut -d' ' -f1`");
  if (send_ping_to_server(g_server_name, "I'm pinging you before I logout"))
    { log_it(error, "Server has shut down without warning."); }
  if (g_mondo_thread)
    {
      res = get_bkgd_prog_result(&g_mondo_thread);
    }
  logout_and_exit(g_server_name);
}



/*-----------------------------------------------------------*/



void *track_backup_task_progress(void*inp)
{
  char fname[MAX_STR_LEN+1],
	tmp[MAX_STR_LEN+1],
	progress_str[MAX_STR_LEN+1],
	old_progstr[MAX_STR_LEN+1],
	*p;
  struct s_client2server_msg_record rec;
  FILE*fin;
  bool biggies=false, regulars=false;
  int prev_percentage=0, i;

  rec.msg_type = progress_rpt;
  strcpy(fname, (char*)inp);
  log_it(debug, "track_backup_task_progres() --- entering");
  fin = fopen(fname, "r");
  old_progstr[0]='\0';
  sprintf(tmp, "fname = %s", fname);
  log_it(debug, tmp);
  sleep(2);
  if (!does_file_exist(fname)) { log_it(fatal, "track_backup_task_progress() -- fname not found"); }
  strcpy(rec.body, "Working");
  if (send_msg_to_server(&rec, g_server_name) < 0) { log_it(error, "Failed to send progress_rpt msg to server."); pthread_exit(NULL); }
  log_it(debug, rec.body);
/* skip initial stuff */
  while(!feof(fin))
    {
      fgets(progress_str, MAX_STR_LEN, fin);
      progress_str[MAX_STR_LEN]='\0';
      if (strlen(progress_str)<2) { continue; }
      if (strstr(progress_str, "TASK")) { break; }
    }
/* skip more stuff */
  while(!feof(fin))
    {
      fgets(progress_str, MAX_STR_LEN, fin);
      progress_str[MAX_STR_LEN]='\0';
      if (strlen(progress_str)<2) { continue; }
      if (!strstr(progress_str, "TASK")) { break; }
    }
/* report on regular+biggie files */
  while(!feof(fin))
    {
      fgets(progress_str, MAX_STR_LEN, fin);
      progress_str[MAX_STR_LEN]='\0';
      if (strstr(progress_str, "rchiving set 0"))
        { prev_percentage = 0; regulars = true; }
      if (strstr(progress_str, "acking up all large files"))
        { prev_percentage = 0; regulars = false; biggies = true; }
      if (strlen(progress_str)<2) { continue; }
      if (!strstr(progress_str, "TASK")) { continue; }
      progress_str[strlen(progress_str)-1] = '\0';
      log_it(debug, progress_str);
      if (!biggies && !regulars)
        { strcpy(progress_str, "Still working..."); }
      if (strcmp(progress_str, old_progstr))
        {
          strcpy(old_progstr, progress_str);
          if (biggies)
            { sprintf(rec.body, "Large files: %s", progress_str+6); }
          else if (regulars)
            { sprintf(rec.body, "Regular files: %s", progress_str+6); }
	  if (!(p=strstr(progress_str, "% done"))) { continue; }

          log_it(info, rec.body);
          if (send_progress_rpt_to_server(g_server_name, rec.body)) { log_it(error, "Failed to send progress_rpt msg to server."); pthread_exit(NULL); }
          continue;

          while(!isdigit(*(p-1))) { p--; }
          strcpy(tmp, p);
          *(strchr(tmp, '%')) = '\0';
          i = atoi(tmp);
          if (i > prev_percentage)
            {
              prev_percentage = i;
              log_it(info, rec.body);
// FIXME - could simply call send_progress_rpt_to_server()
              if (send_msg_to_server(&rec, g_server_name) < 0) { log_it(error, "Failed to send progress_rpt msg to server."); pthread_exit(NULL); }
            }
        }
    }
  log_it(debug, "track_backup_task_progress() --- leaving");
  pthread_exit(NULL);
}



/*-----------------------------------------------------------*/



void *track_compare_task_progress(void*inp)
{
  char fname[MAX_STR_LEN+1],
	tmp[MAX_STR_LEN+1],
	progress_str[MAX_STR_LEN+1],
	old_progstr[MAX_STR_LEN+1],
	*p;
//  struct s_client2server_msg_record rec;
  FILE*fin;
  bool biggies=false, regulars=false;
  int prev_percentage=0, i;

  strcpy(fname, (char*)inp);
  log_it(debug, "track_compare_task_progress() --- entering");
  if (!(fin = fopen(fname, "r"))) { log_it(fatal, "Unable to openin tempdev while comparing"); }
  old_progstr[0]='\0';
  sprintf(tmp, "fname = %s", fname);
  log_it(debug, tmp);
  sleep(2);
  if (!does_file_exist(fname)) { log_it(fatal, "track_compare_task_progress() -- fname not found"); }
  log_it(debug, "Working");
  if (send_progress_rpt_to_server(g_server_name, "Working")) { log_it(error, "Failed to send progress_rpt msg to server."); pthread_exit(NULL); }

/* report on regular+biggie files */
  while(!feof(fin))
    {
      fgets(progress_str, MAX_STR_LEN, fin);
      progress_str[MAX_STR_LEN]='\0';
      log_it(debug, progress_str);
      if (strstr(progress_str, "erifying fileset #0"))
        { prev_percentage = 0; regulars = true; }
      if (strstr(progress_str, "erifying all bigfiles"))
        { prev_percentage = 0; regulars = false; biggies = true; }
      if (strlen(progress_str)<2) { continue; }
      if (!strstr(progress_str, "TASK")) { continue; }
      progress_str[strlen(progress_str)-1] = '\0';
      if (!biggies && !regulars)
        { strcpy(progress_str, "Still working..."); }
      if (strcmp(progress_str, old_progstr))
        {
          strcpy(old_progstr, progress_str);
          if (biggies)
            { sprintf(tmp, "Large files: %s", progress_str+6); }
          else if (regulars)
            { sprintf(tmp, "Regular files: %s", progress_str+6); }
	  if (!(p=strstr(progress_str, "% done"))) { continue; }

          log_it(info, tmp);
          if (send_progress_rpt_to_server(g_server_name, tmp)) { log_it(error, "Failed to send progress_rpt msg to server."); pthread_exit(NULL); }
          continue;

          while(!isdigit(*(p-1))) { p--; }
          strcpy(tmp, p);
          *(strchr(tmp, '%')) = '\0';
          i = atoi(tmp);
          if (i > prev_percentage)
            {
              prev_percentage = i;
              log_it(info, tmp);
// FIXME - could simply call send_progress_rpt_to_server()
              if (send_progress_rpt_to_server(g_server_name, tmp)) { log_it(error, "Failed to send progress_rpt msg to server."); pthread_exit(NULL); }
            }
        }
    }
  log_it(debug, "track_compare_task_progress() --- leaving");
  pthread_exit(NULL);
}



/*-----------------------------------------------------------*/



void *track_restore_task_progress(void*inp)
{
  char fname[MAX_STR_LEN+1],
	tmp[MAX_STR_LEN+1],
	progress_str[MAX_STR_LEN+1],
	old_progstr[MAX_STR_LEN+1],
	*p;
//  struct s_client2server_msg_record rec;
  FILE*fin;
  bool biggies=false, regulars=false;
  int prev_percentage=0, i;

  strcpy(fname, (char*)inp);
  log_it(debug, "track_restore_task_progress() --- entering");
  if (!(fin = fopen(fname, "r"))) { log_it(fatal, "Unable to openin tempdev while restoring"); }
  old_progstr[0]='\0';
  sprintf(tmp, "fname = %s", fname);
  log_it(debug, tmp);
  sleep(2);
  if (!does_file_exist(fname)) { log_it(fatal, "track_restore_task_progress() -- fname not found"); }
  log_it(debug, "Working");
  if (send_progress_rpt_to_server(g_server_name, "Working")) { log_it(error, "Failed to send progress_rpt msg to server."); pthread_exit(NULL); }

// FIXME --- mondorestore does not yet spit out its progress in
// a user-friendly format; so, most of the following code won't
// work. The server will know we're working but it won't know
// how far along we've gotten.

/* report on regular+biggie files */
  while(!feof(fin))
    {
      fgets(progress_str, MAX_STR_LEN, fin);
      progress_str[MAX_STR_LEN]='\0';
      continue;

      log_it(debug, progress_str);
      if (strstr(progress_str, "estoring fileset #0"))
        { prev_percentage = 0; regulars = true; }
      if (strstr(progress_str, "erifying all bigfiles"))
        { prev_percentage = 0; regulars = false; biggies = true; }
      if (strlen(progress_str)<2) { continue; }
      if (!strstr(progress_str, "TASK")) { continue; }
      progress_str[strlen(progress_str)-1] = '\0';
      if (!biggies && !regulars)
        { strcpy(progress_str, "Still working..."); }
      if (strcmp(progress_str, old_progstr))
        {
          strcpy(old_progstr, progress_str);
          if (biggies)
            { sprintf(tmp, "Large files: %s", progress_str+6); }
          else if (regulars)
            { sprintf(tmp, "Regular files: %s", progress_str+6); }
	  if (!(p=strstr(progress_str, "% done"))) { continue; }

          log_it(info, tmp);
          if (send_progress_rpt_to_server(g_server_name, tmp)) { log_it(error, "Failed to send progress_rpt msg to server."); pthread_exit(NULL); }
          continue;

          while(!isdigit(*(p-1))) { p--; }
          strcpy(tmp, p);
          *(strchr(tmp, '%')) = '\0';
          i = atoi(tmp);
          if (i > prev_percentage)
            {
              prev_percentage = i;
              log_it(info, tmp);
// FIXME - could simply call send_progress_rpt_to_server()
              if (send_progress_rpt_to_server(g_server_name, tmp)) { log_it(error, "Failed to send progress_rpt msg to server."); pthread_exit(NULL); }
            }
        }
    }
  log_it(debug, "track_restore_task_progress() --- leaving");
  pthread_exit(NULL);
}



/*-----------------------------------------------------------*/



void *watch_port_for_triggers_from_server(void*arg)
/*
Purpose:Watch client port for incoming trigger
Params: none; uses global vars instead
Return: none
*/
{
  struct pollfd ufds;
  struct s_server2client_msg_record incoming_rec;
  int len, res, new_s;
  char tmp[MAX_STR_LEN+1];

  strcpy(tmp,(char*)arg);
  log_it(info, "Awaiting triggers from server");
  for(;;)
    {
      /* wait for ack/pong/feedback/trigger */
      ufds.fd = g_sClient;
      ufds.events = POLLIN|POLLPRI;
      poll(&ufds, 1, 1000);
      if (!ufds.revents) { continue; } /* wait again */
      len = accept_and_recv_thru_client_port(g_sClient, &new_s, &g_sinClient, (char*)&incoming_rec, sizeof(incoming_rec));
      if (len<0) { log_it(error, "Unable to receive incoming trigger from server"); continue; }
      switch(incoming_rec.msg_type)
	{
	case trigger_backup:
	  res = back_my_smitch_up(incoming_rec.body, new_s); /* no need to multitask: it's the client! :) */
	  break;
	case trigger_compare:
	  res = compare_archives(incoming_rec.body, new_s);
	  break;
	case trigger_restore:
	  res = restore_archives(incoming_rec.body, incoming_rec.bodyAux, new_s);
	  break;
        case logout_ok:
          if (!g_logging_out)
            { log_it(fatal, "Server has forcibly logged you out. Has server shut down?"); }
          g_logged_in_currently = false;
          g_logging_out = false;
//	  exit(0);
          pthread_exit("Thou hast been logged out.");
        case login_fail:
          log_it(fatal, "Failed to login. Server thinks we're logged in already.");
	default:
	  sprintf(tmp, "Received %s - '%s'", tmsg_to_string(incoming_rec.msg_type), incoming_rec.body);
	  log_it(info, tmp);
	}
      close(new_s);
    }
}

	

/*-----------------------------------------------------------*/



int main(int argc, char*argv[])
/*
Purpose: main subroutine
Parameters: none
Return: result (0=success, nonzero=failure)
*/
{
  int client_port;
  char hostname[MAX_STR_LEN+1], tmp[MAX_STR_LEN+1];
//  char msg[MAX_STR_LEN+1];
//  bool done;
//  pthread_t thread;

  log_it(info, "---------- Monitas (client) by Hugo Rabson ----------");
  register_pid(getpid(), "client");
  set_signals(true);
  srandom(time(NULL));
  /* FIXME - add Ctrl-C / sigterm trapping */

/*
  call_program_in_background(&thread, "ls aaa");
  if (program_still_running("ls aaa"))
    { printf("Program is still running\n"); }
  else
    { printf("Program is no longer running\n"); }
  exit(0);
*/
  if (argc == 2)
    {
      strncpy(g_server_name, argv[1], MAX_STR_LEN);
      g_server_name[MAX_STR_LEN]='\0';
    }
  else
    {
      fprintf(stderr, "client <server addr>\n");
      exit(1);
    }
  gethostname(hostname, sizeof(hostname));
  sprintf(tmp, "Logging onto server as client '%s'", hostname);
  log_it(info, tmp);
  client_port = login_to_server(hostname, g_server_name);
  if (client_port <= 0)
    {
      fprintf(stderr, "Unable to login to server.\n");
      log_it(error, "Unable to login to server.");
      exit(1);
    }

//  set_param_in_rcfile(CLIENT_RCFILE, "mondoarchive_params", "-1 -L");
  log_it(debug, "Awaiting commands from FIFO");
  create_and_watch_fifo_for_commands(CLIENT_COMDEV);
  logout_and_exit(g_server_name);
  exit(0);
}
/* end main() */



