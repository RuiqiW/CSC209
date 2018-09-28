#include <stdlib.h>
#include <stdio.h>

/*
 * Define a function void fib(...) below. This function takes parameter n
 * and generates the first n values in the Fibonacci sequence.  Recall that this
 * sequence is defined as:
 *         0, 1, 1, 2, 3, 5, ... , fib[n] = fib[n-2] + fib[n-1], ...
 * The values should be stored in a dynamically-allocated array composed of
 * exactly the correct number of integers. The values should be returned
 * through a pointer parameter passed in as the first argument.
 *
 * See the main function for an example call to fib.
 * Pay attention to the expected type of fib's parameters.
 */

/* Write your solution here */
void fib(int **fib_pt, int count){
    *fib_pt = malloc(sizeof(int) * count);
    if(count == 1){
        (*fib_pt)[0] = 0;
    }else{
        (*fib_pt)[0] = 0;
        (*fib_pt)[1] = 1;
        if(count > 2){
            for(int i = 2; i < count; i++){
                (*fib_pt)[i] = (*fib_pt)[i-1] + (*fib_pt)[i-2];
            }
        }
    }
}


int main(int argc, char **argv) {
    /* do not change this main function */
    int count = strtol(argv[1], NULL, 10);
    int *fib_sequence;

    fib(&fib_sequence, count);
    for (int i = 0; i < count; i++) {
        printf("%d ", fib_sequence[i]);
    }
    free(fib_sequence);
    return 0;
}
