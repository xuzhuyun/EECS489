/*
 * Copyright (c) 2014, 2015, 2016 University of Michigan, Ann Arbor.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of Michigan, Ann Arbor. The name of the University 
 * may not be used to endorse or promote products derived from this 
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Sugih Jamin (jamin@eecs.umich.edu)
 *
*/
#include <stdio.h>         // fprintf(), perror(), fflush()
#include <assert.h>        // assert()
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>      // socklen_t
#else
#include <string.h>        // memset(), memcmp(), strlen(), strcpy(), memcpy()
#include <unistd.h>        // getopt(), STDIN_FILENO, gethostname()
#include <signal.h>        // signal()
#include <netdb.h>         // gethostbyname(), gethostbyaddr()
#include <netinet/in.h>    // struct in_addr
#include <arpa/inet.h>     // htons(), inet_ntoa()
#include <sys/types.h>     // u_short
#include <sys/socket.h>    // socket API, setsockopt(), getsockname()
#include <sys/ioctl.h>     // ioctl(), FIONBIO
#endif

#include "socks.h"
#include "netimg.h"

void
socks_init()
{
#ifdef _WIN32
  int err;
  WSADATA wsa;

  err = WSAStartup(MAKEWORD(2,2), &wsa);  // winsock 2.2
  net_assert(err, "socks_init: WSAStartup");
#else // _WIN32
  signal(SIGPIPE, SIG_IGN);    /* don't die if peer is dead */
#endif // _WIN32

  return;
}

void
socks_close(int td)
{
#ifdef _WIN32
  closesocket(td);
#else
  close(td);
#endif // _WIN32
  return;
}

/*
 * Lab1 Task 1: Fill out this function.
 *
 * socks_clntinit: creates a new socket to connect to the provided
 * server, either at the IPv4 address given in the sin_addr field of
 * the "server" argument, or, if the sin_addr field is 0, at the host
 * name given by the "sname" argument.  One of IPv4 address or host
 * name MUST be specified. If server->sin_addr contains a valid IPv4
 * address, sname may be NULL.  In either case, the server->sin_port
 * must be valid and must contain the port number of the server, IN
 * NETWORK BYTE ORDER.  Set the socket to linger for NETIMG_LINGER
 * upon closing, to give outstanding data a chance to arrive at the
 * destination.
 *
 * You can ignore the third argument "reuse" in Lab1.  For Lab2 and
 * PA1, "reuse" specifies whether the socket will be re-using the
 * provided port number.  If "reuse" is set (=1), you should set the
 * socket options SO_REUSEADDR and SO_REUSEPORT (#ifndef _WIN32).
 *
 * On success, return the newly created socket descriptor.
 * On connect() error, close the newly created socket descriptor and
 *   return SOCKS_UNINIT_SD.  On any other error, terminate process.
 */
int
socks_clntinit(struct sockaddr_in *server, char *sname, int reuse)
{
  int sd;
  /* create a new TCP socket */
  /* YOUR CODE HERE */
  if ((sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
	perror("opening TCP socket");
  	assert(0);
  }
  /* make the socket wait for NETIMG_LINGER time unit when closing
     to ensure that all data sent has been delivered */
  /* YOUR CODE HERE */
  struct linger so_linger;
  so_linger.l_onoff = 1;
  so_linger.l_linger = NETIMG_LINGER;
  if (setsockopt(sd, SOL_SOCKET, SO_LINGER, (char *)&so_linger, sizeof(struct linger)) < 0){
  	perror("linger");
  	assert(0);
  }
  /* Lab2 Task 2 (ignore for Lab1): reuse local port so that the call
     to bind in socks_servinit(), to bind to the same ephemeral port,
     doesn't complain of "Address already in use" or "Can't assign
     requested address." */
  /* LAB 2: YOUR CODE HERE */

  /* initialize the socket address with server's address and port
     number.  If provided server's address is NULL, obtain the
     server's address from sname. Be sure to also initialize server's
     address family. */
  /* YOUR CODE HERE */
  server->sin_family = AF_INET;
  if (server->sin_addr.s_addr == 0){
  	struct hostent *sp;
  	sp = gethostbyname(sname);
  	memcpy(&server->sin_addr, sp->h_addr, sp->h_length);
  }
  /* connect to server.  If connect() fails, close sd and return
     SOCKS_UNINIT_SD */
  /* YOUR CODE HERE */
  if (connect(sd, (struct sockaddr *) server, sizeof(struct sockaddr_in)) < 0){
  	socks_close(sd);
  	return SOCKS_UNINIT_SD;
  }
  return sd;
}

/* Lab1 Task 2: Fill out the rest of this function.
 *
 * sock_servinit: sets up a TCP socket listening for connection: bind
 * the socket with the "self" argument, which MUST have its sin_addr
 * and sin_port variables set by the caller.  Set the listen queue
 * length to NETIMG_QLEN.  If the provided sin_port is 0, obtain from
 * the socket the ephemeral port number assigned to it and update the
 * provided "self" variable with the assigned port number, in network
 * byte order. Next find out the FQDN of the current host and store it
 * in the provided variable "sname". Caller must ensure that "sname"
 * be of size NETIMG_MAXFNAME.  Determine and print out the assigned
 * port number to screen so that user would know which port to use to
 * connect to this server.
 *
 * You can ignore the "reuse" argument in Lab1.  For Lab2 and PA1,
 * "reuse" specifies whether the socket will be re-using the provided
 * port number.  If "reuse" is set (=1), you should set the socket
 * options SO_REUSEADDR and SO_REUSEPORT (#ifndef _WIN32).
 *
 * Terminate process on error.
*/
int
socks_servinit(char *progname, struct sockaddr_in *self, char *sname, int reuse)
{
  int sd;
  struct hostent *sp;
  unsigned int localaddr;
  u_short localport;

  /* create a TCP socket */
  /* YOUR CODE HERE */
  if ((sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
	perror("opening TCP socket");
  	assert(0);
  }
  /* initialize self */
  self->sin_family = AF_INET;
  self->sin_addr.s_addr = INADDR_ANY;
  localport = self->sin_port; // set by caller

  /* Lab2 Task 1: ignore for Lab1 
   * reuse local port so that the call to bind with the same 
   * ephemeral port, doesn't complain of "Address already 
   * in use" or "Can't assign requested address." 
  */
  /* LAB 2: YOUR CODE HERE */

  /* bind address to socket */
  /* YOUR CODE HERE */
  if (bind(sd, (struct sockaddr *) self, (socklen_t) sizeof(sockaddr_in)) < 0) {
	perror("bind");
	printf("Cannot bind socket to address\n");
  	assert(0);
  }
  /* listen on socket */
  /* YOUR CODE HERE */
  if (listen(sd, NETIMG_QLEN) < 0) {
	perror("error listening");
  	assert(0);
  }
  if (!localport) {
    /*
     * if self->sin_port was 0, obtain the ephemeral port assigned to
     * socket and store it in the member variable "self".
     */
    /* YOUR CODE HERE */
    int n = sizeof(self);
    if (getsockname(sd, (struct sockaddr *) self, (socklen_t*) &n) < 0){
    	perror("get sock name");
  	assert(0);
    }
  }
  /* Find out the FQDN of the current host and store it in the
     variable "sname".  gethostname() is usually sufficient. */
  /* YOUR CODE HERE */
  if (gethostname(sname, NETIMG_MAXFNAME) < 0){
    	perror("get host name");
  	assert(0);
  }
  /* Check if the hostname is a valid FQDN or a local alias.
     If local alias, set sname to "localhost" and addr to 127.0.0.1 */
  sp = gethostbyname(sname);
  if (sp) {
    memcpy(&self->sin_addr, sp->h_addr, sp->h_length);
  } else {
    localaddr = (unsigned int) inet_addr("127.0.0.1");
    memcpy(&self->sin_addr, (char *) &localaddr, sizeof(unsigned int));
    strcpy(sname, "localhost");
  }
    
  /* inform user which port this server is listening on */
  fprintf(stderr, "%s address is %s:%d\n",
          progname, sname, ntohs(self->sin_port));
    
  return sd;
}

/* Lab1 Task 2: Fill out the rest of this function.
 *
 * socks_accept: accepts connection on the given socket, sd.
 *
 * On connection, set the linger option for NETIMG_LINGER to allow
 * data to be delivered to client.  If "remote" is not NULL, store the
 * remote host's struct sockadr_in in *remote.  If "verbose" == 1,
 * print out the remote's name/address and port #.
 * Return the descriptor of the connected socket.
 * Terminates process on error.
*/
int
socks_accept(int sd, struct sockaddr_in *remote, int verbose)
{
  int td;
  struct sockaddr_in sockaddr;
  struct sockaddr_in *client;
  struct hostent *cp;

  client = remote ? remote : &sockaddr;

  /* Accept the new connection.  Use the variable "td" to hold the new
   * connected socket.  Use the local variable "client" in the call to
   * accept() to hold the connected client's sockaddr_in info.
  */
  /* YOUR CODE HERE */
  int len = sizeof(struct sockaddr_in);
  td = accept(sd, (struct sockaddr *) client, (socklen_t *)&len);
  /* make the socket wait for NETIMG_LINGER time unit upon closing
     to ensure that all data sent has been delivered. */
  /* YOUR CODE HERE */
  struct linger so_linger;
  so_linger.l_onoff = 1;
  so_linger.l_linger = NETIMG_LINGER;
  if (setsockopt(sd, SOL_SOCKET, SO_LINGER, (char *)&so_linger, sizeof(struct linger)) < 0){
  	perror("linger");
  	assert(0);
  }
  if (remote != NULL){
  	remote->sin_addr = client->sin_addr;
  	remote->sin_port = client->sin_port;
  }
  if (verbose) {
    /* inform user of connection */
    cp = gethostbyaddr((char *) &client->sin_addr,
                       sizeof(struct in_addr), AF_INET);
    fprintf(stderr, "Connected from client %s:%d\n",
            ((cp && cp->h_name) ? cp->h_name : inet_ntoa(client->sin_addr)),
            ntohs(client->sin_port));
  }

  return(td);
}

/*
 * Lab2 Task 2
 * socks_clear: remove every bit of data present in the TCP receive
 * queue until there's no more data left.
 *
 * If socket is blocking (nonblock=1), set it to non-blocking first,
 * then after removing all bits, restore blocking.
 *
 * [Note: winsocks has no way to check whether a socket is blocking.
 * It doesn't have fcntl() or equivalent.]
 */
void
socks_clear(int sd, int nonblock)
{
  /* YOUR CODE HERE */

  return;
}
