
#define PAGE_SIZE 50
#define MAIN_MEM_SIZE 10
#define DISK_SIZE 100
#define MAX_PROCESS_PAGES 50
#define MAX_NUM_PROCESS 100

#define PAGE_NUM_MASK 4095

struct Page
{
	char isAllocated;
	int processID;
	int processPageIndex;
	char data[PAGE_SIZE];
};

struct Process
{
	long* pageTable;
	int processID;
	char* processName;
	long diskLocation;
	int numberPages;
};

struct LinkedList
{
	void* data;
	struct LinkedList* next;
};


//Function headers
//Struct creators
struct Page* createMainMemory(int size);
struct Page* createDisk(int size);
struct Process* createProcess(char* processName, int pages);

extern char getByte(long physicalAdress);
extern void setByte(char byte, long physicalAddress);
extern long getPhysicalAddress(long logicalAddress, int processID);
extern int allocateMemory(int processID);
extern void displayDisk();
extern void displayMemory();

//Linked queue functions
extern void addItem(void* data, struct LinkedList** queue);
extern void* dequeue(struct LinkedList* queue);
extern void displayQueue(struct LinkedList* queue);

//Display and UI
extern void handleInput(char input);

