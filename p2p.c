/*  Sriram Shantharam
	sriramsh@buffalo.edu
	Fall 2014
	"wow. such code"
*/


#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define STDIN 0


// Define command types

#define HELP 0
#define MYIP 1
#define MYPORT 2
#define REGISTER 3
#define CONNECT 4
#define LIST 5
#define TERMINATE 6
#define EXIT 7
#define DOWNLOAD 8
#define CREATOR 9
#define UPLOAD 10
#define STATISTICS 11
#define INVALID 12
#define UPDATELIST 13 //update list
#define DEREGISTER 14

#define TERM "***"
#define TRUE 1
#define FALSE 0
#define DEBUG TRUE
#define HOST_NAME_MAX 100


struct networkentity{
	char token[6];
	int id;
	char hostname[HOST_NAME_MAX];
	char ip[INET6_ADDRSTRLEN];
	int port;

};
//globals
struct networkentity peerlist[5]={0};
char localIP[INET6_ADDRSTRLEN];
char hostname[HOST_NAME_MAX+1];
char port[5];
char * tokenptr,*regptr,*connectptr,*termptr,*dlptr,*recvptr;

//function declaration;
void listpeers();
void addToList(char *, char *, char*);
void zprintf(char *);
void sendlistbroadcast();
void deregisterClient();
int getIDByHostname();
void sendMessage(char *, int, void *);

void strToLower(char string[]) {
   
    int i;

    for (i = 0; string[i] != '\0'; i++)
        string[i] = (char)tolower(string[i]);

    
}

int getCommandType(char * token){
	strToLower(token);
	if(strcmp(token,"help")==0)
		return HELP;
	else if(strcmp(token,"myip")==0)
		return MYIP;
	else if(strcmp(token,"myport")==0)
		return MYPORT;
	else if(strcmp(token,"register")==0)
		return REGISTER;
	else if(strcmp(token,"connect")==0)
		return CONNECT;
	else if(strcmp(token,"list")==0)
		return LIST;
	else if(strcmp(token,"terminate")==0)
		return TERMINATE;
	else if(strcmp(token,"exit")==0)
		return EXIT;
	else if(strcmp(token,"quit")==0)
		return EXIT;
	else if(strcmp(token,"q")==0)
		return EXIT;
	else if(strcmp(token,"download")==0)
		return DOWNLOAD;
	else if(strcmp(token,"creator")==0)
		return CREATOR;
	else if(strcmp(token,"r")==0)
		return REGISTER;
	else if(strcmp(token,"ul")==0)
		return UPDATELIST;
	else if(strcmp(token,"d")==0)
		return DEREGISTER;
	else
		return INVALID;
}

void zprintf(char * msg){
	if(DEBUG){
		fprintf(stderr, "%s\n", msg);
	}
}

//Function to get the ip given hostname (** PLS IGNORE WEIRD NAMING CONVENTIONS)
void getHostIP(char * hostname){ //from Beej's Newtworking Guide
	//char dnsServer[] = "8.8.8.8"; //Google DNS server
	char * dnsServer = "www.google.co.uk"; //Google DNS server
	char dnsPort[] = "53"; //dns port
	int status;
	struct addrinfo hints;
	struct addrinfo *results,*p; //p :iterating over the results
	char ipstr[INET6_ADDRSTRLEN];
	if(hostname!=NULL){
		strncpy(dnsServer,hostname,sizeof hostname);
	}
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(dnsServer, dnsPort, &hints, &results)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(20);

	}
	
	for(p=results;p!=NULL;p=p->ai_next){
		void *addr;
		char *ipver;

		if(p->ai_family==AF_INET){
			struct sockaddr_in * ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}else{
			struct sockaddr_in6 * ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
	}
	freeaddrinfo(results);

}

void getMyIP(char * buf){
	char dnsServer[] = "8.8.8.8"; //or any valid ip. Get one using getHostIP()
	int status;
	int fd;
	struct sockaddr_in dnsaddr,myaddr;
	int myaddrlen;
	
	
	if((fd=socket(AF_INET,SOCK_DGRAM,0))<0){
		fprintf(stderr,"Error while creating socket %d",fd);
		exit(21);
	}

	dnsaddr.sin_family = AF_INET;
	dnsaddr.sin_addr.s_addr = inet_addr(dnsServer);
	dnsaddr.sin_port = htons(53);

	if((status=connect(fd, (struct sockaddr *)&dnsaddr, sizeof dnsaddr))<0){
		fprintf(stderr,"Error while connecting... %d",status);
		exit(22);

	}
	myaddrlen = sizeof(myaddr);
	if((status = getsockname(fd,(struct sockaddr *)&myaddr,&myaddrlen))){
		fprintf(stderr,"Error while getting local ip %d",status);
		exit(23);

	}

	strcpy(buf,inet_ntoa(myaddr.sin_addr));
	close(fd);
	


}

void registerClient(char * message){
	zprintf("In register client\n");
	char i_hostname[HOST_NAME_MAX],i_clientIP[INET6_ADDRSTRLEN],i_clientPort[5];
	strcpy(i_hostname,strtok_r(NULL,"|",&regptr));
	strcpy(i_clientIP,strtok_r(NULL,"|",&regptr));
	strcpy(i_clientPort,strtok_r(NULL,"|",&regptr));

	addToList(i_hostname,i_clientIP,i_clientPort);


}

void deregisterClient(){
	char message[1024];
	int hostid = getIDByHostname();
	snprintf(message, sizeof message, "D|%d|",hostid);
	sendMessage(peerlist[1].ip,peerlist[1].port,message);
}

int getIDByHostname(){
	int i;

	for(i=0;i<5;i++){
		if(strcmp(peerlist[i].hostname,hostname)==0)
			return peerlist[i].id;
	}
}

void addToList(char * i_hostname, char * i_clientIP, char * i_clientPort){
	zprintf("In addToList \n");
	int i = 0;
	
	while(peerlist[i].id!=0){
		if((strcmp(i_hostname,peerlist[i].hostname)+strcmp(i_clientIP,peerlist[i].ip)+(atoi(i_clientPort)-peerlist[i].port))==0){
			//fprintf(stderr,"Client already registered. \n");
			zprintf("Client already registered. \n");
			return;
		}

		i++;
		if(i==5){
			fprintf(stderr,"Maximum number of connected peers (5) reached. You can't add more \n");
			break;
		}
	}
	strcpy(peerlist[i].token,"LIST|");

	peerlist[i].id=i+1;
	strcpy(peerlist[i].hostname,i_hostname);
	strcpy(peerlist[i].ip,i_clientIP);
	peerlist[i].port=atoi(i_clientPort);
	//listpeers();
	sendlistbroadcast();
}

void sendMessage(char * i_ip, int i_port, void * i_message){ //messages less than 1024 bytes 
	int i;
	
	int lStatus;
	int lFD;
	struct sockaddr_in servaddr;
	int myaddrlen;
	char regMsg[512];
	char temp[10];
	
		if(DEBUG){
			fprintf(stderr,"Connecting to: %s\n",peerlist[i].hostname);
		}

		if((lFD=socket(AF_INET,SOCK_STREAM,0))<0){
			fprintf(stderr,"Error while creating socket %d\n",lFD);
			exit(21);
		}
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(i_ip);
		servaddr.sin_port = htons(i_port);

		if((lStatus=connect(lFD, (struct sockaddr *)&servaddr, sizeof servaddr))<0){
			fprintf(stderr,"Error while connecting... %d\n",lStatus);
			exit(22);

		}
		int bytesSent = send(lFD,i_message, sizeof i_message, 0);
		if(DEBUG)
			fprintf(stderr,"Bytes sent: %d\n",bytesSent);

		close(lFD);

	
}

void sendlistbroadcast(){
	int i;
	char serverIP[INET6_ADDRSTRLEN];
	char serverPort[10];
	int lStatus;
	int lFD;
	struct sockaddr_in servaddr;
	int myaddrlen;
	char regMsg[512];
	char temp[10];
	for (i =0;i<4 && peerlist[i].id!=0;i++){
		if(DEBUG){
			fprintf(stderr,"Connecting to: %s\n",peerlist[i].hostname);
		}

		if((lFD=socket(AF_INET,SOCK_STREAM,0))<0){
			fprintf(stderr,"Error while creating socket %d\n",lFD);
			exit(21);
		}
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(peerlist[i].ip);
		servaddr.sin_port = htons(peerlist[i].port);

		if((lStatus=connect(lFD, (struct sockaddr *)&servaddr, sizeof servaddr))<0){
			fprintf(stderr,"Error while connecting... %d\n",lStatus);
			exit(22);

		}
		int bytesSent = send(lFD,peerlist, sizeof peerlist, 0);
		if(DEBUG)
			fprintf(stderr,"Bytes sent: %d\n",bytesSent);

		close(lFD);

	}

}

void listpeers(){
	zprintf("In listpeers \n");
	int i = 0;
	while(peerlist[i].id!=0 && i <5){
		printf("%5d%35s%20s%8d\n", peerlist[i].id, peerlist[i].hostname, peerlist[i].ip, peerlist[i].port); 
		i++;
	}
}



void init(){
	zprintf("In init() \n");
	//get the IP
	getMyIP(localIP);
	//get the hostname
	if(gethostname(hostname, sizeof (hostname)-1)!=0){
		perror("Error while retrieving hostname");
		exit(23);
	}
	addToList(hostname,localIP,port);


}

int main(int argc, char * argv[]){
	char mode[1];
	char command[100]={0};
	char tokencommand[10]={0};
	
	
	fd_set master, readfds;
	int fdmax;
	int listener;
	int newfd;
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;
	int runmode; //0 for server, 1 for client

	char buf[256];
	int nbytes;

	char remoteIP[INET6_ADDRSTRLEN];

	int yes = 1;
	int i,j,rv;

	

	struct addrinfo hints, *ai, *p;
	printf("Initializing...\n");
	if(argc<3){
		fprintf(stderr,"Usage %s s/c port\n", argv[0] );
		exit(1) ;
	}

	strncpy(mode,argv[1],1);
	//printf("%s\n",mode);
	//printf("%d \n",strncmp(mode,"s",1));
	//printf("%d \n",strncmp(mode,"c",1));
	if(strncmp(mode,"s",1)!=0&&strncmp(mode,"c",1)!=0){
		fprintf(stderr,"Argument 1 needs to be either s or c, denoting either server or client\n");
		exit(2);
	}

	if(strncmp(mode,"s",1)==0){
		runmode=0;
		fprintf(stderr,"Running as server\n");
	}else{
		runmode=1;
		fprintf(stderr,"Running as client\n");
	}
	
	strcpy(port,argv[2]);

	


	FD_ZERO(&master);
	FD_ZERO(&readfds);

	memset(&hints,0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	


	if((rv=getaddrinfo(NULL,port,&hints,&ai)) != 0){
		fprintf(stderr,"selectserver: %s \n", gai_strerror(rv));
		exit(3);
	}
	
	/*
	for(p = ai; p != NULL; p = p->ai_next){  //From Beej's guide to network programming. Rev1: Useless. Doesn't work.
		struct sockaddr_in * tempaddr = (struct sockaddr_in *)p->ai_addr;
		fprintf(stderr,"Inet addr: %s\n",inet_ntoa(tempaddr->sin_addr));
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(listener < 0){
			fprintf(stderr,"Listener < 0 during socket op\n");
			continue;
		}

		setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));

		if(bind(listener,p->ai_addr,p->ai_addrlen)<0){
			close(listener);
			fprintf(stderr,"Error while binding\n");
			continue; 
		}
		break;
	}
	*/

	

	listener = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if(listener < 0){
		perror("Error while creating socket \n");
		exit(4);
	}
	if(setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))<0){
		perror("Error while setting socket for reuse\n");
		exit(5);
	}


	struct sockaddr_in listenAddr;
	listenAddr.sin_family= AF_INET;
	getMyIP(localIP);

	inet_aton(localIP,&listenAddr.sin_addr);
	listenAddr.sin_port=htons(atoi(port));
	/*
	if(bind(listener,(struct sockaddr *)&listenAddr,sizeof listenAddr)<0){
		close(listener);
		fprintf(stderr,"Error while binding\n");
		exit(44);
	}*/

	if(bind(listener,ai->ai_addr,ai->ai_addrlen)<0){
		close(listener);
		perror("Error while binding\n");
		exit(44);
	}


	




	////////////////////////////////////////////
/*

	if(listener == NULL){
		fprintf(stderr,"selectserver: failed to bind \n");
		exit(4);
	}
*/
	freeaddrinfo(ai);
	if (listen(listener, 10) == -1) {  //Lister to clients if running as a server!
		perror("listen");
		exit(5);
	}

	//DEBUG CODE
	if(DEBUG){
	int retval;
	socklen_t len = sizeof(retval);
	if (getsockopt(listener, SOL_SOCKET, SO_ACCEPTCONN, &retval, &len) == -1)
	    printf("fd %d is not a socket\n", listener);
	else if (retval)
	    printf("fd %d is a listening socket. Returned %d\n", listener,retval);
	else
	    printf("fd %d is a non-listening socket. Returned %d\n", listener,retval);

	}


	init(); //bug fix: Init() should be after the client/server has started listening to incoming requests. 


	if(DEBUG){
		fprintf(stderr, "struct size = %ld\n", sizeof peerlist);
		struct networkentity templist[5]={0};
		memcpy(templist,peerlist,sizeof peerlist);
		fprintf(stderr, "struct size = %ld\n", sizeof templist);
		int i = 0;
		char temptok[5];
		strcpy(temptok,strtok((char *)templist,"|"));
		fprintf(stderr, "%s\n", temptok);
		while(templist[i].id!=0 && i <5){
			fprintf(stderr, "printing %d \n",i);
			fprintf(stderr, "%5d%35s%20s%8d\n", templist[i].id, templist[i].hostname, templist[i].ip, templist[i].port); 
			i++;
		}
		
	}

	FD_SET(listener,&master);
	fdmax = listener;
	FD_SET(STDIN,&master);
	fprintf(stderr,">>> ");

	/* Format of network commands
		OP|Param1|Param2|Param3||
		*/

	int turnswitch = TRUE;
	for(;;){
		FD_ZERO(&readfds);
		readfds = master;
		if(select(fdmax+1,&readfds,NULL,NULL,NULL)==-1){
			perror("SELECT failed");
			exit(6);
		}
		fprintf(stderr,">>> ");
		
		for(i=0;i<=fdmax;i++){
			//fprintf(stderr, "checking fd %d\n",i);
			if(FD_ISSET(i,&readfds)){
				//printf("fd set for %d\n",i);
				if(i==STDIN){
					FD_CLR(0,&readfds);
					fgets(command, sizeof (command),stdin);
					int len = strlen(command) - 1;
					//fprintf(stderr,"Length=%d",len);
					if(len==0){
						continue;
					}
        			if (command[len] == '\n')
            			command[len] = '\0';

            		strToLower(command);
					//fprintf(stderr,"Input: %s\n",command);
					
					strcpy(tokencommand,strtok_r(command," ",&tokenptr));
					if(strlen(tokencommand)<1){
						FD_CLR(0,&readfds);
						continue;
					}
					if(tokencommand==NULL ||tokencommand=='\0'){
						//fprintf(stderr,"enterpressed\n\n");
						FD_CLR(0,&readfds);
						continue;
					}
					
					int commandtype = getCommandType(tokencommand);
					switch (commandtype){
						case HELP:
							fprintf(stderr,"HELP shows this message\n\n");
							fprintf(stderr,"MYIP Display the IP address of this process\n\n");
							fprintf(stderr,"MYPORT Display the port on which this process is listening for incoming connections\n\n");
							fprintf(stderr,"REGISTER <server IP> <port_no> This command is used by the client to register itself with the server and to get the IP and listening port numbers of all the peers currently registered with the server\n\n");
							fprintf(stderr,"CONNECT <destination> <port no>: This command establishes a new TCP connection to the specified <destination> at the specified < port no>\n\n");
							fprintf(stderr,"LIST Display a numbered list of all the connections this process is part of\n\n");
							fprintf(stderr,"TERMINATE <connection id> This command will terminate the connection listed under the specified number when LIST is used to display all connections\n\n");
							fprintf(stderr,"EXIT Close all connections and terminate this process\n\n");
							fprintf(stderr,"DOWNLOAD <file_name> <file_chunk_size_in_bytes>\n\n");
							fprintf(stderr,"CREATOR creator of this program\n\n");

							break;
						case MYIP:
							//getMyIP(localIP);
							fprintf(stderr,"IP address:%s \n",localIP);
							break;
						case MYPORT:
							fprintf(stderr,"Port number:%s\n",port);
							break;
						case REGISTER:
							if(runmode==0){
								fprintf(stderr, "Cannot 'Register' on a server\n" );break;}
							char serverIP[INET6_ADDRSTRLEN];
							char serverPort[10];
							strcpy(serverIP,strtok_r(NULL," ",&tokenptr));
							strcpy(serverPort,strtok_r(NULL," ",&tokenptr));
							fprintf(stderr,"Connecting to: %s:%s\n",serverIP,serverPort);
							//R|myip|port to serverIP:serverPort
							int lStatus;
							int lFD;
							struct sockaddr_in servaddr;
							int myaddrlen;
							if((lFD=socket(AF_INET,SOCK_STREAM,0))<0){
							fprintf(stderr,"Error while creating socket %d\n",lFD);
								exit(21);
							}

							servaddr.sin_family = AF_INET;
							servaddr.sin_addr.s_addr = inet_addr(serverIP);
							servaddr.sin_port = htons(atoi(serverPort));

							if((lStatus=connect(lFD, (struct sockaddr *)&servaddr, sizeof servaddr))<0){
								fprintf(stderr,"Error while connecting... %d\n",lStatus);
								exit(22);

							}
							//getMyIP(localIP);


							char regMsg[512];
							char temp[10];
							
							snprintf(regMsg, sizeof regMsg, "R|%s|%s|%s|%s|",hostname,localIP,port,TERM);
							fprintf(stderr,"%s\n",regMsg);

							int bytesSent = send(lFD,regMsg, strlen(regMsg), 0);
							fprintf(stderr,"Bytes sent: %d\n",bytesSent);

							break;
						case CREATOR:
							fprintf(stderr,"(c) 2014 Sriram Shantharam (sriramsh@buffalo.edu)\n\n");
							fprintf(stderr,"I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity\n\n");
							break;
						case EXIT:
							deregisterClient();
							exit(0);
						case LIST:
							listpeers();
							break;
						default:
							fprintf(stderr,"Invalid command. For a list of supported commands, type 'help'\n");


					}
				}
				if(i==listener){
					FD_CLR(listener,&readfds);
					fprintf(stderr,"Received something\n");
					struct sockaddr_in newclientmsg;
					int newclientmsgsize;
					int newfd = accept(listener,(struct sockaddr *)&newclientmsg, &newclientmsgsize);
					char newMessage[1024];
					int numbytes;
					if((numbytes = recv(newfd, newMessage, sizeof newMessage, 0))<=0){
						perror("ERROR ON RECV \n");
						exit (24);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
					}
					if(DEBUG)
					fprintf(stderr,"Received message=%s\n",newMessage);
					char recvtype[2]={0};
					strcpy(recvtype,strtok_r(newMessage,"|",&regptr));
					if(DEBUG)
					fprintf(stderr,"%s\n",recvtype);

					int recvcode = getCommandType(recvtype);
					if(DEBUG)
						fprintf(stderr,"recvcode = %d",recvcode);
					
					switch (recvcode){
						case REGISTER: 
							zprintf("client registering...\n");
							registerClient(newMessage);
							break;
						case LIST:
							zprintf("update list");
							memcpy(peerlist,newMessage,sizeof peerlist);
							listpeers();
							break;
						case DEREGISTER:
							zprintf("A client deregistered");
							break;






					}
				}
				fprintf(stderr,">>> ");
				//FD_CLR(0,&readfds);
			}
		}
		
		

	}
	return 0;
}