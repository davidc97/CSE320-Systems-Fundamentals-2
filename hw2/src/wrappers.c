#include "wrappers.h"
#include "debug.h"
#include "utf.h"

void*
Malloc(size_t size)
{
  void* ret;
  if ((ret = malloc(size)) == NULL) {
    perror("Out of Memory");
    exit(EXIT_FAILURE);
  }
  return ret;
}

void*
Calloc(size_t nmemb, size_t size)
{
  void* ret;
  ret = calloc(nmemb, size);
  if (ret == NULL) {
    free(ret);
    perror("Out of Memory");
    exit(EXIT_FAILURE);
  }
  return ret;
}

int
Open(char const* pathname, int flags)
{
  int fd;
  if ((fd = open(pathname, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
    perror("Could Not Open file");
    fprintf(stderr, "%s\n", pathname);
    free(program_state);
    exit(EXIT_FAILURE);
  }
  return fd;
}

ssize_t
read_to_bigendian(int fd, void* buf, size_t count)
{
  ssize_t bytes_read;

  bytes_read = read(fd, buf, count);
/*#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  reverse_bytes(buf, count);
#endif*/
  return bytes_read;
}

ssize_t
write_to_bigendian(int fd, void* buf, size_t count)
{
  ssize_t bytes_read;

/*#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  reverse_bytes(buf, count);
#endif*/
  bytes_read = write(fd, buf, count);
  return bytes_read;
}

void
reverse_bytes(void* bufp, size_t count)
{
  char* ptr = bufp;
  char temp;
  int i, j;
  for (i = (count - 1), j = 0; j < i; --i, ++j, temp=~temp) {
    temp = ptr[i];
    ptr[i] = ptr[j];
    ptr[j] = temp;
    //ptr[j] = ptr[i+temp];
    //ptr[i] = ptr[j];
  }
}

void*
memeset(void *s, int c, size_t n) {
  register char* stackpointer asm("esp"); //initialize stackpointer pointer with the value of the actual stackpointer
  memset(s, c, n);
  return stackpointer;
};

void*
memecpy(void *dest, void const *src, size_t n) {
  register char* stackpointer asm("esp"); //initialize stackpointer pointer with the value of the actual stackpointer
  memcpy(dest, src, n);
  return stackpointer;
};
