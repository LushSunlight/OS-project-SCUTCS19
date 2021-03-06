#pragma once
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <stack>
#include <regex>
#pragma warning(disable:4996)

// measured in "BYTES" 
#define INITIAL_DISK_SIZE  16 * 1024 * 1024
#define INITIAL_BLOCK_SIZE  1 * 1024
#define INITIAL_BLOCK_NUM  1024
#define INITIAL_SUPERBLOCK_SIZE  1 * 1024
// #define INITIAL_BITMAP_SIZE  2 * 1024
#define INITIAL_INODE_NUMBER  4096
#define INITIAL_DATA_BLOCK_NUMBER 2040 * 8
#define INITLAL_FREEPTR  16541362
#define DISK_PATH "disk.dat"
#define NUM_INDIRECT_ADDRESSES 341
#define INITIAL_INODE_SIZE 128
// measured in "BITS"
// #define BITMAP_RESERVE_BITS 3
// others
// #define MAXIMUM_ABSOLUTE_FILENAME_LENGTH 768
// #define MAXIMUM_FILE_PER_DIRECTORY 128
#define MAXIMUM_FILENAME_LENGTH 62
#define DIRECT_ADDRESS_NUMBER 10
// measured in "BYTES" 
#define MAXIMUM_FILE_SIZE DIRECT_ADDRESS_NUMBER * INITIAL_BLOCK_SIZE + 1 * NUM_INDIRECT_ADDRESSES * INITIAL_BLOCK_SIZE

int fileSeek(FILE*, long, int, bool=false);
FILE* fileOpen(const char*, const char*, bool=false);
size_t fileRead(void*, size_t, size_t, FILE*, bool=false);
size_t fileWrite(const void*, size_t, size_t, FILE*, bool= false);
int filePutCharacter(int, FILE*);

const char magic_number[] = "xtyshrfs";

struct Address
{
	unsigned char addr[3];
	Address() {
		memset(addr, 0, sizeof addr);
	}
	Address(int addrInt) {
		from_int(addrInt);
	}
	int to_int() const {
		return (int)addr[0] * 1 + (int)addr[1] * 256 + (int)addr[2] * 256 * 256;
	}
	void from_int(int addrInt) {
		addr[0] = addrInt % 256;
		addrInt /= 256;
		addr[1] = addrInt % 256;
		addrInt /= 256;
		addr[2] = addrInt % 256;
	}
	Address block_addr() {
		return Address((this->to_int() / 1024) << 10);
	}
	Address offset() {
		return Address(this->to_int() % 1024);
	}
	Address operator+(const Address& a) {
		return Address(this->to_int() + a.to_int());
	}
	Address operator+(const int& a) {
		return Address(this->to_int() + a);
	}
	Address operator-(const Address& a) {
		return Address(this->to_int() - a.to_int());
	}
	Address operator-(const int& a) {
		return Address(this->to_int() - a);
	}
	bool operator==(const Address& a) {
		return this->to_int() == a.to_int();
	}
	bool operator==(const int& a) {
		return this->to_int() == a;
	}
	bool operator!=(const Address& a) {
		return !(*this == a);
	}
	bool operator!=(const int& a) {
		return !(*this == a);
	}
};

class Diskblock {  
	// can get the content from or write the content to a specific disk block
public:
	unsigned char content[INITIAL_BLOCK_SIZE];
	Diskblock() { refreshContent(); }
	void refreshContent();
	Diskblock(int); 
	Diskblock(Address); 
	void load(int,FILE* = NULL, int = INITIAL_BLOCK_SIZE);			// load a block from disk given an address to the content buffer
	void load(Address, FILE* = NULL, int = INITIAL_BLOCK_SIZE);		// load a block from disk given an address to the content buffer
	void write(int, FILE* = NULL, int = INITIAL_BLOCK_SIZE);		// write the content buffer to the specific disk block
	void write(Address, FILE* = NULL, int = INITIAL_BLOCK_SIZE);	// write the content buffer to the specific disk block

};

class IndirectDiskblock {
public:
	Address addrs[NUM_INDIRECT_ADDRESSES];  // addresses loaded from an indirect disk block
	void load(Address,FILE* =NULL);			//load addresses given an indirect block address
	void write(Address,FILE* =NULL);		//write the pointers to the specific block
};

class iNode
{
public:
	unsigned fileSize;
	time_t inode_create_time;
	time_t inode_access_time;
	time_t inode_modify_time;
	bool isDir;

	int parent;
	int inode_id;

	Address direct[DIRECT_ADDRESS_NUMBER];
	Address indirect;

	iNode(unsigned, int, int, bool=true);
	iNode() {}
	void updateCreateTime();
	void updateModifiedTime();
	void updateAccessTime();
	std::string getCreateTime();
	std::string getModifiedTime();
	std::string getAccessTime();
};

class superBlock
{
public:
	superBlock();
	
	unsigned inodeNumber;				    // amount of inodes
	unsigned freeInodeNumber;				// amount of free inodes
	unsigned dataBlockNumber;				// amount of blocks
	unsigned freeDataBlockNumber;	        // amount of free blocks
	
	char inodeMap[INITIAL_INODE_NUMBER / 8];

	unsigned BLOCK_SIZE;
	unsigned INODE_SIZE;
	unsigned SUPERBLOCK_SIZE;
	unsigned TOTAL_SIZE;
	
	//int free_block_SP;					// free block stack pointer
	//int *free_block;					// free block stack

	unsigned superBlockStart;
	//unsigned inodeBitmapStart;
	//unsigned blockBitmapStart;
	unsigned inodeStart;
	unsigned blockStart;
    bool updateSuperBlock(FILE* = NULL);
	Address freeptr;
	
	int allocateNewInode(unsigned, int, Address[], Address*, FILE* = NULL, bool=true);
	bool freeInode(int, FILE* = NULL);
	iNode loadInode(short, FILE* = NULL);
	bool writeInode(iNode, FILE* = NULL);
};


class DiskblockManager {

public:
	Address freeptr;
	void initialize(superBlock*,FILE* =NULL);		// initialize when the disk is created
	Address alloc(FILE* =NULL);						// allocate a free block and return the free block address
	void free(Address addr,FILE* =NULL);			// recycle the unused block and push it to the stack
	void printBlockUsage(superBlock*, FILE* =NULL);	// print disk block usage
	int getFreeBlock(FILE*);						// get free block number
	int getFreeBlock(int);							// get free block number from linked list block number
	int getLinkedListBlock(FILE*);					// get number of blocks used for address linked list
	int getDedicateBlock(superBlock*,FILE*);		// get dedicated block number
};

struct fileEntry {
	char fileName[MAXIMUM_FILENAME_LENGTH];
	short inode_id;
	fileEntry() {};
	fileEntry(const char* , short);
};

struct Directory {
	std::vector<fileEntry> files;
	short findInFileEntries(const char*);
};


class Disk
{
public:
	Disk();
	~Disk();
	FILE* diskFile;
	
	
	void run();
	bool loadDisk();
	void initializeRootDirectory();
	Address allocateNewBlock(FILE* =NULL);
	bool freeBlock(Address addr, FILE* = NULL);
	bool setCurrentInode(int inode_id);

	void parse(char* str);

	Directory readFileEntriesFromDirectoryFile(iNode);
	bool writeFileEntriesToDirectoryFile(Directory, iNode);
	int createUnderInode(iNode&, const char*, int);

	short allocateResourceForNewDirectory(iNode);
	short allocateResourceForNewFile(iNode, unsigned);
	bool deleteFile(iNode);

	int parentBlockRequired(iNode&);
	bool freeBlockCheck(int);
	bool freeInodeCheck();
	int blockUsedBy(iNode&);
	int inodeUsedBy(iNode&);
	bool copy(iNode&, const char*, iNode&); // copy directory or file
	short copyFile(iNode&, iNode&);

	void listDirectory(iNode);
	void printCurrentDirectory(const char* ="\0");
	std::string getFullFilePath(iNode, const char* = "\0");
	std::string getFileName(iNode);
	bool changeDirectory(const char*);
	std::vector<std::string> stringSplit(const std::string&, const std::string&);
	short locateInodeFromPath(std::string);
	void recursiveDeleteDirectory(iNode);
	
	unsigned getDirectorySize(iNode);
	void printWelcomeInfo();
	void printHelpInfo();
private:
	superBlock super;
	DiskblockManager dbm;
	iNode currentInode;
	Diskblock db;
	std::regex fileNamePattern;
};
