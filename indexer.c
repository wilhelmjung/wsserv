#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <strings.h>

#include <event.h>

// file entry:
// file_type, file_name, file_size, file_mtime, file_atime
// TODO: return file list as JSON string.
// // file entry
// {
// 	"current_dir" : "/home/william/k",
//	"file_name" : ".",
//	"file_type" : "DIR",
//	"file_size" : 18890,
//	"file_mtime" : "2013-7-16 16:09",
//	"file_atime" : "2013-7-16 16:09",
// }
//char *file_entry_fmt =
#define file_entry_fmt \
	"{\n" \
	"\t\"current_dir\" : \"%s\",\n" \
	"\t\"file_name\"   : \"%s\",\n" \
	"\t\"file_type\"   : %lu,   \n" \
	"\t\"file_size\"   : %lu,   \n" \
	"\t\"file_mtime\"  : \"%s\",\n" \
	"\t\"file_atime\"  : \"%s\",\n" \
	"\t\"file_ctime\"  : \"%s\",\n" \
	"}"

struct evbuffer *evb_list = NULL;

// fill file info into evb_list
static int
stat_file(const char *path, const char *cwd)
{
fprintf(stderr, "[%d] +%s()\n", __LINE__, __func__);
	char *name, *m_time, *a_time, *c_time, *ri;
	int type;
	ulong size, mode;

	struct stat sb;
	if (stat(path, &sb) == -1) {
		perror("stat");
		return -1;
	}

	mode = sb.st_mode & S_IFMT;
	//XXX exclude other types.
	if (mode == S_IFREG || mode == S_IFDIR)
		printf("mode: %lu\n", mode);
	else
		return -2;

	size = sb.st_size;
	////////////////// ALLOC MEM
	m_time = calloc(1, 256);
	ctime_r(&sb.st_mtime, m_time); // modify
	// remove trailling '\n'
	ri = rindex(m_time, '\n');
	*ri = 0;
	////////////////// ALLOC MEM
	a_time = calloc(1, 256);
	ctime_r(&sb.st_atime, a_time); // access
	// remove trailling '\n'
	ri = rindex(a_time, '\n');
	*ri = 0;
	////////////////// ALLOC MEM
	c_time = calloc(1, 256);
	ctime_r(&sb.st_ctime, c_time); // last change
	// remove trailling '\n'
	ri = rindex(c_time, '\n');
	*ri = 0;
	
	// fill buffer
	evbuffer_add_printf(evb_list, file_entry_fmt,
		cwd, path,
		mode, size,
		m_time, a_time, c_time);

	////////////////// FREE MEM
	free(m_time);
	////////////////// FREE MEM
	free(a_time);
	////////////////// FREE MEM
	free(c_time);

fprintf(stderr, "[%d] -%s()\n", __LINE__, __func__);
	return 0;
}

int
list_dir(const char *path)
{
fprintf(stderr, "[%d] %s()\n", __LINE__, __func__);
	DIR *d;
	struct dirent *de;
	char cwd[512];

	d = opendir(path);
	if (d == NULL)
		return -1;

	getcwd(cwd, sizeof cwd);

	/////////////////////  ALLOC MEM
	evb_list = evbuffer_new();

	evbuffer_add_printf(evb_list, "[\n");

	int ret;
	// fill buffer
	while ((de = readdir(d)) != NULL) {
		ret = stat_file(de->d_name, cwd);
		if (ret) {
			fprintf(stderr, "stat file error.\n");
			continue;
		}
		evbuffer_add_printf(evb_list, ",\n");
	}
	closedir(d);

	evbuffer_add_printf(evb_list, "]\n");

	// dump file list
	printf("buf: \n%s", evb_list->buffer);
	/////////////////////  FREE MEM
	evbuffer_free(evb_list);

	return 0;
}

int
list_file(const char *path)
{
	DIR *d;
	struct dirent *de;
	d = opendir(path);
	if (d == NULL)
		return -1;

	while ((de = readdir(d)) != NULL) {
		char tc;
		switch (de->d_type) {
		case DT_DIR:  tc = '/';	// dir
		break;
		case DT_REG:  tc = ' ';	// dir
		break;
		default: tc = '?';
		break;
		}
		printf("%s%c\n", de->d_name, tc);
	}

	closedir(d);
	return 0;
}

int
stat_file0(const char *path)
{
	struct stat sb;

	if (stat(path, &sb) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}

	printf("File type:");

	switch (sb.st_mode & S_IFMT) {
		case S_IFBLK:  printf("block device\n");            break;
		case S_IFCHR:  printf("character device\n");        break;
		case S_IFDIR:  printf("directory\n");               break;
		case S_IFIFO:  printf("FIFO/pipe\n");               break;
		case S_IFLNK:  printf("symlink\n");                 break;
		case S_IFREG:  printf("regular file\n");            break;
		case S_IFSOCK: printf("socket\n");                  break;
		default:       printf("unknown?\n");                break;
	}

	printf("I-node num: %ld\n", (long) sb.st_ino);

	printf("Mode:                     %lo (octal)\n",
			(unsigned long) sb.st_mode);

	printf("Link count:               %ld\n", (long) sb.st_nlink);
	printf("Ownership:                UID=%ld   GID=%ld\n",
			(long) sb.st_uid, (long) sb.st_gid);

	printf("Preferred I/O block size: %ld bytes\n",
			(long) sb.st_blksize);
	printf("File size:                %lld bytes\n",
			(long long) sb.st_size);
	printf("Blocks allocated:         %lld\n",
			(long long) sb.st_blocks);

	printf("Last status change:       %s", ctime(&sb.st_ctime));
	printf("Last file access:         %s", ctime(&sb.st_atime));
	printf("Last file modification:   %s", ctime(&sb.st_mtime));

	exit(EXIT_SUCCESS);
}

int
main(int c, char **v)
{
#if 0
	char cwd[512];
	getcwd(cwd, sizeof cwd);
	printf("cwd: %s\n", cwd);
	list_file(".");
#endif

	list_dir(".");

	return 0;
}

