/*************************************************************************
*	This program is used to send file through socket in the following    *
* 	format:                                                              *
* 		./executable tcp recv <ip> <port>                                *
* 		./executable tcp send <ip> <filename>                     		 *
* 		./executable udp recv <ip> <port>                                *
* 		./executable udp send <ip> <filename>                    		 *                                                       *
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdbool.h>
#include <time.h>
#include <arpa/inet.h>

int progress[20];
bool check[20];
void print_time();
void print_log(unsigned long count, unsigned long len);

int main(int argc, char *argv[]){
	int sockfd, newsockfd, portno;
	socklen_t clilen, servlen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	clilen = sizeof(cli_addr);
	unsigned long count = 0, len;
	for(int i = 0; i < 20; ++i){
		check[i] = false;
		progress[i] = 5*i;
	}
	int n, b;
	if(argc < 5){
		fprintf(stderr, "not enough arguments\n");
		exit(1);
	}
	/********************tcp*********************/
	if(strcmp(argv[1], "tcp") == 0){
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0){
			fprintf(stderr, "ERROR opening socket\n");
			exit(1);
		}
		bzero((char *) &serv_addr, sizeof(serv_addr));
		/********************sender(server)********************/
		if(strcmp(argv[2], "send") == 0){
			portno = atoi(argv[3]);
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(portno);
			serv_addr.sin_addr.s_addr = INADDR_ANY;
			if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
				fprintf(stderr, "ERROR on binding\n");
				exit(1);
			}
			listen(sockfd, 5);
			newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
			if(newsockfd < 0){
				fprintf(stderr, "ERROR on accepting\n");
				exit(1);
			}
			/*receive request message*/
			n = read(newsockfd, buffer, 255);
			if(n < 0){
				fprintf(stderr, "ERROR reading from socket\n");
				exit(1);
			}
			FILE *read_ptr = fopen(argv[4], "r");
			if(!read_ptr){
				printf("error reading\n");
				exit(1);
			}
		    bzero(buffer,256);
			/*find file size*/
			fseek(read_ptr, 0, SEEK_END);
			len = (unsigned long)ftell(read_ptr);
			fseek(read_ptr, 0, SEEK_SET);
			/*transmit file size information*/
			sprintf(buffer, "%ld", len);
			write(newsockfd, buffer, 255);
			/*transmit*/
			while((b = fread(buffer, sizeof(buffer[0]), sizeof(buffer)-1, read_ptr)) > 0){
				n = write(newsockfd, buffer, b);
		    	if (n < 0){ 
		       		fprintf(stderr, "ERROR writing to socket\n");
					exit(1);
				}
		    	bzero(buffer,256);
				count += b;
				print_log(count, len);
			}
			/*close connection*/
		    close(sockfd);
			close(newsockfd);
			fclose(read_ptr);
		}	
		/********************receiver(client)**********************/
		else if(strcmp(argv[2], "recv") == 0){
			portno = atoi(argv[4]);
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(portno);
			struct hostent *server;
			server = gethostbyname(argv[3]);
			if(server == NULL){
				fprintf(stderr, "ERROR, no such host\n");
				exit(1);
			}
			bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    		if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        		fprintf(stderr, "ERROR connecting");
				exit(1);
			}
			/*send request message*/
			n = write(sockfd, "request", 8);
			if(n < 0){
				fprintf(stderr, "ERROR writing to socket\n");
				exit(1);
			}
			FILE *write_ptr;
			write_ptr = fopen("file", "w");
			if(!write_ptr){
				printf("Could not write file\n");
				exit(1);
			}
			/*read file size information*/
			read(sockfd, buffer, 255);
			len = atol(buffer);
			/*receive data and write to "file"*/
			while(count != len){
				bzero(buffer, 256);
				n = read(sockfd, buffer, 255);
				if(n < 0){ 
					fprintf(stderr, "ERROR reading from socket\n");
					exit(1);
				}
				count += n;
				print_log(count, len);
				fwrite(buffer, 1, n, write_ptr);
			}
			/*close connection*/
			close(sockfd);
			fclose(write_ptr);
		}
		else{
			fprintf(stderr, "bad argument 2\n");
			exit(1);
		}
	}
	/*********************udp*********************/
	else if(strcmp(argv[1], "udp") == 0){					
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(sockfd < 0){
			fprintf(stderr, "ERROR opening socket\n");
			exit(1);
		}
		bzero((char *) &serv_addr, sizeof(serv_addr));
		/*********************sender(server)********************/
		if(strcmp(argv[2], "send") == 0){
			portno = atoi(argv[3]);
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(portno);
			serv_addr.sin_addr.s_addr = INADDR_ANY;			
			if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
				fprintf(stderr, "ERROR on binding\n");
				exit(1);
			}
			/*receive request message*/
			recvfrom(sockfd, buffer, 255, 0, (struct sockaddr*) &cli_addr, &clilen);
			FILE *read_ptr = fopen(argv[4], "r");
			if(!read_ptr){
				printf("error reading\n");
				exit(1);
			}
			/*find file size*/
			fseek(read_ptr, 0, SEEK_END);
			len = (unsigned long)ftell(read_ptr);
			fseek(read_ptr, 0, SEEK_SET);
			/*transmit file size information*/
			sprintf(buffer, "%ld", len);
			sendto(sockfd, buffer, 255, MSG_CONFIRM, (struct sockaddr*) &cli_addr, clilen);
			/*transmit*/
			while((b = fread(buffer, sizeof(buffer[0]), sizeof(buffer)-1, read_ptr)) > 0){
				n = sendto(sockfd, buffer, b, MSG_CONFIRM, (struct sockaddr*) &cli_addr, clilen);
		    	if (n < 0){ 
		       		fprintf(stderr, "ERROR writing to socket");
					exit(1);
				}
		    	bzero(buffer,256);
				count += b;
				print_log(count, len);
			}
			sendto(sockfd, "end_connection", 16, MSG_CONFIRM, (struct sockaddr*) &cli_addr, clilen);
			/*close connection*/
			close(sockfd);
			fclose(read_ptr);
		}
		/*********************receiver(client)********************/
		else if(strcmp(argv[2], "recv") == 0){
			portno = atoi(argv[4]);
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(portno);
			struct hostent *server;
			server = gethostbyname(argv[3]);
			if(server == NULL){
				fprintf(stderr, "ERROR, no such host\n");
				exit(1);
			}
			bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
			FILE *write_ptr;
			write_ptr = fopen("file", "w");
			if(!write_ptr){
			printf("Could not write file\n");
				exit(1);
			}
			/*send request*/
			sendto(sockfd, "request", 8, MSG_CONFIRM, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
			/*read file size information*/
			recvfrom(sockfd, buffer, 255, 0, (struct sockaddr*) &serv_addr, &servlen);
			len = atol(buffer);
			/*receive data and write to "file"*/
			while(count != len){
				bzero(buffer, 256);
				n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr*) &serv_addr, &servlen);
				if(n < 0){ 
					fprintf(stderr, "ERROR reading from socket\n");
					exit(1);
				}
				if(strcmp(buffer, "end_connection") == 0){
					printf("Complete!\n");
					break;
				}
				count += n;
				print_log(count, len);
				fwrite(buffer, 1, n, write_ptr);
			}
			/*close connection*/
			close(sockfd);
			fclose(write_ptr);
		}
		else{
			fprintf(stderr, "bad argument 2\n");
			exit(1);
		}
	}
	else{
		fprintf(stderr, "bad argument 1\n");
		exit(1);
	}
	return 0;
}
/********************print current time********************/
void print_time(){
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	printf("%d/%d/%d %d:%02d:%02d\n", 1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}
/********************print log and call print_time()*/
void print_log(unsigned long count, unsigned long len){
	for(int i = 0; i < 20; ++i){
		if(check[i] == false && (float)count/len*100 >= progress[i]){
			check[i] = true;
			printf("%2d%% ", progress[i]);
			print_time();
		}
	}
	if(count == len)
		printf("Complete!\n");
}	

