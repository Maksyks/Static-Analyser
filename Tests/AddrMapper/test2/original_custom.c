void both_valid_and_invalid(Node *p, int choose)
{
    Node *q = NULL;
    Node *x = NULL;
    if (p ->next != NULL)                
        if (p == NULL)      
                q = x->next->next;          
int main(void)
{
    Node *a = malloc(sizeof(Node));
    Node *b = malloc(sizeof(Node));
    if (!a || !b) return 1;
    a->next = b;
    b->next = NULL;
    a->other = NULL;
    both_valid_and_invalid(a, 1);
