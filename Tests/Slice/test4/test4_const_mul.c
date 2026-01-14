#include <stdio.h>
#include <stdlib.h>

//Slice with criterion p:17
void step_add(int *p, int x) {
    if (!p) return;
    *p += x;                 
}

void step_mul(int p, int m) {
    if (!p) return;
    p = m;                 
}

void step_xor(int *p, int mask) {
    if (!p) return;
    *p ^= mask;              
}

int main(void) {
    int *ptr = (int*)malloc(sizeof *ptr);
    if (!ptr) {
        perror("malloc failed");
        return 1;
    }

    *ptr = 7;
    printf("init:   addr=%p  val=%d\n", (void*)ptr, *ptr);

    step_add(ptr, 5);
    printf("after add: addr=%p  val=%d\n", (void*)ptr, *ptr);

    step_mul(3, 3);
    printf("after mul: addr=%p  val=%d\n", (void*)ptr, *ptr);

    step_xor(ptr, 0x55);
    printf("after xor: addr=%p  val=%d\n", (void*)ptr, *ptr);

    free(ptr);
    return 0;
}
