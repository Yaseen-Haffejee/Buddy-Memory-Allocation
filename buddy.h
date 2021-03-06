#include <stddef.h>
#ifndef buddy_h
# define buddy_h
#define LEVELS 8
//put your additional function headers here

struct head *new();
struct head *buddy(struct head*);
struct head *split(struct head*);
struct head *primary(struct head*);
void *hide(struct head*);
struct head *magic(void*);
int level(int);
void* balloc(size_t size);
void bfree(void* memory);
void dispblocklevel(struct head*);
void dispblockstatus(struct head*);
void blockinfo(struct head*);
void test();
void testGivenFunctions();
void testBallocAndBfree();
#endif


