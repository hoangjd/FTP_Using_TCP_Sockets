/*
** client.c -- a stream socket client demo
*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/times.h>

#define MAXDATASIZE 50 // max number of bytes we can get at once 

int noRepeats[25] = {0};
double start, stop, used;
double ftime(void);
// get sockaddr, IPv4 or IPv6:

// speed checker
double ftime (void)
{
    struct tms t;

    times ( &t );

    return (t.tms_utime + t.tms_stime) / 100.0;
}


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//signal handler
void sig_func(int sig){

if (sig==SIGINT){
	raise(SIGTERM);
}
}

//random file generator
void randm(char * buf2, int *noRepeats){
        char rfile[14] = "./file000.txt";
        char rnum[2];
	int r, check;
	r = rand() % 25 + 1;
	while (noRepeats[r]== 1){
		r = rand() % 25 + 1;	
	}
	noRepeats[r] = 1; 
        sprintf(rnum,"%d",r);
        if (r<10){
                rfile[8] = rnum[0];
        }
        else {
                rfile[7]= rnum[0];
                rfile[8] = rnum[1];
        }
        strcpy(buf2,rfile);
}

//Does the sending and recieving of information between client and server
void sendRecieve(int socket){
	char buf[50];
	char buf2[50];
	FILE *file;
	int numbytes;

       // printf("%s",buf);
	randm(buf2,noRepeats);
        printf("Client sent request for file: %s\n",buf2+2);
	send(socket,buf2,50,0);
	file = fopen(buf2,"w");
	while(strcmp(buf,"cmsc257")!= 0){
               	if ((numbytes = recv(socket, buf,50, 0)) == -1) {
                	perror("recv");
                        exit(1);
                }
                buf[numbytes] = '\0';

                if(strcmp(buf,"cmsc257")!=0){
	   	        fprintf(file,"%s",buf);
                }
        }
	fclose(file);
        printf("client: received %s\n",buf2+2);

}


int main(int argc, char **argv)
{
	int sockfd[25], numbytes; 
	int sleepVal, numFiles;
	char *port;  
	struct addrinfo hints, *servinfo, *p;
	int rv, i;
	int count = 0;
	char s[INET6_ADDRSTRLEN];
	struct sigaction new_action, old_action;
	pthread_t tid[25];
     	pthread_attr_t attr;

	new_action.sa_handler = sig_func;
	new_action.sa_flags = SA_NODEFER | SA_ONSTACK;
	sigaction(SIGINT, &new_action, &old_action);
	srand(time(0));	
	if (argc != 5) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	sleepVal = atoi(argv[4]);
	numFiles = atoi(argv[3]);
	port = argv[2];
	if ((rv = getaddrinfo(argv[1], port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	// loop through all the results and connect to the first we can
	while(count < numFiles) {
	
		for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd[count] = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
			if (connect(sockfd[count], p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd[count]);
			continue;
		}

		break;
		}
		count++;
	}

	count = 0;

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
	printf("client: connecting to %s\n", s);
	freeaddrinfo(servinfo); // all done with this structure

	pthread_attr_init(&attr);
	start = ftime();

	//spawn threads
	while(count < numFiles){	
		pthread_create(&tid[count],&attr,(void*)sendRecieve,(void *)sockfd[count]);	
		sleep(sleepVal);
		count++;
	}
	for(i=0; i<numFiles; i++){
		pthread_join(tid[i],NULL);
		close(sockfd[i]);
	}
	stop = ftime();
        used = stop - start;
        printf("Elapsed time: %10.2f\n", used);
	fflush(stdout);
	return 0;
}
