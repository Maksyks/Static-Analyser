#include <stdio.h>
//Slice with criterion sum:7
int sum_array(int arr[], int n) {
    int sum = 0;
    int *p = arr; 
    for (int i = 0; i < n; ++i) {
        sum += *(p + i);  
    }
    return sum;
}

int main(void) {
    int a[5] = {1, 2, 3, 4, 5};
    int result = sum_array(a, 5);
    printf("Sum = %d\n", result);
    return 0;
}
