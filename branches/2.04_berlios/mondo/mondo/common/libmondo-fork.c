/* libmondo-fork.c
   $Id: libmondo-fork.c,v 1.3 2004/06/21 20:20:36 hugo Exp $

- subroutines for handling forking/pthreads/etc.


06/20/2004
- create fifo /var/log/partimagehack-debug.log and empty it
  to keep ramdisk from filling up

04/13/2004
- >= should be <= g_loglevel

11/15/2003
- changed a few []s to char*s
  
10/12
- rewrote partimagehack handling (multiple fifos, chunks, etc.)

10/11
- partimagehack now has debug level of N (set in my-stuff.h)

10/08
- call to partimagehack when restoring will now log errors to /var/log/....log

10/06
- cleaned up logging a bit

09/30
- line 735 - missing char* cmd in sprintf()

09/28
- added run_external_binary_with_percentage_indicator()
- rewritten eval_call_to_make_ISO()

09/18
- call mkstemp instead of mktemp

09/13
- major NTFS hackage

09/12
- paranoid_system("rm -f /tmp/ *PARTIMAGE*") before calling partimagehack

09/11
- forward-ported unbroken feed_*_partimage() subroutines
  from early August 2003

09/08
- detect & use partimagehack if it exists

09/05
- finally finished partimagehack hack :)

07/04
- added subroutines to wrap around partimagehack

04/27
- don't echo (...res=%d...) at end of log_it()
  unnecessarily
- replace newtFinished() and newtInit() with
  newtSuspend() and newtResume()

04/24
- added some assert()'s and log_OS_error()'s

04/09
- cleaned up run_program_and_log_output()

04/07
- cleaned up code a bit
- let run_program_and_log_output() accept -1 (only log if _no error_)

01/02/2003
- in eval_call_to_make_ISO(), append output to MONDO_LOGFILE
  instead of a temporary stderr text file

12/10
- patch by Heiko Schlittermann to handle % chars in issue.net

11/18
- if mkisofs in eval_call_to_make_ISO() returns an error then return it,
  whether ISO was created or not

10/30
- if mkisofs in eval_call_to_make_ISO() returns an error then find out if
  the output (ISO) file has been created; if it has then return 0 anyway

08/01 - 09/30
- run_program_and_log_output() now takes boolean operator to specify
  whether it will log its activities in the event of _success_
- system() now includes 2>/dev/null
- enlarged some tmp[]'s
- added run_program_and_log_to_screen() and run_program_and_log_output()

07/24
- created
*/


#include "my-stuff.h"
#include "mondostructures.h"
#include "libmondo-fork.h"
#include "libmondo-string-EXT.h"
#include "libmondo-gui-EXT.h"
#include "libmondo-files-EXT.h"
#include "libmondo-tools-EXT.h"
#include "lib-common-externs.h"

/*@unused@*/
//static char cvsid[] = "$Id: libmondo-fork.c,v 1.3 2004/06/21 20:20:36 hugo Exp $";

extern char *g_tmpfs_mountpt;
extern t_bkptype g_backup_media_type;
extern bool g_text_mode;
pid_t g_buffer_pid=0;


/**
 * Call a program and retrieve its last line of output.
 * @param call The program to run.
 * @return The last line of its output.
 * @note The returned value points to static storage that will be overwritten with each call.
 */
char *
call_program_and_get_last_line_of_output (char *call)
{
	/*@ buffers ******************************************************/
  static char result[512];
  char *tmp;

	/*@ pointers *****************************************************/
  FILE *fin;

	/*@ initialize data **********************************************/
  malloc_string(tmp);
  result[0] = '\0';
  tmp[0]    = '\0';

  /*@*********************************************************************/

  assert_string_is_neither_NULL_nor_zerolength(call);
  if ((fin = popen (call, "r")))
    {
      for (fgets (tmp, MAX_STR_LEN, fin); !feof (fin);
	   fgets (tmp, MAX_STR_LEN, fin))
	{
	  if (strlen (tmp) > 1)
	    {
	      strcpy (result, tmp);
	    }
	}
      paranoid_pclose (fin);
    }
  else
    {
      log_OS_error("Unable to popen call");
    }
  strip_spaces (result);
  return (result);
}






#define MONDO_POPMSG  "Your PC will not retract the CD tray automatically. Please call mondoarchive with the -m (manual CD tray) flag."

/**
 * Call mkisofs to create an ISO image.
 * @param bkpinfo The backup information structure. Fields used:
 * - @c bkpinfo->manual_cd_tray
 * - @c bkpinfo->backup_media_type
 * - @c bkpinfo->please_dont_eject_when_restoring
 * @param basic_call The call to mkisofs. May contain tokens that will be resolved to actual data. The tokens are:
 * - @c _ISO_ will become the ISO file (@p isofile)
 * - @c _CD#_ becomes the CD number (@p cd_no)
 * - @c _ERR_ becomes the logfile (@p g_logfile)
 * @param isofile Replaces @c _ISO_ in @p basic_call. Should probably be the ISO image to create (-o parameter to mkisofs).
 * @param cd_no Replaces @c _CD#_ in @p basic_call. Should probably be the CD number.
 * @param logstub Unused.
 * @param what_i_am_doing The action taking place (e.g. "Making ISO #1"). Used as the title of the progress dialog.
 * @return Exit code of @c mkisofs (0 is success, anything else indicates failure).
 * @bug @p logstub is unused.
 */
int
eval_call_to_make_ISO (struct s_bkpinfo *bkpinfo,
	   char *basic_call, char *isofile,
	   int cd_no, char *logstub, char *what_i_am_doing)
{

	/*@ int's  ****/
  int retval = 0;


	/*@ buffers      ****/
  char *midway_call, *ultimate_call, *tmp, *command, *incoming, *old_stderr, *cd_number_str;
  char *p;

/*@***********   End Variables ***************************************/

  log_msg(3, "Starting");
  assert(bkpinfo!=NULL);
  assert_string_is_neither_NULL_nor_zerolength(basic_call);
  assert_string_is_neither_NULL_nor_zerolength(isofile);
  assert_string_is_neither_NULL_nor_zerolength(logstub);
  if (!(midway_call = malloc(1200))) { fatal_error("Cannot malloc midway_call"); }
  if (!(ultimate_call = malloc(1200))) { fatal_error("Cannot malloc ultimate_call"); }
  if (!(tmp = malloc(1200))) { fatal_error("Cannot malloc tmp"); }
  if (!(command = malloc(1200))) { fatal_error("Cannot malloc command"); }
  malloc_string(incoming);
  malloc_string(old_stderr);
  malloc_string(cd_number_str);

  incoming[0]='\0';
  old_stderr[0] = '\0';

  sprintf (cd_number_str, "%d", cd_no);
  resolve_naff_tokens (midway_call, basic_call, isofile, "_ISO_");
  resolve_naff_tokens (tmp, midway_call, cd_number_str, "_CD#_");
  resolve_naff_tokens (ultimate_call, tmp, MONDO_LOGFILE, "_ERR_");
  log_msg (4, "basic call = '%s'", basic_call);
  log_msg (4, "midway_call = '%s'", midway_call);
  log_msg (4, "tmp = '%s'", tmp);
  log_msg (4, "ultimate call = '%s'", ultimate_call);
  sprintf( command, "%s >> %s", ultimate_call, MONDO_LOGFILE );

  log_to_screen("Please be patient. Do not be alarmed by on-screen inactivity.");
  log_msg(4, "Calling open_evalcall_form() with what_i_am_doing='%s'", what_i_am_doing);
  strcpy (tmp, command);
  if (bkpinfo->manual_cd_tray)
    {
      p = strstr (tmp, "2>>");
      if (p)
        {
          sprintf (p, "   ");
          while(*p==' ') { p++; }
          for (; *p != ' '; p++)
	    {
	      *p = ' ';
	    }
        }
      strcpy(command, tmp);
#ifndef _XWIN
      if (!g_text_mode) { newtSuspend(); }
#endif
      log_msg (1, "command = '%s'", command);
      retval += system (command);
      if (!g_text_mode) { newtResume(); }
      if (retval)
	{
	  log_msg (2, "Basic call '%s' returned an error.", basic_call);
	  popup_and_OK ("Press ENTER to continue.");
          popup_and_OK("mkisofs and/or cdrecord returned an error. CD was not created");
	}
    }
  /* if text mode then do the above & RETURN; if not text mode, do this... */
  else
    {
      log_msg (3, "command = '%s'", command);
//      yes_this_is_a_goto:
      retval = run_external_binary_with_percentage_indicator_NEW(what_i_am_doing, command);
    }

  paranoid_free(midway_call);
  paranoid_free(ultimate_call);
  paranoid_free(tmp);
  paranoid_free(command);
  paranoid_free(incoming);
  paranoid_free(old_stderr);
  paranoid_free(cd_number_str);
/*
  if (bkpinfo->backup_media_type == dvd && !bkpinfo->please_dont_eject_when_restoring)
    {
      log_msg(3, "Ejecting DVD device");
      eject_device(bkpinfo->media_device);
    }
*/
  return (retval);
}




/**
 * Run a program and log its output (stdout and stderr) to the logfile.
 * @param program The program to run. Passed to the shell, so you can use pipes etc.
 * @param debug_level If @p g_loglevel is higher than this, do not log the output.
 * @return The exit code of @p program (depends on the command, but 0 almost always indicates success).
 */
int
run_program_and_log_output (char *program, int debug_level)
{
	/*@ buffer *******************************************************/
  char callstr[MAX_STR_LEN*2];
  char incoming[MAX_STR_LEN*2];
  char tmp[MAX_STR_LEN*2];
  char initial_label[MAX_STR_LEN*2];

	/*@ int **********************************************************/
  int res;
  int i;
  int len;
  bool log_if_failure=FALSE;
  bool log_if_success=FALSE;

	/*@ pointers ****************************************************/
  FILE *fin;
  char *p;

	/*@ end vars ****************************************************/

  assert(program!=NULL);
  if (!program[0])
    {
      log_msg(2, "Warning - asked to run zerolength program");
      return(1);
    }
//  if (debug_level == TRUE) { debug_level=5; }

  //  assert_string_is_neither_NULL_nor_zerolength(program);

  if (debug_level <= g_loglevel) { log_if_success = TRUE; log_if_failure = TRUE; }
  sprintf (callstr, "%s > /tmp/mondo-run-prog-thing.tmp 2> /tmp/mondo-run-prog-thing.err",
	   program);
  while ((p = strchr (callstr, '\r')))
    {
      *p = ' ';
    }
  while ((p = strchr (callstr, '\n')))
    {
      *p = ' ';
    }				/* single '=' is intentional */


  len = (int) strlen (program);
  for (i = 0; i < 35 - len / 2; i++)
    {
      tmp[i] = '-';
    }
  tmp[i] = '\0';
  strcat (tmp, " ");
  strcat (tmp, program);
  strcat (tmp, " ");
  for (i = 0; i < 35 - len / 2; i++)
    {
      strcat (tmp, "-");
    }
  strcpy(initial_label, tmp);
  res = system (callstr);
  if (((res == 0) && log_if_success) || ((res != 0) && log_if_failure)) {
      log_msg (0, "running: %s", callstr);
      log_msg (0, "--------------------------------start of output-----------------------------");
  }
  if (log_if_failure && system ("cat /tmp/mondo-run-prog-thing.err >> /tmp/mondo-run-prog-thing.tmp 2> /dev/null"))
    {
      log_OS_error("Command failed");
    }
  unlink ("/tmp/mondo-run-prog-thing.err");
  fin = fopen ("/tmp/mondo-run-prog-thing.tmp", "r");
  if (fin)
    {
      for (fgets (incoming, MAX_STR_LEN, fin); !feof (fin);
	   fgets (incoming, MAX_STR_LEN, fin))
	{
	  /* patch by Heiko Schlittermann */
	  p = incoming;
	  while (p && *p)
	    {
	      if ((p = strchr(p, '%'))) 
		{
		  memmove(p, p+1, strlen(p) +1);
		  p += 2;
		}
	    }
	  /* end of patch */
	  strip_spaces (incoming);
          if ((res==0 && log_if_success) || (res!=0 && log_if_failure))
            { log_msg (0, incoming); }
	}
      paranoid_fclose (fin);
    }
  unlink("/tmp/mondo-run-prog-thing.tmp");
  if ((res==0 && log_if_success) || (res!=0 && log_if_failure))
    {
      log_msg (0, "--------------------------------end of output------------------------------");
      if (res) { log_msg (0,"...ran with res=%d", res); }
      else { log_msg(0, "...ran just fine. :-)"); }
    }
//  else
//    { log_msg (0, "-------------------------------ran w/ res=%d------------------------------", res); }
  return (res);
}



/**
 * Run a program and log its output to the screen.
 * @param basic_call The program to run.
 * @param what_i_am_doing The title of the evalcall form.
 * @return The return value of the command (varies, but 0 almost always means success).
 * @see run_program_and_log_output
 * @see log_to_screen
 */
int
run_program_and_log_to_screen (char *basic_call, char *what_i_am_doing)
{
	/*@ int *********************************************************/
  int retval = 0;
  int res = 0;
  int i;

	/*@ pointers *****************************************************/
  FILE *fin;

	/*@ buffers *****************************************************/
  char tmp[MAX_STR_LEN*2];
  char command[MAX_STR_LEN*2];
  char lockfile[MAX_STR_LEN];

	/*@ end vars ****************************************************/

  assert_string_is_neither_NULL_nor_zerolength(basic_call);

  sprintf (lockfile, "/tmp/mojo-jojo.blah.XXXXXX");
  mkstemp (lockfile);
  sprintf (command,
	   "echo hi > %s ; %s >> %s 2>> %s; res=$?; sleep 1; rm -f %s; exit $res",
	   lockfile, basic_call, MONDO_LOGFILE, MONDO_LOGFILE, lockfile);
  open_evalcall_form (what_i_am_doing);
  sprintf (tmp, "Executing %s", basic_call);
  log_msg (2, tmp);
  if (!(fin = popen (command, "r")))
    {
      log_OS_error("Unable to popen-in command");
      sprintf (tmp, "Failed utterly to call '%s'", command);
      log_to_screen (tmp);
      return (1);
    }
  if (!does_file_exist(lockfile))
    {
      log_to_screen("Waiting for external binary to start");
      for (i = 0; i < 60 && !does_file_exist (lockfile); sleep (1), i++)
        {
          log_msg(3, "Waiting for lockfile %s to exist", lockfile);
        }
    }
#ifdef _XWIN
  /* This only can update when newline goes into the file,
     but it's *much* prettier/faster on Qt. */
  while (does_file_exist (lockfile)) {
      while (!feof (fin)) {
	  if (!fgets (tmp, 512, fin)) break;
	  log_to_screen (tmp);
      }
      usleep (500000);
  }
#else
  /* This works on Newt, and it gives quicker updates. */
  for (; does_file_exist (lockfile); sleep (1))
    {
      log_file_end_to_screen (MONDO_LOGFILE, "");
      update_evalcall_form (1);
    }
#endif
  paranoid_pclose (fin);
  retval += res;
  close_evalcall_form ();
  unlink (lockfile);
  return (retval);
}





/**
 * Thread callback to run a command (partimage) in the background.
 * @param xfb A transfer block of @c char, containing:
 * - xfb:[0] A marker, should be set to 2. Decremented to 1 while the command is running and 0 when it's finished.
 * - xfb:[1] The command's return value, if xfb:[0] is 0.
 * - xfb+2:  A <tt>NULL</tt>-terminated string containing the command to be run.
 * @return NULL to pthread_join.
 */
void *call_partimage_in_bkgd(void*xfb)
{
  char *transfer_block;
  int retval=0;

  g_buffer_pid = getpid();
  unlink("/tmp/null");
  log_msg(1, "starting");
  transfer_block = (char*)xfb;
  transfer_block[0] --; // should now be 1
  retval = system(transfer_block+2);
  if (retval) { log_OS_error("partimage returned an error"); }
  transfer_block[1] = retval;
  transfer_block[0] --; // should now be 0
  g_buffer_pid = 0;
  log_msg(1, "returning");
  pthread_exit(NULL);
}


/**
 * File to touch if we want partimage to wait for us.
 */
#define PAUSE_PARTIMAGE_FNAME "/tmp/PAUSE-PARTIMAGE-FOR-MONDO"

/**
 * Apparently unused. @bug This has a purpose, but what?
 */
#define PIMP_START_SZ "STARTSTARTSTART9ff3kff9a82gv34r7fghbkaBBC2T231hc81h42vws8"
#define PIMP_END_SZ "ENDENDEND0xBBC10xBBC2T231hc81h42vws89ff3kff9a82gv34r7fghbka"

/**
 * Marker to start the next subvolume for Partimage.
 */
#define NEXT_SUBVOL_PLEASE "I-grew-up-on-the-crime-side,-the-New-York-Times-side,-where-staying-alive-was-no-jive"

/**
 * Marker to end the partimage file.
 */
#define NO_MORE_SUBVOLS "On-second-hand,momma-bounced-on-old-man,-and-so-we-moved-to-Shaolin-Land."

int copy_from_src_to_dest(FILE*f_orig, FILE*f_archived, char direction)
{
// if dir=='w' then copy from orig to archived
// if dir=='r' then copy from archived to orig
  char*tmp;
  char*buf;
  long int bytes_to_be_read, bytes_read_in, bytes_written_out=0, bufcap, subsliceno=0;
  int retval=0;
  FILE*fin;
  FILE*fout;
  FILE*ftmp;

  log_msg(5, "Opening.");
  malloc_string(tmp);
  tmp[0] = '\0';
  bufcap = 256L*1024L;
  if (!(buf = malloc(bufcap))) { fatal_error("Failed to malloc() buf"); }

  if (direction=='w')
    {
      fin = f_orig;
      fout = f_archived;
      sprintf(tmp, "%-64s", PIMP_START_SZ);
      if (fwrite(tmp, 1, 64, fout)!=64) { fatal_error("Can't write the introductory block"); }
      while(1)
        {
          bytes_to_be_read = bytes_read_in = fread(buf, 1, bufcap, fin);
	  if (bytes_read_in == 0) { break; }
          sprintf(tmp, "%-64ld", bytes_read_in);
          if (fwrite(tmp, 1, 64, fout)!=64) { fatal_error("Cannot write introductory block"); }
          log_msg(7, "subslice #%ld --- I have read %ld of %ld bytes in from f_orig", subsliceno,  bytes_read_in, bytes_to_be_read);
          bytes_written_out += fwrite(buf, 1, bytes_read_in, fout);
          sprintf(tmp, "%-64ld", subsliceno);
          if (fwrite(tmp, 1, 64, fout)!=64) { fatal_error("Cannot write post-thingy block"); }
	  log_msg(7, "Subslice #%d written OK", subsliceno);
	  subsliceno++;
        }
      sprintf(tmp, "%-64ld", 0L);
      if (fwrite(tmp, 1, 64L, fout)!=64L) { fatal_error("Cannot write final introductory block"); }
    }
  else
    {
      fin = f_archived;
      fout = f_orig;
      if (fread(tmp, 1, 64L, fin)!=64L) { fatal_error("Cannot read the introductory block"); }
      log_msg(5, "tmp is %s", tmp);
      if (!strstr(tmp, PIMP_START_SZ)){ fatal_error("Can't find intro blk"); }
      if (fread(tmp, 1, 64L, fin)!=64L) { fatal_error("Cannot read introductory blk"); }
      bytes_to_be_read = atol(tmp);
      while(bytes_to_be_read > 0)
        {
	  log_msg(7, "subslice#%ld, bytes=%ld", subsliceno, bytes_to_be_read);
          bytes_read_in = fread(buf, 1, bytes_to_be_read, fin);
          if (bytes_read_in != bytes_to_be_read) { fatal_error("Danger, WIll Robinson. Failed to read whole subvol from archives."); }
          bytes_written_out += fwrite(buf, 1, bytes_read_in, fout);
          if (fread(tmp, 1, 64, fin)!=64) { fatal_error("Cannot read post-thingy block"); }
          if (atol(tmp) != subsliceno) { log_msg(1, "Wanted subslice %ld but got %ld ('%s')", subsliceno, atol(tmp), tmp); }
	  log_msg(7, "Subslice #%ld read OK", subsliceno);
	  subsliceno++;
          if (fread(tmp, 1, 64, fin)!=64) { fatal_error("Cannot read introductory block"); }
          bytes_to_be_read = atol(tmp);
        }
    }

//  log_msg(4, "Written %ld of %ld bytes", bytes_written_out, bytes_read_in);

  if (direction=='w')
    {
      sprintf(tmp, "%-64s", PIMP_END_SZ);
      if (fwrite(tmp, 1, 64, fout)!=64) { fatal_error("Can't write the final block"); }
    }
  else
    {
      log_msg(1, "tmpA is %s", tmp);
      if (!strstr(tmp, PIMP_END_SZ))
        {
          if (fread(tmp, 1, 64, fin)!=64) { fatal_error("Can't read the final block"); }
          log_msg(5, "tmpB is %s", tmp);
          if (!strstr(tmp, PIMP_END_SZ))
            {
              ftmp = fopen("/tmp/out.leftover", "w");
              bytes_read_in = fread(tmp, 1, 64, fin);
              log_msg(1, "bytes_read_in = %ld", bytes_read_in);
//      if (bytes_read_in!=128+64) { fatal_error("Can't read the terminating block"); }
              fwrite(tmp, 1, bytes_read_in, ftmp);
              sprintf(tmp, "I am here - %ld", ftell(fin));
//	  log_msg(0, tmp);
              fread(tmp, 1, 512, fin);
              log_msg(0, "tmp = '%s'", tmp);
	      fwrite(tmp, 1, 512, ftmp);
	      fclose(ftmp);
              fatal_error("Missing terminating block");
	    }
        }
    }

  paranoid_free(buf);
  paranoid_free(tmp);
  log_msg(3, "Successfully copied %ld bytes", bytes_written_out);
  return(retval);
}


/**
 * Call partimage from @p input_device to @p output_fname.
 * @param input_device The device to read.
 * @param output_fname The file to write.
 * @return 0 for success, nonzero for failure.
 */
int dynamically_create_pipes_and_copy_from_them_to_output_file(char*input_device, char*output_fname)
{
  char *curr_fifo;
  char *prev_fifo;
  char *next_fifo;
  char*command;
  char *sz_call_to_partimage;
  int fifo_number=0;
  struct stat buf;
  pthread_t partimage_thread;
  int res=0;
  char *tmpstub;
  FILE*fout;
  FILE*fin;
  char *tmp;

  malloc_string(tmpstub);
  malloc_string(curr_fifo);
  malloc_string(prev_fifo);
  malloc_string(next_fifo);
  malloc_string(command);
  malloc_string(sz_call_to_partimage);
  malloc_string(tmp);

  log_msg(1, "g_tmpfs_mountpt = %s", g_tmpfs_mountpt);
  if (g_tmpfs_mountpt && g_tmpfs_mountpt[0] && does_file_exist(g_tmpfs_mountpt))
  	{ strcpy(tmpstub, g_tmpfs_mountpt); }
  else
  	{ strcpy(tmpstub, "/tmp"); }
  paranoid_system("rm -f /tmp/*PARTIMAGE*");
  sprintf(command, "rm -Rf %s/pih-fifo-*", tmpstub);
  paranoid_system(command);
  sprintf(tmpstub+strlen(tmpstub), "/pih-fifo-%ld", (long int)random());
  mkfifo(tmpstub, S_IRWXU|S_IRWXG); // never used, though...
  sprintf(curr_fifo, "%s.%03d", tmpstub, fifo_number);
  sprintf(next_fifo, "%s.%03d", tmpstub, fifo_number+1);
  mkfifo(curr_fifo, S_IRWXU|S_IRWXG);
  mkfifo(next_fifo, S_IRWXU|S_IRWXG); // make sure _next_ fifo already exists before we call partimage
  sz_call_to_partimage[0]=2;
  sz_call_to_partimage[1]=0;
  sprintf(sz_call_to_partimage+2, "partimagehack " PARTIMAGE_PARAMS " save %s %s > /tmp/stdout 2> /tmp/stderr", input_device, tmpstub);
  log_msg(5, "curr_fifo   = %s", curr_fifo);
  log_msg(5, "next_fifo   = %s", next_fifo);
  log_msg(5, "sz_call_to_partimage call is '%s'", sz_call_to_partimage+2);
  if (!lstat(output_fname, &buf) && S_ISREG(buf.st_mode))
    { log_msg(5, "Deleting %s", output_fname); unlink(output_fname); }
  if (!(fout = fopen(output_fname, "w"))) { fatal_error("Unable to openout to output_fname"); }
  res = pthread_create(&partimage_thread, NULL, call_partimage_in_bkgd, (void*)sz_call_to_partimage);
  if (res) { fatal_error("Failed to create thread to call partimage"); }
  log_msg(1, "Running fore/back at same time");
  log_to_screen("Working with partimagehack...");
  while(sz_call_to_partimage[0]>0)
    {
      sprintf(tmp, "%s\n", NEXT_SUBVOL_PLEASE);
      if (fwrite(tmp, 1, 128, fout)!=128) { fatal_error("Cannot write interim block"); }
      log_msg(5, "fifo_number=%d", fifo_number);
      log_msg(4, "Cat'ting %s", curr_fifo);
      if (!(fin = fopen(curr_fifo, "r"))) { fatal_error("Unable to openin from fifo"); }
//      if (!sz_call_to_partimage[0]) { break; }
      log_msg(5, "Deleting %s", prev_fifo);
      unlink(prev_fifo); // just in case
      copy_from_src_to_dest(fin, fout, 'w');
      paranoid_fclose(fin);
      fifo_number ++;
      strcpy(prev_fifo, curr_fifo);
      strcpy(curr_fifo, next_fifo);
      log_msg(5, "Creating %s", next_fifo);
      sprintf(next_fifo, "%s.%03d", tmpstub, fifo_number+1);
      mkfifo(next_fifo, S_IRWXU|S_IRWXG); // make sure _next_ fifo exists before we cat this one
      system("sync");
      sleep(5);
    }
  sprintf(tmp, "%s\n", NO_MORE_SUBVOLS);
  if (fwrite(tmp, 1, 128, fout)!=128) { fatal_error("Cannot write interim block"); }
  if (fwrite(tmp, 1, 128, fout)!=128) { fatal_error("Cannot write interim block"); }
  if (fwrite(tmp, 1, 128, fout)!=128) { fatal_error("Cannot write interim block"); }
  if (fwrite(tmp, 1, 128, fout)!=128) { fatal_error("Cannot write interim block"); }
  paranoid_fclose(fout);
  log_to_screen("Cleaning up after partimagehack...");
  log_msg(3, "Final fifo_number=%d", fifo_number);
  paranoid_system("sync");
  unlink(next_fifo);
  unlink(curr_fifo);
  unlink(prev_fifo);
  log_to_screen("Finished cleaning up.");

//  if (!lstat(sz_wait_for_this_file, &statbuf))
//    { log_msg(3, "WARNING! %s was not processed.", sz_wait_for_this_file); }
  log_msg(2, "Waiting for pthread_join() to join.");
  pthread_join(partimage_thread, NULL);
  res = sz_call_to_partimage[1];
  log_msg(2, "pthread_join() joined OK.");
  log_msg(1, "Partimagehack(save) returned %d", res);
  unlink(tmpstub);
  paranoid_free(curr_fifo);
  paranoid_free(prev_fifo);
  paranoid_free(next_fifo);
  paranoid_free(tmpstub);
  paranoid_free(tmp);
  paranoid_free(command);
  return(res);
}


/**
 * Feed @p input_device through partimage to @p output_fname.
 * @param input_device The device to image.
 * @param output_fname The file to write.
 * @return 0 for success, nonzero for failure.
 */
int feed_into_partimage(char*input_device, char*output_fname)
{
// BACKUP
  int res;

  if (!does_file_exist(input_device)) { fatal_error ("input device does not exist"); }
  if ( !find_home_of_exe("partimagehack")) { fatal_error( "partimagehack not found" ); }
  res = dynamically_create_pipes_and_copy_from_them_to_output_file(input_device, output_fname);
  return(res);
}





int
run_external_binary_with_percentage_indicator_OLD (char *tt, char *cmd)
{

	/*@ int ****************************************************************/
  int res = 0;
	int percentage = 0;
	int maxpc = 0;
	int pcno = 0;
	int last_pcno = 0;

	/*@ buffers ************************************************************/
  char *command;
  char *tempfile;
  char *title;
	/*@ pointers ***********************************************************/
  FILE *pin;

  malloc_string(title);
  malloc_string(command);
  malloc_string(tempfile);
  assert_string_is_neither_NULL_nor_zerolength(cmd);
  assert_string_is_neither_NULL_nor_zerolength(title);

  strcpy (title, tt);
  strcpy (tempfile,
	  call_program_and_get_last_line_of_output
	  ("mktemp -q /tmp/mondo.XXXXXXXX"));
  sprintf (command, "%s >> %s 2>> %s; rm -f %s", cmd, tempfile, tempfile,
	   tempfile);
  log_msg (3, command);
  open_evalcall_form (title);
  if (!(pin = popen (command, "r")))
    {
      log_OS_error ("fmt err");
      return (1);
    }
  maxpc = 100;
// OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
  for (sleep (1); does_file_exist (tempfile); sleep (1))
    {
      pcno = grab_percentage_from_last_line_of_file (MONDO_LOGFILE);
      if (pcno < 0 || pcno > 100)
	{
	  log_msg (5, "Weird pc#");
	  continue;
	}
      percentage = pcno * 100 / maxpc;
      if (pcno <= 5 && last_pcno > 40)
	{
	  close_evalcall_form ();
	  strcpy (title, "Verifying...");
	  open_evalcall_form (title);
	}
      last_pcno = pcno;
      update_evalcall_form (percentage);
    }
// OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
  close_evalcall_form ();
  if (pclose (pin)) { res++; log_OS_error("Unable to pclose"); }
  unlink (tempfile);
  paranoid_free(command);
  paranoid_free(tempfile);
  paranoid_free(title);
  return (res);
}




void*run_prog_in_bkgd_then_exit(void*info)
{
  char*sz_command;
  static int res=4444;

  res=999;
  sz_command = (char*)info;
  log_msg(4, "sz_command = '%s'", sz_command);
  res = system(sz_command);
  if (res>256 && res!=4444) { res=res / 256; }
  log_msg(4, "child res = %d", res);
  sz_command[0] = '\0';
  pthread_exit((void*)(&res));
}




int
run_external_binary_with_percentage_indicator_NEW (char*tt, char*cmd)
{

	/*@ int ****************************************************************/
  int res = 0;
	int percentage = 0;
	int maxpc = 100;
	int pcno = 0;
	int last_pcno = 0;
	int counter=0;

	/*@ buffers ************************************************************/
  char *command;
  char *title;
	/*@ pointers ***********************************************************/
  static int chldres=0;
  int *pchild_result;
  pthread_t childthread;

  pchild_result = &chldres;
  assert_string_is_neither_NULL_nor_zerolength(cmd);
  assert_string_is_neither_NULL_nor_zerolength(tt);
  *pchild_result = 999;

  malloc_string(title);
  malloc_string(command);
  strcpy (title, tt);
  sprintf(command, "%s 2>> %s", cmd, MONDO_LOGFILE);
  log_msg (3, "command = '%s'", command);
  if ((res = pthread_create(&childthread, NULL, run_prog_in_bkgd_then_exit, (void*)command)))
	{ fatal_error("Unable to create an archival thread"); }

  log_msg(8, "Parent running");
  open_evalcall_form (title);
  for (sleep (1); command[0]!='\0'; sleep (1))
    {
      pcno = grab_percentage_from_last_line_of_file (MONDO_LOGFILE);
      if (pcno <= 0 || pcno > 100)
	{
	  log_msg (8, "Weird pc#");
	  continue;
	}
      percentage = pcno * 100 / maxpc;
      if (pcno <= 5 && last_pcno >= 40)
	{
	  close_evalcall_form ();
	  strcpy (title, "Verifying...");
	  open_evalcall_form (title);
	}
      if (counter++ >= 5)
        {
	  counter=0;
          log_file_end_to_screen (MONDO_LOGFILE, "");
	}
      last_pcno = pcno;
      update_evalcall_form (percentage);
    }
  log_file_end_to_screen (MONDO_LOGFILE, "");
  close_evalcall_form ();
  pthread_join(childthread, (void*)(&pchild_result));
  if (pchild_result) { res = *pchild_result; }
  else { res = 666; }
  log_msg(3, "Parent res = %d", res);
  paranoid_free(command);
  paranoid_free(title);
  return (res);
}



#define PIH_LOG "/var/log/partimage-debug.log"


/**
 * Feed @p input_fifo through partimage (restore) to @p output_device.
 * @param input_fifo The partimage file to read.
 * @param output_device Where to put the output.
 * @return The return value of partimagehack (0 for success).
 * @bug Probably unnecessary, as the partimage is just a sparse file. We could use @c dd to restore it.
 */
int feed_outfrom_partimage(char*output_device, char*input_fifo)
{
// RESTORE
  char *tmp;
//  char *command;
  char *stuff;
  char *sz_call_to_partimage;
  pthread_t partimage_thread;
  int res;
  char *curr_fifo;
  char *prev_fifo;
  char *oldest_fifo;
  char *next_fifo;
  char *afternxt_fifo;
  int fifo_number=0;
  char *tmpstub;
  FILE *fin;
  FILE *fout;

  malloc_string(curr_fifo);
  malloc_string(prev_fifo);
  malloc_string(next_fifo);
  malloc_string(afternxt_fifo);
  malloc_string(oldest_fifo);
  malloc_string(tmp);
  sz_call_to_partimage = malloc(1000);
  malloc_string(stuff);
  malloc_string(tmpstub);

  log_msg(1, "output_device=%s", output_device);
  log_msg(1, "input_fifo=%s", input_fifo);
/* I don't trust g_tmpfs_mountpt
  if (g_tmpfs_mountpt[0])
    {
      strcpy(tmpstub, g_tmpfs_mountpt);
    }
  else
    {
*/
      strcpy(tmpstub, "/tmp");
//    }
  log_msg(1, "tmpstub was %s", tmpstub);
  strcpy(stuff, tmpstub);
  sprintf(tmpstub, "%s/pih-fifo-%ld", stuff, (long int)random());
  log_msg(1, "tmpstub is now %s", tmpstub);
  unlink("/tmp/PARTIMAGEHACK-POSITION");
  unlink(PAUSE_PARTIMAGE_FNAME);
  paranoid_system("rm -f /tmp/*PARTIMAGE*");
  sprintf(curr_fifo, "%s.%03d", tmpstub, fifo_number);
  sprintf(next_fifo, "%s.%03d", tmpstub, fifo_number+1);
  sprintf(afternxt_fifo, "%s.%03d", tmpstub, fifo_number+2);
  mkfifo(PIH_LOG, S_IRWXU|S_IRWXG);
  mkfifo(curr_fifo, S_IRWXU|S_IRWXG);
  mkfifo(next_fifo, S_IRWXU|S_IRWXG); // make sure _next_ fifo already exists before we call partimage
  mkfifo(afternxt_fifo, S_IRWXU|S_IRWXG);
  system("cat " PIH_LOG " > /dev/null &");
  log_msg(3, "curr_fifo   = %s", curr_fifo);
  log_msg(3, "next_fifo   = %s", next_fifo);
  if (!does_file_exist(input_fifo)) { fatal_error ("input fifo does not exist" ); }
  if (!(fin = fopen(input_fifo, "r"))) { fatal_error ("Unable to openin from input_fifo"); }
  if ( !find_home_of_exe("partimagehack")) { fatal_error( "partimagehack not found" ); }
  sz_call_to_partimage[0]=2;
  sz_call_to_partimage[1]=0;
  sprintf(sz_call_to_partimage+2, "partimagehack " PARTIMAGE_PARAMS " restore %s %s > /dev/null 2>> %s", output_device, curr_fifo, MONDO_LOGFILE);
  log_msg(1, "output_device = %s", output_device);
  log_msg(1, "curr_fifo = %s", curr_fifo);
  log_msg(1, "sz_call_to_partimage+2 = %s", sz_call_to_partimage+2);
  res = pthread_create(&partimage_thread, NULL, call_partimage_in_bkgd, (void*)sz_call_to_partimage);
  if (res) { fatal_error("Failed to create thread to call partimage"); }
  log_msg(1, "Running fore/back at same time");
  log_msg(2," Trying to openin %s", input_fifo);
  if (!does_file_exist(input_fifo)) { log_msg(2, "Warning - %s does not exist", input_fifo); }
  while(!does_file_exist("/tmp/PARTIMAGEHACK-POSITION"))
    {
      log_msg(6, "Waiting for partimagehack (restore) to start");
      sleep(1);
    }
  while(sz_call_to_partimage[0]>0)
    {
      if (fread(tmp, 1, 128, fin)!=128) { fatal_error("Cannot read introductory block"); }
      if (strstr(tmp, NEXT_SUBVOL_PLEASE))
        { log_msg(2, "Great. Next subvol coming up."); }
      else if (strstr(tmp, NO_MORE_SUBVOLS))
        { log_msg(2, "Great. That was the last subvol."); break; }
      else
        { log_msg(2, "WTF is this? '%s'", tmp); fatal_error("Unknown interim block"); }
      if (feof(fin)) { log_msg(1, "Eof(fin) detected. Breaking."); break; }
      log_msg(3, "Processing subvol %d", fifo_number);
      log_msg(5, "fifo_number=%d", fifo_number);
      if (!(fout = fopen(curr_fifo, "w"))) { fatal_error("Cannot openout to curr_fifo"); }
      log_msg(6, "Deleting %s", oldest_fifo);
      copy_from_src_to_dest(fout, fin, 'r');
      paranoid_fclose(fout);
      fifo_number ++;
      unlink(oldest_fifo); // just in case
      strcpy(oldest_fifo, prev_fifo);
      strcpy(prev_fifo, curr_fifo);
      strcpy(curr_fifo, next_fifo);
      strcpy(next_fifo, afternxt_fifo);
      sprintf(afternxt_fifo, "%s.%03d", tmpstub, fifo_number+2);
      log_msg(6, "Creating %s", afternxt_fifo);
     mkfifo(afternxt_fifo, S_IRWXU|S_IRWXG); // make sure _next_ fifo already exists before we access current fifo
     fflush(fin);
//      system("sync");
      usleep(1000L*100L);
    }
  paranoid_fclose(fin);
  paranoid_system("sync");
  log_msg(1, "Partimagehack has finished. Great. Fin-closing.");
  log_msg(1, "Waiting for pthread_join");
  pthread_join(partimage_thread, NULL);
  res = sz_call_to_partimage[1];
  log_msg(1, "Yay. Partimagehack (restore) returned %d", res);
  unlink(prev_fifo);
  unlink(curr_fifo);
  unlink(next_fifo);
  unlink(afternxt_fifo);
  unlink(PIH_LOG);
  paranoid_free(tmp);
  paranoid_free(sz_call_to_partimage);
  paranoid_free(stuff);
  paranoid_free(prev_fifo);
  paranoid_free(curr_fifo);
  paranoid_free(next_fifo);
  paranoid_free(afternxt_fifo);
  paranoid_free(oldest_fifo);
  paranoid_free(tmpstub);
  return(res);
}

