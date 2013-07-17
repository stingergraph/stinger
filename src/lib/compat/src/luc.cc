#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(__MTA__)
#include <sys/mta_task.h>
#include <machine/runtime.h>
#include <luc/luc_exported.h>
#include <snapshot/client.h>
#endif

extern "C" {
void luc_io_init (void);
void luc_snapin (const char*, void*, size_t);
void luc_snapout (const char*, void*, size_t);
void luc_stat (const char*, size_t*);
}

using namespace std;

#if defined(__MTA__)
static int use_NFS = 0;
void
luc_io_init (void)
{
  int err;
  if (mta_get_max_teams () < 2)
    use_NFS = 1;
  else if ((err = snap_init ()) != SNAP_ERR_OK) {
    fprintf (stderr, "snap_init failed: %d\n", err);
    abort ();
  }
}
#else
void
luc_io_init (void)
{
}
#endif

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

#if defined(__MTA__)
void
luc_snapin (const char *fname, void* buf, size_t sz)
{
  luc_error_t err;
  int snap_error_wtf;
  if (use_NFS) {
    read_in (fname, buf, sz);
    return;
  }
  err = snap_restore (fname, buf, sz, &snap_error_wtf);
  if (err != SNAP_ERR_OK) {
    fprintf (stderr, "snap_restore of %s failed, errors %d and %d\n",
      fname, err, snap_error_wtf);
    abort ();
  }
}
#else
void
luc_snapin (const char *fname, void* buf, size_t sz)
{
  read_in (fname, buf, sz);
}
#endif

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

#if defined(__MTA__)
void
luc_snapout (const char *fname, void* buf, size_t sz)
{
  luc_error_t err;
  int snap_error_wtf;

  if (use_NFS) {
    write_out (fname, buf, sz);
    return;
  }

  err = snap_snapshot (fname, buf, sz, &snap_error_wtf);
  if (err != SNAP_ERR_OK) {
    fprintf (stderr, "snap_snapshot of %s failed, errors %d and %d\n",
      fname, err, snap_error_wtf);
    abort ();
  }
}
#else
void
luc_snapout (const char *fname, void* buf, size_t sz)
{
  write_out (fname, buf, sz);
}
#endif

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

#if defined(__MTA__)
void
luc_stat (const char *fname, size_t *sz)
{
  snap_stat_buf statBuf;
  int64_t sn_err = 0;
  luc_endpoint_id_t swEP = SNAP_ANY_SW;

  if (use_NFS) {
    stat_fname (fname, sz);
    return;
  }

  if (!fname || snap_stat(fname, swEP, &statBuf, &sn_err) != LUC_ERR_OK)
    perror("Can't stat sources file.\n");

  *sz = statBuf.st_size;
}
#else
void
luc_stat (const char *fname, size_t *sz)
{
    stat_fname (fname, sz);
}
#endif
