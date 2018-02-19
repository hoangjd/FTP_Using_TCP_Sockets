/*
** server.c -- a stream socket server demo
*/

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

#define BACKLOG 10	 // how many pending connections queue will hold



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// signal handler
void sig_func(int sig){
if (sig==SIGCHLD){
        printf("Signal handler hooked up with SIGCHLD\n");
}

if (sig==SIGINT){
	fflush(stdout);
        raise(SIGTERM);
}
}

int main(int argc, char **argv)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	FILE *file;
	char *filename;
	char buf[50];
	char buf2[50],buf3[50];
	int rv,count =0;
	int pid;
	struct sigaction new_action, old_action;
        new_action.sa_handler = sig_func;
        new_action.sa_flags = SA_NODEFER | SA_ONSTACK;
        sigaction(SIGCHLD, &new_action, &old_action);
        sigaction(SIGINT, &new_action, &old_action);

	if (argc != 2){
		fprintf(stderr,"Too few arguements");
		exit(1);
	}
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}


	printf("server: waiting for connections...\n");
		
	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			printf("\n");
			continue;
		}
		pid = fork();
		if (pid <0){
			fprintf(stderr,"fork failed");
			exit(1);
		}
		else if (pid > 0) {
			waitpid(-1,NULL,WNOHANG);
		}
		else {
		
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		//which file to open
                if(recv(new_fd, buf2, 50, 0)!=-1){
			printf("request for %s received from client\n",buf2+2);
		}
		
		//open file sent from client				
		file = fopen(buf2,"r" );
		
		//keep scaning and sending until end of file
		while(!feof(file)){
			fscanf(file,"%50s",buf);
			send(new_fd,buf,50,0);
		}
		//send terminating string
		send(new_fd,"cmsc257",8,0);
			printf("%s sent!\n",buf2+2);
		//close file
		fclose(file);
		close(new_fd);
		exit(0);
		}
	}
	return 0;
}

