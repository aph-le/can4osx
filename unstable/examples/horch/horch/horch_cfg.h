/*
* Configuration of horch
*
*/


/*!
 * maxmum number of Clients which van connect to the TCP/IP server 
 * --------------------------------------------------------------------*/
#ifndef HORCH_MAX_CLIENTS 
# define HORCH_MAX_CLIENTS 10
#endif /* HORCH_MAX_CLIENTS  */

/* Buffer sizes - String buffer for each client
 * --------------------------------------------------------------------*/
#ifndef TCPIP_BUFFER_MAX 
# if MAX_CANMESSAGES_PER_FRAME > 20
#  define TCPIP_BUFFER_MAX 300000
# else
   /* default */
#  define TCPIP_BUFFER_MAX 30000
# endif
#endif

/*!
 * If the number of bytes reaches the TCPIP_BUFFER_TRANSMIT limit
 * the server tries to transmit it
 */
#ifdef TARGET_LINUX_ARM 
#  define TCPIP_BUFFER_TRANSMIT	(10000)
#else
#  define TCPIP_BUFFER_TRANSMIT	(TCPIP_BUFFER_MAX - 500)
#endif

/*!
 * Number of bytes should left in the buffer
 * application stops filling it.
 */
#define TCPIP_BUFFER_STOP	(TCPIP_BUFFER_MAX - 160)

/* Buffer size for  debug messages, 80 should be enough, but ... 
 * -------------------------------------------------------------------*/
#define DEBUGMSG_BUFFER_MAX 2056

