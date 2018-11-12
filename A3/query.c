#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

#include "freq_list.h"
#include "worker.h"

/* A query engine that uses the indexes produced by indexer
 * and searches for words under a given directory.
 * Take the directory for search from an optional argument. 
 * Then read one word a time from stdin and print to stdout
 * the frequency array of the word in order from highest to lowest.
 */
int main(int argc, char **argv) {
    char ch;
    char path[PATHLENGTH];
    char *startdir = ".";

    /* Use getopt to process command-line flags and arguments */
    while ((ch = getopt(argc, argv, "d:")) != -1) {
        switch (ch) {
        case 'd':
            startdir = optarg;
            break;
        default:
            fprintf(stderr, "Usage: queryone [-d DIRECTORY_NAME]\n");
            exit(1);
        }
    }

    // Open the directory provided by the user (or current working directory)
    DIR *dirp;
    if ((dirp = opendir(startdir)) == NULL) {
        perror("opendir");
        exit(1);
    }

    /* For each entry in the directory, eliminate . and .., and check
     * to make sure that the entry is a directory, then fork child process
     * to process the index file contained in the directory.
     */
    struct dirent *dp;
    int workerCount = 0;      // count the number of worker processes we have
    int fpr[MAXWORKERS][2];   // read from children
    int fpw[MAXWORKERS][2];   // write to children

    while ((dp = readdir(dirp)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 ||
            strcmp(dp->d_name, "..") == 0 ||
            strcmp(dp->d_name, ".svn") == 0 ||
            strcmp(dp->d_name, ".git") == 0) {
                continue;
        }

        strncpy(path, startdir, PATHLENGTH);
        strncat(path, "/", PATHLENGTH - strlen(path));
        strncat(path, dp->d_name, PATHLENGTH - strlen(path));
        path[PATHLENGTH - 1] = '\0';

        struct stat sbuf;
        if (stat(path, &sbuf) == -1) {
            // This should only fail if we got the path wrong
            // or we don't have permissions on this entry.
            perror("stat");
            exit(1);
        }

        // Only create worker process if it is a directory
        // Otherwise ignore it.
        if (S_ISDIR(sbuf.st_mode)) {

            if(workerCount >= MAXWORKERS){
                fprintf(stderr, "too many subdirectories");
                exit(1);
            }

            if(pipe(fpr[workerCount]) == -1){
                perror("pipe");
                exit(1);
            }
            if(pipe(fpw[workerCount]) == -1){
                perror("pipe");
                exit(1);
            }
            int result = fork();
            if(result < 0){
                perror("fork");
                exit(1);
            }else if(result == 0){      //child process
                // close previously opened reading ends and writing ends
                for(int i = 0; i <= workerCount; i++){  
                    if(close(fpw[i][1]) == -1){
                        perror("close write end of pipe in child");
                        exit(1);
                    }
                    if(close(fpr[i][0]) == -1){
                        perror("close read end of pipe in child");
                        exit(1);
                    }
                }
                run_worker(path, fpw[workerCount][0], fpr[workerCount][1]);
                exit(0);
            }else{      //parent process
               if(close(fpw[workerCount][0]) == -1){
                   perror("close read end of pipe in parent");
                   exit(1);
               }
               if(close(fpr[workerCount][1]) == -1){
                   perror("close write end of pipe in parent");
                   exit(1);
               }
                workerCount++;
            }
        }
    }

    char word[MAXWORD];
    int temp;

    while((temp = read(STDIN_FILENO, word, MAXWORD)) != 0){
        if(temp == -1){
            perror("read");
            exit(1);
        }

        // write the word read from stdin to pipes
        for(int i = 0; i < workerCount; i++){
            if( write(fpw[i][1], word, MAXWORD) == -1){
                perror("write to pipe");
                exit(1);
            }
        }

        FreqRecord maxrecord[MAXRECORDS + 1];
        int recordcount = 0;

        // read freqrecords
        for(int i = 0; i < workerCount; i++){
            int t;
            FreqRecord record;
            while((t = read(fpr[i][0], &record, sizeof(FreqRecord))) != 0){
                if(t == -1){
                    perror("read from pipe");
                    exit(1);
                }
                
                if(record.freq == 0){
                    break;
                }else{
                    if(recordcount < MAXRECORDS + 1){
                        maxrecord[recordcount] = record;
                        recordcount++;
                        qsort(maxrecord, recordcount, sizeof(FreqRecord), cmpfunc);
                    }else{
                        if(record.freq > maxrecord[MAXRECORDS].freq){
                            maxrecord[MAXRECORDS] = record;
                            qsort(maxrecord, MAXRECORDS, sizeof(FreqRecord), cmpfunc);
                        }
                    }
                }
            }
        }
        if(maxrecord[recordcount].freq != 0){
            FreqRecord lastrecord;
            lastrecord.freq = 0;
            lastrecord.filename[0] = '\0';
            if(recordcount == MAXRECORDS + 1){
                maxrecord[MAXRECORDS] = lastrecord;
            }else{
                maxrecord[recordcount] = lastrecord;
                recordcount++;
            }
        }
        print_freq_records(maxrecord);
    }

    // close writing end of pipe
    for(int i = 0; i < workerCount; i++){
       if(close(fpw[i][1]) < 0){
           perror("close writing end of pipe in parent");
       }
    }

    // wait for child processes
    for(int i = 0; i < workerCount; i++){
        pid_t pid;
        int status;
        if((pid = wait(&status) ) == -1){
            perror("wait");
            exit(1);
        }else{
            if(WIFEXITED(status)){
                if(WEXITSTATUS(status) == 1){
                    exit(1);
                }
            }
        }
    }

    if (closedir(dirp) < 0)
        perror("closedir");

    return 0;
}
