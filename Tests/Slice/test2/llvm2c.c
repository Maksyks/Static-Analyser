// function declarations
int main(void);
unsigned int sum_array(unsigned int* var0, unsigned int var1);

// global variable definitions
unsigned int __const_main_a[5] = {1,2,3,4,5,};
unsigned char _str[10] = {83,117,109,32,61,32,37,100,10,0,};

int main(void){
    unsigned int a[5];
    block0:
    a = __const_main_a;
    sum_array(&(a[0]), 5);
    return 0;
}

unsigned int sum_array(unsigned int* var0, unsigned int var1){
    unsigned int* arr;
    unsigned int n;
    unsigned int sum;
    unsigned int* p;
    unsigned int i;
    block0:
    arr = var0;
    n = var1;
    sum = 0;
    p = arr;
    i = 0;
    goto block1;
    block1:
    if (((int)i) < ((int)n)) {
        sum = (((int)sum) + ((int)(*(((unsigned int*)(p)) + ((long)((int)i))))));
        i = (((int)i) + ((int)1));
        goto block1;
    } else {
        return 0;
    }
}

