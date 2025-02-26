#define _LARGEFILE64_SOURCE
#define FUSE_USE_VERSION 30

#include <err.h>
#include <libzfs.h>
#include <stdio.h>

// if it comes before libzfs.h, shit breaks :/
#include <fuse.h>

#define die(...)                                                               \
	{                                                                          \
		fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);                        \
		err(1, __VA_ARGS__);                                                   \
	}

#define arrlen(x) (sizeof(x) / sizeof(x[0]))

#define shift(argc, argv) (assert(argc > 0), argc--, *argv++)

static libzfs_handle_t* zfs  = NULL;
static zpool_handle_t*  pool = NULL;

static uint64_t pool_alloc = 0;
static uint64_t pool_free  = 0;
static uint64_t pool_size  = 0;

static void cleanup(void) {
	if(pool != NULL) zpool_close(pool);
	if(zfs != NULL) libzfs_fini(zfs);
}

static void refresh() {
	pool_alloc = zpool_get_prop_int(pool, ZPOOL_PROP_ALLOCATED, NULL);
	pool_size  = zpool_get_prop_int(pool, ZPOOL_PROP_SIZE, NULL);
	pool_free  = pool_size - pool_alloc;
}

static int my_getattr(
	const char*            path, //
	struct stat*           info, //
	struct fuse_file_info* fi    //
) {
	(void) path;
	(void) fi;

	refresh();

	memset(info, 0, sizeof(*info));
	info->st_blocks = pool_alloc / 512;
	info->st_mode   = S_IFDIR;

	return 0;
}

static int my_readdir(
	const char*             path,   //
	void*                   buf,    //
	fuse_fill_dir_t         filler, //
	off_t                   offset, //
	struct fuse_file_info*  fi,     //
	enum fuse_readdir_flags flags   //
) {
	(void) path;
	(void) buf;
	(void) filler;
	(void) offset;
	(void) fi;
	(void) flags;

	return 0;
}

static int my_statfs(const char* path, struct statvfs* info) {
	(void) path;

	refresh();

	memset(info, 0, sizeof(*info));
	info->f_frsize = 1;
	info->f_blocks = pool_size;
	info->f_bfree  = pool_free;
	info->f_bavail = pool_free;

	return 0;
}

int main(int argc, char** argv) {
	atexit(cleanup);

	const char* program_name = shift(argc, argv);

	if(argc != 2) errx(1, "Usage: %s <pool> <mountpoint>", program_name);
	const char* pool_name  = shift(argc, argv);
	const char* mountpoint = shift(argc, argv);

	zfs = libzfs_init();
	if(zfs == NULL) die("libzfs_init");

	pool = zpool_open(zfs, pool_name);
	if(pool == NULL) die("zpool_open %s", pool_name);

	struct fuse_operations ops = {
		.getattr = my_getattr,
		.readdir = my_readdir,
		.statfs  = my_statfs,
	};

	const char* args[] = {
		program_name,
		"-f",
		"-oallow_other",
		mountpoint,
	};
	return fuse_main(arrlen(args), (char**) args, &ops, NULL);
}
