/* Include files */
#include <stdlib.h>        
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>              
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "MP3_Main.h"
#include "remote.h"

/* External functions and variables (defined elsewhere, and used here) */

/* Global functions and variables (defined here, and used here & elsewhere) */

/* Local functions and variables (defined here, used here) */

/* Start of code */

void
remote_comm (remote_command_t *ctrl, remote_status_t *status)
{
  static int init = 0;
  static int rcv_sock;
  static struct sockaddr_in rcv_addr, snd_addr;
  fd_set readfds;
  struct timeval timeout;
  int rlen;
  unsigned char rbuf[1000], tbuf[1000];
  socklen_t fromlen = sizeof (struct sockaddr_in);
  

  if (init == 0) {
    init = 1;
    
    if ((rcv_sock = socket (PF_INET, SOCK_DGRAM, 0)) < 0) {
      EXIT ("Cannot open remote control UDP receive socket!");
    }
    
    bzero ((char *) &rcv_addr, sizeof (rcv_addr));
    rcv_addr.sin_family = AF_INET;
    rcv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    rcv_addr.sin_port = htons (SND_UDP_PORT);
    
    if ((bind (rcv_sock, (struct sockaddr *) &rcv_addr, 
	       sizeof (rcv_addr))) < 0) {
      EXIT ("Cannot bind remote control UDP port to local address!");
    }
    
  }

  memcpy ((void *) tbuf, (void *) ctrl, sizeof (*ctrl));

  bzero ((char *) &snd_addr, sizeof (snd_addr));
  snd_addr.sin_family = AF_INET;
  snd_addr.sin_addr.s_addr = inet_addr ("127.0.0.1");
  snd_addr.sin_port = htons (RCV_UDP_PORT);

  if (sendto (rcv_sock, (const void *) tbuf, sizeof (*ctrl), 0,
	      (const struct sockaddr *) &snd_addr, fromlen) != sizeof (*ctrl)) {
    EXIT ("Cannot send remote control UDP data to peer!");
  }

  do {
    /* Check if there's any incoming message for us */
    FD_ZERO (&readfds);
    FD_SET (rcv_sock, &readfds);
    /* No wait */
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;
    
    if (select (rcv_sock+1, &readfds, (fd_set *) NULL, (fd_set *) NULL, 
		&timeout) > 0) {	/* Got something */
      if ((rlen = recv (rcv_sock, (void *) rbuf, sizeof (rbuf), 0)) > 0) {
	
	memcpy ((void *) status, (void *) rbuf, sizeof (*status));
	
#if 0
	printf ("Song: \"%s\" pos: %1.2f secs: %d rate: %d "
		"stereo: %d br: %d\n", status->song_name, status->position, 
		status->secs, status->sample_rate, status->stereo, 
		status->bitrate);
#endif

	/* Done */
	return;
      } 
	       
    } else {
      ERR ("Got no response!");
      return;
    }

    usleep (30000);

  } while (1);

  exit (0);
    
}

