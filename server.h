/*
 * server.h
 *
 *  Created on: Jul 29, 2016
 *      Author: rob
 */

#ifndef SERVER_H_
#define SERVER_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>



const int    NET_CHAR_RECIEVE_INT   = 4;		//How many characters are being recieved (int in ASCII)
const int	   NET_CHAR_SEND_INT    = 5;	//How many characters are being sent. (int in ASCII)
const int    MESSAGE_LENGTH         = 500;	//Message Length. Can go up to 999 and function as normal.
const int	   CHAR_OFFSET			= 100;	//offset used to send same # of characters always.

enum action_command {LS, G, ERROR};

typedef struct command {
	int    action; //for action enum
	char * dataPort;
	char * fileName;
}command;

int  makeServer(char * portnumber);
void sigchild_handler(int signal);
void clientAction(int clientID, struct sockaddr_storage *client_address, socklen_t * length);
void sendFileSize(int client_fd, int fileSize);
void _getCommand(char * command, int sockfd);
command * _parseCommand(char * commandlist);
int makeConnection(struct sockaddr_storage * client_address, socklen_t * sin_size, struct command * commands);

#endif /* SERVER_H_ */
