#include "../libWad/Wad.h"

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

Wad *wadVar;

static int getattr_callback(const char *path, struct stat *stbuf) {

  memset(stbuf, 0, sizeof(struct stat));

  //string* pth = path;
  //const string test = path;
  std::string test(path);

  if(wadVar->isDirectory(test)){
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}
	//add normal directory stuff
	stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;

  }else{
	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;
	stbuf->st_size = wadVar->getSize(test);
	return 0;
  }

  return -ENOENT;

}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  (void) offset;
  (void) fi;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  vector<string> entries;
  int numFiles = wadVar->getDirectory(path, &entries);
  if(numFiles == 0){
	return 0;
  }
  for(int i = 0; i < entries.size(); i++){
	const char* name = entries[i].c_str();
	filler(buf, name, NULL, 0);
  }
  return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

  if (!wadVar->isDirectory(path)) {
    size_t len = wadVar->getSize(path);
    if (offset >= len) {
      return 0;
    }

    if (offset + size > len) {
      return wadVar->getContents(path, buf, len-offset, offset);
    }

    return wadVar->getContents(path, buf, size, offset);
  }
  return -ENOENT;
}

static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
};

int main(int argc, char* argv[])
{
  char* dir[2];
  dir[0] = argv[0];
  dir[1] = argv[2];
  wadVar = Wad::loadWad(argv[1]);
  argc--;
  return fuse_main(argc, dir, &fuse_example_operations, NULL);
}
