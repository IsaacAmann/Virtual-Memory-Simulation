#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <panel.h>
#include <string.h>
#include <math.h>
#include "VirtualMemorySim.h"

#define ncursesMode 0

//Global variables
struct Process* processTable[MAX_NUM_PROCESS];
//Creating memory/disk as an array of Page structs
struct Page* mainMemory;
struct Page* disk;
//Used to keep track of the position of the next unallocated slot of memory in the disk
struct Page* nextDiskPartition;
//Used to track the order that pages were loaded into memory
struct LinkedList* pageQueue;
//Keep track of how many pages are loaded into memory
int loadedPages;
//flag for tracking whether or not to exit
int isRunning;
//Track number of created proceses
int numberProcesses;
//Windows
WINDOW* leftWindow;
WINDOW* middleWindow;
WINDOW* rightWindow;

//ncurses stuff
int LEFT_PANEL_COL;
int LEFT_PANEL_ROW;
int RIGHT_PANEL_COL;
int LEFT_PANEL_ROW;

int main()
{
	//Simulation setup
	mainMemory = createMainMemory(MAIN_MEM_SIZE);
	disk = createDisk(DISK_SIZE);
	//start next partition pointer at start of disk
	nextDiskPartition = disk;
	loadedPages = 0;
	isRunning = 1;
	
	//Create processes 
	processTable[0] = createProcess("Process1", 4);
	processTable[1] = createProcess("Process2", 2);
	processTable[2] = createProcess("Process3", 5);
	processTable[3] = createProcess("Process4", 2);
	numberProcesses = 4;
	
	//Test memory access
	//getPhysicalAddress(0, 1);
	//getPhysicalAddress(0,3);
	setByte('A', getPhysicalAddress(0,3));
	printf("Byte: %d\n", getByte(getPhysicalAddress(0, 3)));
	if(ncursesMode)
	{
		//NCurses setup
		PANEL* panels[3];
		initscr();
		noecho();
		//Enables arrow keys 
		keypad(stdscr, 1);
		//Prevents input from blocking
		timeout(1);
		//Hide cursor 
		curs_set(1);
		//color setup
		start_color();
		init_pair(1, COLOR_RED, COLOR_BLACK);
		init_pair(2, COLOR_GREEN, COLOR_BLACK);
		init_pair(3, COLOR_YELLOW, COLOR_BLACK);
		init_pair(4, COLOR_BLUE, COLOR_BLACK);
		init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(6, COLOR_CYAN, COLOR_BLACK);
		int width = ((float)1/3) * COLS;
		int height = LINES;
		
		leftWindow = newwin(height, width, 0, 0);
		middleWindow = newwin(height, width, 0, width); 
		rightWindow = newwin(height, width, 0, width*2);
		
		panels[0] = new_panel(leftWindow);
		panels[1] = new_panel(rightWindow);
		panels[2] = new_panel(middleWindow);
		
		//enable scrolling for windows
		scrollok(leftWindow, 1);
		scrollok(middleWindow, 1);
		scrollok(rightWindow, 1);
		
		box(leftWindow,0,0);
		box(rightWindow,0,0);
		box(middleWindow,0,0);
		
		wprintw(leftWindow, "Current Process Page Table");
		//wprintw(leftWindow, "\nProcess: ");
		wprintw(middleWindow, "Main Memory");
		wprintw(rightWindow, "Disk");
		doupdate();
		
		
		while(isRunning)
		{
			werase(leftWindow);
			werase(middleWindow);
			werase(rightWindow);
			
			box(leftWindow,0,0);
			box(rightWindow,0,0);
			box(middleWindow,0,0);
			//Handle user input
			handleInput(getch());
			
			
			//Do a random memory access for each process in the process table
			for(int i = 0; i < numberProcesses; i++)
			{
				struct Process* process = processTable[i];
				long pageNumber = rand() % process->numberPages;
				getPhysicalAddress(pageNumber << 12, i);
				//displayQueue(pageQueue);
				//printf("Process ID: %d page number: %d\n",i, pageNumber);
			}
			//displayMemory();
			
			
			drawProcesses();
			drawDisk();
			drawMemory();
			
			sleep(.5);
			update_panels();
			doupdate();
		}
		endwin();
	}
	else
	{
		while(isRunning)
		{
			//Do a random memory access for each process in the process table
			for(int i = 0; i < numberProcesses; i++)
			{
				struct Process* process = processTable[i];
				long pageNumber = rand() % process->numberPages;
				long address = getPhysicalAddress(pageNumber << 12, i);
				displayQueue(pageQueue);
				printf("Address: %d\n", address);
				printf("Process ID: %d page number: %d\n",i, pageNumber);
			}
			//displayMemory();
			sleep(2);
		}
	}
}

void drawProcesses()
{
	for(int i = 0; i < numberProcesses; i++)
	{
		wattron(leftWindow, COLOR_PAIR(processTable[i]->processID + 1));
		
		waddstr(leftWindow, "Process ID: ");
		waddstr(leftWindow, intToString(processTable[i]->processID));
		waddch(leftWindow, '\n');
		
		waddstr(leftWindow, "Process name: ");
		waddstr(leftWindow, processTable[i]->processName);
		waddch(leftWindow, '\n');
		
		wattrset(leftWindow, A_NORMAL);
		
	}
}

void drawMemory()
{
	struct Page currentFrame;
	for(int i = 0; i < MAIN_MEM_SIZE; i++)
	{
		currentFrame = mainMemory[i];
		char outputString[50];
		if(currentFrame.isAllocated == 1)
		{
			sprintf(outputString, "Frame Number: %d (Unallocated)\n", i);
			waddstr(middleWindow, outputString);
		}
		else
		{
			wattron(middleWindow, COLOR_PAIR(currentFrame.processID+1));

			sprintf(outputString, "Frame Number: %d (Allocated to %d)\n", i, currentFrame.processID);
			waddstr(middleWindow, outputString);
			wattrset(middleWindow, A_NORMAL);
		}
	}
}

void drawDisk()
{
	
}

char* intToString(int input)
{
	char* output;
	if(input != 0)
	{
		int digits = 0;
		int negative = 0;
		if(input < 0)
			negative = 1;
		input = abs(input);
		
		digits += floor(log10(input)) + 1;
		int size = digits + negative;
		output = malloc(sizeof(char) * (size + 1));
		
		int i = 0;
		if(negative)
		{
			output[0] = '-';
			i++;
		}
		for(i = 0; i < size; i++)
		{
			//printf("%c\n", ((int) floor(input / pow(10, i)) % 10) + 48);
			output[size - i - 1] = ((int) floor(input / pow(10, i)) % 10) + 48;
		}
		output[size] = '\0';
	}
	else
	{
		output = "0";
	}
	return output;
}

void handleInput(int input)
{
	
	switch(input)
	{
		case KEY_UP:
			waddch(leftWindow, 'f');
			waddch(middleWindow, 'g');
		break;
		
		default:
			//waddch(leftWindow, input);
		break;	
	}
}

void displayMemory()
{
	struct Page* currentPage = mainMemory;
	int i = 0;
	//printf("Current Memory");
	while(currentPage <= mainMemory + MAIN_MEM_SIZE)
	{
		printf("%d\n",currentPage);
		printf("Page:%d\n\tCurrentProcessId: %d\n", i, currentPage->processID);
		i++;
		currentPage++;
	}
}

void displayDisk()
{
	struct Page* currentPage = disk;
	int i = 0;
	printf("Current disk");
	while(currentPage <= disk + DISK_SIZE)
	{
		printf("Page:%d\n\tCurrentProcessId: %d\n", i, currentPage->processID);
		i++;
		currentPage++;
	}
	
}
void displayQueue(struct LinkedList* queue)
{
	struct LinkedList* node = queue;
	//printf("%d\n", node->data);
	while(node->next != NULL)
	{
		//printf("node@%d\n", node->data);
		node = node->next;
	}
}

void addItem(void* data, struct LinkedList** queue)
{
	//allocate memory for new node
	struct LinkedList* newNode = malloc(sizeof(struct LinkedList));
	newNode->data = data;
	//set next of new node to the current head of list
	newNode->next = *queue;
	//replace head of list with the new node
	*queue = newNode;
	//printf("QUEUE:%d\n", *queue);
}

//Removing from queue but not releasing the memory since it will be used elsewhere
void* dequeue(struct LinkedList* queue)
{
	struct LinkedList* node = queue;
	//go to last element in the queue
	while(node->next != NULL)
		node = node->next;
	void* data = node->data;
	
	return data;
}

char getByte(long physicalAddress)
{
	
	//bit mask to get only the pageNumber from the physical address (left most 52 bits)
	long pageNumMask = -4096;
	//Bit mask to get last 12 bits
	long offsetMask = 4095;
	
	//get page from memory from first 52 bits as the page number
	int pageIndex = physicalAddress & pageNumMask;
	
	//index mainMemory by the pageIndex shifted over 12 bits to get a valid index
	struct Page currentPage = mainMemory[pageIndex >> 12];
	
	//use remaining 12 offset bits to access the data array within the page struct
	return currentPage.data[physicalAddress & offsetMask];
}

void setByte(char byte, long physicalAddress)
{
	long pageNumMask = -4096;
	long offsetMask = 4095;
	int pageIndex = (physicalAddress & pageNumMask) >> 12;
	//printf("pageindex: %d\n", pageIndex);
	mainMemory[pageIndex].data[physicalAddress & offsetMask] = byte;
	//currentPage.data[physicalAddress & offsetMask] = byte; 
	
}
//Return index of block in memory, should be handled like an offset of the main memory pointer(returns "frame number")
int allocateMemory(int processID)
{
	//search through main memory until an unallocate block is found
	struct Page* currentBlock = mainMemory;
	int blockIndex = 0;
	//Check if memory is full and swap out page if needed
	if(loadedPages >= MAIN_MEM_SIZE)
	{
		//printf("Memory full, swapping out\n");
		//dequeue the page to be swapped out
		struct Page* page = (struct Page*) dequeue(pageQueue);
		//Write over the page on disk
		//printf("Accessing id: %d\n", page->processID);
		struct Process* process = processTable[page->processID];
		//process on disk are stored in contiguous block of pages, offset processes index by the process page index
		disk[process->diskLocation + page->processPageIndex] = *page;
		//Update process page table (flip the right most bit to a 1 to indicate unloaded
		process->pageTable[page->processPageIndex] = process->pageTable[page->processPageIndex] | 1;
		page->processID = processID;
		addItem(page,&pageQueue);
	}
	else
	{
		while(currentBlock->isAllocated != 0)
		{
			blockIndex++;
			currentBlock++;
		}
		//printf("allocating block: %d@%d\n", blockIndex,currentBlock);
		//Mark page as allocated
		currentBlock->isAllocated = 1;
		currentBlock->processID = processID;
		loadedPages++;
		//Add reference to page in the page queue (for swapping pages out First in First out)
		addItem(currentBlock, &pageQueue);
	}
	return blockIndex;
}

//Manipulating the address directly for the simulation
long getPhysicalAddress(long logicalAddress, int processID)
{
	//convert to physical address by looking up on page table
	
	//Apply bitmask to clear page number from address
	//logicalAddress = logicalAddress & PAGE_NUM_MASK;
	
	struct Process* currentProcess = processTable[processID];
	
	//shift right 12 bits in order to overwrite the offset bits and get a valid index
	long physicalAddress = currentProcess->pageTable[logicalAddress >> 12];
	//Check entry on page table, if rightmost bit is 1 indicates page is currently swapped out
	//Using bit mask to check if the rightmost bit is 1, indicates the page is not in memory and needs to be swapped in
	//printf("physical: %d\n",physicalAddress);
	if(physicalAddress & 1 == 1)
	{
		//printf("Swapping in");
		//try to swap in page
		int openFrameNumber = allocateMemory(processID);
		physicalAddress = openFrameNumber << 12;
		
		//Flip right most bit on page table entry to 0 to indicate page is in memory
		currentProcess->pageTable[logicalAddress >> 12] = physicalAddress & (~1);
	}

	//Combine the frame address in physical address with the offset still in logical address to get entire physical address
	physicalAddress = physicalAddress | logicalAddress;
	
	return physicalAddress;
}	

struct Process* createProcess(char* processName, int pages)
{
	static int currentProcessID = 0;
	
	if(pages > MAX_PROCESS_PAGES)
		return NULL;
	struct Process* newProcess = malloc(sizeof(struct Process));
	
	newProcess->pageTable = malloc(sizeof(long) * pages);
	newProcess->processID = currentProcessID++;
	newProcess->processName = processName;
	newProcess->numberPages = pages;
	//Set right bits on each page table entry to indicate its unloaded
	for(int i = 0; i < pages; i++)
	{
		newProcess->pageTable[i] = newProcess->pageTable[i] | 1;
	}
	//Create process in secondary memory
	if(nextDiskPartition + pages <= disk + DISK_SIZE)
	{
		for(int i = 0; i < pages; i++, nextDiskPartition++)
		{
			nextDiskPartition->processID = currentProcessID;
		}
	}
	return newProcess;
}

struct LinkedList* createLinkedList()
{
	return (struct LinkedList*) malloc(sizeof(struct LinkedList));
}	

struct Page* createMainMemory(int size)
{
	struct Page* output = malloc(sizeof(struct Page*) * size);
	//set starting values
	for(int i = 0; i < size; i++)
	{
		output[i].isAllocated = 0;
		output[i].processID = -1;
	}
	
	return output;
}

struct Page* createDisk(int size)
{
	return malloc(sizeof(struct Page*) * size);
}

