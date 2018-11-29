#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hcq.h"

#ifndef PORT
  #define PORT 59787
#endif
#define BUF_SIZE 32


struct buffer{
    char buf[BUF_SIZE];
    int inbuf;
    char *after;
};


struct client {
    int sock_fd;
    char *username;
    char type;
    int state;
    struct buffer *command; 
    struct client *next;
};

typedef struct buffer Buffer;
typedef struct client Client;

// have exactly one TA list and one student list
Ta *ta_list = NULL;
Student *stu_list = NULL;

Course *courses;  
int num_courses = 3;


// /* Accept a connection. Note that a new file descriptor is created for
//  * communication with the client. The initial socket descriptor is used
//  * to accept connections, but the new socket is used to communicate.
//  * Return the new client's file descriptor or -1 on error.
//  */
int accept_connection(int fd, Client **client_list) {

    // Accept a connection
    int client_fd = accept(fd, NULL, NULL);
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    //Write welcome to client
    char *msg = "Welcome to the Help Centre, what is your name?\r\n";
    if(write(client_fd, msg, strlen(msg)) != strlen(msg)){
        close(client_fd);
        return -1;
    }

    // Create a client
    Client *new_client = malloc(sizeof(Client));
    new_client->sock_fd = client_fd;
    new_client->username = NULL;
    new_client->type = '\0';
    new_client->state = 0;
    new_client->next = NULL;
    new_client->command = malloc(sizeof(Buffer));
    memset(new_client->command->buf, '\0', BUF_SIZE);
    new_client->command->inbuf = 0;
    new_client->command->after =  new_client->command->buf;

    if(*client_list != NULL){
        new_client->next = *client_list;
    }else{
        new_client->next = NULL;
    }

    *client_list = new_client;
    
    return client_fd;
}


/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n) {
    for(int i = 0; i < n - 1; i++){
        if(buf[i] == '\r' && buf[i+1] == '\n'){
            return i + 2;
        }
    }
    return -1;
}

/* Read a message from client.
 * Return the fd if it has been closed or client writes over 30 characters, 
 * return 0 otherwise.
 */
int read_from(Client *client, char *new_command) {
    int fd = client->sock_fd;
    Buffer *command = client->command;
   
    int room = sizeof(command->buf);  // bytes remaining in buffer
    int nbytes;

    // Read messages
    if ((nbytes = read(fd, command->after, room)) == 0) {
        return fd;
    }else{
        command->inbuf += nbytes;
        int where;
        // find_network_newline to determine if a full line has been read from the client.
        if ((where = find_network_newline(command->buf, command->inbuf)) > 0) {
            // we have a full line, copy the message from buf to command
            command->buf[where - 2] = '\0';
            command->buf[where - 1] = '\0';
            strncpy(new_command, command->buf, strlen(command->buf) + 1);
            new_command[strlen(command->buf)] = '\0';
            
            // update inbuf and remove the full line from the buffer
            command->inbuf -= where;
            memmove(command->buf, command->buf + where, command->inbuf);
            // update after and room, in preparation for the next read.
            command->after = &command->buf[command->inbuf];

        //If buf is full but no new-line character is found
        }else if(command->inbuf == BUF_SIZE){
            return fd;
        }      
    }
    
    return 0;
}




int main(void) {

    Client *client_list;

    // Create course list.
    if ((courses = malloc(sizeof(Course) * 3)) == NULL) {
        perror("malloc for course list\n");
        exit(1);
    }
    strcpy(courses[0].code, "CSC108");
    strcpy(courses[1].code, "CSC148");
    strcpy(courses[2].code, "CSC209");

    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("server: socket");
        exit(1);
    }

    // Set information about the port (and IP) we want to be connected to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;
    memset(&server.sin_zero, 0, 8);

    // Release port after service process terminates.
    int on = 1;
    int status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(status == -1) {
        perror("setsockopt -- REUSEADDR");
    }

    // Bind the selected port to the socket.
    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("server: bind");
        close(sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(sock_fd, 1024) < 0) {
        perror("server: listen");
        close(sock_fd);
        exit(1);
    }

    // The client accept - message accept loop. 
    // First initialize a set of file descriptors.
    int max_fd = sock_fd;
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);

    while (1) {
        // select updates the fd_set it receives, so we always use a copy and retain the original.
        fd_set listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit(1);
        }

        // Is it the original socket? Create a new connection ...
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd, &client_list);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
        }

        // Next, check the clients.
        Client *head = client_list;
        while(head != NULL){
            if (FD_ISSET(head->sock_fd, &listen_fds)) {
                int client_closed;
                // // get name
                // if(client->state == 0){
                //     client_closed = read_name(head);
                // // get type
                // }else if(client->state == 1){
                //     client_closed = read_type(head);
                // }else{
                //     // other commands
                //     if(client->type == 'T'){
                //         client_closed = read_from_ta(head);
                //     }else if(client->type == 'S'){
                //         // get student's course
                //         if(client->state == 2){
                //             client_closed = read_course(head);
                //         }else{
                //             client_closed = read_from_student(head);
                //         }
                //     }
                // }

                // handle disconnected client
                if(client_closed > 0){
                    FD_CLR(client_closed, &all_fds);
                    // remove client from client_list
                }
            }
            head = head->next;
        }
    }

    // Should never get here.
    return 1;
}
