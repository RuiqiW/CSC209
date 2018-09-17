#include <stdio.h>
#include <stdlib.h>

int compare (const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

/* Each of the n elements of array elements, is the address of an
 * array of n integers.
 * Return 0 if every integer is between 1 and n^2 and all
 * n^2 integers are unique, otherwise return 1.
 */
int check_group(int **elements, int n) {
    // TODO: replace this return statement with a real function body
    if(n == 1){
        if(**elements == 1){
            return 0;
        }else{
            return 1;
        }
    }
    int list[n * n];
    for(int i = 0; i < n; i++){
        for(int j = 0; j <n; j++){
            list[i*n + j] = elements[i][j];
        }
    }
    qsort(list, n * n, sizeof(int), compare);
    if(list[0] != 1){
        return 1;
    }
    for(int i = 1; i < n * n; i++){
        if(list[i] - list[i-1] != 1){
            return 1;
        }
    }
    return 0;
}

/* puzzle is a 9x9 sudoku, represented as a 1D array of 9 pointers
 * each of which points to a 1D array of 9 integers.
 * Return 0 if puzzle is a valid sudoku and 1 otherwise. You must
 * only use check_group to determine this by calling it on
 * each row, each column and each of the 9 inner 3x3 squares
 */
int check_regular_sudoku(int **puzzle) {
    // check rows
    for(int i = 0; i < 9; i++){
        int *checklist[3] = {(int *)puzzle[i], (int *)puzzle[i] + 3, (int *)puzzle[i] + 6};
        if(check_group(checklist, 3)){
            return 1;
        }
    }
    // check columns
    for(int i = 0; i < 9; i++){
        int col[9];
        for(int j = 0; j < 9; j++){
            col[j] = puzzle[j][i];  // ith col, jth row
        }
        int *checklist[3] = {(int *)col, (int *)col + 3, (int *) col + 6};
        if(check_group(checklist, 3)){
            return 1;
        }
    }

    // check inner boxes
    for(int i = 0; i < 9; i++){
        int row = (i / 3) * 3;
        int col = (i % 3) * 3; 
        int *checklist[3] = {(int *)puzzle[row] + col, (int *)puzzle[row + 1] + col, (int *)puzzle[row + 2] + col};
        if(check_group(checklist, 3)){
            return 1;
        }
    }
    return 0;
}
