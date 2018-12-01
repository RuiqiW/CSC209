#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hcq.h"
#include "server.h"


/* Accept a connection. A new file descriptor is created for
 * communication with the client. 
 * Return the new client's file descriptor or -1 on error.
 */
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

    // Add new_client to client_list
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

/* A helper function to read a message from client.
 * Return the fd if it has been closed or client writes over 30 characters, 
 * return -1 if no full line message has been read, return 0 otherwise.
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


/* Read username from client and update its username and state,
 * then send client a message to ask for its role.
 * Return the fd if it has been closed or writes over 30 characters, 
 * return 0 otherwise.
 */
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


/* Read client's role, add a new ta to ta_list if the role is 'T', 
 * then send client a message to inform it of the next step.
 * Return the fd if it has been closed or writes over 30 characters, 
 * return 0 otherwise.
 */
int read_type(Client *client, Ta **ta_list_ptr){
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

        // Create a Ta struct
        if(client->type == 'T'){
            add_ta(ta_list_ptr, client->username);
        }
    }
    free(type);
    return 0; 
}


/* Read student client's course, add a new student to stu_list if 
 * the student is not in the list and the course is valid, 
 * otherwise, disconnect client from the server.
 * Return the fd if it has been closed, return 0 otherwise.
 */
int read_course(Client *client, Student **stu_list_ptr, Course *courses, int num_courses){
    char *course = malloc(BUF_SIZE);
    int closed;
    char *msg;

    if( (closed = read_from(client, course)) > 0){
        free(course);
        return closed;
    }else if(closed == 0){
        int success = add_student(stu_list_ptr, client->username, course, courses, num_courses);
        if ( success == 2){
            msg = "This is not a valid course. Good-bye.\r\n";
        }else if(success == 1){
            msg = "You are already in the queue and cannot be added again for any course. Good-bye.\r\n";
        }else{
            msg = "You have been entered into the queue. While you wait, you can use the command stats to see which TAs are currently serving students.\r\n";
        }

        // Write subsequent messages to client
        if(write(client->sock_fd, msg, strlen(msg)) != strlen(msg)){
            free(course);
            return client->sock_fd;
        }

        // If student is not added to stu_list
        if(success > 0){
            close(client->sock_fd);
            free(course);
            return client->sock_fd;
        }

        // student is added to stu_list, update its state
        client->state = 3;
    }
    free(course);
    return 0; 
}


/* Read student's command, send it currently serving list if the command is valid,
 * otherwise, send it an error message.
 * Return the fd if it has been closed or writes over 30 characters, 
 * return 0 otherwise.
 */
int read_from_student(Client *client, Student **stu_list_ptr, Ta **ta_list_ptr){
    char *query = malloc(BUF_SIZE);
    int closed;
    char *msg;
    int msg2free = 0;  //whether msg is returned from helper function

    // if student disconnect from the server
    if( (closed = read_from(client, query)) > 0){
         give_up_waiting(stu_list_ptr, client->username); // remove from stu_list
         free(query);
         return closed;

    }else if(closed == 0){
         // Check if the input is a valid command
         if(strcmp(query, "stats")){
             msg = "Incorrect syntax\r\n";
         }else{
             msg = print_currently_serving(*ta_list_ptr);
             msg2free = 1;
        }

        // Write subsequent messages to client
        if(write(client->sock_fd, msg, strlen(msg)) != strlen(msg)){
            give_up_waiting(stu_list_ptr, client->username); // remove from stu_list
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


/* A helper function to remove client from client_list and
 * free the memory when a client is disconnected from server.
 * Return the client after it in the client_list.
*/

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


/* A helper function for read_from_ta. Find the student client to serve,
 * send it a message and disconnect it from server.
 * Return student client's fd if it is disconnected from server,
 * return -1 if the student is not found.
 */
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


/* Read ta's command, send student queue or take next student if the command is valid,
 * otherwise, send an error message.
 * Return the sock_fd of the next student to serve if taking next student successfully,
 * or return the fd if it has been closed or writes over 30 characters,
 * return 0 otherwise.
 */
int read_from_ta(Client *client, Client **client_list_ptr, Student **stu_list_ptr, Ta **ta_list_ptr){

    char *query = malloc(BUF_SIZE);
    int closed;
    char *msg;
    int msg2free = 0; //whether msg is returned from helper function

    // if TA disconnect from the server
    if( (closed = read_from(client, query)) > 0){
        remove_ta(ta_list_ptr, client->username); // remove from ta_list
        free(query);
        return closed;
    }else if(closed == 0){
         // take next student 
        if(!strcmp(query, "next")){
            int stu_fd = 0;
            if(*stu_list_ptr != NULL){
                stu_fd = serve_student((*stu_list_ptr)->name, client_list_ptr);
            }
            if(next_overall(client->username, ta_list_ptr, stu_list_ptr) == 1){
                // if ta not exit, shouldn't reach here
                fprintf(stderr, "Ta does not exit\n");
                exit(1);
            }
            free(query);
            return stu_fd;
        }else{
            // print waitlist
            if(!strcmp(query, "stats")){
                msg = print_full_queue(*stu_list_ptr);
                msg2free = 1;
            }else{
                msg = "Incorrect syntax\r\n";
            }
            
            // Write subsequent messages to TA client
            if(write(client->sock_fd, msg, strlen(msg)) != strlen(msg)){
                remove_ta(ta_list_ptr, client->username); // remove from ta_list
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
    free(query);
    return 0; 
}
