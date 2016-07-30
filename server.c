#include "server.h"

//https://www.cerberusftp.com/support/help/configuring.htm
//http://slacksite.com/other/ftp.html

int main(int argc, char *argv[])
{
	int sockID, client_fd;
	struct sigaction sa;
	socklen_t sin_size;
	struct sockaddr_storage client_address;

	/*
	 * This code is taken from beej.us guide and sets up the signal handler.
	 * */
	sa.sa_handler = sigchild_handler;
	sigemptyset(&sa.sa_mask);	//This initializes the signal set.
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("error creating sigaction");
		exit(1);
	}


/*
 * Make the server
 * */

	sockID = makeServer(argv[1]);

	/*
	 * This code is based off the example code from beej.us guide, especially the simple server.
	 * */
	if(listen(sockID, 10) == -1)
	{
		perror("Server failed to listen.");
		close(sockID);
		exit(1);
	}

while(1)
{
	printf("Server waiting for connections on port %s.\n", argv[1]);

	sin_size 	 = sizeof(client_address);
	client_fd	 = accept(sockID,(struct sockaddr*)&client_address, &sin_size);
	if(client_fd == -1)
	{
		perror("Error getting file descriptor from accept.");
		exit(1);
	}

	printf("Successful connect!\n");
	if(!fork())
	{
		close(sockID);
		clientAction(client_fd, &client_address, &sin_size);
		close(client_fd);
		return 0;

	}
	else
	{
	printf("I am the parent. I will show up once.\n");
	}
}

	close(sockID);
	return 0;
}

void clientAction(int client_fd, struct sockaddr_storage * client_address, socklen_t * sin_size)
{
	char getUserInput[MESSAGE_LENGTH];
	_getCommand(getUserInput, client_fd);
	long fileSize = 5;
    sendFileSize(client_fd, fileSize);
	close(client_fd);

}

void sendFileSize(int client_fd, long fileSize)
{
	char numberString[256];
    memset(numberString, '\0', sizeof(numberString));
	numberString[49] = '\n';
	gcvt(fileSize, 50,numberString);
	printf("In the child. I will show up once.\n");
	 if (send(client_fd, numberString, sizeof(numberString),0) == -1)
	 {
		 perror("Problem in sendFileSize function");
	 }


}

/*
 * This code is based off of the tutorial code from beej.us guide suggested by the reading.
 * I have commented through the code explaining how it works and slightly tweaked parts of it.
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#simpleserver
 *
 * */
int makeServer(char * portnum)
{
	int sockID;
	struct addrinfo addrRestrictions, *servinfo, *p;
	int bool = 1; //for setsockpt funtion.
	int rv;
	//set up restrictions for getaddrinfo call.
	//http://man7.org/linux/man-pages/man3/getaddrinfo.3.html

    memset(&addrRestrictions, 0, sizeof(addrRestrictions));
	addrRestrictions.ai_family   =  AF_UNSPEC; //ipv4 or ipv6 are both acceptable.
	addrRestrictions.ai_socktype =  SOCK_STREAM; //TCP Address
	addrRestrictions.ai_flags    =  AI_PASSIVE; // This is the same as InADDR_ANY for IPv4 and IN6ADDR_ANY_INIT in IPv6
												//If this flag is not set it will make a loopback function in getaddrinfo.
	/*
	 * Get addr Info being set to null allows for AI_PASSIVE flag to be set which allows for INADDR_ANY behavior.
	 * Servinfo will contain an array of pointers
	 * */
	if((rv = getaddrinfo(NULL, portnum, &addrRestrictions, &servinfo)) !=0)
	{
        printf("getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	   for(p = servinfo; p != NULL; p = p->ai_next) {
	        if ((sockID = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
	            perror("server: socket");
	            continue;
	        }

	        //set at level SOL_Socket as we defined TCP earlier.
	        //SO_REUSEARR allows for local addresses to be reused.
	        //Yes is required (it is a boolean value of 1) because of SO_REUSEADDR
	        //http://pubs.opengroup.org/onlinepubs/009695399/functions/setsockopt.html
	        //This will allow us to configure some of the finer of the socket.
	        //http://stackoverflow.com/questions/4233598/about-setsockopt-and-getsockopt-function
	        if (setsockopt(sockID, SOL_SOCKET, SO_REUSEADDR, &bool, sizeof(int)) == -1) {
	            perror("setsockopt");
	            exit(1);
	        }


	        /*
	         * This will bind the socket to the address.
	         * */
	        if (bind(sockID, p->ai_addr, p->ai_addrlen) == -1) {
	            close(sockID);
	            perror("server: bind");
	            continue;
	        }
	        //Only successfully bound sockets will have made it this far. Break out of the loop.
	        break;
	   }

	   /*
	    * In order to not have a memory leak the servinfo needs to be freed.
	    * */
	   freeaddrinfo(servinfo);


	   if(p == NULL)
	   {
		   perror("For loop in makeServer has failed to bind a socket.");
		   exit(1);
	   }

	   //If we have a socket return to main.
	   return sockID;
}

/*
 * This is from the tutorial code on beej.us. This is the sig action function that will handle
 * dead children from the main.
 * */
void sigchild_handler(int s)
{
	int e = errno;

	while(waitpid(WAIT_ANY,NULL,WNOHANG) > 0); //While there is a process to reap reap it.

	errno = e; //reset error number.
}


void _getCommand(char * message, int sockfd)
{
	int index, n, filesize;
	memset(message, '\0', MESSAGE_LENGTH);

	/*
	 * This will read in 4 characters(3 number characters + a newline), between 101 and 601. This is because each message is 500 characters.
	 * */
    index = 0;
	while(index < NET_CHAR_RECIEVE_INT)
	 {
	     n = read(sockfd, &message[index], NET_CHAR_RECIEVE_INT - index);
        if (n < 0)
           perror("ERROR reading from socket");
     	 index+=n;
	 }
	/*
	 * Manipulate the message to see value of string heading our way.
	 * */
	filesize  = atoi(message);
	filesize -= CHAR_OFFSET;

	/*
	 * Set size of index to 0, reset message, and receive the message from server.
	 * */
	index = 0;
	memset(message, '\0', MESSAGE_LENGTH);
	while(index < filesize)
	 {
		n = read(sockfd, &message[index], (filesize-index));
		if (n < 0)
			perror("ERROR reading from socket");
			index+=n;
	 }
}

