void step_add(int *p, int x) {
    if (!p) return;
    *p += x;                 
void step_xor(int *p, int mask) {
    if (!p) return;
    *p ^= mask;              
int main(void) {
    int *ptr = (int*)malloc(sizeof *ptr);
    if (!ptr) {
    *ptr = 7;
    step_add(ptr, 5);
    step_xor(ptr, 0x55);
