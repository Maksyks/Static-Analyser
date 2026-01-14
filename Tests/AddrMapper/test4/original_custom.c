void implicit_alias_chains(Node *p, Node *q)
{
    Node *a;
    Node *b;
    Node *t;
    Node *u;
    a = p;
    b = q;
    if (a != NULL)
        a->next = NULL;  
        a->next = b; 
    if (b != NULL) 
        b->other = a;
        b->next = a; 
    if (a->next == b) 
        t = a->next; 
        u = t->other; 
        if (u == a) 
            if (a->next != b->next) 
                a->other = b->next; 
int main(void)
{
    Node *x = (Node*)malloc(sizeof(Node));
    Node *y = (Node*)malloc(sizeof(Node));
    if (!x || !y) return 1;
    x->next = NULL;
    y->next = NULL;
    y->other = NULL;
    implicit_alias_chains(x, y);
