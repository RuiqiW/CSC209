#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void count_name(char *name, char *names0, char *names1, int *processes){
    if(!strcmp(name, names0)){  //name same as names0
        processes[0] ++;
    }else if(!strcmp(name, names1)){  //name same as names1
        processes[1] ++;
    }else{      //a third name occurs
        if(processes[0] >= processes[1]){
            processes[1] = 1;
            strcpy(names1, name);
        }else{
            processes[0] = 1;
            strcpy(names0, name);
        }
    }
}

int main(int argc, char **argv) {
    
    // Invalid arguments
    if (argc > 2) {
        printf("USAGE: most_processes [ppid]\n");
        return 1;
    }else{
        char s[1024];
        char names0[31] = {'\0'}, names1[31] = {'\0'};
        int processes[2] = {0, 0};
       
        // if no argument
        if(argc == 1){
             while((fgets(s,1024,stdin))!=NULL){
                char name[31]; 
                sscanf(s, "%[^ ]", name);
                count_name(name, names0, names1, processes);
            }  
            
        }else{
            int ppid = strtol(argv[1], NULL, 10);
            while((fgets(s,1024,stdin))!=NULL){
                char name[31]; 
                int cur_ppid;
                sscanf(s, "%s %*d %d", name, &cur_ppid);
                if(cur_ppid == ppid){
                    count_name(name, names0, names1, processes);
                }
            }
        }

        // print the result
        if(processes[0] >= processes[1]){
            if(strcmp(names0, "")){
                printf("%s %d\n", names0, processes[0]);
            }
        }else{
            if(strcmp(names1, "")){
                printf("%s %d\n", names1, processes[1]);
            }
        }
    }
    return 0;
}

