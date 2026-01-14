#include <stdio.h>
//Slice with criterion sum:12
int main() {
    int arr[5]    = {1, 2, 3, 4, 5};
    int arr1[5]    = {1, 2, 3, 4, 5};
    int *p_sum    = arr;
    int *p_prod   = arr1;
    int sum  = 0;
    int prod = 1;

    for (int i = 0; i < 5; ++i) {
        sum  += *(p_sum  + i);
    }

    for (int i = 0; i < 5; ++i) {
        prod *= *(p_prod + i);
    }

    return 0;
}
