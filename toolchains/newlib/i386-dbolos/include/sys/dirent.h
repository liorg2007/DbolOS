#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/types.h>

#define NAME_MAX 255

typedef struct {
    int dd_fd;		/* directory file */
    int dd_loc;		/* position in buffer */
    int dd_seek;
    char *dd_buf;	/* buffer */
    int dd_len;		/* buffer length */
    int dd_size;	/* amount of data in buffer */
} DIR;

struct dirent {
    unsigned long d_ino;          // Inode number
    unsigned long d_off;          // Offset to next dirent (not used in some filesystems)
    unsigned short d_reclen; // Length of this record
    char d_name[];        // Null-terminated filename (flexible array member)
};

#ifdef __cplusplus
extern "C" {
#endif

DIR *opendir(const char *);
struct dirent *readdir(DIR *);
int closedir(DIR *);

#ifdef __cplusplus
}
#endif

#endif