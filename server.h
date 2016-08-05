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
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

const int    NET_CHAR_RECIEVE_INT   = 4;		//How many characters are being received (int in ASCII)
const int	   NET_CHAR_SEND_INT    = 5;	//How many characters are being sent. (int in ASCII)
const int    MESSAGE_LENGTH         = 500;	//Message Length. Can go up to 999 and function as normal.
const int	   CHAR_OFFSET			= 100;	//offset used to send same # of characters always.

enum action_command {LS, G, ERROR};

typedef struct command {
	int    action; //for action enum
	char * dataPort;
	char * fileName;
}command;

int  makeServer(char * portnumber);	//makes server
void sigchild_handler(int signal);	//function to assign to sig handler
void _signalHandler();	//sighandler function
void clientAction(int clientID, struct sockaddr_storage *client_address, socklen_t * length); //crux of client program. Almost the "main" of the client connection.
void sendFileSize(int client_fd, int fileSize);	//utility function to send file size
void _getCommand(char * command, int sockfd);	//works with parse command to parse client instructions.
command * _parseCommand(char * commandlist);	//works with get command to parse client instructions
int makeConnection(struct sockaddr_storage * client_address, socklen_t * sin_size, struct command * commands); //makes connection (used for ftp data port)
void _G_Command(int ftpConnection, int client_fd, struct command * args); //G command instructions.
void _LS_Command(int ftpConnection, int client_fd);							//L command instructions.
#endif /* SERVER_H_ */
