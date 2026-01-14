void chain_guards(Node *head)
{
    Node *a;
    Node *b;
    Node *c;
    Node *d;
    a = head;
    if (a != NULL) 
        b = a->next; 
        if (b != NULL)  
            c = b->other; 
            if (c == NULL) 
                a->other = b; 
            a->other = c->next; 
            if (a->other != NULL) 
                if (a->other->next == NULL)
                    d = a->other->next; 
int main(void)
{
    Node *x = (Node*)malloc(sizeof(Node));
    Node *y = (Node*)malloc(sizeof(Node));
    if (!x || !y) return 1;
    x->next = y;
    x->other = NULL;
    y->next = NULL;
    y->other = NULL;
    chain_guards(x);
