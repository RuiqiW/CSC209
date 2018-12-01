#ifndef SERVER_H
#define SERVER_H

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


int accept_connection(int fd, Client **client_list);
int find_network_newline(const char *buf, int n);
int read_from(Client *client, char *new_command);
int read_name(Client *client);
int read_type(Client *client, Ta **ta_list_ptr);
int read_course(Client *client, Student **stu_list_ptr, Course *courses, int num_courses);
int read_from_student(Client *client, Student **stu_list_ptr, Ta **ta_list_ptr);
Client *free_client(Client *client, Client **client_list_ptr);
int serve_student(char *name, Client **client_list_ptr);
int read_from_ta(Client *client, Client **client_list_ptr, Student **stu_list_ptr, Ta **ta_list_ptr);

#endif /* SERVER_H */
