#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include "freq_list.h"
#include "worker.h"

/* Complete this function for Task 1. Including fixing this comment.
*/
FreqRecord *get_word(char *word, Node *head, char **file_names) {
    Node *cur = head;
   
    // find the word
    while(cur != NULL){
        if(strcmp(cur ->word, word) == 0){
            break;
        }else{
            cur = cur->next;
        }
    }
    // if word exists
    if(cur != NULL){
        int count = 0;

        // count files containing this word
        for(int i = 0; i < MAXFILES; i++){
            if(cur->freq[i] != 0){
                count++;
            }
        }
        FreqRecord *record = malloc(sizeof(FreqRecord) * (count + 1));
        if(record == NULL){
            perror("malloc");
            exit(1);
        }

        int j = 0;
        for(int i = 0; i < MAXFILES; i++){
            if(cur->freq[i] != 0){
                strncpy(record[j].filename, file_names[i], PATHLENGTH);
                record[j].filename[PATHLENGTH - 1] = '\0';
                record[j].freq = cur->freq[i];
                j++;
            }
        }
        record[count].filename[0] = '\0';
        record[count].freq = 0;
        return record;
    }
    
    //if word doesn't exist
    FreqRecord *record = malloc(sizeof(FreqRecord));
    record[0].filename[0] = '\0';
    record[0].freq = 0;
    return record;

}

/* Print to standard output the frequency records for a word.
* Use this for your own testing and also for query.c
*/
void print_freq_records(FreqRecord *frp) {
    int i = 0;

    while (frp != NULL && frp[i].freq != 0) {
        printf("%d    %s\n", frp[i].freq, frp[i].filename);
        i++;
    }
}

/* Complete this function for Task 2 including writing a better comment.
*/
void run_worker(char *dirname, int in, int out) {
    return;
}
