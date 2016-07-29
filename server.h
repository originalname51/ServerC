#ifndef SERVER_H_
#define SERVER_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

const int    NET_CHAR_RECIEVE_INT = 4;		//How many characters are being recieved (int in ASCII)
const int	   NET_CHAR_SEND_INT    = 5;	//How many characters are being sent. (int in ASCII)
const int    MESSAGE_LENGTH       = 500;	//Message Length. Can go up to 999 and function as normal.
const int	   CHAR_OFFSET			= 100;	//offset used to send same # of characters always.

#endif /* SERVER_H_ */
