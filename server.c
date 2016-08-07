#include "server.h"


/*https://www.cerberusftp.com/support/help/configuring.htm
http://slacksite.com/other/ftp.html
Main function. Will make a server on port number argv[1].
Will then get in infinite loop accepting connections until terminated.
*/
int main(int argc, char *argv[]) {
	int sockID, client_fd;
	socklen_t sin_size;
	struct sockaddr_storage client_address;
	_signalHandler();
	/*
	 * Make the server
	 * */
	sockID = makeServer(argv[1]);
	/*
	 * This code  and the below code
	 * is based off the example code from beej.us guide,
	 * especially the simple server example.
	 *  http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#simpleserver
	 * */
	if (listen(sockID, 10) == -1) {
		perror("Server failed to listen.");
		close(sockID);
		exit(1);
	}
	while (1) {
		printf("Server awaiting for connections on port %s.\n", argv[1]);
		sin_size = sizeof(client_address);
		client_fd = accept(sockID, (struct sockaddr*) &client_address,
				&sin_size);
		if (client_fd == -1) {
			perror("Error getting file descriptor from accept.");
			exit(1);
		}
		printf("Successful connect!\n");
		//If not the child, loop, else go in client action.
		if (!fork()) {
			close(sockID); //Close server socket(because it's the child process)
			//This is action the child process will take.
			clientAction(client_fd, &client_address, &sin_size);
			close(client_fd);//Close client connection socket.
			return 0;
		}
	}
	//The following code SHOULD next execute.
	printf("This *should* never, ever, ever, ever, *ever* execute.");
	close(sockID);
	return 0;
}

/* This code is taken from beej.us guide and sets up the signal handler.
 * This will create a sigaction and assign the function sigchild_handler(which reaps dead children).
 * This is done to help the server not create zombies.
 */
void _signalHandler() {
	struct sigaction sa;
	sa.sa_handler = sigchild_handler;
	sigemptyset(&sa.sa_mask);	//This initializes the signal set.
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("error creating sigaction");
		exit(1);
	}
}
/*
 * This is from the tutorial code on beej.us. This is the sig action function that will handle
 * dead children from the main. This is used by the sigaction in _signalHandler() function.
 * */
void sigchild_handler(int s) {
	int e = errno;
//	printf("\nCaught Signal %d",s);
	while (waitpid(WAIT_ANY, NULL, WNOHANG) > 0); //While there is a process to reap reap it.
	errno = e; //reset error number.
}

/*
 * The following code is the heart of this program. This will parse a command sent by the client using _getCommand
 * and _parseCommand.
 *
 * It will send a value of "ERRO" if there is an error, or "PASS" if it is a valid command.
 *
 *	If it passes it will connect to the client and pass the program to one of the arguments.
 *
 *	Upon completion it will close the client connection as well as free files.
 * */
void clientAction(int client_fd, struct sockaddr_storage * client_address, socklen_t * sin_size) {
	int ftpConnection;
	char getUserInput[MESSAGE_LENGTH];
	_getCommand(getUserInput, client_fd);
	command * args = _parseCommand(getUserInput);

	//Send pass or fail from parsed arguments.
	if (args->action == ERROR) {
		sendFileSize(client_fd,-1);
		close(client_fd);
		return;
	} else {
		sendFileSize(client_fd,0);
	}
	sleep(1); //Give the client a chance to make a socket.
	ftpConnection = makeConnection(client_address, sin_size, args);	//make ftp data connection
	if (args->action == LS) {
		_LS_Command(ftpConnection, client_fd);	//Go to LS function.
		close(ftpConnection);
	} else if (args->action == G) {
		_G_Command(ftpConnection, client_fd, args); //Go to G function.
		free(args->fileName); //Free filename (only applicable in G function).
	}

	//Close and free relevant ports and memory.
	free(args->dataPort);
	free(args);
	close(client_fd);
}

/*
 * This code is based off the beej.us client code as well as the following.
 * This will make a connection to the client.
http://beej.us/guide/bgnet/output/html/multipage/getaddrinfoman.html
http://beej.us/guide/bgnet/output/html/multipage/getnameinfoman.html
http://beej.us/guide/bgnet/output/html/multipage/getpeernameman.html
 * */
int makeConnection(struct sockaddr_storage * client_address, socklen_t * sin_size, struct command * commands) {

	int dataConnectionFD;
	char ipstr[INET6_ADDRSTRLEN];
	struct sockaddr_in * ipv4 = NULL;
	struct sockaddr_in6 * ipv6 = NULL;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	/*This is modified from the following code:
	 *http://beej.us/guide/bgnet/output/html/multipage/getpeernameman.html
	 *It is unnecessary to actually call gerpeername because we already have the information we need.
	 * */
	if (client_address->ss_family == AF_INET) {
		ipv4 = (struct sockaddr_in *) client_address;
		inet_ntop(AF_INET, &ipv4->sin_addr, ipstr, sizeof ipstr);
	} else { // AF_INET6
		ipv6 = (struct sockaddr_in6 *) client_address;
		inet_ntop(AF_INET6, &ipv6->sin6_addr, ipstr, sizeof ipstr);
	}
//This code closely follows the beej.us guide's client server.
//http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#simpleclient
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo("Blackbird", commands->dataPort, &hints, &servinfo))!= 0) { //change Blackbird to ipstr on flip.
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((dataConnectionFD = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(dataConnectionFD, p->ai_addr, p->ai_addrlen) == -1) {
			close(dataConnectionFD);
			perror("client: connect");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		exit(2);
	}
	freeaddrinfo(servinfo); //no memory leak.
	printf("Connected to address: %s on port %s:\n", ipstr,commands->dataPort);
	return dataConnectionFD;
}

/*
 *
 * This has the following protocol :
 *
 * 1: Read in as much as possible to the buffer.
 * 2: send the length of how much the buffer is to the client. (if file doesn't exist or read error send -1)
 * 3: send buffer to client.
 * 4: repeat steps 2-3 until entire file has been sent.
 * 5: send 0 to terminate data connection.
 *
 * This will transfer bytes in a character array (which is the size of a byte and the same as a byte array in java.).
 *
 * This is primarily using code adapted from
 * http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
  http://stackoverflow.com/questions/883659/hardcode-byte-array-in-c
  http://stackoverflow.com/questions/32150119/transferring-values-from-a-char-array-to-an-integer-array-in-c-programming
  https://docs.oracle.com/javase/7/docs/api/java/io/BufferedReader.html
  http://www.rgagnon.com/javadetails/java-0542.html
*/
void _G_Command(int ftpConnection, int client_fd, struct command * args) {

	int filefd;
	char buffer[BUFSIZ];// https://www.gnu.org/software/libc/manual/html_node/Controlling-Buffering.html
	//Open the file. Send an error message and return if fail.
	filefd = open(args->fileName, O_RDONLY);
	if (filefd == -1) {
		printf("File not found. Sending error to client...");
		sendFileSize(client_fd, -1); //send a -1 to indicate error message
		return;
	}
	int fsize = _getFileSize(args);
	printf("Sending file size %d over to client... \n",fsize);
	sendFileSize(client_fd, fsize);


	printf("Sending File %s to client....\n", args->fileName);
	int read_return = 0;
	while (1) {
		read_return = read(filefd, buffer, BUFSIZ);
		if (read_return == 0)
			break;	//This terminates the loop and indicates the file has reached the end.
		if (read_return == -1) {
			perror("read");
			exit(1);
		}
		if (write(ftpConnection, buffer, read_return) == -1) { //send over the buffer
			perror("write");
			exit(1);
		}
	}
	printf("File Sent!\n"); //Server has sent the file.
}

/*
 * https://www.gnu.org/software/libc/manual/html_node/Simple-Directory-Lister.html
 * This function is adapted from the GNU manual directory lister.
 *
 * 1) First it opens and iterates through the directory and counts the files in the directory.
 * 2) Send the number of files to the clinet (so the client knows how many to expect)
 * 3) Iterate through directory sending files
 *
 * */
void _LS_Command(int ftpConnection, int client_fd) {

	char buffer[256];
	DIR *dp;
	struct dirent *ep;
	int counter = 0;

	//Iterate through count files.
	dp = opendir("./");
	if (dp != NULL) {
		while ((ep = readdir(dp))) {
			counter++;
		}
		(void) closedir(dp);
	} else
		perror("Couldn't open the directory");

	sendFileSize(client_fd, counter); //send file count.

	printf("Sending filelist to client....\n");
	//iterate through and send files.
	dp = opendir("./");
	if (dp != NULL) {
		while ((ep = readdir(dp))) {
			memset(buffer, '\0', 256);
			strcpy(buffer, ep->d_name);
			write(ftpConnection, buffer, 256);
		}
		(void) closedir(dp);
	} else
		perror("Couldn't open the directory");

	printf("Filelist sent.\n");
}

/*
 * http://stackoverflow.com/questions/14564813/how-to-convert-integer-to-character-array-using-c
 * This will be able to send a number under 256 characters long.
 * Primarily this uses sprintf to print the integer to a string and then sends it to the client.
 * */
void sendFileSize(int client_fd, int fileSize) {
	char numberString[18];
	memset(numberString, '\0', sizeof(numberString));
	sprintf(numberString,"%d",fileSize);
	numberString[17] = '\0';
	int sentSoFar = 0;
	while(sentSoFar != 17)
	{
	sentSoFar = write(client_fd, numberString, 17); //send 16 bytes.
			if(sentSoFar == -1){
		perror("Problem in sendFileSize function");
		sentSoFar += sentSoFar;
	}
	}
}

/*
 * This code is based off of the tutorial code from beej.us guide suggested by the reading.
 * I have commented through the code explaining how it works and slightly tweaked parts of it.
 *
 *This will set up a server and return a file descriptor for it.
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#simpleserver
 *
 * */
int makeServer(char * portnum) {
	int sockID;
	struct addrinfo addrRestrictions, *servinfo, *p;
	int rv;
	//set up restrictions for getaddrinfo call.
	//http://man7.org/linux/man-pages/man3/getaddrinfo.3.html

	memset(&addrRestrictions, 0, sizeof(addrRestrictions));
	addrRestrictions.ai_family = AF_UNSPEC; //ipv4 or ipv6 are both acceptable.
	addrRestrictions.ai_socktype = SOCK_STREAM; //TCP Address
	addrRestrictions.ai_flags = AI_PASSIVE; // This is the same as InADDR_ANY for IPv4 and IN6ADDR_ANY_INIT in IPv6
											//If this flag is not set it will make a loopback function in getaddrinfo.
	/*
	 * Get addr Info being set to null allows for AI_PASSIVE flag to be set which allows for INADDR_ANY behavior.
	 * Servinfo will contain an array of pointers
	 * */
	if ((rv = getaddrinfo(NULL, portnum, &addrRestrictions, &servinfo)) != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockID = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}

		//set at level SOL_Socket as we defined TCP earlier.
		//SO_REUSEARR allows for local addresses to be reused.
		//Yes is required (it is a boolean value of 1) because of SO_REUSEADDR
		//http://pubs.opengroup.org/onlinepubs/009695399/functions/setsockopt.html
		//This will allow us to configure some of the finer of the socket.
		//http://stackoverflow.com/questions/4233598/about-setsockopt-and-getsockopt-function
		int bool = 1; //for setsockpt funtion.
		if (setsockopt(sockID, SOL_SOCKET, SO_REUSEADDR, &bool, sizeof(int))
				== -1) {
			perror("setsockopt");
			freeaddrinfo(servinfo);
			exit(1);
		}
		//This will bind the socket to the address.
		if (bind(sockID, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockID);
			perror("server: bind");
			continue;
		}
		//Only successfully bound sockets will have made it this far. Break out of the loop.
		break;
	}

	//In order to not have a memory leak the servinfo needs to be freed.
	freeaddrinfo(servinfo);
	if (p == NULL) {
		perror("For loop in makeServer has failed to bind a socket.");
		exit(1);
	}
	//If we have a socket return to main.
	return sockID;
}

/*  http://stackoverflow.com/questions/266357/tokenizing-strings-in-c
 * 	This will parse commands and return the command struct. It will correctly parse port numbers
 * 	and command values. commandList is not affected.
 *
 * */
command * _parseCommand(char * commandList) {

	command * comReturn;
	comReturn = malloc(sizeof(command));

	int counter = 1;
	char tokenized[MESSAGE_LENGTH];

	strcpy(tokenized, commandList);	//I do not want to destroy command list so I copy it.

	char * commandArg = strtok(tokenized, " ");
	while (commandArg) {
		if (counter == 3) {
			if (strcmp(commandArg, "-l") == 0) {
				comReturn->action = LS;
			} else if (strcmp(commandArg, "-g") == 0) {
				comReturn->action = G;
			} else {
				comReturn->action = ERROR;
				return comReturn;
			}
		}
		if (counter == 4) {
			if (comReturn->action == LS) {
				comReturn->dataPort = strdup(commandArg);
			}
			if (comReturn->action == G) {
				comReturn->fileName = strdup(commandArg);
			}
		}
		if (counter == 5) {
			if (comReturn->action == G) {
				comReturn->dataPort = strdup(commandArg);
			}
		}
		commandArg = strtok(NULL, " ");
		counter++;
	}
	return comReturn;
}


/*
 * This was adapted from my chat server module. It will read in a character array
 * of how long a message is and then accept a message of however long it was from sockfd.
 * */
void _getCommand(char * message, int sockfd) {
	int index, n, filesize;
	memset(message, '\0', MESSAGE_LENGTH);

	//This will read in 4 characters(3 number characters + a newline), between 101 and 999.
	index = 0;
	while (index < NET_CHAR_RECIEVE_INT) {
		n = read(sockfd, &message[index], NET_CHAR_RECIEVE_INT - index);
		if (n < 0)
		{
			perror("ERROR reading from socket");
			exit(1);
		}
		index += n;
	}
	//Manipulate the message to see value of string heading our way.
	filesize = atoi(message);
	filesize -= CHAR_OFFSET;
	 //Set size of index to 0, reset message, and receive the message from server.
	index = 0;
	memset(message, '\0', MESSAGE_LENGTH);
	while (index < filesize) {
		n = read(sockfd, &message[index], (filesize - index));
		if (n < 0)
			perror("ERROR reading from socket");
		index += n;
	}
}


/*
 * This will return the byte size of the program. It is unnecessary
 * 1) Open the file.
 * 2) Use fseek to go to the end of a file.
 * 3) use Ftell to get the file location (which coincidentally is the file size in bytes)
 * 4) fseek to start of file
 * 5) close the file.
 * 6) return the size.
 * http://ask.systutorials.com/68/how-to-get-the-size-of-a-file-in-c
 * */
int _getFileSize(struct command * args)
{
	int size;
	FILE *fp;
	fp = fopen(args->fileName, "r");
	fseek(fp, 0,SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fclose(fp);
	return size;
}

