#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256
#define MAX_PASSWORD 10

#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void) {
  char user_id[MAXLINE];
  char password[MAXLINE];

  if(fgets(user_id, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  if(fgets(password, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }

  // pipe
  int fd[2];
  if(pipe(fd) == -1){
        perror("pipe");
        exit(1);
    }
 
  // fork
  int result = fork();

  // child process
  if(result == 0){

      close(fd[1]);

      if(dup2(fd[0], fileno(stdin)) == -1){
          perror("dup2");
          exit(1);
      }
      execl("./validate", "validate", NULL);
      perror("exec");
      exit(1);
  
  }else if(result > 0){ // parent process
      
      close(fd[0]);

      if(write(fd[1], user_id, MAX_PASSWORD) == -1){
          perror("write to pipe");
          exit(1);
      }
      if(write(fd[1], password, MAX_PASSWORD) == -1){
          perror("write to pipe");
          exit(1);
      }
      
      close(fd[1]);
      
      
  }else{ // fork failed
      perror("fork");
      exit(1);
  }

  pid_t pid;
  int status;
  
  if((pid = wait(&status)) == -1){
      perror("wait");
    }else{
        if(WIFEXITED(status)){
            if(WEXITSTATUS(status) == 1){
                exit(1);
            }else if(WEXITSTATUS(status) == 2){
                printf(INVALID);
            }else if(WEXITSTATUS(status) == 3){
                printf(NO_USER);
            }else{
                printf(SUCCESS);
        }
    }

  return 0;
}
