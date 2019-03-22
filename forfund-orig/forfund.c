#include <unistd.h>
//#include <syslog.h>
#include <stdlib.h>
#include <signal.h>
//#include <string.h>
#include <stdio.h>
#include <fcntl.h>
//#include <time.h>
#include "config.h"
#include "forfun.h"

int
main (int argc, char *argv[])
{

  port_number = PORT;
  do_daemon = 1;

  puts ("This is " PACKAGE_STRING ".");

/********************  GET_OPTIONS  ********************************/
  while ((opt = getopt (argc, argv, "dfhip:")) != -1)
    // TODO: userid-option? And log and pid file directories options?
    //       also, for sure, a timer: run for max xx seconds or xx connections or something
    {
      switch (opt)
	{
	case 'd':		// print debug messages to standard output
	  do_debug = 1;		// should be first option to check here!
	  debug_print ("    [DEBUG] will print debug messages.");
	  break;
	case 'f':		// do not daemonize
	  do_daemon = 0;
	  debug_print ("    [DEBUG] will not daemonize.");
	  break;
	case 'i':		// create a pid
	  do_pid = 1;
	  debug_print ("    [DEBUG] will create pid.");
	  break;
	case 'p':
	  if (!optarg || (atoi (optarg) > 0 && atoi (optarg) <= 65535))
	    {
	      port_number = atoi (optarg);
	    }
	  else
	    {
	      printf
		("[ERROR] missing or invalid port number with -p option.\n");
	      exit (EXIT_FAILURE);
	    }
	  if (port_number != PORT)
	    {
	      sprintf (str,
		       "    [DEBUG] using portnumber %d instead of default %d",
		       port_number, PORT);
	      debug_print (str);
	    }
	  break;
	case ':':
	  printf ("option needs a value\n");
	  break;
	case 'h':
	  usage ();
	  break;
	case '?':
	  // the printf turns out to be not needed
	  //TODO but maybe we can log the message?
	  // printf ("unknown option: %c\n", optopt);
	  exit (EXIT_FAILURE);
	  break;
	}
    }

  // optind is for the extra arguments 
  // which are not parsed 
  for (; optind < argc; optind++)
    {
      printf
	("[ERROR] unexpected command line arguments: '%s'\n(give -h for help on usage).\n",
	 argv[optind]);
      exit (EXIT_FAILURE);
    }

  // catch some signals in our own handler
  signal (SIGCHLD, SIG_IGN);
  signal (SIGTSTP, SIG_IGN);
  signal (SIGTTOU, SIG_IGN);
  signal (SIGTTIN, SIG_IGN);
  signal (SIGHUP, signal_handler);
  signal (SIGTERM, signal_handler);
  signal (SIGINT, signal_handler);
  // SIGKILL results in the PID file not being removed
  // SIGINT is for when we don't daemonize and want to ctrl-c to tool

  // print some information (after all this is a testing and learning program)
  printf ("Starting like this: [uid: %d - pid: %d - parent pid: %d]\n",
	  getuid (), getpid (), getppid ());

  // daemonize
  if (do_daemon)
    {
      daemonize ();		
    }
  // start the echoserver (has also the init() )
  echoserver_init ();
  // change uid, change dir, open syslog, create pid file
  change_userid ();
  // Send a first message to /var/log/forfund.log
  // But only after we transfered to the proper userid
  log_message ("Startup");
  change_dir ();		
  open_syslog ();
  if (do_pid)
    {
      create_pid ();
    }
  echoserver_run ();		// TODO: run the echoserver only for certain amount of time
  // wrapping up
  wrap_up ();			// remove pid file basically
  debug_print ("    [DEBUG] signing off (from main) - bye!");
  exit (EXIT_SUCCESS);
}
