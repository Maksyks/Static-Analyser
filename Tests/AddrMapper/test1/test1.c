#include <stdlib.h>
//Slice with criterion root:19
typedef struct Node {
    int key;
    struct Node *lt;
    struct Node *rt;
} Node;

void rotate_left(Node *root)
{
    Node *x, *y;

    x = root->rt;        
    if (x != NULL) {     
        y = x->lt;       

        x->lt = y->rt;   
        y->rt = root;    
        root->rt = y;  
    }
}

int main(void)
{
    Node *root = malloc(sizeof(Node));
    Node *A    = malloc(sizeof(Node));
    Node *B    = malloc(sizeof(Node));

    root->lt = A;
    root->rt = B;
    A->lt = A->rt = NULL;
    B->lt = B->rt = NULL;

    rotate_left(root);
    return 0;
}
