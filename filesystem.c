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
		memset(fs.meta[i].name, 0, MAX_FILENAME_LENGTH);
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

// write meta[k] to fs file
int writeMeta(int k) {
	FILE *f = fopen(FILE_PATH, "r+");
	fseek(f, k * sizeof(fmeta), SEEK_SET);
	fwrite(getMetaByNumber(k), sizeof(fmeta), 1, f);
	fclose(f);
	return 0;
}

// write data
int writeData(fmeta *meta, char *data) {
	int size = sizeof(data);
	if (size <= 0) return -1;
	FILE *f = fopen(FILE_PATH, "r+");
	int i, k = 0;
	i = meta->start;
	int count = BLOCK_SIZE;
	int skip = sizeof(fmeta) * FILE_NUMBER + sizeof(int) * BLOCK_NUMBER; // meta & blocks
	while (i != -1) { // not eof
		int left = size - k * BLOCK_SIZE;
		if (left >= BLOCK_SIZE) 
			count = BLOCK_SIZE;
		else 
			count = left;
		fseek(f, skip + i * BLOCK_SIZE, SEEK_SET);
		fwrite(data + k * BLOCK_SIZE, 1, count, f);
		i = fs.block[i];
		k++;
	}
	meta->size = size;
	fclose(f);
	return 0;	
}

// read data
int readData(fmeta *meta, char **data) {
	if (meta == NULL) return -1;
	FILE *f = fopen(FILE_PATH, "r");
	char *buf = null;
	int size = meta->size;
	buf = (char *)malloc(sizeof(char) * size);
	int i = meta->start, k = 0;
	int skip = sizeof(fmeta) * FILE_NUMBER + sizeof(int) * BLOCK_NUMBER; // meta & blocks
	while (i != -1) {
		int left = size - k * BLOCK_SIZE;
		if (left >= BLOCK_SIZE) 
			count = BLOCK_SIZE;
		else 
			count = left;
		fseek(f, skip + i * BLOCK_SIZE, SEEK_SET);
		fread(buf + k * BLOCK_SIZE, 1, count, f);
		i = fs.block[i];
		k++;
	}
	fclose(f);
	*data = buf;
	return 0;
}

int getFileMetaNumber(char *data, char *name) {
	int i = 0;
	int n = sizeof(data) / sizeof(int);
	while (i < n) {
		if (strcmp(fs.meta[((int *)data)[i]].name, name) == 0)
			return ((int *)data)[i];	
	}
	return -1;
}

//get meta by filepath
int getMeta(char *path, meta **meta) {
	if (strcmp(path, "/") == 0) { //root
		*meta = getMetaByNumber(0); 	
		return 0;
	}

	fmeta *m = NULL;
	char *p = path;

	if (*p++ == '/')
		m = getMetaByNumber(0);
	else return -1;

	char *data, *s;
	char name[MAX_FILENAME_LENGTH];
	memset(name, '\0', FILENAME_LENGTH);
	
	while (p - path < strlen(path)) {
		if (m->size == 0)
			return -1;
		readData(m, &data);
		s = p;
		p = strchr(p, '/');
		if (p != NULL)
			strncopy(name, s, p - s);
		else {
			strncopy(name, s, strlen(path) - s);
			p = path + strlen(path);		
		}
		int k = getFileMetaNumber(data, name);
		if (k == -1) return -1;
		m = getMetaByNumber(k);
		memset(name, '\0', FILENAME_LENGHT);
		free(data);
	}

	*meta = m;
	return k;
}

// add file to fs; returns number of meta
int addFile(char* name, int size, int isDir) {
	fmeta *meta = NULL;
	int k = findEmptyMeta();
	if (k == -1) return -1;
	meta = getMetaByNumber(k);
	int start = findEmptyBlock();
	if (start == -1) return -1;

	// write info to meta
	strcpy(meta->name, name);
	meta->start = start;
	meta->size = size;
	meta->isDir = isDir;
	meta->isEmpty = 0;

	fs.block[start] = -1;
	writeMeta(k);
	return k;
}

int createFile(const char* path, int isDir) {
	fmeta *meta;
	char *dir, *name;
	char *data, *moddata;

	name = strrchr(path, '/');
	if (name == NULL) {
		name = path;
		dir = NULL:
	} else {
		name = name + 1;
		strncpy(dir, path, name - path);
	}

	printf("filename: %s\ndirectory: %s", name, dir);

	getMeta(dir, &meta);
	readData(meta, &data);

	moddata = (char *)malloc(sizeof(data));
	memcpy(moddata, data, sizeof(data));
	
	int k = addFile(name, sizeof(data), isDir);
	writeData(meta, moddata);
	
	free(data);
	free(name);
	free(dir);

	return 0;
}

int removeFile(const char* path) {
	char *p = strrchr(path, '/');
	fmeta *fileMeta, *dirMeta;
	char *data, *moddata;
	char *dir;
	if (p != NULL) {
		dir = (char *)malloc((p - path) * sizeof(char));
		strncpy(dir, path, p - path);	
	} else {
		dir = (char *)malloc(2 * sizeof(char));
		strncpy(dir, "/\0");
	}
	int dirMetaNum = getMeta(dir, &dirMeta);
	int fMetaNum = getMeta(path, &fileMeta);
	readData(dirMeta, &data);

	moddata = (char *)malloc(sizeof(data));
	int i = 0, j = 0;
	while (i < sizeof(data)) {
		if (data[i] != fMetaNum)
			moddata[j++] = data[i];
		i++; 
	}

	writeData(dirMeta, moddata);
	dirMeta->size = sizeof(data);
	writeMeta(dirMetaNum);

	free(data);
	free(dir);

	return 0;
}

int openFile(const char* path) {
	fmeta *meta;
	int metaNum = getMeta(path, &meta);
	if (metaNum == -1) return -1;
	return 0;
}

// ============= FUSE OPERATIONS =============

static int fs_getattr(const char* path, struct stat *stbuf) {
	int res = 0;

	fmeta *meta;
	if (getMeta(path, &meta) == -1)
		res = -ENOENT;

	memset(stbuf, 0, sizeof(struct stat));
    	if(meta->isDir) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
    	}
    	else {
        	stbuf->st_mode = S_IFREG | 0444;
        	stbuf->st_nlink = 1;
        	stbuf->st_size = meta->size;
    	}

    	return res;
}

static int fs_mkdir(const char* path, mode_t mode) {
	if (createFile(path, 1) != 0)
		return -1;
	return 0;
}

static int fs_create(const char *path, mode_t mode, struct fuse_file_info *finfo) {
	if (createFile(path, 0) != 0)
		return -1;
	return 0;
}

static int fs_rmdir(const char *path) {
	int res = removeFile(path);
	if (res != 0) return -1;
	return 0;
}

static int fs_unlink(const char *path) {
	int res = removeFile(path);
	if (res != 0) return -1;
	return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
		off_t offset, struct fuse_file_info *fi) {

    	filler(buf, ".", NULL, 0);
    	filler(buf, "..", NULL, 0);

	fmeta *meta;
	int metaNum = getMeta(path, &meta);
	if (metaNum == -1) return -ENOENT;

	char *data;
	readData(meta, &data);

    	int i = 0, n = sizeof(data)/sizeof(int);
	while (i < n) {
		filler(buf, fs.meta[((int*)data)[i]].name, NULL, 0);
	}

    	return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {

	fmeta *meta;
	int metaNum = getMeta(path, &meta);
	if (metaNum == -1) return -ENOENT;

	char *data;
	if (readData(meta, &data) == -1) 
		return 0ENOENT;
	int length = sizeof(data);

    	if (offset < length) {
		if (offset + size > length)
			size = length - offset;
		memcpy(buf, data + offset, size);
	} else size = 0;

    	return size;
}

static int fs_open(const char *path, struct fuse_file_info *fi)
{
	if (openFile(path) == -1) return -ENOENT;
	return 0;
}

static int fs_opendir(const char *path, struct fuse_file_info *fi)
{
	if (openFile(path) == -1) return -ENOENT;
	return 0;
}

static int fs_write(const char *path, const char *buf, size_t nbytes, 
			off_t offset, struct fuse_file_info *fi) {
	
}

static struct fuse_operations fs_oper = {

};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &fs_oper, NULL);
}