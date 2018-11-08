#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "freq_list.h"
#include "worker.h"

int main(int argc, char **argv) {
    Node *head = NULL;
    char **filenames = init_filenames();
    char arg;
    char *listfile = "index";
    char *namefile = "filenames";

    /* Use getop to process command-line flags and arguments
     * cited from printindex.c
     */
    while ((arg = getopt(argc,argv,"i:n:")) > 0) {
        switch(arg) {
        case 'i':
            listfile = optarg;
            break;
        case 'n':
            namefile = optarg;
            break;
        default:
            fprintf(stderr, "Usage: printindex [-i FILE] [-n FILE]\n");
            exit(1);
        }
    }

    read_list(listfile, namefile, &head, filenames);
    display_list(head, filenames);

    /* Read words from stdin and print their freqrecords if such word exists 
     * in some file in the directory. 
    */
    char word[MAXWORD];
     while((fgets(word, MAXWORD, stdin))!=NULL){
        // Remove '\n' if it exists
        char *newline;
        if((newline=strchr(word, '\n')) != NULL) {
            *newline = '\0';
        }
        FreqRecord *record = get_word(word, head, filenames);
        print_freq_records(record);
    }

    return 0;
}
