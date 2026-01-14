// function declarations
unsigned int* find_max(unsigned int* var0, unsigned int var1);
int main(void);

// global variable definitions
unsigned int __const_main_arr[6] = {3,9,1,7,5,2,};
unsigned char _str[23] = {77,105,110,32,61,32,37,100,32,97,116,32,105,110,100,101,120,32,37,108,100,10,0,};
unsigned char _str_1[23] = {77,97,120,32,61,32,37,100,32,97,116,32,105,110,100,101,120,32,37,108,100,10,0,};
unsigned char _str_2[19] = {69,108,101,109,101,110,116,115,32,62,32,37,100,58,32,37,100,10,0,};

unsigned int* find_max(unsigned int* var0, unsigned int var1){
    unsigned int* p;
    unsigned int n;
    unsigned int* maxp;
    unsigned int i;
    block0:
    p = var0;
    n = var1;
    if (((int)n) <= ((int)0)) {
        return 0;
    } else {
        maxp = p;
        i = 1;
        goto block2;
    }
    block2:
    if (((int)i) < ((int)n)) {
        if (((int)(*(((unsigned int*)(p)) + ((long)((int)i))))) > ((int)(*maxp))) {
            maxp = (&(*(((unsigned int*)(p)) + ((long)((int)i)))));
            goto block5;
        } else {
            goto block5;
        }
    } else {
        return 0;
    }
    block5:
    i = (((int)i) + ((int)1));
    goto block2;
}

int main(void){
    unsigned int arr[6];
    unsigned int n;
    block0:
    arr = __const_main_arr;
    n = 6;
    find_max(&(arr[0]), n);
    return 0;
}

