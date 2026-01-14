// function declarations
unsigned int** create_matrix(unsigned int var0, unsigned int var1);
void init_matrix(unsigned int** var0, unsigned int var1, unsigned int var2);
int main(void);
extern unsigned char* malloc(unsigned long var0);

// global variable definitions
unsigned char _str[22] = {83,117,109,32,111,102,32,100,105,97,103,111,110,97,108,32,61,32,37,100,10,0,};

unsigned int** create_matrix(unsigned int var0, unsigned int var1){
    unsigned int rows;
    unsigned int cols;
    unsigned int** m;
    unsigned int i;
    block0:
    rows = var0;
    cols = var1;
    m = ((unsigned int**)malloc(((unsigned long)((long)((int)rows))) * 8));
    i = 0;
    goto block1;
    block1:
    if (((int)i) < ((int)rows)) {
        (*(((unsigned int**)(m)) + ((long)((int)i)))) = ((unsigned int*)malloc(((unsigned long)((long)((int)cols))) * 4));
        i = (((int)i) + ((int)1));
        goto block1;
    } else {
        return m;
    }
}

void init_matrix(unsigned int** var0, unsigned int var1, unsigned int var2){
    unsigned int** m;
    unsigned int rows;
    unsigned int i;
    unsigned int* row;
    block0:
    m = var0;
    rows = var1;
    i = 0;
    goto block1;
    block1:
    if (((int)i) < ((int)rows)) {
        row = (*(((unsigned int**)(m)) + ((long)((int)i))));
        i = (((int)i) + ((int)1));
        goto block1;
    } else {
        return;
    }
}

int main(void){
    unsigned int n;
    unsigned int** mat;
    block0:
    n = 4;
    mat = create_matrix(n, n);
    init_matrix(mat, n, n);
    return 0;
}

