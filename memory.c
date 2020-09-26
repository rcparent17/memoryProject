#include <stdio.h>
#include <stdlib.h>

#define NUM_ENTRIES 1000
#define TLB_SIZE 16
#define NUM_FRAMES 256
#define F_P_PT_SIZE 256 // 2^8 size for frames, pages, and num of page table entries

struct LogicalAddress{
	u_int32_t original;
	u_int8_t  page;
	u_int8_t  offset;
};

/*
 *
 *-----------------------------------PAGE TABLE----------------------------------
 *
 */

struct PTentry{
	int pageNum; // the page number of the entry
	int frame; // the actual memory address
};

struct PageTable{
	int count; // how many pages there are
	int removeQueue[F_P_PT_SIZE]; // queue for implementing LRU paging
	struct PTentry entries[F_P_PT_SIZE]; // the page table entries
};

// really isn't necessary, but inits an empty page table
struct PageTable initTable(){
	struct PageTable pt;
	pt.count = 0;
	return pt;
}

void addPage(struct PageTable* pt, int page, int frame){
	// set the values of the first empty entry
	pt->entries[pt->count].pageNum = page;
	pt->entries[pt->count].frame = frame;
	pt->removeQueue[pt->count] = page; // add the page number to the end of the remove queue
	pt->count++; // we've added an entry!
}

void nextLRU(struct PageTable* pt){
	int i;
	int removePage = pt->removeQueue[0]; // the number of the page that needs removed
	// overwrite the lru page (removeQueue[0]) and shift everything after it left
	for(i = 0; i < pt->count - 1; i++){
		pt->removeQueue[i] = pt->removeQueue[i+1];
	}
	// also remove the entry from the table
	int removeIndex = 0;
	for(i = 0; i < pt->count; i++){
		if(pt->entries[i].pageNum == removePage){
			removeIndex = i;
			break;
		}
	}
	for(i = removeIndex; i < pt->count - 1; i++){
		pt->entries[i] = pt->entries[i+1];
	}
	pt->count--;
}

int pageFault(struct PageTable* pt, struct LogicalAddress la, FILE* bs){
	return 1;
}

int ptHit(struct PageTable* pt, struct LogicalAddress la){
	return 1;
}

int ptLoad(struct PageTable* pt, struct LogicalAddress la, FILE* bs){
	if(pt->count == 0) return pageFault(pt, la, bs); // if the page table is empty it will always result in a page fault
	for(int i = 0; i < pt->count; i++){
		if(pt->entries[i].pageNum == la.page) return ptHit(pt, la); // if the page of the logical address is in the page table, it's a smash hit
	}
	return pageFault(pt, la, bs); // if we haven't found the page, page fault
}

/*
 *--------------------------------END PAGE TABLE---------------------------------
 *----------------------------------START  TLB-----------------------------------
 */

struct TLB{
	int size;
	struct LogicalAddress entries[TLB_SIZE];
};

int addrEq(struct LogicalAddress l1, struct LogicalAddress l2){
	return 	(l1.original == l2.original) && 
			(l1.page == l2.page) && 
			(l1.offset == l2.offset);
}

u_int16_t tlbHit(struct TLB* tlb, int index){
	return 0;
}

u_int16_t tlbMiss(struct TLB* tlb){
	return 0;
}

u_int16_t tlbSearch(struct TLB* tlb, struct LogicalAddress la){
	int found = 0;
	for(int i = 0; i < tlb->size; i++){
		if(addrEq(la, tlb->entries[i])){
			found = i;
			break;
		}
	}
	if(found) return tlbHit(tlb, found);
	return tlbMiss(tlb);
}

// this function returns a 8 bit bitwise AND between a number and a mask value
u_int16_t bitMask(u_int16_t initial, u_int16_t mask){
	return initial & mask;
}

int main(int argc, char* argv[]){

	// read arguments and open files, and check that everything is where it should be	
	if(argc < 2){
		printf("You must provide a file name as a command-line argument.\n");
		return 1;
	}

	FILE* data = fopen(argv[1], "r");
	if(data == NULL){
		printf("Unable to open file: %s\n", argv[1]);
		return 1;
	}

	FILE* bStore = fopen("BACKING_STORE.bin", "rb+");
	if(bStore == NULL){
		printf("Unable to locate BACKING_STORE.bin. It must be in the same directory as the excecutable.\n");
		return 1;
	}

	// 8 bit mask
	u_int8_t mask8 = 0xFF;

	// array and counter for logical addresses and their information
	struct LogicalAddress addresses[NUM_ENTRIES];
	int currentAddr = 0;

	u_int16_t readBuff = 0; // serves as a buffer for reading with fscanf

	// allocate a logicalAddress for every line in the input file, and add it to our array
	while(fscanf(data, "%hd", &readBuff) > 0){
		struct LogicalAddress current;
		current.original = readBuff;
		current.page = bitMask(readBuff >> 8, mask8); // we need the 2nd from the right byte for the page, so shift the number 8 bits and mask those 8 bits
		current.offset = bitMask(readBuff, mask8); // mask the rightmost byte for the offset value
		addresses[currentAddr++] = current;
	}

	// used to print all gathered data
	for(int i = 0; i < currentAddr; i++){
		printf("l_addr: %d\tpage: %d  \t offset: %d\t\n", addresses[i].original, addresses[i].page, addresses[i].offset);
	}

	return 0;
}

