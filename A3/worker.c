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
 * First load the index from the directory specified by dirname. Then read from 
 * the file descriptor in one word at a time and write to the file descriptor out 
 * one FreqRecord for each file in which the word has a non-zero frequency. 
 * Finally, write one FreqRecord with a frequency of zero and an empty string for the filename.  
*/
void run_worker(char *dirname, int in, int out) {
    Node *head = NULL;
    char **filenames = init_filenames();

    // get the path of index and filenames
    char listfile[PATHLENGTH];
    listfile[0] = '\0';
    strncpy(listfile, dirname, PATHLENGTH);
    strncat(listfile, "/", PATHLENGTH-strlen(listfile));
    strncat(listfile, "index", PATHLENGTH-strlen(listfile));
    listfile[PATHLENGTH - 1] = '\0';
    
    char namefile[PATHLENGTH];
    namefile[0] = '\0';
    strncpy(namefile, dirname, PATHLENGTH);
    strncat(namefile, "/", PATHLENGTH-strlen(namefile));
    strncat(namefile, "filenames", PATHLENGTH-strlen(namefile));
    namefile[PATHLENGTH - 1] = '\0';

    read_list(listfile, namefile, &head, filenames);

    char word[MAXWORD];
    int temp;
    while((temp = read(in, word, MAXWORD)) != 0){
        if(temp == -1){
            perror("read");
            exit(1);
        }
        // Remove '\n' if it exists
        char *newline;
        if((newline=strchr(word, '\n')) != NULL) {
            *newline = '\0';
        }

        FreqRecord *record = get_word(word, head, filenames);

        // write to file descriptor out
        int i = 0;
        while (record != NULL && record[i].freq != 0) {
            if(write(out, &(record[i]), sizeof(FreqRecord)) == -1){
                perror("write");
                exit(1);
            }
            i++;
        }
        //write the last FreqRecord
        if(write(out, &(record[i]), sizeof(FreqRecord))  == -1){
            perror("write");
            exit(1);
        }
        free(record);
    }
    freespace(head, filenames);
}

/* A helper function for sorting FreqRecord.
*/
int cmpfunc (const void * a, const void * b) {
   return ( (*(FreqRecord*)b).freq - (*(FreqRecord*)a).freq);
}

/* A helper function to free head and filenames.
*/
void freespace(Node *head, char **filenames){
    Node *cur;
    while(head != NULL){
        cur = head -> next;
        free(head);
        head = cur;
    }
    for(int i = 0; i < MAXFILES; i++){
        free(filenames[i]);
    }
    free(filenames);
}