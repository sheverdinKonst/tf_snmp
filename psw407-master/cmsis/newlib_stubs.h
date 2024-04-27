#ifndef NEWLIB_STUBS_H_
#define NEWLIB_STUBS_H_
#if 0




int _close(int file);
int _fstat(int file, struct stat *st);
int _isatty(int file);
int _lseek(int file, int ptr, int dir);
caddr_t _sbrk(int incr);
int _read(int file, char *ptr, int len);
int _write(int file, char *ptr, int len);

#endif
#endif /* NEWLIB_STUBS_H_ */
