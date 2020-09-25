#include <stdio.h>
#include <stdlib.h>

#define NUM_ENTRIES 1000

/*
 *
 *-----------------------------------PAGE TABLE----------------------------------
 *
 */

struct PageTableEntry{
	int dirtyBit;
	int protectionBit;
	int validityBit;
	int frameNumber;
};

struct PageTable{
	struct PageTableEntry entries[NUM_ENTRIES];
};

struct PageTableEntry makeEntry(int frameNo, int prot, int valid, int dirty){
	struct PageTableEntry pte;
	pte.frameNumber = frameNo;
	pte.protectionBit = prot;
	pte.validityBit = valid;
	pte.dirtyBit = dirty;
	return pte;
}

struct PageTable makeTable(){
	struct PageTable pt;
	int frameNo = 0;
	for(int i = 0; i < NUM_ENTRIES; i++){
		pt.entries[i] = makeEntry(frameNo++, 0, 1, 0);
	}
	return pt;
}

/*
 *
 *--------------------------------END PAGE TABLE---------------------------------
 *
 */

struct logicalAddress{
	u_int16_t address;
	u_int8_t  page;
	u_int8_t  offset;
};

// this function returns a 8 bit bitwise AND between a number and a mask value
u_int8_t bitMask8(u_int32_t initial, u_int32_t mask){
	return initial & mask;
}

// the same as above but 16 bit
u_int16_t bitMask16(u_int32_t initial, u_int32_t mask){
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

	FILE* BACKING_STORE = fopen("handouts/BACKING_STORE.bin", "rb+");
	if(BACKING_STORE == NULL){
		printf("Unable to locate BACKING_STORE.bin. It must be in the same directory as the excecutable.\n");
		return 1;
	}

	// data masks for 32 bit numbers in the adresses file
	u_int32_t addrMask = 0xFFFF0000;
	u_int32_t pageMask = 0x0000FF00;
	u_int32_t ofstMask = 0x000000FF;

	// array and counter for logical addresses and their information
	struct logicalAddress addresses[NUM_ENTRIES];
	int currentAddr = 0;

	u_int32_t readBuff = 0; // serves as a buffer for reading with fscanf

	// allocate a logicalAddress for every line in the input file, and add it to our array
	while(fscanf(data, "%u", &readBuff) > 0){
		struct logicalAddress current;
		current.address = bitMask16(readBuff, addrMask);
		current.page = bitMask8(readBuff, pageMask);
		current.offset = bitMask8(readBuff, ofstMask);
		addresses[currentAddr++] = current;
	}

	for(int i = 0; i < currentAddr; i++){
		printf("addr: %d\tpage: %d\t offset: %d\t\n", addresses[i].address, addresses[i].page, addresses[i].offset);
	}

	return 0;
}

