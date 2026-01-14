// function declarations
int main(void);
extern unsigned char* malloc(unsigned long var0);
void step_add(unsigned int* var0, unsigned int var1);
void step_xor(unsigned int* var0, unsigned int var1);

// global variable definitions
unsigned char _str[14] = {109,97,108,108,111,99,32,102,97,105,108,101,100,0,};
unsigned char _str_1[25] = {105,110,105,116,58,32,32,32,97,100,100,114,61,37,112,32,32,118,97,108,61,37,100,10,0,};
unsigned char _str_2[28] = {97,102,116,101,114,32,97,100,100,58,32,97,100,100,114,61,37,112,32,32,118,97,108,61,37,100,10,0,};
unsigned char _str_3[28] = {97,102,116,101,114,32,109,117,108,58,32,97,100,100,114,61,37,112,32,32,118,97,108,61,37,100,10,0,};
unsigned char _str_4[28] = {97,102,116,101,114,32,120,111,114,58,32,97,100,100,114,61,37,112,32,32,118,97,108,61,37,100,10,0,};

int main(void){
    unsigned int* ptr;
    block0:
    ptr = ((unsigned int*)malloc(4));
    if (ptr != 0) {
        (*ptr) = 7;
        step_add(ptr, 5);
        step_xor(ptr, 85);
        return 0;
    } else {
        return 0;
    }
}

void step_add(unsigned int* var0, unsigned int var1){
    unsigned int* p;
    unsigned int x;
    unsigned int* var4;
    block0:
    p = var0;
    x = var1;
    if (p != 0) {
        var4 = p;
        (*var4) = (((int)(*var4)) + ((int)x));
        return;
    } else {
        return;
    }
}

void step_xor(unsigned int* var0, unsigned int var1){
    unsigned int* p;
    unsigned int mask;
    unsigned int* var4;
    block0:
    p = var0;
    mask = var1;
    if (p != 0) {
        var4 = p;
        (*var4) = ((*var4) ^ mask);
        return;
    } else {
        return;
    }
}

