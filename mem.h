/************************************CONSTANTS*************************************/


#define NUM_ENTRIES 1000
#define TLB_SIZE 16
#define F_P_PT_SIZE 256
#define NUM_FRAMES F_P_PT_SIZE


/***********************************STRUCTURES*************************************/


// structure for wrapping the physical address and value of a logical address
struct HitData{
	int pAddr; // the physical address
	int value; // the value at that address
};

// stores info about a logical address
struct LogicalAddress{
	int original; // the original number read in
	int page; // the page number of the logical address
	int offset; // the offset value of the logical address
};

// structure for page table entries
struct PTentry{
	int pageNum; // the page number of the entry
	int frame; // the actual memory address
	signed char page[F_P_PT_SIZE]; // the memory used to store the actual page data
};

// page table struct
struct PageTable{
	int count; // how many pages there are
	int removeQueue[NUM_FRAMES]; // queue for implementing LRU paging
	struct PTentry entries[NUM_FRAMES]; // the page table entries
};

// tlb struct
struct TLB{
	int size; // how many elements are in the TLB
	int removeQueue[TLB_SIZE]; // queue for implementing LRU paging
	struct PTentry entries[TLB_SIZE]; // the entries in the TLB
};


/*******************************PAGE TABLE FUNCTIONS*******************************/


// really isn't necessary, but inits an empty page table
struct PageTable initTable();
// adds a page entry to the page table
void addPage(struct PageTable* pt, struct TLB* tlb, int page, int frame, signed char data[]);
// if the page table is full, this will be used to remove the least recently used page
void ptNextLRU(struct PageTable* pt);
// function to read a specific page in from the backing store
void getPage(FILE* bs, signed char buffer[], int page);
// returns the physical address after handling page fault stuff
struct HitData pageFault(struct PageTable* pt, struct TLB* tlb, struct LogicalAddress la, FILE* bs);
// returns the physical address of a page
struct HitData ptHit(struct PageTable* pt, struct LogicalAddress la, int index);
// function to load a page. will cause a page fault if not found
struct HitData ptLoad(struct PageTable* pt, struct TLB* tlb, struct LogicalAddress la, FILE* bs);


/**********************************TLB FUNCTIONS***********************************/


// function to init a TLB struct
struct TLB initTLB();
// adds an entry to the TLB
void addTlbEntry(struct TLB* tlb, int page, int frame, signed char data[]);
// function to take care of removing the least recently used TLB entry if needed
void tlbNextLRU(struct TLB* tlb);
// handles a hit on the TLB, rearranges removeQueue for LRU implementation
struct HitData tlbHit(struct TLB* tlb, struct LogicalAddress la, int index);
// function to load a physical address from the TLB
struct HitData tlbLoad(struct TLB* tlb, struct PageTable* pt, struct LogicalAddress la, FILE* bs);
