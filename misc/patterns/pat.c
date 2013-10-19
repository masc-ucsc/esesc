/* pattfilter.c
 * J. L. Briz briz@unizar.es 2012
 * Input format: 
 * tid type pc data_address 
 * type: 0 (global ld), 1 (global st), 2 (shared ld), 3 (shared st) 
 */ 

#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<inttypes.h>
#include<assert.h>


// dimensions
#define BUFSIZE 512
#define STRTABSIZE 7
 
// pc addresses are [0..]40000xxx
// We use 410...xxx addresses to identify ld/st to the shared space
#define SHARED 0x0000000008000000

#define OVERFLOW -1
#define COLDMISS 0  // We only have cold misses in this lst
#define HIT 1


#define GL_LD 0 // Global Ld
#define GL_ST 1 // Global St
#define SH_LD 2 // Shared Ld
#define SH_ST 3 // Shared St

#define NULLIFY(X, Y) ((X)? (float)(Y)/(X) : 0)



// Statistics
typedef struct strEntry {
	int32_t strideLength;
	float strideHits;
}STR_ENTRY;

typedef struct strStatistics {
	int32_t globalLoads;  
	int32_t globalStores;
	int32_t sharedLoads; 
	int32_t sharedStores;
	int32_t misc;
	STR_ENTRY strGlLd[STRTABSIZE]; 	// Strides:	Global Loads
	STR_ENTRY strGlSt[STRTABSIZE]; 	// 		Gobal Stores
	STR_ENTRY strShLd[STRTABSIZE]; 	// 		Shared Loads
	STR_ENTRY strShSt[STRTABSIZE]; 	//		Shared Stores 
	int32_t freqTable[10]; 		// per-unit signatures hitting a pattern 0.x times 
}STR_STATISTICS; 

// load-store table entry
typedef struct lstEntry {
	uint64_t pc;
	uint64_t tid;
	uint64_t signature;
	uint32_t refType; // global or shared ld or st
	uint64_t lastAddr;
	int32_t stride;
	int32_t conf;   // confidence [0..3]
	int32_t dcount; // dynamic count of this instruction
	int32_t valid;
	STR_ENTRY strPcStats[STRTABSIZE];
}LST_ENTRY;


// Global statistic
 	STR_STATISTICS stats;

uint32_t nearest_pow (uint32_t num)
{
    uint32_t n = num > 0 ? num - 1 : 0;

    	n |= n >> 1;
    	n |= n >> 2;
    	n |= n >> 4;
    	n |= n >> 8;
    	n |= n >> 16;
    	n++;

    	return n;
}

uint32_t hash(	uint64_t signature, 
		uint32_t lstNumEntries)
{
	register int64_t r = signature;
	r >>= 40; r ^= signature;
	return ((int32_t) (r & (lstNumEntries-1))); 
}

LST_ENTRY** alloctable(	uint32_t rows, 
			uint32_t cols, 
			uint32_t type_size)
{
	uint32_t totsize = rows*cols*type_size;
    	LST_ENTRY** irows = (LST_ENTRY**)calloc(rows*sizeof(LST_ENTRY**) + totsize, 1);
    	LST_ENTRY* jcols = (LST_ENTRY*)(irows+rows);
	int32_t i;

    	for(i=0;i<rows;i++)
        	irows[i] = &jcols[i*cols];
    	return irows;
}

void incStrideCounter(int32_t stride, STR_ENTRY *strTable)
{
	if(stride < 0)
		strTable[0].strideLength++; 
	else if(stride == 0)	// scalar pattern
		strTable[1].strideLength++; 
	else if(stride <32)
		strTable[2].strideLength++; 
	else if(stride <64)
		strTable[3].strideLength++; 
	else if(stride <128)
		strTable[4].strideLength++; 
	else if(stride <= 256)
		strTable[5].strideLength++; 
	else 			// stride pattern if cache line size > 256 B
		strTable[6].strideLength++; 
		;
}

void setLstEntry(LST_ENTRY *current, LST_ENTRY *ref)
{
		current->pc = ref->pc;
		current->tid = ref->tid;
		current->signature = ref->signature;
		current->refType = ref->refType;
		current->lastAddr = ref->lastAddr;
		current->stride = 0;
		current->conf = 0;
		current->dcount = 1;
		current->valid = 1;
}

void lstUpdate(LST_ENTRY *current, LST_ENTRY *ref)
{
	int32_t newStride; 
	register int32_t conf; 

	assert(!ref->valid);
	current->dcount++;	
	conf = current->conf;
	newStride = ref->lastAddr -  current->lastAddr;
	current->lastAddr = ref->lastAddr;

// Pattern recognition
// confidence 0-1: no pattern, update; 2-3: avemus pattern
// confidence saturates at both ends (0, 3)
	if(newStride == current->stride){
		if(conf < 3 ) 
			conf++;
		if(conf >= 2 ) {
			switch(ref->refType){
				case GL_LD:
					incStrideCounter(newStride, stats.strGlLd);
					break;	
				case GL_ST:
					incStrideCounter(newStride, stats.strGlSt);
					break;	
				case SH_LD:
					incStrideCounter(newStride, stats.strShLd);
					break;	
				case SH_ST:
					incStrideCounter(newStride, stats.strShSt);
					break;	
				default :
					;
			}	
			incStrideCounter(newStride, current->strPcStats);
		}
	}else{
		if(conf > 0 ) 
			conf--;
		if(conf != 2 ) 
			current->stride = newStride;	
	}
	current->conf = conf;
}

void mksignature(LST_ENTRY *ref)
{ 
	register int64_t signature;
// {0, 1 } is global space; {2, 3} is shared space
	signature = (ref->refType > 1 ? ref->pc | SHARED : ref->pc);
	signature |= (ref->tid << 32); 	// tid significant bits are in the low half
	ref->signature = signature;
}
int32_t lstLookup(	LST_ENTRY **current,
		 	LST_ENTRY **lst, 
			int32_t lstNumEntries, 
			int32_t depth, 
			LST_ENTRY *ref)
// lstLookup returns in current a pointer to the proper lst entry
// either because it contains the reference (hit) 
// or because it was free (cold miss) in which case
// it's initialized with the new reference.
// The lst is a hash table where each element is the
// header of a list with elements matching the same key.
// The list is implemented as a static array of size "depth"
// The function returns OVERFLOW if there is
// neither a hit nor a free element in the list
// associated to the proper key.
{
	register uint64_t signature;
	register uint32_t key, i;
	
	key = hash(ref->signature, lstNumEntries);
	for(i=0; i<depth; i++){
		*current = &lst[key][i];
		if(!(*current)->valid){ //invalid: allocate new entry
			setLstEntry(*current, ref);
			return COLDMISS;
		} else 
			if((*current)->signature == ref->signature){
			//if(((*current)->tid == ref->tid) && ((*current)->pc = ref->pc)){
				lstUpdate(*current, ref);
				return HIT;
			}		// valid but different entry, go on
	
	}
	return OVERFLOW;
}

void updateCounters(LST_ENTRY *ref)
{
	switch(ref->refType){
		case GL_LD	: stats.globalLoads++; break;
		case GL_ST	: stats.globalStores++; break;
		case SH_LD	: stats.sharedLoads++; break;
		case SH_ST	: stats.sharedStores++; break;
		default		: stats.misc++;
	}
}

void updateFreq(float freq)
// access global stats structure
{	
	if(freq  < 0.01)
		stats.freqTable[0]++;
	else if (freq <= 0.1)
		stats.freqTable[1]++;
	else if (freq <= 0.2)
		stats.freqTable[2]++;
	else if (freq <= 0.3)
		stats.freqTable[3]++;
	else if (freq <= 0.4)
		stats.freqTable[4]++;
	else if (freq <= 0.5)
		stats.freqTable[5]++;
	else if (freq <= 0.6)
		stats.freqTable[6]++;
	else if (freq <= 0.7)
		stats.freqTable[7]++;
	else if (freq <= 0.8)
		stats.freqTable[8]++;
	else if (freq <= 1)
		stats.freqTable[9]++;
}
int32_t computePcFreq(LST_ENTRY **lst,
			int32_t lstNumEntries, 
			int32_t depth,
			int32_t total) 
{
	int32_t i, j, valid = 0;
	for(i=0; i<lstNumEntries; i++)
		for(j=0; j< depth; j++)
			if(lst[i][j].valid){
				valid++;
				updateFreq((float) lst[i][j].dcount/total);
				}
	return valid;	
}

void printStridesAbs(char *s, STR_ENTRY* data)
{
	int32_t i;
	printf("%s\t", s);

	for(i=0; i<STRTABSIZE; i++) 	// Absolute
		printf("%d\t", data[i].strideLength);
	printf("\n");
}
void printStridesRel(char *s, STR_ENTRY* data, int32_t total)
{
	int32_t i;
	printf("%s\t", s);

	for(i=0; i<STRTABSIZE; i++)	// as per-unit
		printf("%.3f\t", NULLIFY(total, data[i].strideLength)); 
	printf("\n");
}

void printFreq()
{
	int i;
	for(i=0; i<10; i++)
		printf("%.2f\t",stats.freqTable[i]);
}

void	printStats(LST_ENTRY **lst,
			int32_t lstNumEntries, 
			int32_t depth )
{
	int32_t totGlobal, totShared, total, valid;
	
	totGlobal = stats.globalLoads + stats.globalStores;
	totShared = stats.sharedLoads + stats.sharedStores;
	total = totShared +totGlobal; 
	
	printf("gl ld\tgl st\tsh ld\tsh st\tTOT GL\tTOT SH\n");
	printf("%d\t%d\t%d\t%d\t%d\t%d\n\n", 
		stats.globalLoads, stats.globalStores, 
		stats.sharedLoads, stats.sharedStores,    
		totGlobal, totShared);
	printf("\t\tS <0\tS 0\tS 32\tS64\tS 128\tS 256\tS>256\n");
//	printStridesRel("Global LD", stats.strGlLd, stats.globalLoads);
//	printStridesRel("Global ST", stats.strGlSt, stats.globalStores);
//	printStridesRel("Shared LD", stats.strShLd, stats.sharedLoads);
//	printStridesRel("Shared ST", stats.strShSt, stats.sharedStores);

	printStridesAbs("a\tGlobal LD", stats.strGlLd);
	printStridesAbs("a\tGlobal ST", stats.strGlSt);
	printStridesAbs("a\tShared LD", stats.strShLd);
	printStridesAbs("a\tShared ST", stats.strShSt);
	printf("Normalized\n");
	printStridesRel("r\tGlobal LD", stats.strGlLd, total);
	printStridesRel("r\tGlobal ST", stats.strGlSt, total);
	printStridesRel("r\tShared LD", stats.strShLd, total);
	printStridesRel("r\tShared ST", stats.strShSt, total);

	
//	valid = computePcFreq(lst, lstNumEntries, depth, totGlobal+totShared);
//	printf("\nFrequency Table:\n");
//	printFreq();
//	printf("\nValid entries: %d\n\n", valid);
}

void 	main(int32_t argc, char **argv)
{
	int hit, miss, totrefs;
// Load store table and reference
	uint32_t lstNumEntries, depth;
	LST_ENTRY **lst;
	LST_ENTRY *current;	// current: lst entry
	LST_ENTRY reference; 	// the reference we are processing
// aux
	int32_t i, j;
	char line[BUFSIZE];

// process arguments
// numrefs: hash table size
// list_depth: lenght of hashed lists 
	if(argc < 3){
		fprintf(stderr, "Usage: %s numrefs list_depth\n", argv[0]);
		exit(1);
	}

// allocate the lst
	lstNumEntries = nearest_pow(atoi(argv[1]));
	depth = atoi(argv[2]);
	lst = (LST_ENTRY **) alloctable(lstNumEntries, depth, sizeof(LST_ENTRY));
	
	memset(&reference, 0, sizeof(LST_ENTRY));
	memset(&stats, 0, sizeof(STR_STATISTICS));

	hit = miss = totrefs = 0;
	while(!feof(stdin)){
		totrefs++;
// parse line
		fgets(line, BUFSIZE, stdin);
		sscanf(line, "%"SCNx64" %d %"SCNx64" %"SCNx64"", 
			&reference.tid, &reference.refType, &reference.pc, &reference.lastAddr);
		mksignature(&reference);
		updateCounters(&reference);
// lookup 
		switch (lstLookup(&current, lst, lstNumEntries, depth, &reference)){
			case HIT:
				hit++;
				break;
			case COLDMISS:
				miss++;
				break;
			default:
				fprintf(stderr, "Table Overflow\n");
				exit(1);
		}
		
	}

	printf("Tot Refs:\t%d\tHits:\t%d\tCold misses:\t%d\n\n", totrefs, hit, miss);
	printStats(lst, lstNumEntries, depth);
}  
