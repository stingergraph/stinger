#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

extern "C" {
void luc_io_init (void);
void luc_snapin (const char*, void*, size_t);
void luc_snapout (const char*, void*, size_t);
void luc_stat (const char*, size_t*);
}

using namespace std;

void
luc_io_init (void)
{
}


static void
read_in (const char *fname, void* buf, size_t sz)
{
  char * tmpbuf = (char *) buf;
  int fd;
  fd = open (fname, O_RDONLY);
  if (fd < 0) {
    fprintf (stderr, "Failed to open %s for reading: %s\n",
      fname, strerror (errno));
    abort ();
  }
  size_t bytes_read = 0;
  while (bytes_read < sz) {
    int last = read(fd, tmpbuf, sz);
    bytes_read += last;
    tmpbuf += last;
  }

  if (bytes_read != sz)
  {
    fprintf (stderr, "Failed to read %ld octets from %s: %s\n",
      (long)sz, fname, strerror (errno));
    abort ();
  }
  close (fd);
}


void
luc_snapin (const char *fname, void* buf, size_t sz)
{
  read_in (fname, buf, sz);
}


static void
write_out (const char *fname, void* buf, size_t sz)
{
  int fd;
  fd = open (fname, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (fd < 0) {
    fprintf (stderr, "Failed to open %s for writing: %s\n",
      fname, strerror (errno));
    abort ();
  }
  if (write (fd, buf, sz) < sz) {
    fprintf (stderr, "Failed to write %ld octets to %s: %s\n",
      (long)sz, fname, strerror (errno));
    abort ();
  }
  close (fd);
}


void
luc_snapout (const char *fname, void* buf, size_t sz)
{
  write_out (fname, buf, sz);
}


static void
stat_fname (const char *fname, size_t *sz)
{
  struct stat st;

  if (stat (fname, &st)) {
    fprintf (stderr, "Failed to stat %s: %s\n",
      fname, strerror (errno));
    abort ();
  }
  *sz = st.st_size;
}


void
luc_stat (const char *fname, size_t *sz)
{
    stat_fname (fname, sz);
}

