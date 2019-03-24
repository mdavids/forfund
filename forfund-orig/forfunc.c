//
// Several functions
//
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "config.h"
#include "forfun.h"

/************************  USAGE  **********************************/
void
usage (void)
{
  // TODO make complete
  printf ("Usage here...\n");
  exit (EXIT_SUCCESS);
}

/*********************** DEBUGPRINT  *******************************/
int
debug_print (const char *debugmsg)
{
  if (do_debug)
    {
      puts (debugmsg);
    }
  return 0;
}

/***********************  DEMONIZE  ********************************/
int
daemonize (void)
{
  debug_print ("    [DEBUG] daemonizing...");
  if (daemon (1, NOCLOSE))
    {
      perror ("[ERROR] could not daemonize");
      exit (EXIT_FAILURE);
    }
  // Forking went well
  debug_print ("    [DEBUG] daemonized");
  printf ("pid after daemonizing: [new pid: %d - parent pid: %d]\n",
	  getpid (), getppid ());
  return 0;
}

/******************  CHANGE_USERID  ********************************/
void
change_userid (void)
{
  // Change uid
  if (getuid () != uid_number)
    {
      debug_print ("    [DEBUG] attempt to change uid");
      if (setuid (uid_number) != 0)
	{
	  sprintf (str, "[ERROR] Cannot fulfill the setuid(%d) request",
		   uid_number);
	  perror (str);
	  // Will print out explenation, like: [ERROR] Cannot fulfill the setuid(1000) request.: Operation not permitted
	  exit (EXIT_FAILURE);
	}
      else
	{
	  printf ("My new uid: [new uid: %d ]\n", getuid ());
	}
    }
  else
    {
      debug_print
	("    [DEBUG] change uid not needed, is already the right one");
    }
}

/*********************  CHANGE_DIR  ********************************/
void
change_dir (void)
{
  // Change dir    
  debug_print ("    [DEBUG] attempt to change dir");
  if (chdir (RUNNING_DIR) != 0)
    {
      sprintf (str, "[ERROR] Cannot change do to directory %s", RUNNING_DIR);
      perror (str);
      // Will print out explenation, like: [ERROR] Cannot change do to directory /root: Permission denied
      exit (EXIT_FAILURE);
    }
  debug_print ("    [DEBUG] dir was changed");
}

/********************  OPEN_SYSLOG  ********************************/
void
open_syslog (void)
{
  // Open syslog  
  debug_print ("    [DEBUG] opening syslog and send first message");
  openlog (PACKAGE, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  //TODO: are these flags ok?
  syslog (LOG_NOTICE, "Started as user %d", getuid ());
}

/*********************  CREATE PID  ********************************/
void
create_pid (void)
{
  // Try to create a pid file
  debug_print ("    [DEBUG] attempt to create pid file");
  pid = open (PID_FILE, O_RDWR | O_CREAT | O_EXCL, 0640);
  // TODO: check if we have the right flags
  if (pid < 0)
    {
      syslog (LOG_ERR, "bailing out, could not create pidfile %s", PID_FILE);
      sprintf (str, "[ERROR] Could not create pidfile %s", PID_FILE);
      perror (str);
      // Will print out explenation, like: [ERROR] could not create pidfile /root/forfund.pid: Permission denied
      exit (EXIT_FAILURE);
    }

  // Write pid to pid file
  debug_print ("    [DEBUG] write pid to pid file");
  sprintf (str, "%d\n", getpid ());
  if (write (pid, str, strlen (str)) < 0)
    {
      perror ("[ERROR] writing to pid file");
      // Should be rare
      exit (EXIT_FAILURE);
    }

  debug_print ("    [DEBUG] attempt to close pid file");
  // Close the pid file again, we're done with it
  if (close (pid) != 0)
    {
      perror ("[ERROR] Could not close pid file");
      syslog (LOG_ERR, "could not close pidfile '%s'", PID_FILE);
    }
}

/********************  LOG MESSAGE  ********************************/
void
log_message (char *message)
{

  time_t now;
  struct tm ts;
  char buf[80];
  FILE *logfile;

  // Get current time
  time (&now);
  // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
  ts = *localtime (&now);
  strftime (buf, sizeof (buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);

  debug_print
    ("    [DEBUG] attempt to open logfile and write a message to it");
  logfile = fopen (LOG_FILE, "a");
  if (!logfile)
    {
      perror ("[ERROR] fopen of logfile");
      return;
    }
  if (fprintf (logfile, "%s - %s\n", buf, message) < 0)
    {
      perror ("[ERROR] writing to logfile");
      fclose (logfile);		// the ugly way, without checking
      return;
    }
  if (fclose (logfile) != 0)	// the neath way
    {
      perror ("[ERROR] fclose of logfile");
    }
  debug_print
    ("    [DEBUG] message to logfile was send and logfile was closed again");
}

/********************  SIG HANDLER  ********************************/
void
signal_handler (int sig)
{
  debug_print ("    [DEBUG] signal handler in action");
  switch (sig)
    {
    case SIGHUP:
      log_message ("HangUP signal catched (and ignored)");
      syslog (LOG_NOTICE, "HangUP signal catched (and ignored)");
      break;
    case SIGTERM:
      log_message ("TERMinate signal catched");
      syslog (LOG_NOTICE, "TERMinate signal catched");
      wrap_up ();
      debug_print
	("    [DEBUG] ok, TERM - signing off (from signal handler) - bye!");
      // Since a SIGTERM is normal (systemd) behaviour, we exit with 0 and not with 1
      exit (0);
      break;
    case SIGINT:
      log_message ("INTerrupt signal catched");
      syslog (LOG_NOTICE, "INTerrupt signal catched");
      wrap_up ();
      debug_print
	("    [DEBUG] ok, INT - signing off (from signal handler) - bye!");
      exit (0);
      break;
    }
}

/************************  WRAP UP  ********************************/
void
wrap_up (void)
{
  debug_print
    ("    [DEBUG] wrap up, meaning trying to remove pid (if any) en send final logmessage");
  if (do_pid)
    {
      if (remove (PID_FILE))
	{
	  perror ("[ERROR} Could not delete pid file");
	  syslog (LOG_NOTICE, "could not delete pidfile %s while ending\n",
		  PID_FILE);
	  syslog (LOG_NOTICE,
		  "ended, with some issues while trying to removd the PID file");
	}
    }
  else
    {
      syslog (LOG_NOTICE, "ended normally");
    }
  log_message ("Ended");
}
