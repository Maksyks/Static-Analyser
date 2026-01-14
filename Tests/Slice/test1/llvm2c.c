// function declarations
int main(void);

// global variable definitions
unsigned int __const_main_arr[5] = {1,2,3,4,5,};
unsigned int __const_main_arr1[5] = {1,2,3,4,5,};

int main(void){
    unsigned int arr[5];
    unsigned int* p_sum;
    unsigned int sum;
    unsigned int i;
    block0:
    arr = __const_main_arr;
    p_sum = (&(arr[0]));
    sum = 0;
    i = 0;
    goto block1;
    block1:
    if (((int)i) < ((int)5)) {
        sum = (((int)sum) + ((int)(*(((unsigned int*)(p_sum)) + ((long)((int)i))))));
        i = (((int)i) + ((int)1));
        goto block1;
    } else {
        return 0;
    }
}