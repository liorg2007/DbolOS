#ifndef _NEWLIB_STDIO_H
#define _NEWLIB_STDIO_H

#include <sys/lock.h>
#include <sys/reent.h>

/* DBolOS file type enum recieved from fstat */
typedef enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_REGULAR = 1,   // Regular file (S_IFREG)
    FILE_TYPE_DIRECTORY = 2, // Directory (S_IFDIR)
    FILE_TYPE_CHAR_DEVICE = 3, // Character device (S_IFCHR)
    FILE_TYPE_BLOCK_DEVICE = 4, // Block device (S_IFBLK)
    FILE_TYPE_FIFO = 5,      // FIFO (named pipe) (S_IFIFO)
    FILE_TYPE_SYMLINK = 6,   // Symbolic link (S_IFLNK)
    FILE_TYPE_SOCKET = 7,    // Socket (S_IFSOCK)
} file_type_t;

/* Internal locking macros, used to protect stdio functions.  In the
   general case, expand to nothing. Use __SSTR flag in FILE _flags to
   detect if FILE is private to sprintf/sscanf class of functions; if
   set then do nothing as lock is not initialised. */
#if !defined(_flockfile)
#ifndef __SINGLE_THREAD__
#  define _flockfile(fp) (((fp)->_flags & __SSTR) ? 0 : __lock_acquire_recursive((fp)->_lock))
#else
#  define _flockfile(fp)	(_CAST_VOID 0)
#endif
#endif

#if !defined(_funlockfile)
#ifndef __SINGLE_THREAD__
#  define _funlockfile(fp) (((fp)->_flags & __SSTR) ? 0 : __lock_release_recursive((fp)->_lock))
#else
#  define _funlockfile(fp)	(_CAST_VOID 0)
#endif
#endif

#endif /* _NEWLIB_STDIO_H */
