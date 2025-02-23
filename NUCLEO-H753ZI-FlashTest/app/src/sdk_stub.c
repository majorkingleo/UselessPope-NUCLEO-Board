#include <errno.h>
#include <sys/stat.h>

#pragma warning "aaa"

int _close(int file) { return -1; }

int _fstat(int file, struct stat* st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file) { return 1; }

int _lseek(int file, int ptr, int dir) { return 0; }

int _read(int file, char* ptr, int len) { return 0; }

int _write(int file, char* ptr, int len) { return len; }

int _kill(int pid, int sig)
{
  errno = EINVAL;
  return -1;
}

int _getpid(void) { return 1; }
