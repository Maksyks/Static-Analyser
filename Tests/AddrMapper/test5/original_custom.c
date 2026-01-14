void unequal_vs_alias(Node *x, Node *y)
{
    Node *r;
    if (x != y)
        if (x == y) 
            if (x != NULL)
                x->next = y;  
                r = x->next; 
                if (r != NULL)  
                    if (x->next != y)
                        x->other = NULL;
int main(void)
{
    Node *a = (Node*)malloc(sizeof(Node));
    Node *b = (Node*)malloc(sizeof(Node));
    if (!a || !b) return 1;
    a->next = NULL; a->other = NULL;
    unequal_vs_alias(a, b);
