#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_FILENAME_LENGTH 100
#define BLOCK_NUMBER 2048
#define BLOCK_SIZE 1024
#define FILE_NUMBER 64

#define FILE_PATH "/home/andrey/3 курс/OS/fs/"

typedef struct fmeta {
	char name[MAX_FILENAME_LENGTH]; //name
	int start;			//number of start block
	int size;			//size
	int isDir;			//is directory (or file)
	int isEmpty;			//is empty
} fmeta;

typedef struct filesystem {
	fmeta meta[FILE_NUMBER];

	// block[i] >= 0   number of next block, 
	//          == -1  eof,
	//          == -2  empty
	int block[BLOCK_NUMBER];
} filesystem;

filesystem fs;

// init clear fs in memory
void init() {
	// init meta
	int i = 0;
	int n = sizeof(fs.meta / fs.meta[0]);
	while (i < n) {
		memset(fs.meta[i], 0, MAX_FILENAME_LENGTH);
		fs.meta[i].start = -1;
		fs.meta[i].size = 0;
		fs.meta[i].isDir = 0;
		fs.meta[i].isEmpty = 1;	
		i++;
	}
	
	// init blocks
	i = 0;
	while (i < BLOCK_NUMBER) {
		fs.block[i++] = -2;
	}
}

// load fs from file
void load() {
	FILE *f = fopen(FILE_PATH, "r");
	fread(fs.meta, sizeof(fmeta_t), FILE_NUMBER, f);
	fread(fs.block, sizeof(int), BLOCK_NUMBER, f);
	
	// fix empty blocks
	int i = 0;
	while (i < BLOCK_NUMBER)
		if (fs.block[i] == 0)
			fs.block[i] = -2;

	fclose(f);
}

// find first empty element in meta
int findEmptyMeta() {
	int i = 0;
	int n = sizeof(fs.meta / fs.meta[0]);
	while (i < n) {
		if (fs.meta[i].isEmpty) 
			return i;
		i++;
	}
	return -1; // not found	
}

// find first empty element in block
int findEmptyBlock() {
	int i = 0;
	while (i < BLOCK_NUMBER) {
		if (fs.meta[i] == -2)
			return i;
		i++;
	}
	return -1; // not found
}

// get meta by number 
fmeta *getMetaByNumber(int k) {
	return &fs.meta[k];
}

//get meta
int getMeta(const char *path, fmeta **meta) {
	
}

// write meta[k] to fs file
void writeMeta(int k) {
	FILE *f = fopen(FILE_PATH, "r+");
	fseek(f, k * sizeof(fmeta), SEEK_SET);
	fwrite(getMeta(k), sizeof(fmeta), 1, f);
	fclose(f);
}

// ============= FUSE OPERATIONS =============

static struct fuse_operations fs_oper = {

};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &fs_oper, NULL);
}