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
#include <sys/stat.h>
#include <time.h>




// Define command types
#define STDIN 0
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
#define HOSTNAME 15
#define NOOP 16
#define LISTON 17
#define IM 18
#define INCOMING 19
#define OUTGOING 20
#define DEBUGLEVEL 21

#define TERM "***"
#define TRUE 1
#define FALSE 0
#define DEBUG debug
#define HOST_NAME_MAX 128
#define MAX_LIST_ENTRIES 5
#define MAX_PEER_ENTRIES 3
#define PACKET_SIZE 512
#define VERBOSE FALSE


char * noop = "Z|***|";
int debug=TRUE;
struct networkentity{
	char token[6];
	int id;
	char hostname[HOST_NAME_MAX];
	char ip[INET6_ADDRSTRLEN];
	int port;

};

struct connectionparams{
	int id;
	char ip[INET6_ADDRSTRLEN];
	char hostname[HOST_NAME_MAX];
	int port;
	int fd;
	char filename[HOST_NAME_MAX]; //assume the file name is not longer than host name length/ same constraints
	int isactive;
};

struct dlfile{
	int cid;
	char filename[HOST_NAME_MAX];
};

struct statistic{
	int ul;
	int dl;
	long ubits;
	long dbits;
	double utime;
	double dtime;
};

struct hoststat{
	int isvalid;
	char hostname[HOST_NAME_MAX];
	int port;
	char hostname2[HOST_NAME_MAX];
	struct statistic stats;
};


//globals
struct networkentity peerlist[MAX_LIST_ENTRIES]={0};
//struct networkentity connectedpeers[MAX_PEER_ENTRIES+1]={0};
struct connectionparams connectedlist[MAX_PEER_ENTRIES+1]={0};
//struct connectionparams waitlist[MAX_PEER_ENTRIES]={0};

struct hoststat connstat[4]={0}; //STATS VARIABLE

fd_set master, readfds;
int runmode; //0 for server, 1 for client
int serverFd;
int fdmax;
char localIP[INET6_ADDRSTRLEN];
char hostname[HOST_NAME_MAX+1];
char port[5];
char * tokenptr,*regptr,*connectptr,*termptr,*dlptr,*recvptr;
int active;
//int activeConn[4]={-1};
int numPeers;
int yes = 1;

//function declarations;
void listpeers();
void addToList(char *, char *, char*);
void zprintf(char *);
void sendlistbroadcast();
void deregisterClient();
int getIDByHostname();
void sendMessage(char *, int, void *);
void removeClientFromList(int);
int isValidID(int);
int connectTo(char *, int);
void closeAllFds();
int addToPeerFdList(char * , int, char *, int);
int removeFromPeerFdList(int);
int isConnectedToIp(char * , int);
int isClientOnline(char *, int);
void logupload(char * , long , double);
void logdownload(char * , long , double);


//---------------------------------//
void strToLower(char string[]) {

	int i;

	for (i = 0; string[i] != '\0'; i++)
		string[i] = (char)tolower(string[i]);


}

char * reverse_r(char val, char *s, char *n) //from stackoverflow http://stackoverflow.com/a/9393226
{
    if (*n)
        s = reverse_r(*n, s, n+1);
   *s = val;
   return s+1;
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
	else if(strcmp(token,"hostname")==0)
		return HOSTNAME;
	else if(strcmp(token,"upload")==0)
		return UPLOAD;
	else if(strcmp(token,"statistics")==0)
		return STATISTICS;
	else if(strcmp(token,"stats")==0)
		return STATISTICS;
	else if(strcmp(token,"term")==0)
		return TERMINATE;
	else if(strcmp(token,"z")==0)
		return NOOP;
	else if(strcmp(token,"liston")==0)
		return LISTON;
	else if(strcmp(token,"im")==0)
		return IM;
	else if(strcmp(token, "fu")==0)
		return INCOMING;
	else if(strcmp(token, "fr")==0)
		return OUTGOING;
	else if(strcmp(token, "debug")==0)
		return DEBUGLEVEL;
	else
		return INVALID;
}

void zprintf(char * msg){
	if(DEBUG){
		fprintf(stderr, "%s\n", msg);
	}
}

void toggleDebugLevel(){
	if(DEBUG)
		debug = FALSE;
	else
		debug = TRUE;
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
/**
 *
 **/

void reinitializeList(){
	int i;
	for(i=0;i<MAX_LIST_ENTRIES;i++){
		peerlist[i].id=0;

		peerlist[i].port=0;

	}
}

void getIpFromHost(char * i_hostname, char * i_ipaddr){
	//from beej's guide

	struct hostent *he;
	struct in_addr **addr_list;
	int i;

	if ( (he = gethostbyname( i_hostname ) ) == NULL)
	{
		// get the host info
		perror("gethostbyname: Something went wrong. \n");
		return;
	}

	addr_list = (struct in_addr **) he->h_addr_list;

	for(i = 0; addr_list[i] != NULL; i++)
	{
		//Return the first one;
		strcpy(i_ipaddr , inet_ntoa(*addr_list[i]) );
		return;
	}

	return;


}

void getHostFromIp(char * i_ip, int i_port, char * i_hostname){

	int i;
	for(i=0;i<MAX_LIST_ENTRIES;i++){
		if((strcmp(peerlist[i].ip,i_ip)+i_port-peerlist[i].port)==0){
			strcpy(i_hostname,peerlist[i].hostname);
			return;
		}
	}
	return;
}

void populateConnectionInfo(char * i_ip, struct connectionparams i_cp){

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

int isValidID(int i_id){
	int i;
	for(i=0;i<MAX_LIST_ENTRIES;i++){
		if(peerlist[i].id==i_id)
			return TRUE;
	}
	return FALSE;
}

int isValidFd(int i_fd){
	int i;
	for(i=0;i<MAX_PEER_ENTRIES+1;i++){
		if(connectedlist[i].fd==i_fd)
			return TRUE;
	}
	return FALSE;
}

int isClientOnline(char * c_ip, int c_port){
	int i;
	zprintf("isclientonline: \n");
	for(i =0; i < MAX_LIST_ENTRIES && peerlist[i].id!=0; i++){
		if((strcmp(c_ip,peerlist[i].ip)+peerlist[i].port-c_port)==0)
			return TRUE;
	}
	return FALSE;
}



void registerClient(char * i_hostname, char * i_clientIP, char * i_clientPort){
	zprintf("In register client\n");
	

	addToList(i_hostname,i_clientIP,i_clientPort);


}

void deregisterClient(int connID){
	char message[1024];
	int hostid;



	if(connID==0)
		hostid = getIDByHostname();
	else
		hostid = connID;

	if(!isValidID(hostid)){
		fprintf(stderr,"Invalid connection ID or client might have already deregistered\n");
		return;
	}

	closeAllFds();

	snprintf(message, sizeof message, "D|%d|",hostid);
	zprintf(message);
	sendMessage(peerlist[0].ip,peerlist[0].port,message);
}

/*
void terminateClient(int connID){
	char message[1024];
	int hostid;



	if(connID==0)
		hostid = getIDByHostname();
	else
		hostid = connID;

	if(!isValidID(hostid)){
		fprintf(stderr,"Invalid connection ID or client might have already deregistered\n");
		return;
	}

	snprintf(message, sizeof message, "D|%d|",hostid);
	zprintf(message);
	sendMessage(peerlist[0].ip,peerlist[0].port,message);
}
 */

int addToPeerFdList(char * i_ip, int i_port, char * hostname, int i_fd){
	int i;
	if(numPeers==MAX_PEER_ENTRIES)
		return FALSE;

	if(isConnectedToIp(i_ip, i_port))
		return TRUE;

	for(i=0;i<MAX_PEER_ENTRIES;i++){
		if(connectedlist[i].fd==0){
			strcpy(connectedlist[i].ip,i_ip);
			connectedlist[i].id=i+1;
			connectedlist[i].fd=i_fd;
			connectedlist[i].isactive=FALSE;
			connectedlist[i].port = i_port;
			strcpy(connectedlist[i].hostname, hostname);
			numPeers++;
			return TRUE;
		}
	}
	return FALSE;
}

int removeFromPeerFdList(int fd){
	int i=0;

	/*
	
	


	for(i=0;i<MAX_PEER_ENTRIES;i++){
		if(connectedlist[i].fd==fd){
			connectedlist[i].fd=0;
			connectedlist[i].id=0;
			numPeers--;
			connectedlist[i].isactive=FALSE;
			return TRUE;
		}
	}
	return FALSE;
	 */
	if(!isValidFd(fd)){
		fprintf(stderr, "Invalid fd %d\n", fd);
		return FALSE;
	}

	if(numPeers==0)
		return TRUE;

	if(DEBUG){
		fprintf(stderr, "FD to remove %d\n", fd);
	}


	while(connectedlist[i].fd != fd) //index to be removed can also be just (id-1)
		i++;



	if(i==MAX_PEER_ENTRIES){
		connectedlist[MAX_PEER_ENTRIES].id = 0;
		connectedlist[MAX_PEER_ENTRIES].fd = 0;
		numPeers--;

		return TRUE;
	}

	int j = i+1;
	while(j<MAX_PEER_ENTRIES+1){
		if(connectedlist[j].id==0)
			break;
		connectedlist[j-1]= connectedlist[j];
		connectedlist[j-1].id=j;
		j++;

	}

	connectedlist[j-1].id = 0;
	numPeers--;
	return TRUE;


}




int isConnectedToIp(char * i_ip,  int i_port){
	int i;
	if(numPeers==0)
		return FALSE;
	for(i=0;i<MAX_PEER_ENTRIES;i++){
		if((strcmp(i_ip,connectedlist[i].ip)+(i_port-connectedlist[i].port))==0){
			return TRUE;
		}
	}
	return FALSE;
}

void closeAllFds(){
	int i;
	for(i=0;i<MAX_PEER_ENTRIES;i++){
		if(connectedlist[i].fd!=0){
			close(connectedlist[i].fd);
			connectedlist[i].fd=0;
			numPeers--;
		}
	}
}



int addToWaitList(char * i_ip, int i_fd){
	int i;
	if(numPeers==MAX_PEER_ENTRIES)
		return FALSE;
	for(i=0;i<MAX_PEER_ENTRIES;i++){
		if(connectedlist[i].fd==0){
			strcpy(connectedlist[i].ip,i_ip);
			connectedlist[i].fd=i_fd;

			return TRUE;
		}
	}
	return FALSE;
}


int removeFromWaitList(int fd){
	int i;
	if(numPeers==0)
		return TRUE;
	for(i=0;i<MAX_PEER_ENTRIES;i++){
		if(connectedlist[i].fd==fd){
			connectedlist[i].fd=0;

			return TRUE;
		}
	}
	return FALSE;
}

int isWaiting(int fd){
	int i;

	for(i=0;i<MAX_PEER_ENTRIES;i++){
		if(fd == connectedlist[i].fd){
			return TRUE;
		}
	}
	return FALSE;
}

int getFdFromId(int i_id){  //inherently different from getIDfromFD. Not it's inverse!!
	int i=0;
	for(i=0;i<MAX_PEER_ENTRIES+1;i++){
		if(connectedlist[i].id==i_id)
			return connectedlist[i].fd;
	}
	return -1;
}

int getIdFromFd(int i_fd){
	int i = 0;
	for(i=0; i< MAX_PEER_ENTRIES+1;i++){
		if(connectedlist[i].fd == i_fd){
			//return connectedlist[i].id;
			int j;
			for(j=0;j<MAX_LIST_ENTRIES;j++){
				if((strcmp(connectedlist[i].ip,peerlist[j].ip)+connectedlist[i].port- peerlist[j].port)==0){
					return peerlist[j].id;
				}
			}
		}
	}
	return -1;
}

int connectTo(char * c_ip, int c_port){
	int lStatus=-1;
	int lFD;
	struct sockaddr_in servaddr;
	int myaddrlen;
	char regMsg[512];
	char temp[10];
	char decodedIp[INET6_ADDRSTRLEN];
	int i;

	if(numPeers==MAX_PEER_ENTRIES){
		fprintf(stderr, "Maximum of connected peers reached! \n");
		return FALSE;
	}

	if((strcmp(c_ip,localIP)+c_port-atoi(port))==0){
		fprintf(stderr, "You cannot connect to yourself\n");
		return FALSE;
	}

	for(i=1;i<MAX_LIST_ENTRIES && peerlist[i].id!=0;i++){
		if(strcmp(peerlist[i].ip,c_ip)==0){
			lStatus = 0;
			servaddr.sin_addr.s_addr = inet_addr(c_ip);
			strcpy(decodedIp,c_ip);
			break;
		}

		if(strcmp(peerlist[i].hostname, c_ip)==0){
			lStatus = 0;
			servaddr.sin_addr.s_addr = inet_addr(peerlist[i].ip);
			strcpy(decodedIp,peerlist[i].ip);
			break;
		}
	}


	
	



	if(!isClientOnline(decodedIp,c_port)){
		fprintf(stderr, "Requested client is not online\n");
		return FALSE;
	}

	

	//redundant check

	if(lStatus==-1){
		fprintf(stderr, "Client with the entered IP address ( %s ) is not registered with the server\n", decodedIp);
		return FALSE;
	}


	if(DEBUG){
		fprintf(stderr,"Connecting to: %s\n",decodedIp);
	}

	if((lFD=socket(AF_INET,SOCK_STREAM,0))<0){
		fprintf(stderr,"Error while creating socket %d\n",lFD);
		exit(21);
	}
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(c_port);

	if((lStatus=connect(lFD, (struct sockaddr *)&servaddr, sizeof servaddr))<0){
		fprintf(stderr,"Error while connecting... %d\n",lStatus);
		exit(22);

	}






	// Everything went well. Add this to fdlist
	FD_SET(lFD, &master);
	if(lFD>fdmax){
		fdmax = lFD;
	}
	if(setsockopt(lFD,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))<0){
		perror("Error while setting socket for reuse\n");
		exit(5);
	}

	//FD_CLR(lFD, &readfds);

	char connectedhost[INET6_ADDRSTRLEN];
	getHostFromIp(decodedIp, c_port, connectedhost);

	int res = addToPeerFdList(decodedIp,c_port,connectedhost,lFD);

	if(res==FALSE){
		fprintf(stderr, "Max number of connected peers reached!\n");
		close(lFD);
		return FALSE;
	}

	char connMsg[512];
	snprintf(connMsg, sizeof regMsg, "Z|%s|%s|%s|%s|",hostname,localIP,port,TERM);

	// Needed if you need to do a no-op check by sending dummy messages

	int bytesSent = send(lFD,connMsg, sizeof connMsg, 0);
	if(DEBUG)
		fprintf(stderr,"Bytes sent: %d\n",bytesSent);


	return TRUE;
}

void listconnected(){
	zprintf("In listconnected \n");

	int i=0;

	while(connectedlist[i].id!=0 && i <MAX_PEER_ENTRIES+1){
		if(runmode==0 && i==0){
			i++;
			continue;
		}
		printf("%5d%35s%20s%8d\n", connectedlist[i].id, connectedlist[i].hostname, connectedlist[i].ip, connectedlist[i].port);
		i++;
	}
}


void removeClientFromList(int id){
	int i=0;


	if(!isValidID(id)){
		fprintf(stderr,"Invalid connection ID or client might have already deregistered\n");
		return;
	}

	if(DEBUG) fprintf(stderr, "ID to remove: %d\n", id);

	char * termmsg = "TERM|***|";


	while(peerlist[i].id != id) //index to be removed can also be just (id-1)
		i++;

	//sendMessage(peerlist[i].ip,peerlist[i].port,termmsg);

	if(i==MAX_LIST_ENTRIES-1){
		peerlist[MAX_LIST_ENTRIES-1].id = 0;
		sendlistbroadcast();
		return;
	}

	int j = i+1;
	while(j<MAX_LIST_ENTRIES){
		if(peerlist[j].id==0)
			break;
		peerlist[j-1]= peerlist[j];
		peerlist[j-1].id=j;
		j++;

	}

	peerlist[j-1].id = 0;
	sendlistbroadcast();

}





int getIDByHostname(){
	int i;

	for(i=0;i<MAX_LIST_ENTRIES && peerlist[i].id != 0;i++){
		if((strcmp(hostname,peerlist[i].hostname)+strcmp(localIP,peerlist[i].ip)+(atoi(port)-peerlist[i].port))==0)
			return peerlist[i].id;
	}
	return -1;
}

void addToList(char * i_hostname, char * i_clientIP, char * i_clientPort){
	zprintf("In addToList \n");
	int i = 0;

	while(peerlist[i].id!=0){
		if((strcmp(i_hostname,peerlist[i].hostname)+strcmp(i_clientIP,peerlist[i].ip)+(atoi(i_clientPort)-peerlist[i].port))==0){
			//fprintf(stderr,"Client already registered. \n");
			zprintf("Client already registered. \n");
			//sendlistbroadcast();
			return;
		}

		i++;
		if(i==MAX_LIST_ENTRIES){
			fprintf(stderr,"Maximum number of clients (%d) reached. You can't add more \n",MAX_LIST_ENTRIES);
			break;
		}
	}
	strcpy(peerlist[i].token,"LISTON|");

	peerlist[i].id=i+1;
	strcpy(peerlist[i].hostname,i_hostname);
	strcpy(peerlist[i].ip,i_clientIP);
	peerlist[i].port=atoi(i_clientPort);


	//listpeers();
	sendlistbroadcast();
}

void sendMessage(char * i_ip, int i_port, void * i_message){ //messages less than 1024 bytes

	int lStatus;
	int lFD;
	struct sockaddr_in servaddr;
	int myaddrlen;
	char regMsg[512];
	char temp[10];

	if(DEBUG){
		fprintf(stderr,"Connecting to: %s\n",i_ip);
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
			fprintf(stderr,"Connecting to: %s %d\n",peerlist[i].hostname,peerlist[i].port);
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

int sendToFd(int i_fd, char * buffer, int size){ //From Beej's guide

	
	//return send(i_fd, buffer, size, 0);


	int total = 0;        // how many bytes we've sent
    int bytesleft = size; // how many we have left to send
    int n;

    while(total < size) {
        n = send(i_fd, buffer+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    //*len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

void requestDownload(int c_id, char * c_filename){
	char dlmesg[HOST_NAME_MAX];
	snprintf(dlmesg, sizeof dlmesg, "FR|%s|", c_filename);
	sendToFd(getFdFromId(c_id), dlmesg, sizeof dlmesg);
}

void sendFileTo(int c_id, char * c_filename){
						int targetID = c_id;
						int upfd = getFdFromId(targetID);
						//int upfd = c_fd;
						char upfilename[HOST_NAME_MAX];
						strcpy(upfilename, c_filename);
						struct stat st;
						int retcode = stat(upfilename, &st);
						if (retcode!=0){
							fprintf(stderr, "Invalid file name/ File does not exist\n");
							return;
						}
						

						long upsize = (long)st.st_size;
						char upmsg[100];
						char upfilename_stripped[HOST_NAME_MAX];

						strcpy(upfilename_stripped, upfilename);

						reverse_r(*upfilename_stripped, upfilename_stripped, upfilename_stripped+1);
						if(DEBUG)
							fprintf(stderr, "Reversed file string: %s\n", upfilename);

						

						strcpy(upfilename_stripped, strtok_r(upfilename_stripped,"/", &dlptr));

						reverse_r(*upfilename_stripped, upfilename_stripped, upfilename_stripped+1);
						//reverse_r(*upfilename, upfilename, upfilename+1);
						snprintf(upmsg, sizeof upmsg, "FU|%s|%ld|%s|",upfilename_stripped,upsize,TERM);


						if(DEBUG){
							fprintf(stderr, "Upload command: id %d filename %s\n", targetID, upfilename);
							fprintf(stderr, "File size: %d\n", (int) st.st_size);
							fprintf(stderr, "Command to send %s\n", upmsg);
							fprintf(stderr, "Original name: %s Stripped filename: %s length: %d\n",upfilename, upfilename_stripped, (int)strlen(upfilename_stripped));

						}

						

						FILE * fileptr;

						fileptr = fopen(upfilename, "r");
						if(fileptr==NULL){
							fprintf(stderr, "Error while reading the file. Please make sure the path is correct\n");
							return;
						}

						int qtimes = (int)upsize/PACKET_SIZE;
						int rem = (int)upsize - qtimes * PACKET_SIZE;

						if(DEBUG){
							fprintf(stderr, "qtimes = %d rem = %d \n", qtimes, rem);
						}

						//file ops

						fprintf(stderr, "Upload in progress. Please wait...\n");
						//FD_CLR(STDIN, &master); //disable inputs until uploads complete


						//Send prepare message to receiver
						sendToFd(upfd, upmsg, sizeof upmsg);
						//sleep(1); //wait for acceptor to prepare

						int ii;
						struct timeval starttime, stoptime;

						gettimeofday(&starttime, NULL);
						double starttimems = (starttime.tv_sec) * 1000 + (starttime.tv_usec) / 1000 ;
						if(DEBUG){
							fprintf(stderr, "Starttime %lf\n", starttimems);
						}

						for(ii=0;ii < qtimes; ii++){
							char readbuffer[PACKET_SIZE]={0};
							int bytesread = fread(readbuffer, 1, PACKET_SIZE, fileptr);
							if(DEBUG){
								if (VERBOSE) fprintf(stderr, "Bytes read %d Buffer contents: %s \n", bytesread, readbuffer);
							}


							sendToFd(upfd, readbuffer, PACKET_SIZE);


						}

						if(rem!=0){
							char readbuffer[PACKET_SIZE]={0};
							int bytesread = fread(readbuffer, 1, PACKET_SIZE, fileptr);
							if(DEBUG){
								if (VERBOSE) fprintf(stderr, "Bytes read %d Buffer contents: %s \n", bytesread, readbuffer);
							}
							sendToFd(upfd, readbuffer, rem);
						}

						gettimeofday(&stoptime, NULL);

						double endtimems = (stoptime.tv_sec) * 1000 + (stoptime.tv_usec) / 1000 ;
						if(DEBUG){
							fprintf(stderr, "endtime  %lf\n", endtimems);
						}
						double uprate = (double)upsize * 8* 1000/(endtimems - starttimems); // bits/second
						fprintf(stderr, "Tx: %s -> %s, File size: %ld bytes, Time Taken %lf seconds, Tx Rate: %lf bits/second\n", \
											 hostname, peerlist[targetID].hostname, upsize, (double)(endtimems-starttimems)/1000, \
											 uprate );
						fflush(stdout);

						logupload(peerlist[targetID].hostname, upsize*8, (endtimems-starttimems));

						fclose(fileptr);

						fprintf(stderr, "File upload complete\n");


}

void listpeers(){
	zprintf("In listpeers \n");
	if(connectedlist[0].id==0 && connectedlist[0].fd==0){

		connectedlist[0].id = peerlist[0].id;
		strcpy(connectedlist[0].ip, peerlist[0].ip);
		strcpy(connectedlist[0].hostname,peerlist[0].hostname);
		connectedlist[0].port = peerlist[0].port;
		if(runmode==1){
			connectedlist[0].fd = serverFd;
		}
		if(runmode==0){
			connectedlist[0].fd = -1; // -1 in case of server. Server won't register with itself
		}

	}
	int i=0;
	

	while(peerlist[i].id!=0 && i <MAX_LIST_ENTRIES){

		printf("%5d%35s%20s%8d\n", peerlist[i].id, peerlist[i].hostname, peerlist[i].ip, peerlist[i].port);
		i++;
	}
}

// stat related fuctions 

//Returns if this entity's upload-download activity is logged and returns the index
int islogged(char * hostname){
	int i;
	int max_hosts;
	if(runmode==0)
		max_hosts=4;
	else
		max_hosts=3;

	for(i=0;i<max_hosts;i++){
		if(strcmp(connstat[i].hostname, hostname)==0)
			return i;
	}
	return -1;
}

int getfreelogindex(){
	int i;
	int max_hosts;
	if(runmode==0)
		max_hosts=4;
	else
		max_hosts=3;

	for(i=0;i<max_hosts;i++){
		if(connstat[i].isvalid==FALSE)
			return i;
	}
	return -1;
}



void logupload(char * i_hostname, long i_ubits, double i_utime){
	int p = islogged(i_hostname);
	if(p==-1){
		if(DEBUG)
			fprintf(stderr, "Given hostname %s is not logged\n", i_hostname);

		p = getfreelogindex();
		strcpy(connstat[p].hostname, i_hostname);
		if(p==-1){
			zprintf("Error: Log index returned -1.\n");
			fprintf(stderr, "Warning: something went wrong while logging\n");
			return;
		}
		
	}

	connstat[p].isvalid=TRUE;
	connstat[p].stats.ul++;
	connstat[p].stats.ubits+=i_ubits;
	connstat[p].stats.utime+=i_utime;

	if(DEBUG){
		int j=0;
		for(j=0;j<MAX_PEER_ENTRIES+1;j++){
			if(connstat[j].isvalid){
				fprintf(stderr, "Hostname %s, ups: %d downs: %d ubits: %ld, dbits: %ld, utime: %lf, dtime: %lf \n", \
								connstat[j].hostname, \
								connstat[j].stats.ul, connstat[j].stats.dl, \
								connstat[j].stats.ubits,connstat[j].stats.dbits,\
								connstat[j].stats.utime, connstat[j].stats.dtime);
			}
		}
	}

}

void logdownload(char * i_hostname, long i_dbits, double i_dtime){
	int p = islogged(i_hostname);
	if(p==-1){
		if(DEBUG)
			fprintf(stderr, "Given hostname %s is not logged\n", i_hostname);

		p = getfreelogindex();
		strcpy(connstat[p].hostname, i_hostname);
		if(p==-1){
			zprintf("Error: Log index returned zero.\n");
			fprintf(stderr, "Warning: something went wrong while logging\n");
			return;
		}
		
	}

	connstat[p].isvalid=TRUE;
	connstat[p].stats.dl++;
	connstat[p].stats.dbits+=i_dbits;
	connstat[p].stats.dtime+=i_dtime;

	if(DEBUG){
		int j=0;
		for(j=0;j<MAX_PEER_ENTRIES+1;j++){
			if(connstat[j].isvalid){
				fprintf(stderr, "Hostname %s, ups: %d downs: %d ubits: %ld, dbits: %ld, utime: %lf, dtime: %lf \n", \
								connstat[j].hostname, \
								connstat[j].stats.ul, connstat[j].stats.dl, \
								connstat[j].stats.ubits,connstat[j].stats.dbits,\
								connstat[j].stats.utime, connstat[j].stats.dtime);
			}
		}
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
	if(runmode==0){
		addToList(hostname,localIP,port);
	}
	active = 0;

}

int main(int argc, char * argv[]){

	char command[100]={0};
	char tokencommand[10]={0};


	//fd_set master, readfds;
	//int fdmax;
	int listener;
	int newfd;
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;

	char mode[1];
	char buf[256];
	int nbytes;

	char remoteIP[INET6_ADDRSTRLEN];

	
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

	/*
	if(DEBUG){
		fprintf(stderr, "struct size = %ld\n", sizeof peerlist);
		struct networkentity templist[MAX_LIST_ENTRIES]={0};
		memcpy(templist,peerlist,sizeof peerlist);
		fprintf(stderr, "struct size = %ld\n", sizeof templist);
		int i = 0;
		char temptok[5];
		strcpy(temptok,strtok((char *)templist,"|"));
		fprintf(stderr, "%s\n", temptok);
		while(templist[i].id!=0 && i <MAX_LIST_ENTRIES){
			fprintf(stderr, "printing %d \n",i);
			fprintf(stderr, "%5d%35s%20s%8d\n", templist[i].id, templist[i].hostname, templist[i].ip, templist[i].port);
			i++;
		}

	}
	 */

	FD_SET(listener,&master);
	fdmax = listener;
	FD_SET(STDIN,&master);
	//fprintf(stderr,">>> ");

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
		fprintf(stderr,"\n >>> \n");

		for(i=0;i<=fdmax;i++){
			//fprintf(stderr, "checking fd %d\n",i);
			if(FD_ISSET(i,&readfds)){
				if(DEBUG)
					printf("fd set for %d\n",i);
				if(i==STDIN){
					FD_CLR(0,&readfds);
					fgets(command, sizeof (command),stdin);
					int len = strlen(command) - 1;
					if(DEBUG)
						fprintf(stderr, "from stdin: %s\n", command);
					//fprintf(stderr,"Length=%d",len);
					if(len==0){
						continue;
					}
					if (command[len] == '\n')
						command[len] = '\0';

					//strToLower(command);

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

						if(active){
							fprintf(stderr, "Client is already registered \n");
							break;
						}
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
						if(bytesSent!=strlen(regMsg)){
							fprintf(stderr,"Erro while registering \n");
							exit (31);
						}
						active = 1;

						FD_SET(lFD, &master);
						if(lFD>fdmax){
							fdmax=lFD;
						}
						serverFd = lFD;
						break;
					case CREATOR:
						fprintf(stderr,"(c) 2014 Sriram Shantharam (sriramsh@buffalo.edu)\n\n");
						fprintf(stderr,"I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity\n\n");
						break;
					case EXIT:
						deregisterClient(0);
						exit(0);
					case LIST:
						listconnected();
						break;
					case TERMINATE:
						zprintf("Terminating a client");
						char termID[1];
						strcpy(termID,strtok_r(NULL," ",&tokenptr));
						fprintf(stderr, "term ID= %d\n", atoi(termID));
						int termfd = getFdFromId(atoi(termID));
						close(termfd);
						removeFromPeerFdList(termfd);
						//deregisterClient(atoi(termID));

						break;
					case HOSTNAME:
						zprintf("Hostname resolution");
						char i_hostname[HOST_NAME_MAX];
						char i_resolvedIP[INET6_ADDRSTRLEN];
						strcpy(i_hostname,strtok_r(NULL," ",&tokenptr));
						if(DEBUG)
							fprintf(stderr, "%s\n",i_hostname);
						getIpFromHost(i_hostname, i_resolvedIP);
						fprintf(stderr, "Resolved IP %s\n", i_resolvedIP);
						break;

					case CONNECT:
						zprintf("Connecting to peer");
						char c_ip[INET6_ADDRSTRLEN];
						int c_port;
						strcpy(c_ip,strtok_r(NULL," ",&tokenptr));
						c_port = atoi(strtok_r(NULL," ",&tokenptr));
						if(DEBUG)
							fprintf(stderr, "Connect command: ip: %s port: %d\n",c_ip,c_port );
						connectTo(c_ip,c_port);

						break;
					case UPLOAD:
						zprintf("Trying to upload\n");
						if(DEBUG){
							fprintf(stderr, "Message to process: %s\n", command);
						}
						if(runmode==0){
							fprintf(stderr, "Cannot upload from a server\n");
							break;
						}
						int targetID = atoi(strtok_r(NULL, " ", &tokenptr));
						if(targetID==1){
							fprintf(stderr, "Cannot upload to a server\n");
							break;
						}

						if(!isConnectedToIp(connectedlist[targetID-1].ip, connectedlist[targetID-1].port)){
							fprintf(stderr, "Invalid ID/ Not connected to specified IP\n");
						}
						if(isValidFd(getFdFromId(targetID))==-1){
							fprintf(stderr, "Entered ID does not represent a valid active connection\n");
						}
										
						char upfilename[HOST_NAME_MAX];
						strcpy(upfilename, strtok_r(NULL, " ", &tokenptr));

						sendFileTo(targetID, upfilename);

						//TAG start of function call
						
						//TAG end of potential function call


						break;
					case DOWNLOAD:
						// -x-
						zprintf("In Download\n");
						char * tempptr;
						struct dlfile filelist[3]={0};
						int numArgs=0;
						int k = 0;
						int jj;
						char temptoken[HOST_NAME_MAX];
						
						for(k = 0;k<3;k++){
							
							
							if((tempptr = strtok_r(NULL, " ", &tokenptr))==NULL)
								break;
							zprintf("Looping...");
							filelist[k].cid = atoi(tempptr);
							strcpy(filelist[k].filename,strtok_r(NULL, " ", &tokenptr));
							numArgs++;
						}
						
						
						if(numArgs==0){
							fprintf(stderr, "No arguments entered. \n");
							break;
						}

						if(DEBUG){
							fprintf(stderr, "no of args %d\n", numArgs);
						}
						
						zprintf("Done assigning arguments \n");
						if(DEBUG){
							
							for(jj=0;jj<numArgs;jj++){
								fprintf(stderr, "Conn ID: %d File name: %s\n", filelist[jj].cid, filelist[jj].filename);
							}
						}

						for(k=0;k<numArgs;k++){
							requestDownload(filelist[k].cid, filelist[k].filename);
						}


						break;
					case STATISTICS:
						break;
					case LISTON:
						listpeers();
						break;

					case DEBUGLEVEL:
						toggleDebugLevel();
						break;

					default:
						fprintf(stderr,"Invalid command. For a list of supported commands, type 'help'\n");


					}

				}
				if(i==listener){
					FD_CLR(listener,&readfds);
					zprintf("Received something\n");
					struct sockaddr_in newclientmsg;
					int newclientmsgsize;
					int newfd = accept(listener,(struct sockaddr *)&newclientmsg, &newclientmsgsize);

					if(DEBUG)
						fprintf(stderr, "New fd received %d\n", newfd);

					char newMessage[1024];
					int numbytes;
					if((numbytes = recv(newfd, newMessage, sizeof newMessage, 0))<=0){
						perror("ERROR ON RECV \n");
						exit (24);
					}
					if(DEBUG)
						fprintf(stderr,"Received message=%s\n",newMessage);
					char recvtype[10]={0};
					strcpy(recvtype,strtok_r(newMessage,"|",&regptr));
					if(DEBUG)
						fprintf(stderr,"%s\n",recvtype);

					int recvcode = getCommandType(recvtype);
					if(DEBUG)
						fprintf(stderr,"recvcode = %d",recvcode);

					switch (recvcode){
					case REGISTER:
						zprintf("client registering...\n");
						char i_hostname[HOST_NAME_MAX],i_clientIP[INET6_ADDRSTRLEN],i_clientPort[5];
						strcpy(i_hostname,strtok_r(NULL,"|",&regptr));
						strcpy(i_clientIP,strtok_r(NULL,"|",&regptr));
						strcpy(i_clientPort,strtok_r(NULL,"|",&regptr));
						if(runmode==0){
							addToPeerFdList(i_clientIP, atoi(i_clientPort), i_hostname, newfd);
						}
						registerClient(i_hostname, i_clientIP, i_clientPort);
						break;
					case LIST:
						zprintf("update list\n");
						memcpy(peerlist, newMessage, sizeof peerlist);
						listpeers();
						break;
					case DEREGISTER:
						zprintf("A client deregistered\n");
						int id = atoi(strtok_r(NULL,"|",&regptr));
						removeFromPeerFdList(getFdFromId(id));
						removeClientFromList(id);

						break;
					case TERMINATE:  //This option is not used 
						zprintf("This client was terminated!\n");
						active=FALSE;
						reinitializeList();
						fprintf(stderr, "Client disconnected from server\n");
						break;
					case NOOP:
						zprintf("No Op received \n");
						FD_SET(newfd,&master);
						if(newfd>fdmax){
							fdmax = newfd;
						}

						if(setsockopt(newfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))<0){
							perror("Error while setting socket for reuse\n");
							exit(5);
						}
						//struct sockaddr_in peeraddr;
						//int peeraddrlen = sizeof peeraddr;
						//getpeername(newfd, (struct sockaddr *)&peeraddr, &peeraddrlen);

						//inet_ntop(peeraddr.sin_family, &(peeraddr.sin_addr), peeripstr, sizeof peeripstr);
						char peeripstr[INET6_ADDRSTRLEN];
						int peerport;
						char peerhostname[HOST_NAME_MAX];
						strcpy(peerhostname, strtok_r(NULL,"|",&regptr));
						strcpy(peeripstr,strtok_r(NULL,"|",&regptr));
						peerport = atoi(strtok_r(NULL,"|",&regptr));
						addToPeerFdList(peeripstr,peerport, peerhostname, newfd);
						fprintf(stderr,"Host %s connected to this machine \n ", peerhostname);

						break;
					case LISTON:
						memcpy(peerlist,newMessage,sizeof peerlist);
						listpeers();
						break;

					case INCOMING:
						fprintf(stderr, "Client might not be connnected to the intended machine\n");

						break;



					default:
						fprintf(stderr,"Invalid command received command=%s\n",newMessage);






					}
				}else{  //It's one of the others	
						// step 1. Check if they are offline
					if(i==0){
						continue; //do not handle stdin here
					}
					if(isValidFd(i)){
						if(DEBUG){
							fprintf(stderr, "FD set: %d\n", i);

						}
						int size_r; //not really used except for debugging
						char recvMessage[1024];
					
						if((size_r = recv(i, recvMessage, sizeof recvMessage, 0))<=0){
							if(size_r==0){
								FD_CLR(i, &master);
								close(i);
								zprintf("Received 0 bytes. Need to remove the fd \n");
								removeFromPeerFdList(i);
								removeClientFromList(getIdFromFd(i));
								if(DEBUG){
									fprintf(stderr, "Received size %d \n", size_r);
									fprintf(stderr, "Cleared FD %d\n", i);
								}
							}else{
								if(DEBUG)
									perror("Recv error while receiving something on an existing connection:1537\n");
							}
							break;
						}

						
						if(VERBOSE)
							fprintf(stderr, "Received message through existing connection: %s\n", recvMessage);




						
						char recvtype[10];
						

						strcpy(recvtype, strtok_r(recvMessage,"|", &recvptr));
						

						int recvcode = getCommandType(recvtype);
						switch(recvcode){
							case INCOMING:
								zprintf("Incoming file \n");
								char infilename[HOST_NAME_MAX];
								long infilesize;
								strcpy(infilename, strtok_r(NULL, "|", &recvptr));
								infilesize = atoi(strtok_r(NULL, "|", &recvptr));
								FILE * fileptr;
								fileptr = fopen(infilename, "a");
								if(fileptr == NULL){
									fprintf(stderr, "Error creating file %s\n", infilename);
								}
								int qtimes = (int)infilesize/PACKET_SIZE;
								int rem = (int)infilesize - qtimes*PACKET_SIZE;


								if(DEBUG){
									fprintf(stderr, "qtimes = %d rem = %d\n", qtimes, rem);
								}

								struct timeval starttime, stoptime;

								gettimeofday(&starttime, NULL);
								double starttimems = (starttime.tv_sec) * 1000 + (starttime.tv_usec) / 1000 ;
								if(DEBUG){
									fprintf(stderr, "Starttime %lf\n", starttimems);
								}
								int ii;

								//handle network issues
								
								int itotal;
								int ibytesleft;
								int i_n;


								for(ii=0;ii < qtimes; ii++){
									itotal = 0;
									ibytesleft = PACKET_SIZE;
									
									char readbuffer[PACKET_SIZE]={0};
									while(itotal < PACKET_SIZE){
										i_n = recv(i, readbuffer, PACKET_SIZE, 0);
										if(i_n == -1){break;}
										itotal += i_n;
										ibytesleft -= i_n;
										if(DEBUG){
											if (VERBOSE) fprintf(stderr, "Bytes read %d Buffer contents: %s \n", i_n, readbuffer);
										}
										fwrite(readbuffer, 1, i_n, fileptr);
									}
									
									
									


								}

								ibytesleft = rem;
								itotal = 0;

								if(rem!=0){
									char readbuffer[PACKET_SIZE]={0};
									while(itotal < rem){
										i_n = recv(i, readbuffer, PACKET_SIZE, 0);
										if(i_n == -1){break;}
										itotal += i_n;
										ibytesleft -= i_n;
										if(DEBUG){
											if (VERBOSE) fprintf(stderr, "Bytes read %d Buffer contents: %s \n", i_n, readbuffer);
										}

										fwrite(readbuffer, 1, i_n, fileptr);
									}
									

									
								}

								gettimeofday(&stoptime, NULL);

								double endtimems = (stoptime.tv_sec) * 1000 + (stoptime.tv_usec) / 1000 ;
								if(DEBUG){
									fprintf(stderr, "endtime  %lf\n", endtimems);
								}
								double uprate = (double)infilesize * 8* 1000/(endtimems - starttimems); // bits/second
								fprintf(stderr, "Rx: %s -> %s, File size: %ld bytes, Time Taken %lf seconds, Rx Rate: %lf bits/second\n", \
													 peerlist[getIdFromFd(i)].hostname, hostname, infilesize, (double)(endtimems-starttimems)/1000, \
													 uprate );
								fflush(stdout);

								logdownload(peerlist[getIdFromFd(i)].hostname, infilesize*8, (endtimems - starttimems));
								fclose(fileptr);

								fprintf(stderr, "A new file was downloaded to the local machine\n");
								break;

							case OUTGOING:
								zprintf("File requested\n");
								char reqfilename[HOST_NAME_MAX], * tempptr;
								tempptr = strtok_r(NULL, "|", &recvptr);
								if(tempptr == NULL){
									fprintf(stderr, "Invalid arguments\n");

								}
								strcpy(reqfilename, tempptr);

								int m;
								for(m=0;m<MAX_PEER_ENTRIES+1;m++){
									if(connectedlist[m].fd==i){
										if(DEBUG)
											fprintf(stderr, "Value of m : %d\n", m);
										sendFileTo(m+1, reqfilename);
										break;
									}
								}




							break;

							default:
								break;


						}



					}


				}
				//fprintf(stderr,">>> ");
				//FD_CLR(0,&readfds);
			}
		}



	}
	return 0;
}
