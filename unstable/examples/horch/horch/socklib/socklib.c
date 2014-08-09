/*
 * socklib - simple TCP socket interface
 *
 * Copyright (c) 2003 port GmbH Halle (Saale)
 *------------------------------------------------------------------
 * $Header: /z2/cvsroot/library/co_lib/tools/horch/socklib/socklib.c,v 1.6 2007/02/20 13:15:51 hae Exp $
 *
 *------------------------------------------------------------------
 *
 * modification history
 * --------------------
 * $Log: socklib.c,v $
 * Revision 1.6  2007/02/20 13:15:51  hae
 * bugfix: make client socket non blocking when a new connectionis established instead of the server socket
 *
 * Revision 1.5  2007/02/02 07:31:02  hae
 * updated to version 1.11 of socklib
 * stops hanging of a simple client programmed in C
 *
 * Revision 1.11  2006/03/03 07:35:57  hae
 * pass socket_options as pointer to so_open2
 *
 * Revision 1.10  2006/02/27 08:16:01  hae
 * fix type error
 * flush correct file descriptor
 *
 * Revision 1.9  2006/02/06 14:45:35  hae
 * allow to reuse the server port without waiting for the timeout
 * make use of the TCP_NODELAY socket option
 * this option can be specified with so_open2
 *
 * Revision 1.8  2005/06/17 10:06:42  hae
 * Compiler-Fehlermeldung entfernt
 *
 * Revision 1.7  2005/06/07 13:33:27  hae
 * Anpassung an CAN232 unter Linux
 *
 * Revision 1.6  2005/05/27 08:04:43  hae
 * Prüfung auf Null Zeiger
 * Debugausschriften kommentiert
 *
 * Revision 1.5  2004/12/08 16:27:16  hae
 * removed empty IPC ifdef
 * use write for Linux arm; not send
 *
 * Revision 1.4  2004/10/28 15:14:38  ro
 * so_server_doit() changed - timeout also for Linux
 * special for Linux: timeout == 0 means, no timeout
 * Linux timeout need for CPC driver, e.g. EtherCAN
 *
 * Revision 1.3  2003/10/21 12:08:51  boe
 * bearbeitete fd-Desc aus dem FD_ISSET  gelöscht
 *
 * Revision 1.2  2003/10/16 13:08:58  boe
 * use defines for return values at so_server_doit()
 * move client information to SOCKET_T
 * rename readline to so_readline
 *
 * Revision 1.1  2003/10/16 09:00:51  boe
 * socket library
 *
 *
 *
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#if defined(__WIN32__) || defined(_WIN32)
# ifndef __WIN32__
#  define __WIN32__
# endif
#  include <io.h>
#  include <sys/types.h>
#  include <winsock.h>
#  include <string.h>
#  include <errno.h>

#else
/* LINUX || CYGWIN */
#  include <unistd.h>                                            
#  include <errno.h>
#  include <signal.h>
#  include <sys/socket.h>
#  include <netdb.h>
#  include <strings.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>
#endif


#define S_LIBRARY
#include "socklib.h"

#ifndef MAX_CLIENTS
# define MAX_CLIENTS 10
#endif

#define SO_BUF_LEN 256 

int	so_debug   = 0;			/* enable/disable debugging */
static  SOCK_OPT_T sock_opt = {
	1,	/* TCP_NODELAY - short reaction time, default on */
	0,	/* binary */
	};

typedef struct {
	char data[SO_BUF_LEN];
	int pos;
} so_buf_t;

so_buf_t so_buf[MAX_CLIENTS];

static int so_lookahead(char *, int *,int );

/***************************************************************************
*
* so_open - open a socket stream
*
*
* \retval socket
*	a socket descriptor structure if succesful
* \retval NULL
*	if the socket can not be opened
*/

SOCKET_T *so_open(
	void
    )
{
SOCKET_T *sp;

#ifdef __WIN32__
  static WSADATA wd;

    /* initialize WinSock */
    if (WSAStartup(0x0101, &wd)) {
	fprintf(stderr, "cannot initialize WinSock\n");
	return NULL;
    }
    if (so_debug > 0) {
	printf("windows socket version: 0x%x\n", wd.wVersion);
    }
#endif

    /* get mem for socket structure */
    if ((sp = (SOCKET_T *)malloc(sizeof(SOCKET_T))) == 0) {
	return NULL;
    }

    /* call socket */
    sp->sd = socket(PF_INET, SOCK_STREAM, 0);
#ifdef __WIN32__
    if ( sp->sd == INVALID_SOCKET ) {
	perror("socket()");
	fprintf(stderr, "socket() - error %d\n", WSAGetLastError());
	WSACleanup();
	free(sp);
	return(NULL);
    }
#else
    if ( sp->sd < 0 ) {
# ifdef TARGET_IPC
	fprintf(stderr, "socket() open error %d\n", errno);
# else
	perror("socket()");
# endif
	free(sp);
	return(NULL);
    }
#endif

    sp->client = NULL;
    sp->actClients = 0;
    sp->maxClients = 0;

    return sp;
}

/***************************************************************************
*
* so_open2 - open a socket stream and set options
*
*
* \retval socket
*	a socket descriptor structure if succesful
* \retval NULL
*	if the socket can not be opened
*/
SOCKET_T *so_open2(
	SOCK_OPT_T *socket_options
    )
{
    memcpy(&sock_opt, socket_options, sizeof(SOCK_OPT_T));

    return so_open();
}


/***************************************************************************
*
* so_close - close a socket stream
*
*
* \retval
*	the result of the close() call
*/

int so_close(
	SOCKET_T *sp		/* the socket descriptor from so_sopen() */
	)
{
int sd, i;

    if (sp == NULL)  {
        return 0;
    }

    /* close all connections from server */
    if (sp->client != NULL)  {
	for (i = 0; i < sp->maxClients; i++)  {
	    if (sp->client[i] != -1)  {
#ifdef __WIN32__
		closesocket(sp->client[i]);
#else /* __WIN32__ */
		close(sp->client[i]);
#endif /* __WIN32__ */
	    }
	}
    }

    sd = sp->sd;

#ifdef __WIN32__
    WSACleanup();
#endif

    free(sp);
    return(close(sd));
}


/***************************************************************************
*
* so_client - establish a client connection to a host
*
*
* \retval sd
*	the file descriptor of the socket
*/

int so_client(
	SOCKET_T *sp,		/* the socket descriptor from so_sopen() */
	char *host,		/* name of the host i should connect to */
	int port		/* port number of the service */
	)
{
struct hostent *hostent;

    if((hostent = gethostbyname(host)) == 0) {
	return -1;
    }

    sp->sin.sin_family      = (short)hostent->h_addrtype;
    sp->sin.sin_port        = htons((unsigned short)port);
    sp->sin.sin_addr.s_addr = *(unsigned long *)hostent->h_addr;

    if (connect(sp->sd, (struct sockaddr *)&sp->sin, sizeof(sp->sin)) ==  -1) {
	return -1;
    }

    return (sp->sd);
}


/***************************************************************************
*
* so_server - prepare listen for host
*
*
* \retval 0
*	ok
* \retval errno
*	error
*/

int so_server(
	SOCKET_T *sp,		/* the socket descriptor from so_sopen() */
	int portnumber,		/* port number of the service */
	CLIENT_FD_T *client,	/* pointer to client structure */
	int maxClients		/* max number of clients */
	)
{
static struct sockaddr_in servaddr;
int	i;
int     reuse_addr;
int     ret;

    if (maxClients > MAX_CLIENTS) {
	perror("Too many clients!");
	exit(1);
    }
    /*
    * Bind our local address so that the client can send to us.
    */
    memset((void *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(portnumber);

#ifdef __WIN32__
    /*
     * http://www.itamarst.org/writings/win32sockets.html
     *
     * SO_REUSEADDR is weird 
     * On Windows it lets you bind to the same TCP port multiple times without errors.
     * Verified experimentally by failing test, I never bothered to check out
     * MSDN docs for this one, I just don't use it on Windows.
     */
#else
    reuse_addr = 1;
    ret = setsockopt(sp->sd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
    if (ret < 0) {
	perror("SO_REUSEADDR failed");
    }
#endif

    if (bind(sp->sd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
	perror("bind()");
#ifdef __WIN32__
	fprintf(stderr, "bind() - error %d\n", WSAGetLastError());
#endif /* __WIN32__ */
	so_close(sp);

	return(errno);
    }

    /*
    * Set socket to passive mode and ask kernel to buffer upto LISTENQ (inet.h)
    */
    if (listen(sp->sd, LISTENQ) < 0) {
	perror("listen()");
#ifdef __WIN32__
	fprintf(stderr, "listen() - error %d\n", WSAGetLastError());
#endif /* __WIN32__ */
	so_close(sp);
	return(errno);
    }

    /* initialize client fd array */
    sp->client = client;
    for (i = 0; i < maxClients; i++)  {
	client[i] = -1;
    }
    sp->actClients = 0;
    sp->maxClients = maxClients;
    sp->nready = 0;
    sp->maxfd = sp->sd +1;

    /* set fd list */
    FD_ZERO(&sp->allset);
    FD_SET(sp->sd, &sp->allset);

#ifdef __WIN32__
	{
	unsigned long lame = 1;
	ioctlsocket(sp->sd, FIONBIO, &lame);
	}
#else
	fcntl(sp->sd, F_SETFL, O_NONBLOCK);
#endif


    return(0);
}


/***************************************************************************
*
* so_write - write a buffer to a socket stream
*
*
* \retval val
*	the result of the close() call
*/

int so_write(
	int sd,			/* the sockets file descriptor */
	char *buf,		/* pointer where data are put in */
	int len			/* number of bytes to write */
	)
{

#if defined( __WIN32__) || defined(TARGET_IPC)
    return(send(sd, buf, len, 0));
#else
    return(write(sd, buf, len));
#endif

}

/***************************************************************************
*
* so_readline - read a line from socket 
*
* Read a line from a descriptor into buffer. Try to read
* SO_BUF_LEN bytes. If '\n' is detected earliear store it
* for the next read. We store the newline in the buffer,
* then follow it with a null (the same as fgets(3)).
* We return the number of characters up to, but not including,
* the null (the same as strlen(3)).
*
* \retval n
*	number of bytes read
*/

int so_readline(
	int fd,			/* socket desciptor */
	int idx,		/* changed index */
	char *ptr,		/* pointer where data are put in */
	int maxlen		/* maximum number of bytes to read */
	)
{
int cur_pos;
int len;
int recv_len;
int i;
int lookahead;
int byte_cnt = 0;
int total;
int copied_flag = 0;
int skip_cnt;

    cur_pos = so_buf[idx].pos;
    total = so_buf[idx].pos;
    len = SO_BUF_LEN - cur_pos;
    if ( SO_BUF_LEN > maxlen ) {
	len = maxlen - cur_pos;
    }

    /*
     * fill buffer with received data
     * until max_len is reached or '\n' is detected
     *
     * if '\n' is detected but more data was fetched
     * from the socket copy the rest data to the
     * beginning of the buffer and store the position
     * for the next read of this socket.
     */
    do {
    	recv_len = recv(fd, &so_buf[idx].data[cur_pos], len, 0);

    	if (recv_len < 0) {
	    byte_cnt = recv_len;
	    goto ready;
    	}
    	recv_len += cur_pos;
    	total += len;

    	for (i = 0; i < recv_len; i++ ) {
    	    byte_cnt++;

    	    if ((so_buf[idx].data[i] == '\n') ||
    	    	(so_buf[idx].data[i] == '\r') ) {

		*ptr = '\n';
		ptr++;
		*ptr = '\0';

		so_buf[idx].pos = 0;
		skip_cnt = i;
		lookahead = so_lookahead(&so_buf[idx].data[0],
					    &skip_cnt, recv_len);
		if (lookahead) {
		    /*
		    if (lookahead > skip_cnt) {
			so_buf[idx].pos = lookahead - skip_cnt +1;
		    } else {
			so_buf[idx].pos = lookahead;
		    }
		    */
		    so_buf[idx].pos = lookahead;
		    memcpy( &so_buf[idx].data[0],
		    	&so_buf[idx].data[i + skip_cnt],
			lookahead);
		}
		goto ready;
    	    } else {
    	    	copied_flag = 1;
		*ptr = so_buf[idx].data[i];
		ptr++;
    	    }

    	}
    	cur_pos = 0;
    	len = SO_BUF_LEN;

    /*
     * fails when maxlen < SO_BUF_LEN !!!
     */
    } while ((total < (maxlen - SO_BUF_LEN)));

    so_buf[idx].pos = 0;
    *ptr = '\0';

ready:

    return byte_cnt;
}


/***************************************************************************
*
* so_server_doit - serve until select call is interrupted
*
* server function waits,
* until the select() call is interrupted.
* The return value describes the reason:
* - error (select returns -1)
* - unknown reason (select returns 0)
* - data from client
* - tcp event (new client, client leaved)
* If the set from select() is known
* then the function read the data from the port
* to the given buffer.
* If there are no data,
* then the port was closed by the client.
* All tcp events (new client, client leave)
* are worked by this function itself.
* 
* \retval SRET_SELECT_ERROR
*	select returns value < 0
* \retval SRET_CONN_FAIL
*	new client connection was not possible
* \retval SRET_UNKNOWN_REASON
*	unknown reason for select interrupt
* \retval SRET_CONN_CLOSED
*	client connection has been closed
*	idx contains the index at the client list
* \retval SRET_CONN_NEW
*	new client connection has been etablished
*	idx contains the index at the client list
* \retval SRET_CONN_DATA
*	data from one of the clients
*	idx contains the index at the client list
*	buffer contains the data and dataCnt the length of the data
* \retval SRET_SELECT_USER
*	select with valid handle from user
*	no data are set
*/
int so_server_doit(
	SOCKET_T *sp,			/* socket pointer */
	int	*index,			/* changed index */
	char	*buffer,		/* pointer to receive buffer */
	int	*dataCnt,		/* max/received data count */
	int	timeout			/* timeout in msec for select call */
					/* Linux: timeout == 0 no timeout */
	)
{
static fd_set rset;				
static int lastsock;
int idx;
struct timeval tval;		/* use time out in W32 server */
int ret;

    if (sp->nready == 0)  {

	if (so_debug > 1) {
	    printf("Waiting for connections on port %d\n", 0);
	    fflush(stdout);
	}

	rset = sp->allset;	/* copy allset (structure assignment) */
	lastsock = 0;
    }
 
    /*
    * Wait for one of the file descriptors in rset to be ready for reading
    * (no timeout).
    */
    /*                             read. write, exc, time   */
#ifdef __WIN32__

    /* call with timeout */
    tval.tv_sec = timeout / 1000;
    tval.tv_usec = (timeout % 1000) * 1000;
    sp->nready = select(sp->maxfd, &rset, NULL, NULL, &tval);
#else /* __WIN32__ */
    if (timeout == 0) {
	sp->nready = select(sp->maxfd, &rset, NULL, NULL, NULL);
    } else {
	tval.tv_sec = timeout / 1000;
	tval.tv_usec = (timeout % 1000) * 1000;
	sp->nready = select(sp->maxfd, &rset, NULL, NULL, &tval);
    }
#endif /* __WIN32__ */
    if (sp->nready < 1) {

	/* Kann vom Timer Interrupt unterbrochen sein */
	if (sp->nready == 0) {
	    return SRET_UNKNOWN_REASON;
	} else {
	    /* < 0  means error */
#ifdef __WIN32__
	    /* error 10093 - Destination address required */
	    fprintf(stderr, "select() - error %d\n",WSAGetLastError());
	    so_close(sp);
#endif /* __WIN32__ */
	    sp->nready = 0;
	    return SRET_SELECT_ERROR;
	}
    }

    if ((so_debug > 0) && (sp->nready>0)) {
	printf("select sp->nready %d, lastsock %d\n", sp->nready, lastsock);
	fflush(stdout);
    }
    idx = 0;

    /* sp->nready is > 0 */


    /*
     * While there are remaining fds ready to read,
     * and clients in the array...
    */
    for (idx = 0; ((sp->nready > 0) && (idx < sp->maxClients)); idx++) {

	/* get sock fd */
	int sockfd = sp->client[idx];

	if ((sockfd >= 0) && (FD_ISSET(sockfd, &rset)) && 
	    (sockfd != lastsock) ) {

	    /* signal from this channel */
	    *index = idx;

	    if (sock_opt.binary) {
		*dataCnt = recv(sockfd, buffer, *dataCnt, 0);
	    } else {
		*dataCnt = so_readline(sockfd, idx, buffer, *dataCnt);
	    }

	    /*
	     * let this buffer be marked as pending
	     * with the next call to this function
	     * the buffer will be processed again
	     */
	    /* 
	     * printf(" socket: fd#%d, Buffer Pos: %d\n", sockfd, so_buf[idx].pos);
	    */
	    if (*dataCnt <= 0) {
		/* n == 0, connection closed by client */
		if (so_debug > 0) {
		    printf("Closing connection fd#%d\n", sockfd);
		    fflush(stdout);
		}
#ifdef __WIN32__
		closesocket(sockfd);
#else /* __WIN32__ */
		close(sockfd);
#endif /* __WIN32__ */

		sp->client[idx] = -1;
		sp->nready--;
		FD_CLR((unsigned int)sockfd, &sp->allset);
		sp->actClients --;

		/* recalc maximum socket descriptor */
		sp->maxfd = sp->sd;
		idx = 0; /* we can reuse idx, because we return */
		while (idx < sp->maxClients) {
		    if (sp->client[idx] > -1) {
			sp->maxfd = sp->client[idx];
		    }
		    idx++;
		}
		sp->maxfd++;

		return SRET_CONN_CLOSED;
	    } else {
		/*
		 * received data ok
		 */

		/*
		 * do not alternate sockets to read from when
		 * more than 1 client is connected
		 */
		if (sp->actClients > 1) { 
		    lastsock = sockfd;
		}

		sp->nready--;
		FD_CLR((unsigned int)sockfd, &rset);

		return SRET_CONN_DATA;
	    }

	}
    }

    /*
    * Is this a new client connection ?
    */
    if (FD_ISSET(sp->sd, &rset)) {
	socklen_t clilen;
	int connfd;
	struct sockaddr_in cliaddr;

	sp->nready--;		/* one file descriptor processed */
	FD_CLR((unsigned int)sp->sd, &rset);

	if (so_debug > 0) {
	    printf("new connection requested: %d\n", sp->nready);
	}

	/*
	* Accept the client connection
	*/
	clilen = sizeof(cliaddr);
	connfd = accept(sp->sd, (struct sockaddr *) &cliaddr, &clilen);
	if (connfd < 0) {
	    perror("accept()");
	    return SRET_CONN_FAIL;
	}

	if (so_debug > 0) {
	    printf("New client: %s, port %d; Assigning fd#%d\n",
	    inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), connfd);
	    fflush(stdout);
	}

	/*
	 * Check for a free entry at the client list
	 */
	idx = 0;
	while (idx < sp->maxClients) {
	    if (sp->client[idx] < 0) {
		break;
	    }
	    idx++;
	}

	if (idx == sp->maxClients) {
	    fprintf(stderr, "Error: too many clients - close it\n");
	    fflush(stderr);
#ifdef __WIN32__
	    closesocket(connfd);
#else /* __WIN32__ */
	    close(connfd);
#endif /* __WIN32__ */
	    return SRET_CONN_FAIL;
	}

	if (so_debug > 0) {
	    printf("New Client at index %d \n", idx);
	}

	sp->client[idx] = connfd;	/* save descriptor */
	*index = idx;
	so_buf[idx].pos = 0;

	/* Add the new descriptor to set (maintain sp->maxfd for select) */
	FD_SET(connfd, &sp->allset);
	if (connfd > sp->maxfd) {
	    sp->maxfd = connfd;
	}
	sp->maxfd++;

	/* maintain the maximum index indicator for the client[] array */
	sp->actClients++;

	if (sock_opt.tcp_no_delay == 1) {
	    /* 
	     * normally the 2. parameter should be SOL_TCP
	     * however IPPROTO seems to be just another/older name
	     * and is also used on windows
	     */
	    ret = setsockopt(sp->sd, IPPROTO_TCP, TCP_NODELAY,
		&sock_opt.tcp_no_delay, sizeof(sock_opt.tcp_no_delay));
	    if (ret < 0) {
		perror("TCP_NODLEAY failed");

	    }
	}

#ifdef __WIN32__
	{
	    unsigned long lame = 1;
	    ioctlsocket(sp->client[idx], FIONBIO, &lame);
	}
#else
	ret = fcntl(sp->client[idx], F_SETFL, O_NONBLOCK);
	if (ret < 0) {
	    perror("O_NONBLOCK");
	}
#endif

	return SRET_CONN_NEW;
    }

    sp->nready = 0;
    return SRET_SELECT_USER;
}

/***************************************************************************
*  so_lookahead
*
* check if buffer still holds valid data
* not only control characters and whitespaces
*
* if control characters are at the beginning
* count how many are to be skipped
*/

static int so_lookahead(char *ptr, int *start, int end) {
int lookahead;
int i;
int skip_cnt;	/* counter for characters to skip */
int stop_skip;

    /*
     * lookahead: is there real data or just '\r\n'
     * we also can skip whitespaces and tabs
     */
    lookahead = 0;
    skip_cnt = 0;
    stop_skip = 0;
    for (i = *start; (i < end) && !stop_skip; i++) {
#define LAZY
#ifdef LAZY
	if ( *(ptr + i) > 0x29 )
#else
	if ((*(ptr + i) != '\n') &&
	    (*(ptr + i) != '\r') )
#endif
	{
	    stop_skip = 1;
	} else {
	    skip_cnt++;
	}
    }
    lookahead = end - (skip_cnt + *start);
    *start = skip_cnt;

    return lookahead;
}
