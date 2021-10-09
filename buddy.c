
#include "buddy.h"
#include <sys/mman.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

//Define your Constants here

#define  MIN 5
// size of a page i.e 4KB
#define SIZE 4096
// We have 8 levels in total
#define LEVELS 8
//array with the respective level sizes
const int LevelSizes[8] = {32,64,128,256,512,1024,2048,4096};

int totalAllocated = 0;
enum AEflag { Free, Taken };

struct head {
	enum AEflag status;
	short int level;
	struct head* next;
	struct head* prev;
};

//array that will store linked lists to track blocks
struct head	* flists[LEVELS] = {NULL};

struct head * HEAD;
//Complete the implementation of new here
struct head *new() {
	flists[LEVELS] = malloc(LEVELS*sizeof(struct head));
	// create  a page of default size 4096 bytes = 4KB
	// First param address is NULL so we allow the os to give us a page at any address
	// 4096 is the size of the page we want
	// PROT_READ|PROT_WRITE because we want to be able to read and write to the memory
	// MAP_PRIVATE since we dont share the memory with other processes.
	// MAP_ANONYMOUS because no file is mapped
	HEAD = mmap(NULL,SIZE,PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
	
	// Check for failure of assigning page
	if(HEAD == MAP_FAILED){
		printf("Failed to allocate memory.");
		return NULL;
	}
	assert(((long int)HEAD & 0xfff)==0);
	//level is 7 since it is the full page
	HEAD->level = 7;
	//nothing is stored so it is empty
	HEAD->status = Free;

	flists[7] = HEAD;
	return HEAD;

}

//Complete the implementation of level here
int level(int req) {

	// we need to determine the size of the head structure so we can add it to the size of memory we allocated
	int headSize = sizeof(struct head);

	//the total size required to satisfy the user is req + headSize
	int totalSize = req + headSize;

	//loop through LevelSizes array and return index of the first block size big enough to hold totalSize
	for(int i = 0; i <LEVELS;i++){
		if(LevelSizes[i] >= totalSize){
			return i;
		}
	}

}
//method will return the size a block given the start and end pointers
int getSize(struct head*h1, struct head*h2){

	return (void*)h2-(void*)h1 ;
}

//The length function will tell us how many links are in our linked list at an index of flists
int length(int level){
	struct head * curr = flists[level];
	int len = 1;
	if(curr==NULL){
		return 0;
	}
	if(curr->next == NULL){
		return 1;
	}

	else{
		while(curr->next != NULL){
			curr = curr->next;
			len++;
		}
	}

	return len;
}
// a Function that will insert a new link into the linked list at the respective level
void insert(struct head * current, int level){

	struct head* temp =  flists[level];
	if(temp!=NULL){
		temp->prev = current;
		current->next = temp;
		flists[level] = current;
		// current->status = Taken;
	}
	else{
		flists[level] = current;
	}
	
	
}
// A function that will return the block the
struct head* availableOnLevel(int level){
	struct head* temp = flists[level];
	if(temp->next == NULL){
		return temp;
	}
	else{
		while(temp->next != NULL){
			if(temp->status == Free){
				return temp;
			}
			temp = temp->next;
		}

		if(temp->status == Free){
				return temp;
			}
	}
	return NULL;
}
// A function that will tell us if a block is available on a given level.
int isBlockOnLevelAvailable(int level){
	struct head * h = flists[level];
	if(h == NULL){
		return 0;
	}
	else if(h->next == NULL && h->status == Free){
		return 1;
	}
	else{
		while(h->next != NULL){
			h=h->next;
			if(h->status == Free){
				return 1;
			}
	}
	}
	
	return 0;
}
struct head * allocate(int level,int splitThisLevel){
	//get the level we want to split
	struct head* currentHead = flists[splitThisLevel];
	// the while loop will ensure we get the first unallocated block on the level
	while(currentHead->next != NULL){
		currentHead = currentHead->next;
	}
	//deallocating the respective pointer once we remove it from the level
	if(currentHead->prev != NULL){
		struct head * previous = currentHead->prev;
		previous->next = NULL;
		currentHead->prev = NULL;
	}
	// if the level is empty after block is removed, set it to NULL
	else{
		flists[splitThisLevel] = NULL;
	}
	
	//Number of times we will need to split a block on a level to achieve a block of desired size
	int NumberOfSplits = splitThisLevel - level;

	struct head * newHead = currentHead;
	//Split NumberOfSplits times and always update the level and status
	for(int i = 0; i < NumberOfSplits;i++){
		struct head * child = split(newHead);
		child->level = splitThisLevel - (i+1);
		child->status = Free;
		newHead->level = child->level;
		insert(child,child->level);
	}
	//update flists and update its status
	
	insert(newHead,level);
	
	//we return flists[level] since the split is always added to the front of the Linked List
	return flists[level];
	
}

//Complete the implementation of balloc here
void* balloc(size_t size) {
	
	//if the size is  we return a null pointer
	if(size == 0 ){
		return NULL;
	}
	// If the user did not call new(), we need to do so and allocate a page
	if(HEAD == NULL){
		//create the new page
		new();
	}
	
	//get the level we need in order to store this size
	int requiredLevel =  level(size);
	// available will check if there are actually blocks to split etc
	int available = 0;
	// loop through all possible levels that can be split and check if there is available block
	// if there is, we change the value of available and if not it will remain 0
	for(int i = requiredLevel; i <LEVELS;i++){
		if(isBlockOnLevelAvailable(i)){
			available = 1;
		}
	}
	// if we are unable to find a block to match the request,i.e memory is full
	if(available == 0){
		printf("The size %ld you are requesting is currently unavailable.\n",size);
		return NULL;
	}
	
	//if flists at the required level has a pointer , return it
	struct head * temp = flists[requiredLevel];
	if(isBlockOnLevelAvailable(requiredLevel)){
		while(temp->next != NULL){
			if(temp->status == Free){
				break;
			}
			temp = temp->next;
		}
		temp->status = Taken;

		return hide(temp);
	}
	// do the necessary splitting etc and update flists
	else{
		// find first block that is not null and unallocated and big enough
		// check if flists at the current index is empty. 
		// check if it's level is NULL
		if(flists[requiredLevel]==NULL || !isBlockOnLevelAvailable(requiredLevel) ){
			//Either find blocks larger and split them 
			for(int i = requiredLevel+1; i < LEVELS;i++){
				//loop from level ahead of required level and check if any blocks are available
				struct head * toSplit= flists[i];
				if(toSplit != NULL ){
					struct head* answer = allocate(requiredLevel,toSplit->level);
					answer -> status = Taken;
					return hide(answer);
				}
				
			}
		}

	}

}

//Complete the implementation of bfree here
void bfree(void* memory) {
	//Make sure the incoming pointer is not null
	if(memory == NULL){
		return;
	}
	// magic the incoming pointer to point to the head of the block
	struct head * toFree = magic(memory);
	//level of the block we want to free
	int level =  toFree->level;
	// set the status to free and it automatically be free in flists and available to allocate
	toFree->status = Free;
	// We now need to check if the blocks buddy is free and if it is we can merge them and change their level
	for(int i = level; i<LEVELS-1;i++){
		struct head * bud =  buddy(toFree);
		if(bud->status == Free){
			//merge the blocks
			struct head * Merged = primary(bud);
			//if the length on a level is 2 , it means only the block and it's buddy are there
			// So we can set it to null and insert the merged block into one level higher
			if(length(i)==2){
				flists[i] = NULL;
				Merged -> level = i+1;
				Merged->next = NULL;
				Merged->prev = NULL;
				Merged ->status = Free;
				insert(Merged,i+1);
				toFree = Merged;
			}

			else{
				// we check if the block and it's buddy are not at the start of flists
				if(Merged->prev!=NULL){
					struct head* Previous = Merged->prev;
					Previous->next = NULL;
					Merged->level = i+1;
					Merged->next = NULL;
					Merged->prev = NULL;
					Merged->status = Free;
					insert(Merged,i+1);
					toFree = Merged;
				}
				// if the merged block is at the start of flists we update it like this
				else{
					// the block to be placed in the front will always be .next.next
					struct head* Previous = Merged->next->next;
					//set previous to null since it will be the first block in the list now
					Previous->prev = NULL;
					flists[i] = Previous;
					Merged->level = i+1;
					Merged->next = NULL;
					Merged->prev = NULL;
					Merged->status = Free;
					// insert the merged block one level above
					insert(Merged,i+1);
					toFree = Merged;
				}
			}
		}
		
	}
	


}

//Helper Functions
struct head* buddy(struct head* block) {
	int index = block->level;
	long int mask = 0x1 << (index + MIN);
	return (struct head*)((long int)block ^ mask);

}

struct head* split(struct head* block) {
	int index = block->level - 1;
	int mask = 0x1 << (index + MIN);
	return (struct head*)((long int)block | mask);
}

//used to merge blocks
struct head* primary(struct head* block) {
	int index = block->level;
	long int mask = 0xffffffffffffffff << (1 + index + MIN);
	return (struct head*)((long int)block & mask);
}

void* hide(struct head* block) {
	return (void*)(block + 1);
}


struct head* magic(void* memory) {
	return ((struct head*)memory - 1);
}
	


void dispblocklevel(struct head* block){
	printf("block level = %d\n",block->level);
}
void dispblockstatus(struct head* block){
	printf("block status = %d\n",block->status);
}

void blockinfo(struct head* block){
	printf("===================================================================\n");
	dispblockstatus(block);
	dispblocklevel(block);
	printf("start of block in memory: %p\n", block);
	printf("size of block in memory: %ld in bytes\n",sizeof(struct head));
	printf("===================================================================\n");
}


void PrintFlists(){
	printf("===================================================================\n \n");
	for(int i = 0; i <LEVELS;i++){
		printf("Flists at %d is : %p. Number of links is: %d \n",i,(void *)flists[i],length(i));
	}
	printf("===================================================================\n \n");
	
}
void testNewFunction(struct head * head){
	printf("===================================================================\n");
	printf("Testing the new() function:\n");
	assert(head == HEAD);
	//initially the block is free
	assert(head->status == 0);
	// block should be on level 7
	assert(head->level == 7);
	//flists at the largest level should be the entire block initially or if balloc has never been called
	assert(flists[7]==head);
	printf("All new() tests passed!\n");
	printf("===================================================================\n");
	printf("\n");
}

void testLevelFunction(){
	printf("===================================================================\n");
	printf("Testing the level function: \n");
	printf("===================================================================\n");
    int leveltest1 = level(0);
    // if we ask what level 0 bytes should be on, it's level 0
    assert(leveltest1 == 0);
    int leveltest2 = level(36);
    // 36+24 = 60 so level 1 should be the answer
    assert(leveltest2 == 1);
    int leveltest3 = level(3300);
    // 3300+24 = 3324 which is level 7
    assert(leveltest3 == 7);
    printf("All Level Tests Passed!\n");
	printf("===================================================================\n \n");
}

void testSplitFunction(struct head* head){
	printf("===================================================================\n");
	printf("Testing the splitting function: \n");
	printf("===================================================================\n");
	struct head* sp = split(head);
    // checking heads are the same
    assert(head == head);
    // check that splitting gives the same results
    assert((void*)sp == (void*)split(head));
    // Distance betweem head and returned pointer after one split is 2048
    assert(getSize(head,sp)==2048);
    //merge them again
	primary(sp);
    printf("All split Tests Passed!\n");
	printf("===================================================================\n \n");
}

void testBuddyFunction(struct head* head){
	printf("===================================================================\n");
	printf("Testing the buddy function: \n");
	printf("===================================================================\n");
    struct head* sp = split(head);
	sp->level = 6;
	struct head* bud = buddy(sp);
	// after we split ,the buddy of the block we split is the result of the split
	assert((void*)bud == (void*)head);
	printf("All Buddy Tests Passed!\n");
	primary(sp);
	printf("===================================================================\n \n");

}

void testMergeFunction(struct head* head){
	printf("===================================================================\n");
	printf("Testing the merge function: \n");
	printf("===================================================================\n");
	struct head * child = split(head);
	struct head* newHead = primary(child);
	// splitting and then merging should point to the same struct before we split
	assert((void*)newHead == (void*)head);
	assert(getSize(head,newHead) == 0);
	printf("All merge Tests Passed!\n");
	printf("===================================================================\n \n");
}

void testMagicAndHideFunction(struct head* head){
	printf("===================================================================\n");
	printf("Testing the magic function: \n");
	printf("===================================================================\n");
	struct head* newHead = hide(head);
	// the magic of what we hid, should be the same as the original
	assert((void *)head==(void *)magic(newHead));
	printf("All magic Tests Passed!\n");
	printf("===================================================================\n \n");

}

void testGivenFunctions(){
    struct head* h = new();
    printf("Testing All the Given Functions!\n");
    testLevelFunction();
    testSplitFunction(h);
    testBuddyFunction(h);
    testMergeFunction(h);
    testMagicAndHideFunction(h);
	printf("All given helper functions work!\n");
	printf("===================================================================\n \n");

}
void test(){
	struct head* head = new();
	testNewFunction(head);
	
}
void testBallocAndBfree(){
	printf("===================================================================\n");
	printf("Testing Balloc and Bfree function: \n");
	printf("===================================================================\n");
	// this will spit and give us two blocks of size 2048
	void *temp = balloc(2000); 
	// temp van't be null since it's what we return
    assert(temp != NULL);
	//flists[6] is the first element of the list and is the same as temp
    assert(flists[6] != NULL);
	// status should be 1 since it is Taken
    assert(flists[6]->status == 1);
	// checking our function hides the head of the given block
    assert(hide(flists[6]) == temp);
	//checking the level is correct
    assert(flists[6]->level == 6);
	//the second link on level6 should be there with status 0 
	assert(flists[6]->next!=NULL);
	//checking the status of second link on level 6
	assert(flists[6]->next->status==0);
	//from level 0 to5 all should be null
    for (int i = 0; i < 6; ++i) {
        assert(flists[i] == NULL);
    }
	//flists at 7 should be null since we split the block
	assert(flists[7]== NULL);

	void* b1 = balloc(1);
	void* b2 = balloc(2);
	void* b3 = balloc(3);
	// I should have 4 blocks on level 0 and zero blocks on level 1
	assert(length(0) == 4);
	assert(length(1) == 0);
	for(int i =2;i<7;i++){
		// every block from level 2 to 6 should only have one block 
		assert(length(i)==1);
		// the block should not be null
		assert(flists[i]!= NULL);
		// all the blocks should be free except for level 6 since that was the first block we allocated
		if(i==6)
			assert(flists[i]->status==1);
			//all other blocks should be free
		else
			assert(flists[i]->status==0);
	}
	
	// If i call bfree on b3 , i should have  2 blocks on level 0 and 1 on level 1 since i free it and it gets merged with it's primary and moved up a level
	bfree(b3);
	assert(length(0)==2);
	assert(length(1)==1);

	//calling bfree of b1 and b2 will cause all the unallocated blocks on bigger levels to get merged with their primary and move up a level
	// the result will be two blocks on level 6 since there is an allocated block on level 6 and we can't merge it 
	bfree(b1);
	bfree(b2);
	for(int i=0; i<6;i++){
		// all levels from 0-5 inclusive are empty and thus NULL
		assert(length(i)==0);
		assert(flists[i]==NULL);
	}

	void * b = balloc(0);
	//calling balloc of 0 should return null 
	assert(b == NULL);

	

    
	printf("All Balloc and Bfree Tests Passed!\n");
	printf("===================================================================\n \n");


}




