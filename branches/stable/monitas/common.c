/* common.c - functions common to server.c and client.c



06/19
- fixed silly bug in create_and_watch_fifo_for_commands()

06/16
- added program_still_running()

06/14
- added create_and_watch_fifo_for_commands() from server.c
- if log_it(fatal,...) then call terminate_daemon()

06/11
- added register_pid(), set_signals()
- commented code a bit

06/10
- create function to call external executable in background
- create function to wait for it to terminate & to grab its result
- put them in common.c



*/

#include "structs.h"


extern char g_command_fifo[MAX_STR_LEN+1];
extern char g_logfile[MAX_STR_LEN+1];
extern char *call_program_and_get_last_line_of_output(char*);
extern int call_program_and_log_output (char *);
extern bool does_file_exist (char *);
extern int make_hole_for_file (char *outfile_fname);
extern int process_incoming_command(char*);
extern void terminate_daemon(int);


void call_program_in_background(pthread_t*, char*);
void *call_prog_in_bkgd_SUB(void*);
int create_and_watch_fifo_for_commands(char*);
int get_bkgd_prog_result(pthread_t*);
void log_it_SUB(char*, t_loglevel, char*);
bool program_still_running(char*);
int receive_file_from_socket(FILE*fout, int socket_fd);
void register_pid(pid_t, char*);
void set_signals(bool);
char *tmsg_to_string(t_msg msg_type);
int read_block_from_fd(int, char*, int);
void termination_in_progress(int sig);
int transmit_file_to_socket(FILE*fin, int socket_fd);



/*-----------------------------------------------------------*/



void call_program_in_background(pthread_t *thread, char*command)
/*
Purpose:Execute external binary in a background thread created
        specially for this purpose. Return. Leave binary running.
Params:	thread - [returned] thread running in background
        command - binary call to be executed
Return: 0 if prog started ok; nonzero if failure
NB:     Use get_bkgd_prog_result() to wait for binary to terminate
        and to get binary's result (zero or nonzero)
*/
{
  int res;

  res = pthread_create(thread, NULL, call_prog_in_bkgd_SUB, (void*)command);
  if (res) { log_it(fatal, "Unable to call program in background - thread creation failure"); }
  log_it(debug, "Done creating pthread. Will continue now.");
}



/*-----------------------------------------------------------*/



void *call_prog_in_bkgd_SUB(void*cmd)
{
/*
Purpose:Subroutine of call_program_in_background()
Params:	cmd - binary call to be executed
Return: None
*/
  char*command;
  int res;
  char tmp[MAX_STR_LEN+1];
  static char result_str[MAX_STR_LEN+1];

  command=(char*)cmd;
  sprintf(tmp, "Calling '%s' in background pthread", command);
  log_it(debug, tmp);
  res = system(command);
  sprintf(tmp, "'%s' is returning from bkgd process - res=%d", command, res); 
  log_it(debug, tmp);
  sprintf(result_str, "%d", res);
  pthread_exit((void*)result_str);
}



/*-----------------------------------------------------------*/



int create_and_watch_fifo_for_commands(char*devpath)
/*
Purpose:Create a FIFO (device). Open it. Read commands from it
        if/when they're echoed to it (e.g. backup, restore, ...)
Params:	devpath - path+filename of FIFO to create
Return: result (nonzero=failure; else, infinite loop & notional 0)
*/
{
  char tmp[MAX_STR_LEN+1], incoming[MAX_STR_LEN+1];
  int res=0, fd, len, pos=0;

  res = make_hole_for_file(devpath) + mkfifo(devpath, 700);
  if (res) { log_it(error, "Unable to prepare command FIFO"); return(res); }
  strncpy(g_command_fifo, devpath, MAX_STR_LEN);
  if (!(fd = open(g_command_fifo, O_RDONLY))) { log_it(error, "Unable to openin command FIFO"); return(1); }
//  log_it(info, "Awaiting commands from FIFO");
  for(;;)
    {
      len=read(fd, incoming+pos, 1);
      if (len==0) { usleep(1000); continue; }
      pos+=len;
      incoming[pos]='\0';
      if (incoming[pos-1]=='\n' || pos>=MAX_STR_LEN)
        {
          incoming[pos-1]='\0';
          len=pos=0;
          res=process_incoming_command(incoming);
          if (res)
            {
              sprintf(tmp, "%s <-- errors occurred during processing", incoming);
              log_it(error, tmp);
            }
          else
            {
              sprintf(tmp, "%s <-- command received OK", incoming);
              log_it(info, tmp);
            }
        }
    } // forever
  return(0);
}


/*-----------------------------------------------------------*/



int get_bkgd_prog_result(pthread_t *thread)
/*
Purpose:Wait for external binary (running in background) to terminate.
        Discover how it ended (w/success or w/failure?).
Params:	thread - thread running in background
Return: result (0=success, nonzero=failure)
NB:     Binary was executed by call_program_in_background(). This
        subroutine should be called sometime after that call.
*/
{
  void*vp;
  void**pvp;
  int res;
  char tmp[MAX_STR_LEN+1], *p, result_str[MAX_STR_LEN+1];

  pvp=&vp;
  vp=(void*)result_str;
  strcpy(result_str, "(uninitialized)");
  log_it(debug, "Waiting for bkgd prog to finish, so that we can get its result");
  if (pthread_join(*thread, pvp)) { log_it(fatal, "pthread_join failed"); }
  p = result_str;
  p = (char*)vp;
  res = atoi(p);
  sprintf(tmp, "pthread res = %d ('%s')", res, p);
  log_it(debug, tmp);
  *thread = 0;
  return(res);
}



/*-----------------------------------------------------------*/



void log_it_SUB(char *logfile, t_loglevel level, char *sz_message)
/*
Purpose:Log event and its level of severity.
Params:	level - level of severity (debug/info/warn/error/fatal)
        sz_message - message/event [string]
Return: None
*/
{
  char sz_outstr[MAX_STR_LEN], sz_level[MAX_STR_LEN], sz_time[MAX_STR_LEN];
  time_t time_rec;
  FILE*fout;

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
      while (!(fout=fopen(logfile,"a"))) { usleep((int)(random()%32768)); }
      fprintf(fout, "%s\n",sz_outstr);
      fclose(fout);
    }
  if (level==fatal)
    {
      fprintf(stderr, "Aborting now. See %s for more information.\n", g_logfile);
      terminate_daemon(0);
      exit(1);
    }
}



/*-----------------------------------------------------------*/



bool program_still_running(char*command)
{
  char comm_sub[MAX_STR_LEN+1],
   	tmp[MAX_STR_LEN+1];
  strncpy(comm_sub, command, 40);
  comm_sub[40] = '\0';
  sprintf(tmp, "ps ax | grep \"%s\" | grep -v \"ps ax\" | grep -v \"grep \"", comm_sub);
  if (call_program_and_log_output(tmp))
    { return(false); }
  else
    { return(true); }
}



/*-----------------------------------------------------------*/



int read_block_from_fd(int socket_fd, char*buf, int len_to_read)
/*
Purpose:Read N bytes from socket, into buffer
Params:	socket_fd - socket to be read from
        buf - buffer where incoming data will be stored
        len_to_read - bytes to be read from socket
Return: bytes read
NB:     if bytes read < len_to_read then there is
        probably something wrong (e.g. EOF of pipe)
*/
{
  int length, noof_tries, i=0;
  char tmp[MAX_STR_LEN+1];

//  log_it(debug, "Reading from pipe");
  for(length=noof_tries=0; length<len_to_read && noof_tries<10; length+=i)
    {
      i = read(socket_fd, buf+length, len_to_read-length);
      if (i==0) { noof_tries++; }
      if (i<0) { log_it(error, "Error while reading from fd"); return(-1); }
    }
  if (length != len_to_read) { log_it(error, "Unable to continue reading from fd"); }
  sprintf(tmp, "%d of %d bytes read from fd", length, len_to_read);
  return(length);
}



/*-----------------------------------------------------------*/



int receive_file_from_socket(FILE*fout, int socket_fd)
/*
Purpose:Receive file from socket and save to disk / stream.
Params:	socket_fd - file descriptor of socket
	fout - file descriptor of output file
Return: result (0=success, nonzero=failure)
NB:	socket_fd was created with open() but
	fopen was created with fopen(). Different, see? Nyah.
*/
{
  int res=0, length, rotor=0, incoming_len;
  char tmp[MAX_STR_LEN+1], buf[XFER_BUF_SIZE];
  bool done;
  long long total_length=0;

  signal(SIGPIPE, SIG_IGN);

  length=99999;
//  sleep(1);
  for(done=false; !done; )
    {
      //      usleep(1000);
      length = read_block_from_fd(socket_fd, buf, 3);
      if (length<3) { log_it(debug, "Failed to read header block from fd"); res++; done=true; continue; }
      rotor=(rotor%127)+1;
      sprintf(tmp, "Rotor is %d (I was expecting %d)", buf[0], rotor);
      if (buf[0]==0 && buf[1]==0x7B && buf[2]==0x21) { log_it(debug, "OK, end of stream. Cool."); done=true; continue; }
//      log_it(debug, tmp);
      if (buf[0]!=rotor) { log_it(debug, "Rotors do not match"); res++; break; }
      incoming_len = buf[2];
      if (incoming_len < 0) { incoming_len += 256; }
      incoming_len += buf[1] * 256;
      //      memcpy((char*)&incoming_len, buf+1, 2);
      sprintf(tmp, "Expecting %d bytes in block #%d", incoming_len, rotor);
//      log_it(debug, tmp);
      length = read_block_from_fd(socket_fd, buf, incoming_len);
/*
      for(length=0,i=0; length<incoming_len; length+=i)
	{
	  i = read(socket_fd, buf+length, incoming_len-length);
	  if (length<0) { log_it(debug, "Unable to read block in from socket_fd"); done=true; }
	  else if (length==0) { log_it(debug, "Zero-length block arrived from socket"); }
	  sprintf(tmp, "Read %d bytes from socket", i);
//	  log_it(debug, tmp);
	}
*/
      if (length != incoming_len) { log_it(debug, "Error reading from socket"); res++; }
      if (fwrite(buf, 1, length, fout)!=length) { log_it(debug, "Error writing data to output file"); res++; }
      sprintf(tmp, "Written %d bytes to output stream", length);
      total_length += length;
//      log_it(debug, tmp);
    }
  if (total_length==0) { res++; log_it(debug, "Zero-length file received. Silly. I say that's an error."); }

  signal(SIGPIPE, terminate_daemon);

  return(res);
}




/*-----------------------------------------------------------*/



void register_pid(pid_t pid, char*name_str)
/*
Purpose:Register executable's PID in /var/run/monitas-[name_str].pid;
        store [pid] in data file
Params:	pid - process ID to be stored in data file
        name_str - name (e.g. "client" or "server") to be included
        in the data file name of the PID locking file
Return: None
NB:     Use pid=0 to delete the lock file and unregister the PID
*/
{
  char tmp[MAX_STR_LEN+1], lockfile_fname[MAX_STR_LEN+1];
  int res;
  FILE*fin;

  sprintf(lockfile_fname, "/var/run/monitas-%s.pid", name_str);
  if (!pid)
    {
      log_it(debug, "Unregistering PID");
      if (unlink(lockfile_fname)) { log_it(error, "Error unregistering PID"); }
      return;
    }
  if (does_file_exist(lockfile_fname))
    {
      tmp[0]='\0';
      if ((fin=fopen(lockfile_fname,"r"))) { fgets(tmp, MAX_STR_LEN, fin); fclose(fin); }
      pid = atol(tmp);
      sprintf(tmp, "ps %ld", (long int)pid);
      res = call_program_and_log_output(tmp);
      if (!res)
        {
          sprintf(tmp, "I believe the daemon is already running. If it isn't, please delete %s and try again.", lockfile_fname);
          log_it(fatal, tmp);
        }
    }
  sprintf(tmp, "echo %ld > %s", (long int)getpid(), lockfile_fname);
  if (system(tmp)) { log_it(fatal, "Cannot register PID"); }
}



/*-----------------------------------------------------------*/



void set_signals(bool on)
/*
Purpose:Turn on/off signal-trapping
Params:	on - turn on or off (true=on, false=off)
Return: None
*/
{
  int signals[]= { SIGKILL, SIGPIPE, SIGTERM, SIGHUP, SIGTRAP, SIGABRT, SIGINT, SIGSTOP, 0 };
  int i;
  for (i=0; signals[i]; i++)
    {
      if (on)
        { signal(signals[i], terminate_daemon); }
      else
        { signal(signals[i], termination_in_progress); }
    }
}



/*-----------------------------------------------------------*/



void termination_in_progress(int sig)
{
  log_it(debug, "Termination in progress");
  usleep(1000);
  pthread_exit(0);
}



/*-----------------------------------------------------------*/



char *tmsg_to_string(t_msg msg_type)
/*
Purpose:turn msg_type struct variable into a string
Params:	msg_type - type of message
Return: pointer to static string containing text version of msg_type
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
      case login_fail : strcpy(sz_msg, "login_fail"); break;
      case logout_fail: strcpy(sz_msg, "logout_fail"); break;
      case trigger_backup: strcpy(sz_msg, "trigger_backup"); break;
      case trigger_compare:strcpy(sz_msg, "trigger_compare"); break;
      case trigger_restore:strcpy(sz_msg, "trigger_restore"); break;
      case begin_stream:   strcpy(sz_msg, "begin_stream"); break;
      case end_stream:     strcpy(sz_msg, "end_stream"); break;
      case backup_ok:      strcpy(sz_msg, "backup_ok"); break;
      case backup_fail:    strcpy(sz_msg, "backup_fail"); break;
      case compare_ok:     strcpy(sz_msg, "compare_ok"); break;
      case compare_fail:   strcpy(sz_msg, "compare_fail"); break;
      case restore_ok:     strcpy(sz_msg, "restore_ok"); break;
      case restore_fail:   strcpy(sz_msg, "restore_fail"); break;
      case user_req:       strcpy(sz_msg, "user_req"); break;
      case progress_rpt:   strcpy(sz_msg, "progress_rpt"); break;
      default: strcpy(sz_msg, "(Fix tmsg_to_string please)"); break;
    }
  return(sz_msg);
}



/*-----------------------------------------------------------*/



int transmit_file_to_socket(FILE*fin, int socket_fd)
/*
Purpose:Transmit file to socket, from disk / stream.
Params:	socket_fd - file descriptor of socket
	fin - file descriptor of input file
Return: result (0=success, nonzero=failure)
NB:	socket_fd was created with open() but
	fopen was created with fopen(). Different, see? Nyah.
*/
{
  char tmp[MAX_STR_LEN+1], buf[XFER_BUF_SIZE];
  int length, retval=0, rotor=0;
  bool done;

  signal(SIGPIPE, SIG_IGN);
  for(done=false; !done; )
    {
      if (feof(fin))
	{
	  log_it(debug, "eof(fin) - OK, stopped reading from mondoarchive");
	  done=true;
	  continue;
	}
//      log_it(debug, "Reading from mondoarchive");
      length = fread(buf, 1, XFER_BUF_SIZE, fin); /* read from fifo (i.e. mondoarchive) */
//      sprintf(tmp, "%d bytes read from mondoarchive", length); log_it(debug, tmp);
      if (length > 0)
	{
	  tmp[0] = (char)(rotor=(rotor%127)+1);
	  tmp[1] = (char)(length / 256);
	  tmp[2] = (char)(length % 256);
	  if (write(socket_fd, tmp, 3)!=3) { log_it(debug, "Failed to write rotor and block_len to socket"); retval++; done=true; continue; }
//	  else { log_it(debug, "rotor and block_len written to socket"); }
	  if (write(socket_fd, buf, length)!=length) { log_it(debug, "Failed to write block read from mondoarchive to socket"); retval++; done=true; continue; }
	  else
	    { 
	      sprintf(tmp, "Block #%d (%d bytes) written to socket", rotor, length);
//	      log_it(debug, tmp);
	    }
	}
      else if (length < 0)
	{ log_it(warn, "I had trouble reading from FIFO while calling mondoarchive"); }
    }
  if (retval==0)
    {
//      log_it(debug, "Writing final rotor (the 'end of stream' one) to socket");
      tmp[0]=0;
      tmp[1]=0x7B;
      tmp[2]=0x21;
      if (write(socket_fd, tmp, 3)!=3) { log_it(debug, "Failed to write end-of-stream rotor to socket"); retval++; }
    }
  log_it(debug, "Closed fopen() to tempdev");
//  fclose(fin);
  signal(SIGPIPE, terminate_daemon);
  return(retval);
}








