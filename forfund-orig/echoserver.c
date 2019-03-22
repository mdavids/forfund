#include <stdio.h>
#include <string.h>		//strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>		//close
#include <arpa/inet.h>		//close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>		//FD_SET, FD_ISSET, FD_ZERO macros
#include "forfun.h"

#define TRUE   1		// we could live without...

int opt = TRUE;
int master_socket, addrlen, new_socket, client_socket[30],
  max_clients = 30, activity, i, valread, sd;
int max_sd;
struct sockaddr_in6 address;
char buffer[1025];		//data buffer of 1K  
char addr[INET6_ADDRSTRLEN];
//set of socket descriptors  
fd_set readfds;
//a message
char *message = "ForFun ECHO Daemon ready.\r\n";	// TODO \r\n or just \n ?

void
echoserver_init (void)
{
  debug_print ("    [DEBUG] start echoserver_init");
  //initialise all client_socket[] to 0 so not checked  
  for (i = 0; i < max_clients; i++)
    {
      client_socket[i] = 0;
    }
  //create a master socket  
  if ((master_socket = socket (AF_INET6, SOCK_STREAM, 0)) == 0)
    {
      perror ("socket failed");
      exit (EXIT_FAILURE);
    }

  //set master socket to allow multiple connections ,  
  //this is just a good habit, it will work without this  
  if (setsockopt (master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
		  sizeof (opt)) < 0)
    {
      perror ("setsockopt");
      exit (EXIT_FAILURE);
    }

  //type of socket created  
  address.sin6_family = AF_INET6;
  address.sin6_addr = in6addr_any;
  address.sin6_port = htons (port_number);

  //bind the socket to port_number;
  if (bind (master_socket, (struct sockaddr *) &address, sizeof (address)) <
      0)
    {
      perror ("bind failed");
      exit (EXIT_FAILURE);
    }
  printf ("Listener on port %d \n", port_number);

  //try to specify maximum of 3 pending connections for the master socket  
  if (listen (master_socket, 3) < 0)
    {
      perror ("listen");
      exit (EXIT_FAILURE);
    }

  debug_print ("    [DEBUG] end echoserver init");
  // TODO: (sys)logging as well?
}

void
echoserver_run (void)
{
  debug_print ("    [DEBUG] start echoserver run loop");
  //accept the incoming connection  
  addrlen = sizeof (address);
  while (TRUE)
    {
      //clear the socket set  
      FD_ZERO (&readfds);

      //add master socket to set  
      FD_SET (master_socket, &readfds);
      max_sd = master_socket;

      //add child sockets to set  
      for (i = 0; i < max_clients; i++)
	{
	  //socket descriptor  
	  sd = client_socket[i];

	  //if valid socket descriptor then add to read list  
	  if (sd > 0)
	    FD_SET (sd, &readfds);

	  //highest file descriptor number, need it for the select function  
	  if (sd > max_sd)
	    max_sd = sd;
	}

      //wait for an activity on one of the sockets , timeout is NULL ,  
      //so wait indefinitely  
      activity = select (max_sd + 1, &readfds, NULL, NULL, NULL);


      if ((activity < 0) && (errno != EINTR))
	{
	  printf ("select error");
	}

      //If something happened on the master socket ,  
      //then its an incoming connection  
      if (FD_ISSET (master_socket, &readfds))
	{
	  if ((new_socket = accept (master_socket,
				    (struct sockaddr *) &address,
				    (socklen_t *) & addrlen)) < 0)
	    {
	      perror ("accept");
	      exit (EXIT_FAILURE);
	    }

	  //inform user of socket number - used in send and receive commands
	  //TODO turn into debug_print() or...?
	  inet_ntop (AF_INET6, &address.sin6_addr, addr, INET6_ADDRSTRLEN);
	  sprintf (str,
		   "New connection, socket fd is %d, remote ip is: %s, remote port is: %d",
		   new_socket, addr, ntohs (address.sin6_port));
	  log_message (str);
	  puts (str);

	  //send new connection greeting message  
	  if (send (new_socket, message, strlen (message), 0) !=
	      strlen (message))
	    {
	      perror ("send");
	    }
	  else
	    {
	      debug_print ("    [DEBUG] Welcome message sent successfully");
	      // TODO: some syslogging or var/logging?
	    }

	  //add new socket to array of sockets  
	  for (i = 0; i < max_clients; i++)
	    {
	      //if position is empty  
	      if (client_socket[i] == 0)
		{
		  client_socket[i] = new_socket;
		  printf ("Adding to list of sockets as %d\n", i);

		  break;
		}
	    }
	}

      //else its some IO operation on some other socket 
      for (i = 0; i < max_clients; i++)
	{
	  sd = client_socket[i];

	  if (FD_ISSET (sd, &readfds))
	    {
	      //Check if it was for closing , and also read the  
	      //incoming message  
	      if ((valread = read (sd, buffer, 1024)) == 0)
		{
		  //Somebody disconnected , get his details and print  
		  getpeername (sd, (struct sockaddr *) &address,
			       (socklen_t *) & addrlen);
		  inet_ntop (AF_INET6, &address.sin6_addr, addr,
			     INET6_ADDRSTRLEN);
		  sprintf (str, "Host disconnected , ip %s , port %d",
			   addr, ntohs (address.sin6_port));
		  log_message (str);
		  // TODO turn into debug_print() or...?
		  puts (str);

		  //Close the socket and mark as 0 in list for reuse  
		  close (sd);	// TODO add perror, or...?
		  client_socket[i] = 0;
		}

	      //Echo back the message that came in  
	      else
		{
		  //set the string terminating NULL byte on the end  
		  //of the data read  
		  buffer[valread] = '\0';
		  send (sd, buffer, strlen (buffer), 0);
		}
	    }
	}
    }
  // Most likely won't get here as long as we keep this and enless while
  // TODO make this a while that might end in some way?
  debug_print ("    [DEBUG] end echoserver run loop");
  //return 0; // TODO: keep, or remove?
}
