/* 
 * horch - simple CAN bus analyzer, Header
 *
 *
 * Copyright (c) 1999-2007 port GmbH, Halle
 *------------------------------------------------------------------
 * $Header: /z2/cvsroot/library/co_lib/tools/horch/horch.h,v 1.24.2.3 2007/02/02 07:42:52 hae Exp $
 *
 *--------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *
 *
 * modification history
 * --------------------
 * $Log: horch.h,v $
 * Revision 1.24.2.3  2007/02/02 07:42:52  hae
 * new function set_timestamp
 *
 * Revision 1.24.2.2  2006/02/27 11:13:58  hae
 * add GPL header
 * delete old cvs log messages
 *
 * Revision 1.24.2.1  2005/08/02 13:52:52  oe
 * - added 'o' interactive command
 *
 *
 *
 * old log information deleted 2005/08/02 oe
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TRUE
# define TRUE  1
# define FALSE 0
#endif

#define PIDFILE "/var/run/horch.pid"

#define LOGFILE		"logfile"	/* file name for record log */
#define TESTCOB		0x672		/* ID of debug messages */
#define MAX_CLINE	100		/* max length of input line */

/* value of client_fd[] */
#define NO_CLIENT	(-1)

/* Define for Buffer handling */
#define BUFFER_REMOVE_ALL	(-1)

#define BDEBUG	if(debug) printf

/*---------------------------------------------------------------*/
/* Some options can control the behavour of 'horch' or the CAN driver.
 * Thise options are coded bit-wise in a byte flag.
 * The flag can be changed by the interactive command 'o'.
 * Here is the bit coding:
 */
#define OPTION_SELF		(1 << 0) /* self reception of the CAN driver */
#define OPTION_LISTENONLY	(1 << 1) /* listen mode of the CAN driver */
#define OPTION_TIMESTAMP	(3 << 2) /* select can4linux timestamp mode  */
/*---------------------------------------------------------------*/

/*---------------------------------------------------------------*/
#ifdef TARGET_CPC_WIN_BC
  /* special value for CPC Windows 
   * Windows has to poll the driver.
   * A big number of frames give us the chance to free more CPU time
   * between two FlushMbox() calls.
   */
# define MAX_CANMESSAGES_PER_FRAME 2000
#else /* TARGET_CPC_WIN_BC */

/* horch tries to put as much as possible CAN message in one TCP/IP frame.
 * but there is a limit in the buffer handling, therfore this number
 * must not exceed 20 !
 */
#define MAX_CANMESSAGES_PER_FRAME 20
/* #define MAX_CANMESSAGES_PER_FRAME 1 */
#endif /* TARGET_CPC_WIN_BC */
/*---------------------------------------------------------------*/
#if    defined(TARGET_LINUX_PPC) \
    || defined(TARGET_LINUX_COLDFIRE) \
    || defined(TARGET_LINUX_BF)
#define TARGET_LINUX
#endif

/*---------------------------------------------------------------*/
#if defined(TARGET_LINUX) || defined(TARGET_LINUX_ARM) || defined(TARGET_CPC_LINUX)
/*---------------------------------------------------------------*/
# include <errno.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <signal.h>

typedef struct  {
   struct timeval it_interval;
   struct timeval it_value;
} itimerval;
#endif

/*---------------------------------------------------------------*/
/* ifdef __unix__ */
#ifdef __GNUC__
/*---------------------------------------------------------------*/
/* in this part are includes, that are supported 
 * by Linux and Windows gcc
 */

# include <sys/ioctl.h>
/*---------------------------------------------------------------*/
#endif
/*---------------------------------------------------------------*/

/*---------------------------------------------------------------*/
# if defined(TARGET_LINUX_ARM) || defined(TARGET_CPC_LINUX) || defined(TARGET_OSX)
/*---------------------------------------------------------------*/
   /* cpc driver */
#  define CAN_MSG_LENGTH 8                /**< maximum length of a CAN frame */

#  define MSG_RTR	(1<<0)          /**< RTR Message */
#  define MSG_OVR	(1<<1)          /**< CAN controller Msg overflow error */ 
#  ifndef MSG_EXT
#   define MSG_EXT	(1<<2)          /**< extended message format */
#  endif
#  define MSG_SELF      (1<<3)          /**< message received from own tx */
#  define MSG_PASSIVE	(1<<4)          /**< controller in error passive */
#  define MSG_BUSOFF	(1<<5)          /**< controller Bus Off  */
#  define MSG_		(1<<6)          /**<  */
#  define MSG_BOVR	(1<<7)          /**< receive/transmit buffer overflow */

/**
* mask used for detecting CAN errors in the canmsg_t flags field
*/
#  define MSG_ERR_MASK	(MSG_OVR + MSG_PASSIVE + MSG_BUSOFF + MSG_BOVR)

# ifdef TARGET_CPC_LINUX
#  define STDDEV		"usb/cpc_usb0"
# else
#  define STDDEV		""
# endif

typedef int SOCKET;

typedef struct {
	int flags;
	int cob;
	unsigned long id;
	struct timeval timestamp;
	short int length;
	unsigned char data[CAN_MSG_LENGTH];

} canmsg_t;

#  define CONFIG_EXTENDED_IDENTIFIER 1

/* int	set_bitrate( char *line ); */
/* int	set_acceptance( char *line ); */
/*---------------------------------------------------------------*/
#endif /* defined(TARGET_LINUX_ARM) || defined(TARGET_CPC_LINUX) */
/*---------------------------------------------------------------*/

/*---------------------------------------------------------------*/
#if defined(TARGET_LINUX) 
/*---------------------------------------------------------------*/
   /* can4linux driver */
#  include <can4linux.h>

typedef void sigfunc(int);
sigfunc *signal (int, sigfunc *);

# ifndef MSG_EXT
#  define MSG_EXT	(1<<2)
# endif

# define STDDEV		"/dev/can0"

typedef int SOCKET;

/*---------------------------------------------------------------*/
# endif 


/*---------------------------------------------------------------*/



/*---------------------------------------------------------------*/
#if defined(TARGET_LX_WIN_BC) || defined(TARGET_AC2_WIN_BC)\
    || defined(TARGET_CPC_WIN_BC) || defined(TARGET_NRAY_WIN_BC)
/*---------------------------------------------------------------*/

#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include <winsock.h>
#include <time.h>
#include <dos.h>

#define CAN_MSG_LENGTH 8                /**< maximum length of a CAN frame */

#define MSG_RTR		(1<<0)          /**< RTR Message */
#define MSG_OVR		(1<<1)          /**< CAN controller Msg overflow error */ 
#ifndef MSG_EXT
# define MSG_EXT	(1<<2)          /**< extended message format */
#endif
#define MSG_SELF	(1<<3)          /**< message received from own tx */
#define MSG_PASSIVE	(1<<4)          /**< controller in error passive */
#define MSG_BUSOFF	(1<<5)          /**< controller Bus Off  */
#define MSG_		(1<<6)          /**<  */
#define MSG_BOVR	(1<<7)          /**< receive/transmit buffer overflow */

/**
* mask used for detecting CAN errors in the canmsg_t flags field
*/
#define MSG_ERR_MASK	(MSG_OVR + MSG_PASSIVE + MSG_BUSOFF + MSG_BOVR)


#define STDDEV		""

/* typedef int SOCKET; */

typedef struct {
	int flags;
	int cob;
	unsigned long id;
	struct timeval timestamp;
	short int length;
	unsigned char data[CAN_MSG_LENGTH];

} canmsg_t;

#if defined(TARGET_CPC_ECO_WIN_BC)
# define CONFIG_EXTENDED_IDENTIFIER 1
#endif

/* int	set_bitrate( char *line ); */
/* int	set_acceptance( char *line ); */
/*---------------------------------------------------------------*/
#endif /* TARGET_xxx_WIN_BC */
/*---------------------------------------------------------------*/

/*---------------------------------------------------------------*/
#ifdef TARGET_IPC
/*---------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <ipc/sys/socket.h>


#define STDDEV		"SJA1000"	/* only text */

#define CAN_MSG_LENGTH 8
#define MSG_RTR		(1<<0)		/**< RTR Message */
#define MSG_OVR		(1<<1)		/**< CAN controller Msg overflow error */
#define MSG_EXT		(1<<2)		/**< extended message format */
#define MSG_PASSIVE	(1<<4)		/**< controller in error passive */
#define MSG_BUSOFF      (1<<5)		/**< controller Bus Off  */
#define MSG_       	(1<<6)		/**<  */
#define MSG_BOVR	(1<<7)		/**< receive/transmit buffer overflow */
/**
* mask used for detecting CAN errors in the canmsg_t flags field
*/
#define MSG_ERR_MASK	(MSG_OVR + MSG_PASSIVE + MSG_BUSOFF + MSG_BOVR)


typedef int SOCKET;

struct timeval {
	long int tv_sec;
	long int tv_usec;

};
typedef struct {
	int flags;
	int cob;
	unsigned long id;
	/* struct timeval timestamp; */
        unsigned long timestamp;
	short int length;
	unsigned char data[CAN_MSG_LENGTH];

} canmsg_t;


int getopt ( int argc, char * const *argv, const char *optstring);
/*---------------------------------------------------------------*/
#endif		/* TARGET_IPC */
/*---------------------------------------------------------------*/

#include "filter/filter.h"

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
/* global variables */
/*---------------------------------------------------------------*/
extern int debug;
extern int o_focus;
extern int o_server;
extern int o_udpserver;
extern int o_bitrate;
extern int o_btr;
extern int o_portnumber;
extern int o_timestamp[HORCH_MAX_CLIENTS];
extern long o_period;
extern int o_show_status;
extern int show_time;
/* extern unsigned long  interrupts; */
extern char device[];
extern char horch_revision[];		/* Makefile generated version.c */

#ifdef __SOCKLIB_H
extern CLIENT_FD_T client_fd[HORCH_MAX_CLIENTS];
extern struct sockaddr_in fsin;		/* UDP socket */
#endif

extern float f_busload;


/*---------------------------------------------------------------*/
/* TCPIP Buffer for formatted message 
 * Console mode use client 0 buffer                    */
/*---------------------------------------------------------------*/
#ifdef  TCPIP_BUFFER_MAX
extern char send_line[HORCH_MAX_CLIENTS][TCPIP_BUFFER_MAX];
#endif

/*---------------------------------------------------------------*/
/* For better performance use a macro instead of function.
 * inline is not everytime supported.
 */
/* int buffer_size( const unsigned char client ); */
#define buffer_size(client) buffer_len[(client)]

int buffer_remove( const unsigned char client, const int count );
int buffer_add( const unsigned char client, const char * s );
/*---------------------------------------------------------------*/
#define skip_space(p) while(*(p) == ' ' || *(p) == '\t' ) (p)++
#define skip_word(p)  while(*(p) != ' ' && *(p) != '\t' ) (p)++


/*---------------------------------------------------------------*/
/* function proto types */
/*---------------------------------------------------------------*/
int	set_up(void);			/* hardware depend settings */
void	event_loop(void);		/* Console mode */
int	server_event_loop(void);	/* TCPIP Server mode */

int	udp_event_loop(void); /* unused */

void	clean_up(int);

int	show_message(const canmsg_t * const m); /* CAN Message -> String/TCPIP buffer */
int	show_system_time(char *line);	/* add Time to String buffer */

int	change_format(unsigned char client, char c); /* process commands */
#ifndef __WIN32__
void	Sleep(unsigned int time);
#endif
int	display_line(const unsigned char client);	/* send line through socket or not */
void	reset_send_line(const unsigned char client, const int decLen);
int	write_message(int format, char *line);	/* write CAN message */

void 	sendStatisticInformation(int client); /* send statistc info if possible */
void 	sendVersionInformation(int client); /* send version */
void 	sendFilterInformation(int client); /* Filter information */
void 	set_options(char *line);




/* CAN Driver depend functions */
int	set_acceptance(char *line);	/* set CAN register */
int	set_bitrate(char *line);	/* change bitrate */
void	getStat( char *line );	        /* fill line with status info */
void	set_selfreception(int v);	/* enable/disable self rx */
void	set_timestamp(int v);

const char * getLayer2Version(void);    /* HW part of version String */

