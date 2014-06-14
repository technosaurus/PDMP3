#ifndef _REMOTE_H_
#define _REMOTE_H_

/* Include files */

/* External functions and variables (defined elsewhere, and used here) */

/* Global functions and variables (defined here, and used here & elsewhere) */

/* Local functions and variables (defined here, used here) */
#define RCV_UDP_PORT    11556
#define SND_UDP_PORT    11555

typedef struct {
  char song_name[100];
  float position;		/* 0..1 */
  int secs;			/* Total seconds in song */
  int sample_rate;
  int stereo;
  int bitrate;
} remote_status_t;

typedef enum {
  cmd_getstatus,
  cmd_pause,
  cmd_play,
  cmd_ffwd,
  cmd_rew,
  cmd_setpos,
  cmd_skipnext,
  cmd_skipprev
} remote_cmd_t;

typedef struct {
  remote_cmd_t cmd;
  float argument;		/* setpos position, ffwd or rew speed */
} remote_command_t;

/* Start of code */

#endif /* _REMOTE_H_ */
