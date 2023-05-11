#include <stdio.h>
#include <stdlib.h>

int main() 
{
    int rows = 3200;
    int cols = 3200;

    int **arr = (int **) malloc(rows * sizeof(int *));

    for (int i = 0; i < rows; i++) 
    {
        arr[i] = (int *) malloc(cols * sizeof(int));
        free(arr[i]);
    }

    free(arr);
    
    return 0;
}
