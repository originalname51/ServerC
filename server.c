#include "server.h"

//https://www.cerberusftp.com/support/help/configuring.htm
//http://slacksite.com/other/ftp.html

int main(int argc, char *argv[])
{
	int sockID;

	sockID = makeServer(argv[1]);
	printf("Project made it without error!\n");


	close(sockID);

	return 0;
}


int makeServer(char * portnum)
{
	int sockID;
	struct addrinfo addrRestrictions, *servinfo, *p;
	int bool = 1; //for setsockpt funtion.
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
	if(getaddrinfo(NULL, portnum, &addrRestrictions, &servinfo) !=0)
	{
		perror("Error in getaddrinfo function.");
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
