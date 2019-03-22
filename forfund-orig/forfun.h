//
// ForFun Daemon header file
//

// TODO: only put them here if they are global?

// forfunc.c
#define RUNNING_DIR "/tmp"
#define PID_FILE "/tmp/forfund.pid"
#define LOG_FILE "/tmp/forfund.log"
#define RUNAS_UID (uid_t)1000
// echoserver.c
#define PORT 88

#define NOCLOSE 1		// making this a command line parameter proved to be not very useful
				// and 1 proved to be more useful than 0


int pid;			// forfund.c and forfunc.c
char str[80];			// used in sprintf() in forfunc.c

int opt;			// forfund.c options
int do_daemon;
int do_debug;
int do_pid;
int port_number;
// declarations of functions
void usage (void);
int debug_print (const char *debugmsg);
int daemonize (void);
void change_userid (void);
void change_dir (void);
void open_syslog (void);
void create_pid (void);
void log_message (char *message);
void signal_handler (int sig);
void echoserver_init (void);
void echoserver_run (void);
void wrap_up (void);
