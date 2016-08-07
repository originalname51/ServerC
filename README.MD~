This is a FTP server written in C. It is meant to work with a Java client. 

Run the server by first making the file (type make).

In order to start the server typle: ./server [PORTNUMBER] 

Please note the following:

1) To access the server the following input is expected.

For a LIST command:

serverName serverPortNumber -l ftpPortNumber

For a GET command:

serverName serverPortNumber -g fileName ftpPortNumber


The following are protocols used in order to communicate with the server. Software must be writtent o work with the protocols the server has.

2) The server communicates with the client by parsing the first message by reading 4 characters as integers. 
In order to send over the command first send the amount of characters in the message + 100(CHAR_OFFSET) to the server. 
After this send the message.

3)  For List command : 
	
a) Server will send over amount of files it has in a string of 17 characters. 
Please note: the string sent over will have the integer value in characters and then it will be buffered with '\0'.

b) Server will send over 256 characters each containing a file name. Extra characters will be buffered by a '\0'.

4) For Get command : 

 * 1: Read in as much as possible to the buffer.
	
 * 2: send the length of how much the buffer is to the client. (if file doesn't exist or read error send -1). 
 This is sent over in a string of 17 characters with non integer characters set to '\0'
 * 3: Server will read incrementally from file into a buffer. It will then send the buffer periodically (defined in chunks of BUFSIZ).


  


