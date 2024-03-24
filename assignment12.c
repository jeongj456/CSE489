

#include <stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <netdb.h>
#include "../include/global.h"
#include "../include/logger.h"

#define MSG_LENGTH 1024
#define STDIN 0
#define CMD_SIZE 100
#define BUFFER_SIZE 256
#define EPHEMERAL_PORT 53
#define AUTHOR "AUTHOR\n"
#define IP "IP\n"
#define PORT "PORT\n"
#define LIST "LIST\n"
#define LOGIN "LOGIN\n"


struct client{
    int list_id;
    char *ip;
    int client_fd;
    char *hostname;
    int block_status;
    int client_status;
    int port_no;
    int num_msg_recv;
    int num_msg_sent;
    struct client *next;  
};


int port_no[5];
char client_ip[5][100];
int c =1;
int server(struct client **head_ref, int port_no, int listening_fd);
int client(struct client **c_ref , int port_no, int listening_fd);
void push(struct client** head_ref, int port_no, char* ip);
void print(struct client *headref);
void print_author();
void print_portno(int port_no);
void send_to_client(int sock_index, char *send_to_ip, char *buffer, struct client *temp);
char* find_ip(char *str);
//void find_ip();
void send_port(int listening_port, int server_fd);
void assign_port(char * buffer, struct client *temp);
void tostring(char str[], int num);
void send_client_list(struct client* headref,char client_list[]);
void create_client_list(struct client **c_ref,char *buffer);
void broadcast(struct client *c_ref, char *msg, int srver_fd);
void add_new_client(struct client **head_ref,int fdaccept, struct sockaddr_in client_addr);
int isValidIP(char *ip);
int valid_digit(char *ip_str);
char from_ip[25];




int main(int argc, char *argv[]) {
    
    /*Init. Logger*/
    cse4589_init_log(argv[2]);
    
    /* Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));
    
    int listening_fd=0,temp_result;
    struct client *head_ref = NULL;
    struct client *c_ref = NULL;
    struct sockaddr_in listen_addr;
    
    if(argc!=3){
        exit(-1);
    }

    listening_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listening_fd == 0){
        exit(EXIT_FAILURE);
    }

    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(atoi(argv[2]));
    
    // Binding the socket to a port
    temp_result = bind(listening_fd, (struct sockaddr *)&listen_addr,sizeof(listen_addr));
    if(temp_result <0){
        exit(EXIT_FAILURE);
    }

    temp_result =listen(listening_fd, 4);
    if(temp_result< 0){
        exit(EXIT_FAILURE);
    }
    
    if(*argv[1] == 's')
        server(&head_ref,atoi(argv[2]), listening_fd);
    else
        client(&c_ref, atoi(argv[2]), listening_fd);
    return 0;
    
    
}

int server(struct client **head_ref, int port_no, int listening_fd){
    int server_head,selret, caddr_len=0, fdaccept=0;
    struct sockaddr_in client_addr;
    fd_set master_list, watch_list;
    
    /* Zero select FD sets */
    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);
    /* Register the listening socket */
    FD_SET(listening_fd, &master_list);
    /* Register STDIN */
    FD_SET(STDIN, &master_list);
    
    server_head = listening_fd;
    
    while(1){
        memcpy(&watch_list, &master_list, sizeof(master_list));
        selret = select(server_head + 1, &watch_list, NULL, NULL, NULL);
        if(selret < 0)
            perror("select failed.");
        if(selret > 0){
            for(int sock_index=0; sock_index<=server_head; sock_index+=1){
                if(FD_ISSET(sock_index, &watch_list)){
                    /* Check if new command on STDIN */
                    if (sock_index == STDIN){
                        char *cmd = (char*) malloc(sizeof(char)*CMD_SIZE);
                        memset(cmd, '\0', CMD_SIZE);
                        if(fgets(cmd, CMD_SIZE-1, stdin) == NULL) //Mind the newline character that will be written to cmd
                            exit(-1);
                        //Process PA1 commands here ...
                        //----------------------------------------------------------------------------------------------------
                        if(strcmp(cmd, "AUTHOR\n")==0)
                            print_author();
                        else if(strcmp(cmd, IP)==0){
                            char ip_str[INET_ADDRSTRLEN];
                            strcpy(ip_str,find_ip(ip_str));
                        }
                        else if(strcmp(cmd, PORT)==0){
                            print_portno(port_no);
                        }
                        else if(strcmp(cmd, LIST)==0){
                            print(*head_ref);
                        }
                    }
                    /* Check if new client is requesting connection */
                    else if(sock_index == listening_fd){
                        caddr_len = sizeof(client_addr);
                        fdaccept = accept(listening_fd, (struct sockaddr *)&client_addr, (socklen_t*)&caddr_len);
                        if(fdaccept < 0)
                            perror("Accept failed.");
                        else{
                            FD_SET(fdaccept, &master_list);
                            if(fdaccept > server_head)server_head = fdaccept;
                            add_new_client(head_ref, fdaccept, client_addr);
                        }
                        
                    }else{   /* Read from existing clients */ 
                        if(sock_index == 0)
                            break;
                        char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                        memset(buffer, '\0', BUFFER_SIZE);
                        int recd_bytes;
                        if(recv(recd_bytes = sock_index, buffer, BUFFER_SIZE, 0) <= 0){
                            close(sock_index);
                            FD_CLR(sock_index, &master_list);
                        }else{
                            struct client *temp = *head_ref;
                            char *send_to_ip = (char*) malloc(sizeof(char)*INET_ADDRSTRLEN);
                            send_to_ip = strtok(buffer, " ");
                            
                            if(strcmp(send_to_ip, "Port") == 0){
                                int success =0;
                                assign_port(buffer, temp);
                                char client_list_buf[500];
                                send_client_list(*head_ref, client_list_buf);
                                if(send(fdaccept, client_list_buf, strlen(client_list_buf), 0) == strlen(client_list_buf))
                                    success = 1;
                            }else{
                                char * msg = (char*) malloc(sizeof(char)*MSG_LENGTH);
                                while(buffer != NULL){
                                    buffer= strtok(NULL, " ");
                                    if(buffer!=NULL)
                                        strcpy(msg,buffer);
                                }
                                struct client * temp = *head_ref;
                                temp = *head_ref;
                                send_to_client(sock_index, send_to_ip, msg ,temp);
                            }
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

void add_new_client(struct client **head_ref,int fdaccept, struct sockaddr_in client_addr){
    char str[INET_ADDRSTRLEN];
    char str1[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), str, INET_ADDRSTRLEN);
    struct hostent *hostname = NULL;
    struct in_addr ipv4addr;
    inet_ntop(AF_INET, &(client_addr.sin_addr), str1, INET_ADDRSTRLEN);
    inet_pton(AF_INET, str1, &ipv4addr);
    hostname = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    int hlen = (int)strlen(hostname->h_name);
    
    
    
    struct client* new_node = (struct client*) malloc(sizeof(client));
    new_node->ip = (char*)malloc(sizeof(MSG_LENGTH));
    strcpy(new_node->ip,str);
    strcpy(client_ip[c],str);
    new_node->list_id = c;
    c++;
    new_node->client_fd = fdaccept;
    new_node->hostname = (char*)malloc(sizeof(hlen));
    strcpy(new_node->hostname, hostname->h_name);
    new_node->client_status =1;
    
    if(*head_ref==NULL){
        *head_ref = new_node;
        new_node->next = NULL;
    }else{
        // printf("oye here!")
        new_node->next = *head_ref;
        *head_ref = new_node;
    }
}

void print_author(){
    cse4589_print_and_log("[%s:SUCCESS]\n","AUTHOR");
    cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "jjjeong-bkrishna");
    cse4589_print_and_log("[%s:END]\n","AUTHOR");
}

void print_portno(int port_no){
    cse4589_print_and_log("[%s:SUCCESS]\n","PORT");
    cse4589_print_and_log("PORT:%d\n",port_no);
    cse4589_print_and_log("[%s:END]\n","PORT");
}

void swap(struct client *a, struct client *b){
    int temp = a->port_no;
    a->port_no = b->port_no;
    b->port_no = temp;
}


void bubbleSort(struct client *start){
    int swapped;
    struct client *ptr1 = start;
    struct client *lptr = NULL;
    if (ptr1 == NULL)
        return;
    
    do{
        swapped = 0;
        ptr1 = start;
        while (ptr1->next != lptr){
            if (ptr1->port_no > ptr1->next->port_no){
                swap(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);
}



void swap_int(char *num1, char *num2) {
    int temp;
    temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}


void print(struct client *headref){
    cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "LIST");
    struct client *temp = headref;
    int list_id = 1;
    bubbleSort(headref);
    while(temp!=NULL){
        if(temp->client_status == 1)
        {   cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id,temp->hostname, client_ip[temp->list_id], temp->port_no);}
        temp = temp->next;
        list_id = list_id +1;
    }
    cse4589_print_and_log((char *)"[%s:END]\n", "LIST");
}


void send_to_client(int sock_index, char *send_to_ip, char *buffer, struct client *temp){
    char *str = (char*)malloc(sizeof(MSG_LENGTH));
    char *str1 = (char*)malloc(sizeof(MSG_LENGTH));
    char *str2 = (char*)malloc(sizeof(MSG_LENGTH));
    char *from_ip = (char*)malloc(sizeof(MSG_LENGTH));
    int j;int success = 1;
    struct client *temp1 = temp;
    for(j =1; j<=5; j++){
        if(strcmp(send_to_ip,client_ip[j])==0){
            //printf("\n %d %s", j, client_ip[j]);
            break;
        }
    }

    while(temp1!= NULL){
        if(temp1->client_fd == sock_index)
            strcpy(from_ip,client_ip[temp1->list_id]);
        temp1 = temp1->next;
    }
    strcat(str,from_ip);
    strcat(str, " ");
    strcat(str, buffer);
    while(temp->list_id != j)
        temp = temp->next;
	
    if(send(temp->client_fd,str,strlen(str),0) == -1)
        success = 0;//perror("send");
    else{
        success = 1;
    }
    
    if(success == 1){
        cse4589_print_and_log("[RELAYED:SUCCESS]\n");
        cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",from_ip,send_to_ip,buffer);
        cse4589_print_and_log("[RELAYED:END]\n");
    }else{
        cse4589_print_and_log("[RELAYED:ERROR]\n");
        cse4589_print_and_log("[RELAYED:END]\n");
    }
}


void assign_port(char *buffer, struct client *temp){
    char *port,*clientip;int count =0;
    while(count != 2){
        char *a = (char*) malloc(sizeof(char)*MSG_LENGTH);
        strcpy(a,buffer);
        if(count == 0){
            buffer = strtok(NULL, " ");
            strcpy(a,buffer);
            clientip = a;
            //printf(" %s ", clientip);
        }else if(count == 1){
            buffer = strtok(NULL, "\n");
            strcpy(a,buffer);
            port = a;
            break;
        }
        count +=1;
    }
    int j;
    for(j =1; j<=5; j++){
        if(strcmp(clientip,client_ip[j])==0){
            break;
        }
    }
    
    if(j==6);
    else{
        while(temp->list_id != j)
            temp = temp->next;
        if(temp == NULL);
        else{
            temp->port_no = atoi(port);
        }
    } 
}


void send_port(int listening_port, int server_fd){
    char send_port[100];
    char str_cip[INET_ADDRSTRLEN];
    struct sockaddr_in udp;
    int temp_udp =socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int len = sizeof(udp);
    int result;

    if (temp_udp == -1){
        //printf("Socket creation failed!");
    }
    
    memset((char *) &udp, 0, sizeof(udp));
    udp.sin_family = AF_INET;
    udp.sin_port = htons(EPHEMERAL_PORT);
    inet_pton(AF_INET, "8.8.8.8", &udp.sin_addr);
    
    if (connect(temp_udp, (struct sockaddr *)&udp, sizeof(udp)) < 0){
        result = 0;
    }
    if (getsockname(temp_udp,(struct sockaddr *)&udp,(unsigned int*) &len) == -1){
        perror("getsockname");
        result = 0;
    }
    
    inet_ntop(AF_INET, &(udp.sin_addr), str_cip, len);
    

    char port[INET_ADDRSTRLEN];
    tostring(port, listening_port);
    strcat(send_port, "Port");
    strcat(send_port, " ");
    strcat(send_port, str_cip);
    strcat(send_port, " ");
    strcat(send_port, port);
    strcat(send_port, "\n");
    
    if( send(server_fd, send_port, strlen(send_port),0) == -1 )
        perror("Send");
}


char* find_ip(char *str){
    struct sockaddr_in udp;
    int temp_udp =socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int len = sizeof(udp);
    int result;
    
    if (temp_udp == -1){
        //printf("Socket creation failed!");
    }
    
    memset((char *) &udp, 0, sizeof(udp));
    udp.sin_family = AF_INET;
    udp.sin_port = htons(EPHEMERAL_PORT);
    inet_pton(AF_INET, "8.8.8.8", &udp.sin_addr);
    
    if (connect(temp_udp, (struct sockaddr *)&udp, sizeof(udp)) < 0){
        //printf("\nConnection Failed \n");
        result = 0;
    }
    if (getsockname(temp_udp,(struct sockaddr *)&udp,(unsigned int*) &len) == -1){
        perror("getsockname");
        result = 0;
    }
    
    inet_ntop(AF_INET, &(udp.sin_addr), str, len);
    
    //Success
    if( result!=0){
        cse4589_print_and_log("[%s:SUCCESS]\n","IP");
        cse4589_print_and_log("IP:%s\n", str);
        cse4589_print_and_log("[%s:END]\n","IP");
    }else{
        //Error
        cse4589_print_and_log("[%s:ERROR]\n", "IP");
        cse4589_print_and_log("[%s:END]\n", "IP");
    }
    return str;
}


int isValidIP(char *ip){
    char *ptr = ip;
    strcat(ptr,"\0");
    int dots=0;
    
    if(ip == NULL)
        return 0;
    
    ptr = strtok(ptr, ".");
    while(ptr != NULL ){
        if (!valid_digit(ptr))
            return 0;
        
        if(atoi(ptr)>=0 && atoi(ptr)<=255){
            ptr = strtok(NULL, ".");
            if (ptr != NULL)
                ++dots;
        }else
            return 0;
    }
    if (dots != 3)
        return 0;
    return 1;
}


int valid_digit(char *ip_str){
    while (*ip_str) {
        if (*ip_str >= '0' && *ip_str <= '9')
            ++ip_str;
        else
            return 0;
    }
    return 1;
}


void tostring(char str[], int num){
    int i, rem, len = 0, n;
    
    n = num;
    while (n != 0){
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++){
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\0';
}

void send_client_list(struct client* headref, char client_list[]){
    char buf[500] = "";
    struct client* temp = headref;
    char list_id[10]= "";
    char str[20]= "";
    strcat(buf, "List");
    while(temp != NULL){
        if(temp->client_status == 1){
            strcat(buf, " ");
            tostring(list_id, temp->list_id);
            strcat(buf, list_id);
            strcat(buf, " ");
            tostring(str,temp->port_no);
            strcat(buf,str);
            strcat(buf, " ");
            strcat(buf, temp->hostname);
            strcat(buf, "\n");
            strcat(buf,client_ip[temp->list_id]);
            // strcat(buf, temp->block_status);
            strcat(buf, "\n");
        }
        temp = temp->next;
    }
    strcpy(client_list,buf);
}


void create_client_list(struct client **c_ref, char *buffer){
    
    char *string[256];
    int i =0;
    string[i]=strtok(buffer,"\n");
    while(string[i]!=NULL){
        i++;
        string[i]=strtok(NULL,"\n");
    }
    
    for (int j=0;j<i;j++){
        char *val = strtok(string[j], " ");
        int counter = 0;
        struct client *new_node = (struct client*) malloc(sizeof(client));
        while (val != NULL){
            if(j == 0){
                if(counter == 0);
                else if(counter == 1)
                    new_node->list_id = atoi(val);
                else if(counter ==2){  
                    //printf("\n %s",val);
                    new_node->port_no = atoi(val);
                    //printf("\n port no %d	",new_node->port_no);
                }else if(counter == 3){
                    new_node->hostname = (char*)malloc(sizeof(MSG_LENGTH));
                    strcpy(new_node->hostname,val);
                    //printf("\nhostname %s", new_node->hostname);
                }else if(counter == 4){
                    new_node->ip = (char*)malloc(sizeof(MSG_LENGTH));
                    strcpy(new_node->ip, val);
                    strcpy(client_ip[new_node->list_id],val);
                   // printf("%s Client_ip",client_ip[new_node->list_id]);
                }
            }else{
                if(counter == 0)
                    new_node->list_id = atoi(val);
                else if(counter ==1){ new_node->port_no = atoi(val);
                   // printf("\n Port no %d", new_node->port_no);
                }else if(counter == 2){
                    new_node->hostname = (char*)malloc(sizeof(MSG_LENGTH));
                    strcpy(new_node->hostname,val);
                }else if(counter == 3){
                    new_node->ip = (char*)malloc(sizeof(MSG_LENGTH));
                    strcpy(new_node->ip, val);
                    strcpy(client_ip[new_node->list_id],val);
                }
            }
            val = strtok(NULL, " ");
            counter +=1;
        }
        if(*c_ref==NULL){
            *c_ref = new_node;
            new_node->next = NULL;
        }else{
            new_node->next = *c_ref;
            *c_ref = new_node;
        }
    }
}


void broadcast(struct client *c_ref, char *msg, int server_fd){
    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in udp;
    int temp_udp =socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int len = sizeof(udp);
    int result;
    
    if (temp_udp == -1){
        //printf("Socket creation failed!");
    }
    
    memset((char *) &udp, 0, sizeof(udp));
    udp.sin_family = AF_INET;
    udp.sin_port = htons(EPHEMERAL_PORT);
    inet_pton(AF_INET, "8.8.8.8", &udp.sin_addr);
    
    if (connect(temp_udp, (struct sockaddr *)&udp, sizeof(udp)) < 0){
        //printf("\nConnection Failed \n");
        result = 0;
    }
    if (getsockname(temp_udp,(struct sockaddr *)&udp,(unsigned int*) &len) == -1){
        perror("getsockname");
        result = 0;
    }
    
    inet_ntop(AF_INET, &(udp.sin_addr), ip_str, len);
    int count = 0;
    struct client *temp = c_ref;
    char *send_buf = (char*)malloc(sizeof(char)*MSG_LENGTH);
    while(temp!=NULL){
        if(strcmp(client_ip[temp->list_id],ip_str)!=0){
            strcat(send_buf,client_ip[temp->list_id]);
            strcat(send_buf," ");
            strcat(send_buf,msg);
            if(send(server_fd, send_buf, strlen(send_buf),0) > 0 )
                count+=1;
        }
        temp = temp->next;
    }
    if(count > 0){
        cse4589_print_and_log("[RELAYED:SUCCESS]\n");
        cse4589_print_and_log("msg from:%s, to:255.255.255.255\n[msg]:%s\n",ip_str,msg);
        cse4589_print_and_log("[RELAYED:END]\n");
    }else{
        cse4589_print_and_log("[RELAYED:ERROR]\n");
        //cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",from_ip,send_to_ip,buffer);
        cse4589_print_and_log("[RELAYED:END]\n");
    }

}

int client(struct client **c_ref, int port_no, int listening_fd) {
    struct sockaddr_in server_addr;
    int server_fd, cmax;
    fd_set masterlist, watchlist;
    char *cmd = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *recv_buf = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *send_buf = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *input;
    char *in_port = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *msg = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *serverip = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *clientip;
    size_t nbyte_recvd;
    char *command = (char *)malloc(sizeof(char) * MSG_LENGTH);

    FD_ZERO(&masterlist);
    FD_ZERO(&watchlist);
    FD_SET(0, &masterlist);
    cmax = 0;
    int server;
    server = listening_fd;

 
    while(1){
        watchlist = masterlist;
        int selret = select(cmax + 1, &watchlist, NULL, NULL, NULL);
        if (selret == -1){
            perror("select");
        }

        if (selret > 0){
            for (int i = 0; i <= cmax; i++){
                if (FD_ISSET(i, &watchlist)){
                    memset(cmd, '\0', MSG_LENGTH);
                    memset(recv_buf, '\0', MSG_LENGTH);
                    if (i == STDIN) {
                        if (fgets(cmd, CMD_SIZE - 1, stdin) == NULL)
                            exit(-1);
                        strcpy(command, cmd);
                        input = strtok(cmd, " ");

                        if(strcmp(cmd, "AUTHOR\n") == 0){
                            print_author();
                        }else if(strcmp(cmd, IP) == 0){
                            char ip_str[INET_ADDRSTRLEN];
                            strcpy(ip_str, find_ip(ip_str));
                        }else if(strcmp(cmd, "LIST\n") == 0){
                            print(*c_ref);
                        }else if(strcmp(cmd, "PORT\n") == 0){
                            print_portno(port_no);
                        }else if(strcmp(input, "LOGIN") == 0){
                            int count = 0, result = 1;
                            server_fd = socket(AF_INET, SOCK_STREAM, 0);
                            if(server_fd < 0){
                                perror("Socket creation failed");
                                exit(EXIT_FAILURE);
                            }else{
                                FD_SET(server_fd, &masterlist);
                                if (server_fd > cmax) cmax = server_fd;
                            }

 

                            while (count != 2){
                                char *a = (char *)malloc(sizeof(char) * CMD_SIZE);
                                if (count == 0){
                                    input = strtok(NULL, " ");
                                    if (input == NULL){
                                        result = 0;
                                        break;
                                    }
                                    strcpy(a, input);
                                    serverip = a;
                                }else if(count == 1){
                                    input = strtok(NULL, "\n");
                                    if (input == NULL){
                                        result = 0;
                                        break;
                                    }
                                    strcpy(a, input);
                                    in_port = a;
                                }
                                count++;
                            }

                            if (result == 0){
                                cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
                                cse4589_print_and_log("[%s:END]\n", "LOGIN");
                            }else{
                                server_addr.sin_family = AF_INET;
                                unsigned int port_temp = atoi(in_port);
                                server_addr.sin_port = htons(port_temp);
                                inet_pton(AF_INET, serverip, &(server_addr.sin_addr));

                                if(connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
                                    perror("Connect failed");
                                    cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
                                    cse4589_print_and_log("[%s:END]\n", "LOGIN");
                                }else{
                                    send_port(port_no, server_fd);
                                    cse4589_print_and_log("[%s:SUCCESS]\n", "LOGIN");
                                    cse4589_print_and_log("[%s:END]\n", "LOGIN");
                                }
                            }

                        }else if(strcmp(input, "SEND") == 0){
                            int count = 0;
                            while (count != 2){
                                char *a = (char *)malloc(sizeof(char) * MSG_LENGTH);
                                strcpy(a, input);
                                if (count == 0){
                                    input = strtok(NULL, " ");
                                    if (input == NULL) break;
                                    strcpy(a, input);
                                    clientip = a;
                                }else if(count == 1){
                                    input = strtok(NULL, "\n");
                                    if (input == NULL) break;
                                    strcpy(a, input);
                                    msg = a;
                                    break;
                                }
                                count++;
                            }
                            memset(send_buf, '\0', MSG_LENGTH);
                            server_addr.sin_family = AF_INET;
                            server_addr.sin_port = port_no;
                            inet_pton(AF_INET, clientip, &(server_addr.sin_addr));
                            strcat(send_buf, clientip);
                            strcat(send_buf, " ");
                            strcat(send_buf, msg);
                            strcat(send_buf, "\n");
                            send(server_fd, send_buf, strlen(send_buf), 0);

                        }else if(strcmp(input, "BROADCAST") == 0){
                            char *a = (char *)malloc(sizeof(char) * MSG_LENGTH);
                            while (input != NULL){
                                input = strtok(NULL, "\n");
                                if (input != NULL) strcpy(a, input);
                                break;
                            }
                            broadcast(*c_ref, a, server_fd);
                        }else if(strcmp(input, "BLOCK") == 0){
                            input = strtok(NULL, "\n");
                            struct client *temp = *c_ref;
                            int found = 0;
                            while (temp != NULL){
                                if (strcmp(input, client_ip[temp->list_id]) == 0) {
                                    temp->block_status = 1;
                                    found += 1;
                                }
                                temp = temp->next;
                            }
                            if (found >= 1){
                                cse4589_print_and_log("[%s:SUCCESS]\n", "BLOCK");
                                cse4589_print_and_log("[%s:END]\n", "BLOCK");
                            }else{
                                cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
                                cse4589_print_and_log("[%s:END]\n", "BLOCK");
                            }
                        }else if(strcmp(input, "UNBLOCK") == 0){
                            input = strtok(NULL, "\n");
                            struct client *temp = *c_ref;
                            int found = 0;
                            while (temp != NULL){
                                if (strcmp(input, client_ip[temp->list_id]) == 0){
                                    temp->block_status = 0;
                                    found += 1;
                                }
                                temp = temp->next;
                            }
                            if(found >= 1){
                                cse4589_print_and_log("[%s:SUCCESS]\n", "UNBLOCK");
                                cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
                            } else {
                                cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
                                cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
                            }
                        }else if(strcmp(cmd, "LOGOUT") == 0){
                            cse4589_print_and_log("[%s:SUCCESS]\n", "LOGOUT");
                            close(server_fd);
                            cse4589_print_and_log("[%s:END]\n", "LOGOUT");
                            return 0;
                        }else if(strcmp(cmd, "EXIT") == 0){
                            cse4589_print_and_log("[%s:SUCCESS]\n", "EXIT");
                            close(server_fd);
                            cse4589_print_and_log("[%s:END]\n", "EXIT");
                            exit(0);
                        }
                    }else{
                        memset(recv_buf, '\0', MSG_LENGTH);
                        if (i == 0 && i == listening_fd)
                            break;
                        else{
                            nbyte_recvd = recv(i, recv_buf, MSG_LENGTH, 0);
                            recv_buf[nbyte_recvd] = '\0';
                            char *input = (char *)malloc(sizeof(char) * MSG_LENGTH);
                            strcpy(input, recv_buf);
                            struct client *temp = *c_ref;
                            char *identify = (char *)malloc(sizeof(char) * MSG_LENGTH);
                            char *msg_from = (char *)malloc(sizeof(MSG_LENGTH));
                            char *a = (char *)malloc(sizeof(MSG_LENGTH));
                            char *msg = (char *)malloc(sizeof(MSG_LENGTH));
                            int count = 0;
                            if(recv_buf != NULL){
                                input = strtok(input, " ");
                                strcpy(msg_from, input);
                                while (input != NULL) {
                                    input = strtok(NULL, " ");
                                    if (input != NULL) strcpy(msg, input);
                                }
                                cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                                cse4589_print_and_log("msg from:%s\n[msg]:%s\n", msg_from, msg);
                                cse4589_print_and_log("[RECEIVED:END]\n");
                            }else{
                                cse4589_print_and_log("[RECEIVED:ERROR]\n");
                                cse4589_print_and_log("msg from:%s\n[msg]:%s\n", msg_from, msg);
                                cse4589_print_and_log("[RECEIVED:END]\n");
                            }
                        }
                        fflush(stdout);
                    }
                }
            }
        }
    }
}
