void rotate_left(Node *root)
{
    Node *x, *y;
    x = root->rt;        
    if (x != NULL) {     
        y = x->lt;       
        root->rt = y;  
int main(void)
{
    Node *root = malloc(sizeof(Node));
    Node *B    = malloc(sizeof(Node));
    root->rt = B;
    B->lt = B->rt = NULL;
    rotate_left(root);
