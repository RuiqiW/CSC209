#ifndef SERVER_H
#define SERVER_H

#ifndef PORT
  #define PORT 59787
#endif
#define MAX_BACKLOG 5
#define BUF_SIZE 32


/* A buffer of client to store partial reads 
 * before a newline charactor is received.
 */
struct buffer{
    char *buf;
    int inbuf;
    char *after;
};

/* Clients are kept in reverse order of addition. 
 * The newest client is at the head of list. 
 * type should be 'T' or 'S'.
 * state represents what information has been read from client:
 * state 0: client accepted
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

// accept new clients
int accept_connection(int fd, Client **client_list);

// remove client from client_list when the client has been disconnected from server
Client *free_client(Client *client, Client **client_list_ptr);

// helper function for read_from, to find network newlines in client's buffer
int find_network_newline(const char *buf, int n);

// read messages from clients
int read_from(Client *client, char *new_command);
int read_name(Client *client);
int read_type(Client *client, Ta **ta_list_ptr);
int read_course(Client *client, Student **stu_list_ptr, Course *courses, int num_courses);

// handle "stats" command from student or report invalid command
int read_from_student(Client *client, Student **stu_list_ptr, Ta **ta_list_ptr);

// helper function for read_from_ta, send message to student client when the student is taken by ta
int serve_student(char *name, Client **client_list_ptr);

// handle "stats" or "next" command from ta or report invalid command
int read_from_ta(Client *client, Client **client_list_ptr, Student **stu_list_ptr, Ta **ta_list_ptr);

#endif /* SERVER_H */
