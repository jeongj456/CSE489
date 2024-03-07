/**
 * @assignment1
 * @author  Team Members <ubitname@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */


 /*---------------***Sourced client and server files from the demo code***----------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../include/global.h"
#include "../include/logger.h"

#define TRUE 1
#define MSG_SIZE 256
#define BUFFER_SIZE 256

#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 256


int server(int argc, char **argv);
int client(int argc, char **argv)
int connect_to_host(char *server_ip, char *server_port);

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv){
	char server_client = argv[1];
	int port_number = argv[2];
	/*Init. Logger*/
	cse4589_init_log(port_number);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	if(strcmp(server_client, "c") == 0){
		return client(argc, argv)
	}else if(strcmp(server_client, "s") == 0){
		return server(argc, argv)
	}
	return 0;

	
}


/* <!---------------------------------------------------------------------------------------------> */
/* <!---------------------------------------------------------------------------------------------> */


int server(int argc, char **argv){
	if(argc != 2) {
		printf("Usage:%s [port]\n", argv[0]);
		exit(-1);
	}
	
	int server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
	struct sockaddr_in client_addr;
	struct addrinfo hints, *res;
	fd_set master_list, watch_list;

	/* Set up hints structure */
	memset(&hints, 0, sizeof(hints));
    	hints.ai_family = AF_INET;
    	hints.ai_socktype = SOCK_STREAM;
    	hints.ai_flags = AI_PASSIVE;

	/* Fill up address structures */
	if (getaddrinfo(NULL, argv[1], &hints, &res) != 0)
		perror("getaddrinfo failed");
	
	/* Socket */
	server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(server_socket < 0)
		perror("Cannot create socket");
	
	/* Bind */
	if(bind(server_socket, res->ai_addr, res->ai_addrlen) < 0 )
		perror("Bind failed");

	freeaddrinfo(res);
	
	/* Listen */
	if(listen(server_socket, BACKLOG) < 0)
		perror("Unable to listen on port");
	
	/* ---------------------------------------------------------------------------- */
	
	/* Zero select FD sets */
	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);
	
	/* Register the listening socket */
	FD_SET(server_socket, &master_list);
	/* Register STDIN */
	FD_SET(STDIN, &master_list);
	
	head_socket = server_socket;
	
	while(TRUE){
		memcpy(&watch_list, &master_list, sizeof(master_list));
		
		printf("\n[PA1-Server@CSE489/589]$ ");
		fflush(stdout);
		
		/* select() system call. This will BLOCK */
		selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
		if(selret < 0)
			perror("select failed.");
		
		/* Check if we have sockets/STDIN to process */
		if(selret > 0){
			/* Loop through socket descriptors to check which ones are ready */
			for(sock_index=0; sock_index<=head_socket; sock_index+=1){
				
				if(FD_ISSET(sock_index, &watch_list)){
					
					/* Check if new command on STDIN */
					if (sock_index == STDIN){
						char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);
						
						memset(cmd, '\0', CMD_SIZE);
						if(fgets(cmd, CMD_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to cmd
							exit(-1);
						
						printf("\nI got: %s\n", cmd);
						
						//Process PA1 commands here ...
						if (strcmp(cmd, "AUTHOR") == 0){ // Print out declaration of AI
							cse4589_print_and_log("[%s:SUCCESS]\n", cmd); //What are we supposed to check for to see if it passes or fails?
							cse4589_print_and_log("I, jjjeong-bkrishna, have read and understood the courst academic integrity policy.\n")
							cse4589_print_and_log("[%s:END]\n", cmd);
						}else if (strcmp(cmd, "IP") == 0){ //Print out the ip address
							cse4589_print_and_log("[%s:SUCCESS]\n", cmd)
							cse4589_print_and_log("IP:%s\n", ip_addr)
							cse4589_print_and_log("[%s:END]\n", cmd)
						}else if (strcmp(cmd, "PORT") == 0){ //Print out the port number
							cse4589_print_and_log("[%s:SUCCESS]\n", cmd)
							cse4589_print_and_log("PORT:%d\n", port)
							cse4589_print_and_log("[%s:END]\n", cmd)
						}else if (strcmp(cmd, "LIST") == 0){ /*Print out the list of all currently logged-in clients
						-->Should display hostname, IP address, and the listening port numbers)
							and should be sorted by their listening port numbers in increasing order*/
							cse4589_print_and_log("[%s:SUCCESS]\n", cmd)
							cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, hostname, ip_addr, port_num)
							cse4589_print_and_log("[%s:END]\n", cmd)
						}		// Add the other commands----------------------------------------------------------------------------------------
						else{
							cse4589_print_and_log("[%s:ERROR]\n", cmd)
							cse4589_print_and_log("[%s:END]\n", cmd)
						}
						
						free(cmd);
					}
					/* Check if new client is requesting connection */
					else if(sock_index == server_socket){
						caddr_len = sizeof(client_addr);
						fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
						if(fdaccept < 0)
							perror("Accept failed.");
						
						printf("\nRemote Host connected!\n");                        
						
						/* Add to watched socket list */
						FD_SET(fdaccept, &master_list);
						if(fdaccept > head_socket) head_socket = fdaccept;
					}
					/* Read from existing clients */
					else{
						/* Initialize buffer to receieve response */
						char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
						memset(buffer, '\0', BUFFER_SIZE);
						
						if(recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0){
							close(sock_index);
							printf("Remote Host terminated connection!\n");
							
							/* Remove from watched list */
							FD_CLR(sock_index, &master_list);
						}
						else {
							//Process incoming data from existing clients here -----------------------------------------------------------------------------------
							
							printf("\nClient sent me: %s\n", buffer);
							printf("ECHOing it back to the remote host ... ");
							if(send(fdaccept, buffer, strlen(buffer), 0) == strlen(buffer))
								printf("Done!\n");
							fflush(stdout);
						}
						
						free(buffer);
					}
				}
			}
		}
	}
	
	return 0;

}


/* <!---------------------------------------------------------------------------------------------> */
/* <!---------------------------------------------------------------------------------------------> */


int client(int argc, char **argv){
	if(argc != 3) {
		printf("Usage:%s [ip] [port]\n", argv[0]);
		exit(-1);
	}
	
	int server;
	server = connect_to_host(argv[1], argv[2]);
	
	while(TRUE){
		printf("\n[PA1-Client@CSE489/589]$ ");
		fflush(stdout);
		
		char *msg = (char*) malloc(sizeof(char)*MSG_SIZE);
		memset(msg, '\0', MSG_SIZE);
		if(fgets(msg, MSG_SIZE-1, stdin) == NULL) //Is this the command?
			exit(-1);
		
		printf("I got: %s(size:%d chars)", msg, strlen(msg));
		
		if (strcmp(msg, "AUTHOR") == 0){ // Print out declaration of AI
			cse4589_print_and_log("[%s:SUCCESS]\n", msg); //What are we supposed to check for to see if it passes or fails?
			cse4589_print_and_log("I, jjjeong-bkrishna, have read and understood the courst academic integrity policy.\n")
			cse4589_print_and_log("[%s:END]\n", msg);
		}else if (strcmp(msg, "IP") == 0){ //Print out the ip address
			cse4589_print_and_log("[%s:SUCCESS]\n", msg)
			cse4589_print_and_log("IP:%s\n", ip_addr)
			cse4589_print_and_log("[%s:END]\n", msg)
		}else if (strcmp(msg, "PORT") == 0){ //Print out the port number
			cse4589_print_and_log("[%s:SUCCESS]\n", msg)
			cse4589_print_and_log("PORT:%d\n", port)
			cse4589_print_and_log("[%s:END]\n", msg)
		}else if (strcmp(msg, "LIST") == 0){ 
			cse4589_print_and_log("[%s:SUCCESS]\n", msg)
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, hostname, ip_addr, port_num)
			cse4589_print_and_log("[%s:END]\n", msg)
		}else if (strstr(msg, "LOGIN")){
			LOGIN()//When a client logs in, print a list of clients that are currently on the server(should be in a list, look to see if there is a automatically resizing array in cstadard library)
			cse4589_print_and_log("[%s:SUCCESS]\n", "LOGIN")
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, hostname, ip_addr, port_num)
			cse4589_print_and_log("[%s:END]\n", "LOGIN")
		}else if (strcmp(msg, "REFRESH") == 0){
			cse4589_print_and_log("[%s:SUCCESS]\n", msg)
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, hostname, ip_addr, port_num)
			cse4589_print_and_log("[%s:END]\n", msg)
		}else if (strcmp(msg, "EXIT") == 0){
			cse4589_print_and_log("[%s:SUCCESS]\n", msg)
			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, hostname, ip_addr, port_num)
			cse4589_print_and_log("[%s:END]\n", msg)
			return 0
		}else{
			cse4589_print_and_log("[%s:ERROR]\n", msg)
			cse4589_print_and_log("[%s:END]\n", msg)
		}

		printf("\nSENDing it to the remote server ... ");
		if(send(server, msg, strlen(msg), 0) == strlen(msg))
			printf("Done!\n");
		fflush(stdout);
		
		/* Initialize buffer to receieve response */
		char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
		memset(buffer, '\0', BUFFER_SIZE);
		
		if(recv(server, buffer, BUFFER_SIZE, 0) >= 0){
			printf("Server responded: %s", buffer);
			fflush(stdout);
		}
	}
}



int connect_to_host(char *server_ip, char* server_port)
{
	int fdsocket;
	struct addrinfo hints, *res;

	/* Set up hints structure */	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	/* Fill up address structures */	
	if (getaddrinfo(server_ip, server_port, &hints, &res) != 0)
		perror("getaddrinfo failed");

	/* Socket */
	fdsocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(fdsocket < 0)
		perror("Failed to create socket");
	
	/* Connect */
	if(connect(fdsocket, res->ai_addr, res->ai_addrlen) < 0)
		perror("Connect failed");
	
	freeaddrinfo(res);

	return fdsocket;
}
