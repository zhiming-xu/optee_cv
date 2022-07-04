#ifndef OFS_OPS_H
#define OFS_OPS_H 1


/* dummy TODO: use macro to generate stub */
#define open(...) ofs_stub()
#define write(...) ofs_stub()
#define read(...) ofs_stub()
#define lseek(...) ofs_stub()
#define fsync(...) ofs_stub()
#define stat(...) ofs_stub()
#define fstat(...) ofs_stub()
#define mmap(...) ofs_stub()
#define munmap(...) ofs_stub()
#define chdir(...) ofs_stub()
#define getcwd(...) ofs_stub()
#define mkdir(...) ofs_stub()
#define unlink(...) ofs_stub()
#define access(...) ofs_stub()
#define close(...) ofs_stub()

#define fopen(...) ofs_stub()
#define fclose(...) ofs_stub()
#define fopen_s(...) ofs_stub()
#define sprintf(...) ofs_stub()
#define sprintf_s(...) ofs_stub()


/* from sys/stat.h */
#define	S_ISDIR(m)	((m & 0170000) == 0040000)	/* directory */
#define	S_ISCHR(m)	((m & 0170000) == 0020000)	/* char special */
#define	S_ISBLK(m)	((m & 0170000) == 0060000)	/* block special */
#define	S_ISREG(m)	((m & 0170000) == 0100000)	/* regular file */
#define	S_ISFIFO(m)	((m & 0170000) == 0010000)	/* fifo */

#define O_RDONLY	00
#define O_WRONLY	01
#define O_RDWR		02
#define O_CREAT		0100
#define __O_SYNC	04000000
#define O_DSYNC		00010000
#define O_SYNC		(__O_SYNC | O_DSYNC)

#define MAP_FAILED ((void *) -1)

int ofs_stub(void);

#endif
