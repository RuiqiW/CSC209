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

#ifndef PORT
  #define PORT 59787
#endif


// have exactly one TA list and one student list
Ta *ta_list = NULL;
Student *stu_list = NULL;

Course *courses;  
int num_courses = 3;


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
        // select updates the fd_set it receives.
        fd_set listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit(1);
        }

        // Create a new connection
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = accept_connection(sock_fd, &client_list);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            if(client_fd >= 0){
                FD_SET(client_fd, &all_fds);
            }
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
                    client_closed = read_type(head, &ta_list);
                }else{
                    if(head->type == 'S'){
                        // get course
                        if(head->state == 2){
                            client_closed = read_course(head, &stu_list, courses, num_courses);
                        // get student's command
                        }else{
                            client_closed = read_from_student(head, &stu_list, &ta_list);
                        }
                    // get ta's command
                    }else if(head->type == 'T'){
                        client_closed = read_from_ta(head, &client_list, &stu_list, &ta_list);
                    }else{
                        // shouldn't reach here
                        fprintf(stderr, "Invalid client\n");
                        exit(1);
                    }
                }

                // handle disconnected client
                if(client_closed > 0){
                    FD_CLR(client_closed, &all_fds);
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
