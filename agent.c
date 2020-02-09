#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <unistd.h>
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <errno.h> 
#include <netdb.h> 
#include <string.h>

#include "common.h" 

int PORT = 21839;
int MANAGER_PORT = 8080;
int LIFETIME = 10;
char* MANAGER_IP = "localhost";

#define MAXLINE 1024
#define INTERVAL 2
#define MAX 1024
#define SA struct sockaddr
//#define _POSIX_SOURCE
#include<sys/utsname.h>

void GetLocalOS(char OS[], int *valid);
void GetLocalTime(int *time, int *valid);
void listener(int sockfd) ;

void getIPAddress(char arr[]);

/*
 * Function: getLocalOS: OS,valid
 * -----------------------------
 * Get's underlying Operating System name and copies it into the char array 'OS'
 * If OS name cannot be determined, it sets the flag valid to 0
 *
 */
void getLocalOS(char OS[MAX], int *valid){
	*valid = 1;
	struct utsname uts;
	if (uname(&uts)){
		*valid = 0;
		strcpy(OS,"Unknown!");	
	}
	else{
		strcpy(OS,uts.sysname);
		strcat(OS,"\n");
	}
}
/* Function: geetLocalTime: t, valid
 * ---------------------------------
 * Set's the local time in seconds to the value t.
 * Set's flag valid to 0 if time cannot be determined.
 *
 */
 void getLocalTime(int *t, int *valid){
	 time_t seconds;
	 *t = (unsigned) time(&seconds);
	 *valid = (*t > 0) ? 1 : 0;
 }

// Returns hostname for the local computer
// geegsforgeegks.com 
void checkHostName(int hostname) 
{ 
    if (hostname == -1) 
    { 
        perror("gethostname"); 
        exit(1); 
    } 
} 
  
// Returns host information corresponding to host name
// geegksforgeeks.com 
void checkHostEntry(struct hostent * hostentry) 
{ 
    if (hostentry == NULL) 
    { 
        perror("gethostbyname"); 
        exit(1); 
    } 
} 
  
// Converts space-delimited IPv4 addresses 
// to dotted-decimal format
// geeksforgeeks.com
void checkIPbuffer(char *IPbuffer) 
{ 
    if (NULL == IPbuffer) 
    { 
        perror("inet_ntoa"); 
        exit(1); 
    } 
}

/*
 * Function getIPAddress: arr[]
 * ----------------------------
 *  => Grabs the local ip in the form of a char*, and tokenizes it into a char[]
 *  => return the ip address in array format. eg.[122,123,23,227]
 *
 *  => Code adapted from geeksforgeeks.com
 */
void getIPAddress(char arr[]){
	char hostbuffer[256]; 
    char *IPbuffer; 
    struct hostent *host_entry; 
    int hostname;
  
    // To retrieve hostname 
    hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
    checkHostName(hostname); 
  
    // To retrieve host information 
    host_entry = gethostbyname(hostbuffer); 
    checkHostEntry(host_entry); 
  
    // To convert an Internet network 
    // address into ASCII string 
    IPbuffer = inet_ntoa(*((struct in_addr*) 
                           host_entry->h_addr_list[0])); 
  
    printf("\nAgent domain: %s\n", hostbuffer); 
    printf("Agent IP: %s\n", IPbuffer);

    /* get the first token */
    char* delim = ".";
    char* token = strtok(IPbuffer, delim);
    
    int i=0;
	/* walk through other tokens */
	while( token != NULL ) {
	  //printf( "%s ", token );
	  arr[i++] = atoi(token);
	  token = strtok(NULL, delim);
	}

	printf("\n");

}

/*
 * Function BeaconSender: value
 * ----------------------------
 * =>  Given a beacon value, create a datagram and send the beacon to the server.
 * =>  This function sends beacons in between prespecified intervals over a period
 *     of a prespecified lifetime.
 * =>  On the third try of sending a beacon, function deliberatly stalls to simulate
 *     packet loss. (This loss must be detected by the receiving end/Manager program)
 *
 * => Code adapted from geeksforgeeks.com
 */

void *BeaconSender(void *value) { 

	char arr[4];
	getIPAddress(arr);

	struct BEACON* b = (struct BEACON *) value;
	for (int i=0;i<4;i++){
	 	b->IP[i] = arr[i];
	}
	printf("BeaconSender running:\n\n");
	printf("ID %d\n",b->ID);
	printf("Startup %d\n",b->startUpTime);
	printf("Interval %d\n",b->timeInterval);
	printf("IP %d.%d.%d.%d\n",b->IP[0],b->IP[1],b->IP[2],b->IP[3] & 0xff);
	printf("Port %d\n\n",b->cmdPort);


	int sockfd; 
	char buffer[MAXLINE]; 
	struct sockaddr_in	 servaddr; 

	// Creating socket file descriptor 
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	memset(&servaddr, 0, sizeof(servaddr)); 
	
	// Filling server information 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(MANAGER_PORT);//htons(MANAGER_PORT); 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	
	int n, len;

	int i;
	for (i=1;i<=LIFETIME;i++){
		sendto(sockfd, b, sizeof(struct BEACON),
		MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
			sizeof(servaddr)); 
		printf("Beacon sent: %d\n",i); 
			
		// n = recvfrom(sockfd, (char *)buffer, MAXLINE, 
		// 			MSG_WAITALL, (struct sockaddr *) &servaddr, 
		// 			&len); 
		// buffer[n] = '\0'; 
		// printf("Server : %s\n", buffer);
		
		//Die and resurrect on the third beacon 
		sleep(i == 3 ? (b->timeInterval * 2) + 1 : b->timeInterval);
	}
	
	

	close(sockfd);

	return NULL; 
}

/*
 * Function listener: sockfd
 * --------------------------
 *  => Given a socket descriptor, listen for TCP packets and decode commands.
 *  => Expects two kinds of commands. 'OS' and 'TIME'.
 *
 *  => For the sake of demo purposes, the commands have not been parsed. Instead,
 *     they are displayed sequentially as this program intermittently replies with
 *     values to the remote program.
 *
 *  => Code adapted from geeksforgeeks.com
 */

void listener(int sockfd) 
{ 
    char buff[MAX];
    int t, n, valid; 
    // infinite loop for chat 
    //for (;;) {
  
    // Read first command from Manager and respond
    read(sockfd, buff, sizeof(buff)); 
    printf("Command: %s", buff); 
    bzero(buff, MAX);
    getLocalOS(buff,&valid);
    printf("Reply %s\n",buff);
    write(sockfd, buff, sizeof(buff));

    // Read second command from Manager and respond
    read(sockfd,buff,sizeof(buff));
    printf("Command: %s", buff);
    bzero(buff, MAX);
    getLocalTime(&t,&valid);
    printf("Reply %ld\n",t);
    //t = htonl(t);
    write(sockfd, &t, sizeof(t));
  
        // if msg contains "Exit" then server exit and chat ended. 
        /*if (strncmp("exit", buff, 4) == 0) { 
            printf("Server Exit...\n"); 
            return ; 
        } */
    //} 
} 

/*
 * Function CmdAgent: value:
 * -------------------------
 *  => This thread sets up a TCP server that accepts commands from a remote client.
 *
 *  => Code adapted from geeksforgeeks.com
 */

void *CmdAgent(void *value){
	printf("CmdAgent running...\n");
	struct BEACON* b = (struct BEACON *) value;
	int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    //else
        //printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 

  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT); 

  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    //else
       // printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("CMD Agent listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    //else
        //printf("server acccept the client...\n"); 
  
    // Function for chatting between client and server 
    listener(connfd); 
  
    // After chatting close the socket 
    close(sockfd); 



	return NULL;
}

int main(int argc, char *argv[]){ 
	int port = PORT;
	int interval = INTERVAL;
	if (argc >= 2){
		//Agent's TCP port
		port = atoi(argv[1]);
		PORT = port;
	}
	if (argc >= 3){
		//Agent's time interval for sending beacons
		interval = atoi(argv[2]);
	}
	if (argc >= 4){
		//Agen'ts lifetime in seconds
		LIFETIME = atoi(argv[3]);
	}
	if (argc >= 5){
		//Manger's IP address
		MANAGER_IP = argv[4];
	}
	if (argc >= 6){
		//Manager's UDP port number
		MANAGER_PORT = atoi(argv[5]);
	}



	time_t t;
	/* Intializes random number generator */
   	srand((unsigned) time(&t));

	struct BEACON beacon;
	beacon.ID = rand();
	beacon.startUpTime = t;
	beacon.timeInterval = interval;
	beacon.cmdPort = port;

	pthread_t thread1,thread2;
	pthread_create(&thread2,NULL, CmdAgent,NULL);
	sleep(1);
	pthread_create(&thread1, NULL, BeaconSender, &beacon); 
	pthread_join(thread2, NULL); 
	pthread_join(thread1, NULL);
	
	exit(0); 
}
