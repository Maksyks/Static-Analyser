#include <stdlib.h>
//Slice with criterion x:35
typedef struct Node {
    int value;
    struct Node *next;
    struct Node *other;
} Node;

void unequal_vs_alias(Node *x, Node *y)
{
    Node *r;

    if (x != y)
    {
        if (x == y) 
        {
            x->other = y;
        }
        else
        {
            if (x != NULL)
            {
                x->next = y;  
                r = x->next; 

                if (r != NULL)  
                {
                    if (r != y)
                    {
                        r->next = NULL;
                    }

                    if (x->next != y)
                    {
                        x->other = NULL;
                    }
                }
            }
        }
    }
}

int main(void)
{
    Node *a = (Node*)malloc(sizeof(Node));
    Node *b = (Node*)malloc(sizeof(Node));
    if (!a || !b) return 1;
    a->next = NULL; a->other = NULL;
    b->next = NULL; b->other = NULL;

    unequal_vs_alias(a, b);
    return 0;
}
