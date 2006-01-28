/* from-mondo.c

- subroutines copied from Mondo

*/

#include "structs.h"


extern void log_it_SUB(char*, t_loglevel, char*);
char *call_program_and_get_last_line_of_output(char*);
int call_program_and_log_output (char *);
bool does_file_exist (char *);
int make_hole_for_file (char *);
void strip_spaces(char*);



extern char g_logfile[MAX_STR_LEN+1];


/*************************************************************************
 * *call_program_and_get_last_line_of_ouput() -- Hugo Rabson             *
 *                                                                       *
 * Purpose:  						                 *
 * Called by:                                                            *
 * Returns:                                                              *
 *************************************************************************/

char *
call_program_and_get_last_line_of_output (char *call)
{
	/** buffers ******************************************************/
  static char result[MAX_STR_LEN+1];
  char tmp[MAX_STR_LEN+1];

	/** pointers *****************************************************/
  FILE *fin;

	/** initialize data **********************************************/
  result[0] = '\0';

  /***********************************************************************/
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
      pclose (fin);
    }
  strip_spaces (result);
  return (result);
}






/*************************************************************************
 * call_program_and_log_output() -- Hugo Rabson                           *
 *                                                                       *
 * Purpose:                                                              *
 * Called by:                                                            *
 * Returns:                                                              *
 *************************************************************************/
int
call_program_and_log_output (char *program)
{
	/** buffer *******************************************************/
  char callstr[MAX_STR_LEN+1];
  char incoming[MAX_STR_LEN+1];
  char tmp[MAX_STR_LEN+1];

	/** int **********************************************************/
  int res;
  int i;
  int len;

	/** pointers ****************************************************/
  FILE *fin;
  char *p;

	/** end vars ****************************************************/
  sprintf (callstr, "%s > /tmp/mondo-run-prog.tmp 2> /tmp/mondo-run-prog.err",
	   program);
  while ((p = strchr (callstr, '\r')))
    {
      *p = ' ';
    }
  while ((p = strchr (callstr, '\n')))
    {
      *p = ' ';
    }				/* single '=' is intentional */
  res = system (callstr);
  len = strlen (program);
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
  log_it (debug, tmp);
  system ("cat /tmp/mondo-run-prog.err >> /tmp/mondo-run-prog.tmp");
  unlink ("/tmp/mondo-run-prog.err");
  fin = fopen ("/tmp/mondo-run-prog.tmp", "r");
  if (fin)
    {
      for (fgets (incoming, MAX_STR_LEN, fin); !feof (fin);
	   fgets (incoming, MAX_STR_LEN, fin))
	{
/*	  for(i=0; incoming[i]!='\0'; i++);
	  for(; i>0 && incoming[i-1]<=32; i--);
	  incoming[i]='\0';
*/
	  strip_spaces (incoming);
	  log_it (debug, incoming);
	}
      fclose (fin);
    }
  log_it
    (debug, "--------------------------------end of output------------------------------");
  if (res)
    {
      log_it (debug, "...ran with errors.");
    }
  /* else { log_it(debug, "...ran just fine. :-)"); } */
  return (res);
}





/*************************************************************************
 * does_file_exist() -- Hugo Rabson                                      *
 *                                                                       *
 * Purpose:                                                              *
 * Called by:                                                            *
 * Returns:                                                              *
 *************************************************************************/
bool
does_file_exist (char *filename)
{

	/** structures ***************************************************/
  struct stat buf;
	/*****************************************************************/

  if (lstat (filename, &buf))
    {
      return (false);
    }
  else
    {
      return (true);
    }
}





/*-----------------------------------------------------------*/



/*************************************************************************
 * make_hole_for_file() -- Hugo Rabson                                   *
 *                                                                       *
 * Purpose:                                                              *
 * Called by:                                                            *
 * Returns:                                                              *
 *************************************************************************/
int
make_hole_for_file (char *outfile_fname)
{
	/** buffer *******************************************************/
  char command[MAX_STR_LEN+1];

	/** int  *********************************************************/
  int res = 0;

	/** end vars ****************************************************/

  sprintf (command, "mkdir -p \"%s\" 2> /dev/null", outfile_fname);
  res += system (command);
  sprintf (command, "rmdir \"%s\" 2> /dev/null", outfile_fname);
  res += system (command);
  sprintf (command, "rm -f \"%s\" 2> /dev/null", outfile_fname);
  res += system (command);
  unlink (outfile_fname);
  return (0);
}









/*************************************************************************
 * strip_spaces() -- Hugo Rabson                                         *
 *                                                                       *
 * Purpose:                                                              *
 * Called by:                                                            *
 * Returns:                                                              *
 *************************************************************************/
void
strip_spaces (char *in_out)
{
	/** buffers ******************************************************/
  char tmp[MAX_STR_LEN+1];

	/** pointers *****************************************************/
  char *p;

	/** int *********************************************************/
  int i;

	/** end vars ****************************************************/
  for (i = 0; in_out[i] <= ' ' && i < strlen (in_out); i++);
  strcpy (tmp, in_out + i);
  for (i = strlen (tmp); i>0 && tmp[i - 1] <= 32; i--);
  tmp[i] = '\0';
  for (i = 0; i < 80; i++)
    {
      in_out[i] = ' ';
    }
  in_out[i] = '\0';
  i = 0;
  p = tmp;
  while (*p != '\0')
    {
      in_out[i] = *(p++);
      in_out[i + 1] = '\0';
      if (in_out[i] < 32 && i > 0)
	{
	  if (in_out[i] == 8)
	    {
	      i--;
	    }
	  else if (in_out[i] == 9)
	    {
	      in_out[i++] = ' ';
	    }
	  else if (in_out[i] == '\t')
	    {
	      for (i++; i % 5; i++);
	    }
	  else if (in_out[i] >= 10 && in_out[i] <= 13)
	    {
	      break;
	    }
	  else
	    {
	      i--;
	    }
	}
      else
	{
	  i++;
	}
    }
  in_out[i] = '\0';
/*  for(i=strlen(in_out); i>0 && in_out[i-1]<=32; i--) {in_out[i-1]='\0';} */
}











