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
#define MAX_BACKLOG 5
#define BUF_SIZE 32


struct buffer{
    char *buf;
    int inbuf;
    char *after;
};

/* state 0: accepted
 * state 1: name read
 * state 2: type read
 * state 3: course read(student client only)
*/
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
        return -1;
    }

    // Create a client
    Client *new_client = malloc(sizeof(Client));
    if(new_client == NULL){
        perror("malloc");
        exit(1);
    }
    new_client->sock_fd = client_fd;
    new_client->username = NULL;
    new_client->type = '\0';
    new_client->state = 0;
    new_client->next = NULL;
    new_client->command = malloc(sizeof(Buffer));
    if(new_client->command == NULL){
        perror("malloc");
        exit(1);
    }
    new_client->command->buf = malloc(BUF_SIZE);
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


/* Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
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
   
    int room = BUF_SIZE - command->inbuf;  // bytes remaining in buffer
    int nbytes;

    // Read messages
    if ((nbytes = read(fd, command->after, room)) == 0) {
        return fd;
    }else{
        command->inbuf += nbytes;
        int where;
        
        // find_network_newline to determine if a full line has been read from the client.
        if ((where = find_network_newline(command->buf, command->inbuf)) > 0) {
            command->buf[where - 2] = '\0';
            command->buf[where - 1] = '\0';

            // copy the full line to new_command
            strncpy(new_command, command->buf, strlen(command->buf) + 1);
            new_command[strlen(command->buf)] = '\0';
            
            // update inbuf and remove the full line from the buffer
            command->inbuf -= where;
            memmove(command->buf, command->buf + where, command->inbuf);
            
            // update after, in preparation for the next read.
            command->after = &command->buf[command->inbuf];
            return 0;

        //If buf is full but no new-line character is found, disconnect client
        }else if(command->inbuf == BUF_SIZE){
            close(client->sock_fd);
            return fd;
            
        //No new instruction
        }else{
            // update after, in preparation for the next read.
            command->after = &command->buf[command->inbuf];
            return -1;
        }      
    }
}

int read_name(Client *client){
     char *name = malloc(BUF_SIZE);
     int closed;
     if( (closed = read_from(client, name)) > 0){
         free(name);
         return closed;
     }else if(closed == 0){
         if( (client->username = malloc(strlen(name) + 1)) == NULL){
             perror("malloc");
             exit(1);
         }

         // update username and state
         strncpy(client->username, name, strlen(name) + 1); 
         client->username[strlen(name)] = '\0';
         client->state = 1;

         // ask for role
         char *msg = "Are you a TA or a Student (enter T or S)?\r\n";
         if(write(client->sock_fd, msg, strlen(msg)) != strlen(msg)){
             free(name);
             return client->sock_fd;
        }
    }
    free(name);
    return 0;
}

int read_type(Client *client){
    char *type = malloc(BUF_SIZE);
    int closed;
    char *msg;

    if((closed = read_from(client, type)) > 0){
        free(type);
        return closed;
    }else if(closed == 0){
        // Check if the input is a valid type of client
        if(strlen(type) == 1){
            if(type[0] == 'T'){
                client->type = 'T';
                client->state = 2;
                msg = "Valid commands for TA:\r\n\tstats\r\n\tnext\r\n\t(or use Ctrl-C to leave)\r\n";
            }else if(type[0] == 'S'){
                client->type = 'S';
                client->state = 2;
                msg = "Valid courses: CSC108, CSC148, CSC209\r\nWhich course are you asking about?\r\n";
            }
        // If role is invalid 
        }else{
            msg = "Invalid role (enter T or S)\r\n";
        }

        // Write subsequent messages to client
        if(write(client->sock_fd, msg, strlen(msg)) != strlen(msg)){
            free(type);
            return client->sock_fd;
        }

        // TODO: error checking for add_ta
        // Create a Ta struct
        if(client->type == 'T'){
            add_ta(&ta_list, client->username);
        }
    }
    free(type);
    return 0; 
}

int read_course(Client *client){
    char *course = malloc(BUF_SIZE);
    int closed;
    char *msg;
    int valid = 1;

    if( (closed = read_from(client, course)) > 0){
        free(course);
        return closed;
    }else if(closed == 0){
        // Check if the input is a valid type of client
        if(strcmp(course, "CSC108") && strcmp(course, "CSC148") && strcmp(course, "CSC209")){
            msg = "This is not a valid course. Good-bye.\r\n";
            valid = 0;
        }else{
            msg = "You have been entered into the queue. While you wait, you can use the command stats to see which TAs are currently serving students.\r\n";
        }

        // Write subsequent messages to client
        if(write(client->sock_fd, msg, strlen(msg)) != strlen(msg)){
            free(course);
            return client->sock_fd;
        }

        // Create a student struct if course code is valid
        if(valid){
            add_student(&stu_list, client->username, course, courses, num_courses);
            client->state = 3; 
        }
    }
    free(course);
    return 0; 
}

int read_from_student(Client *client){
    char *query = malloc(BUF_SIZE);
    int closed;
    char *msg;
    int msg2free = 0;  //whether msg is returned from helper function

    // if student disconnect from the server
    if( (closed = read_from(client, query)) > 0){
         give_up_waiting(&stu_list, client->username);
         free(query);
         return closed;

    }else if(closed == 0){
         // Check if the input is a valid command
         if(strcmp(query, "stats")){
             msg = "Incorrect syntax\r\n";
         }else{
             msg = print_currently_serving(ta_list);
             msg2free = 1;
        }

        // Write subsequent messages to client
        if(write(client->sock_fd, msg, strlen(msg)) != strlen(msg)){
            give_up_waiting(&stu_list, client->username);
            free(query);
            if(msg2free){
                free(msg);
            }
            return client->sock_fd;
        }
        if(msg2free){
            free(msg);
        }
    }
    free(query);
    return 0; 
}


Client *free_client(Client *client, Client **client_list_ptr){

    Client *next_client = client->next;
    Client *head = *client_list_ptr;
    if(client->sock_fd == head->sock_fd){
        *client_list_ptr = client->next;
    }else{
        // find the predecessor of this client
        while(head->next != NULL){
            if(head->next->sock_fd == client->sock_fd){
                break;
            }
            head = head->next;
        }
        head->next = client->next;
    }

    free(client->command->buf);
    free(client->command);
    if(client->username != NULL){
        free(client->username);
    }
    free(client);
    return next_client;
}



int serve_student(char *name, Client **client_list_ptr){
    Client *head = *client_list_ptr;
    Client *toServe = NULL;

    // Find student client
    while(head != NULL){
        if(head->type == 'S' && !strcmp(head->username, name)){
            toServe = head;
            break;
        }
        head = head->next;
    }

    if(toServe == NULL){
        return -1;

    // Serve student client
    }else{
        int fd = toServe->sock_fd;
        char *msg = "Your turn to see the TA.\r\nWe are disconnecting you from the server now. Press Ctrl-C to close nc\r\n";
        write(fd, msg, strlen(msg));
        close(fd);
        free_client(toServe, client_list_ptr);
        return fd;
    }
}



int read_from_ta(Client *client, Client **client_list_ptr){
    char *query = malloc(BUF_SIZE);
    int closed;
    char *msg;
    int msg2free = 0; //whether msg is returned from helper function

    // if TA disconnect from the server
    if( (closed = read_from(client, query)) > 0){
        remove_ta(&ta_list, client->username); 
        free(query);
        return closed;
    }else if(closed == 0){
         // take next student 
        if(!strcmp(query, "next")){
            int stu_fd = 0;
            if(stu_list != NULL){
                stu_fd = serve_student(stu_list->name, client_list_ptr);
            }
            if(next_overall(client->username, &ta_list, &stu_list) == 1){
                // if ta not exit, shouldn't reach here
                fprintf(stderr, "Ta not exits\n");
                exit(1);
            }
            free(query);
            return stu_fd;
        }else{
            // print waitlist
            if(!strcmp(query, "stats")){
                msg = print_full_queue(stu_list);
                msg2free = 1;
            }else{
                msg = "Incorrect syntax\r\n";
            }
            
            // Write subsequent messages to TA client
            if(write(client->sock_fd, msg, strlen(msg)) != strlen(msg)){
                remove_ta(&ta_list, client->username);
                free(query);
                if(msg2free){
                    free(msg);
                }
                return client->sock_fd;
            }

            if(msg2free){
                free(msg);
            }
        }
    }
    return 0; 
}



int main(void) {

    Client *client_list = NULL;

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
    if (listen(sock_fd, MAX_BACKLOG) < 0) {
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
                int client_closed = 0;
                // get name
                if(head->state == 0){
                    client_closed = read_name(head);
                // get type
                }else if(head->state == 1){
                    client_closed = read_type(head);
                }else{
                    if(head->type == 'S'){
                        if(head->state == 2){
                            client_closed = read_course(head);
                        }else{
                            client_closed = read_from_student(head);
                        }
                    }else if(head->type == 'T'){
                        client_closed = read_from_ta(head, &client_list);
                    }else{
                        // shouldn't reach here
                        fprintf(stderr, "Invalid client\n");
                        exit(1);
                    }
                }

                // handle disconnected client
                if(client_closed > 0){
                    FD_CLR(client_closed, &all_fds);

                    // change head
                    if(client_closed == head->sock_fd){
                        Client *new_head = free_client(head, &client_list);
                        head = new_head;
                        continue;
                    }  
                }
            }
            head = head->next;
        }
    }

    // Should never get here.
    return 1;
}
