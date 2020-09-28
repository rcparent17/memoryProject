#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

int tlbHits = 0;
int pageFaults = 0;
int nextFrame = 0;

/*--------------------------------------------------------------------------------------------------------------------------------
 *--------------------------------------------------------------------------------------------------------------------------------
 *-----------------------------------------------------------PAGE TABLE-----------------------------------------------------------
 *--------------------------------------------------------------------------------------------------------------------------------
 *------------------------------------------------------------------------------------------------------------------------------*/


// really isn't necessary, but inits an empty page table
struct PageTable initTable(){
	struct PageTable pt;
	pt.count = 0;
	return pt;
}


void addPage(struct PageTable* pt, struct TLB* tlb, int page, int frame, signed char data[]){
	
	if(pt->count == F_P_PT_SIZE) ptNextLRU(pt); // remove the lru if the page table is full

	// set the values of the first empty entry
	pt->entries[pt->count].pageNum = page;
	pt->entries[pt->count].frame = frame;
	memcpy(pt->entries[pt->count].page, data, F_P_PT_SIZE); // copy data into the entry

	addTlbEntry(tlb, page, frame, data); // also add the entry to the page table

	pt->removeQueue[pt->count] = page; // add the page number to the end of the remove queue
	pt->count++; // we've added an entry!
}


// removes the least recently used entry in the page table
void ptNextLRU(struct PageTable* pt){
	
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


// function to read a specific page in from the backing store
void getPage(FILE* bs, signed char buffer[], int page){
	fseek(bs, page * F_P_PT_SIZE, SEEK_SET); // go to the first byte in the page (pageNum * 256)
	fread(buffer, F_P_PT_SIZE, 1, bs); // store the following 256 bytes in the buffer
}


struct HitData pageFault(struct PageTable* pt, struct TLB* tlb, struct LogicalAddress la, FILE* bs){
	pageFaults++; // increment page fault counter
	signed char pageBuffer[F_P_PT_SIZE]; // buffer for backing store reading

	// read 256 byte page in
	getPage(bs, pageBuffer, la.page);

	// add entry and update page table
	addPage(pt, tlb, la.page, nextFrame++, pageBuffer);

	return ptHit(pt, la, la.page);
}


// return the physical address and value if there's a hit
struct HitData ptHit(struct PageTable* pt, struct LogicalAddress la, int index){

	// adjust remove queue
	int pageHit = la.page; // the page that got hit
	for(int i = pageHit; i < pt->count - 1; i++){
		pt->removeQueue[i] = pt->removeQueue[i+1]; // shift everything after left
	}
	pt->removeQueue[pt->count] = pageHit; // add it to the end

	struct HitData hd; // the physical address and value retrieved from the logical address
	hd.pAddr = (pt->entries[index].frame << 8) | la.offset; // translate the page frame left and add the offset to get the physical address
	hd.value = pt->entries[index].page[la.offset]; // retrieve the value at the specified address
	if(hd.value > 0x7F) hd.value -= F_P_PT_SIZE; // if the number's sign bit is 1 flip it

	return hd;
}


// attempt to load a page from the page table
struct HitData ptLoad(struct PageTable* pt, struct TLB* tlb, struct LogicalAddress la, FILE* bs){

	if(pt->count == 0) return pageFault(pt, tlb, la, bs); // if the page table is empty it will always result in a page fault

	for(int i = 0; i < pt->count; i++){
		if(pt->entries[i].pageNum == la.page) return ptHit(pt, la, i); // if the page of the logical address is in the page table, it's a smash hit
	}

	return pageFault(pt, tlb, la, bs); // if we haven't found the page, page fault
}


/*--------------------------------------------------------------------------------------------------------------------------------
 *---------------------------------------------------------END PAGE TABLE---------------------------------------------------------
 *-----------------------------------------------------------START  TLB-----------------------------------------------------------
 *------------------------------------------------------------------------------------------------------------------------------*/


// function to init a TLB struct
struct TLB initTLB(){
	struct TLB tlb;
	tlb.size = 0;
	return tlb;
}


// adds an entry to the TLB
void addTlbEntry(struct TLB* tlb, int page, int frame, signed char data[]){
	
	if(tlb->size == TLB_SIZE) tlbNextLRU(tlb); // if the TLB is full, remove the least recently used entry

	// set all of the details of the new entry
	tlb->removeQueue[tlb->size] = page;
	tlb->entries[tlb->size].pageNum = page;
	tlb->entries[tlb->size].frame = frame;
	memcpy(tlb->entries[tlb->size].page, data, F_P_PT_SIZE);

	tlb->size++; // we've added an entry!
}


// function to take care of removing the least recently used TLB entry if needed
void tlbNextLRU(struct TLB* tlb){
	int i;
	int removePage = tlb->removeQueue[0]; // the number of the page that needs removed
	// overwrite the lru page (removeQueue[0]) and shift everything after it left
	for(i = 0; i < tlb->size - 1; i++){
		tlb->removeQueue[i] = tlb->removeQueue[i+1];
	}

	// also remove the entry from the table by shifting everything after it left
	for(i = removePage; i < tlb->size - 1; i++){
		tlb->entries[i] = tlb->entries[i+1];
	}
	tlb->size--; // we've removed an entry!
}


// handles a hit on the TLB, rearranges removeQueue for LRU implementation
struct HitData tlbHit(struct TLB* tlb, struct LogicalAddress la, int index){

	// adjust remove queue
	int pageHit = la.page; // the page that got hit
	for(int i = pageHit; i < tlb->size - 1; i++){
		tlb->removeQueue[i] = tlb->removeQueue[i+1]; // shift everything after left
	}
	tlb->removeQueue[tlb->size] = pageHit; // add it to the end

	struct HitData hd; // the physical address and value retrieved from the logical address
	hd.pAddr = (tlb->entries[index].frame << 8) | la.offset; // translate the page frame left and add the offset to get the physical address
	hd.value = tlb->entries[index].page[la.offset]; // retrieve the value at the specified address
	if(hd.value > 0x7F) hd.value -= F_P_PT_SIZE; // if the number's sign bit is 1 flip it

	return hd;
}


// function to load a physical address from the TLB
struct HitData tlbLoad(struct TLB* tlb, struct PageTable* pt, struct LogicalAddress la, FILE* bs){
	if(tlb->size == 0) return ptLoad(pt, tlb, la, bs); // if the tlb is empty, look through the page table

	for(int i = 0; i < tlb->size; i++){
		if(tlb->entries[i].pageNum == la.page){
			tlbHits++;
			return tlbHit(tlb, la, i); // if found, it's a hit
		}
	}

	return ptLoad(pt, tlb, la, bs); // if not found, try loading the logical address the page table
}


/*--------------------------------------------------------------------------------------------------------------------------------
 *------------------------------------------------------------END TLB-------------------------------------------------------------
 *--------------------------------------------------------------------------------------------------------------------------------
 *------------------------------------------------------------------------------------------------------------------------------*/


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
	FILE* data = fopen(argv[1], "r"); // our address file
	if(data == NULL){
		printf("Unable to open file: %s\n", argv[1]);
		return 1;
	}
	FILE* bStore = fopen("BACKING_STORE.bin", "rb+"); // the backing store, which gets treated like RAM if a page fault occurs
	if(bStore == NULL){
		printf("Unable to locate BACKING_STORE.bin. It must be in the same directory as the excecutable.\n");
		return 1;
	}


	// bit masks
	u_int8_t mask8 = 0xFF;
	u_int16_t mask16 = 0xFFFF;


	// array and counter for logical addresses and their information
	struct LogicalAddress addresses[NUM_ENTRIES];
	int currentAddr = 0;


	int readBuff = 0; // serves as a buffer for reading with fscanf
	// allocate a logicalAddress for every line in the input file, and add it to our array
	while(fscanf(data, "%d", &readBuff) > 0){
		struct LogicalAddress current;
		current.original = readBuff;
		current.page = bitMask(readBuff, mask16) >> 8; // we need the 2nd from the right byte for the page, so shift the number 8 bits and mask those 8 bits
		current.offset = bitMask(readBuff, mask8); // mask the rightmost byte for the offset value
		addresses[currentAddr++] = current;
	}


	struct HitData physicalAddresses[currentAddr]; // the return values from our tlb/page table/backing store
	struct PageTable pt = initTable(); // our page table
	struct TLB tlb = initTLB(); // our TLB


	// loop through our addresses, and try to load them from the tlb. also print the virtual and physical addresses and the value at that address
	for(int i = 0; i < currentAddr; i++){
		physicalAddresses[i] = tlbLoad(&tlb, &pt, addresses[i], bStore);
		printf("original: %i     \t real: %i \t value: %i\n", addresses[i].original, physicalAddresses[i].pAddr, physicalAddresses[i].value);
	}

	// calclate our page fault rate and tlb hit rate, and print them
	float faultRate = 100.0 * (float) pageFaults / (float) NUM_ENTRIES;
	float tlbHitRate = 100.0 * (float) tlbHits / (float) NUM_ENTRIES;
	printf("\n\n\nNumber of page faults: %i \tPage fault rate: %.3f%%\n\nNumber of TLB hits: %i \t\tTLB hit rate: %.3f%%\n\n", pageFaults, faultRate, tlbHits, tlbHitRate);


	return 0;
}

