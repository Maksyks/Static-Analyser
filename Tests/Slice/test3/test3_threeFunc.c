#include <stdio.h>
//Slice with criterion maxp:19
int* find_min(int *p, int n) {
    if (n <= 0) return p;
    int *minp = p;  
    for (int i = 1; i < n; ++i) {
        if (*(p + i) < *minp) {
            minp = p + i; 
        }
    }
    return minp;
}

int* find_max(int *p, int n) {
    if (n <= 0) return p;
    int *maxp = p;      
    for (int i = 1; i < n; ++i) {
        if (*(p + i) > *maxp) {
            maxp = p + i;   
        }
    }
    return maxp;
}

int count_greater(int *p, int n, int key) {
    int cnt = 0;
    for (int i = 0; i < n; ++i) {
        if (*(p + i) > key) {
            ++cnt;
        }
    }
    return cnt;
}

int main(void) {
    int arr[6] = {3, 9, 1, 7, 5, 2};
    int n = 6;
    int key = 4;

    int *minp = find_min(arr, n);
    int *maxp = find_max(arr, n);
    int cnt = count_greater(arr, n, key);

    printf("Min = %d at index %ld\n", *minp, minp - arr);
    printf("Max = %d at index %ld\n", *maxp, maxp - arr);
    printf("Elements > %d: %d\n", key, cnt);

    return 0;
}
