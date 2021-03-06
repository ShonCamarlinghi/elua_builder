// MMC filesystem implementation using FatFs
#include "mmcfs.h"
#include "type.h"
#include <string.h>
#include <errno.h>
#include "devman.h"
#include <stdio.h>
#include "ioctl.h"

#include "platform_conf.h"
#ifdef BUILD_MMCFS
#include "ff.h"
#include "diskio.h"
#include <fcntl.h>

#define MMCFS_MAX_FDS   4
static FIL mmcfs_fd_table[ MMCFS_MAX_FDS ];
static int mmcfs_num_fd;

// Data structures used by FatFs
static FATFS mmc_fs;
static FIL mmc_fileObject;
//static DIR mmc_dir;
//static FILINFO mmc_fileInfo;

#define PATH_BUF_SIZE   40
static char mmc_pathBuf[PATH_BUF_SIZE];

static int mmcfs_find_empty_fd( void )
{
  int i;

  for (i = 0; i < MMCFS_MAX_FDS; i ++)
    if (mmcfs_fd_table[i].fs == NULL)
      return i;
  return -1;
}

static int mmcfs_open_r( struct _reent *r, const char *path, int flags, int mode )
{
  int fd;
  int mmc_mode;

  if (mmcfs_num_fd == MMCFS_MAX_FDS)
  {
    r->_errno = ENFILE;
    return -1;
  }

  // Default to top directory if none given
  mmc_pathBuf[0] = 0;
  if (strchr(path, '/') == NULL)
    strcat(mmc_pathBuf, "/");
  strcat(mmc_pathBuf, path);

  // Translate fcntl.h mode to FatFs mode (by jcwren@jcwren.com)
  if (((flags & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC)) && (flags & (O_RDWR | O_WRONLY)))
    mmc_mode = FA_CREATE_ALWAYS;
  else if ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
    mmc_mode = FA_OPEN_EXISTING;
  else if ((flags & O_CREAT) == O_CREAT)
    mmc_mode = FA_OPEN_ALWAYS;
  else if ((flags == O_RDONLY) || (flags == O_WRONLY) || (flags == O_RDWR))
    mmc_mode = FA_OPEN_EXISTING;
  else
  {
    r->_errno = EINVAL;
    return -1;
  }

  if ((flags & O_ACCMODE) == O_RDONLY)
    mmc_mode |= FA_READ;
  else if ((flags & O_ACCMODE) == O_WRONLY)
    mmc_mode |= FA_WRITE;
  else if ((flags & O_ACCMODE) == O_RDWR)
    mmc_mode |= (FA_READ | FA_WRITE);
  else
  {
    r->_errno = EINVAL;
    return -1;
  }

  // Open the file for reading
  if (f_open(&mmc_fileObject, mmc_pathBuf, mmc_mode) != FR_OK)
  {
    r->_errno = ENOENT;
    return -1;
  }

  if (mode & O_APPEND)
    mmc_fileObject.fptr = mmc_fileObject.fsize;
  fd = mmcfs_find_empty_fd();
  memcpy(mmcfs_fd_table + fd, &mmc_fileObject, sizeof(FIL));
  mmcfs_num_fd ++;
  return fd;
}

static int mmcfs_close_r( struct _reent *r, int fd )
{
  FIL* pFile = mmcfs_fd_table + fd;

  f_close( pFile );
  memset(pFile, 0, sizeof(FIL));
  mmcfs_num_fd --;
  return 0;
}

static _ssize_t mmcfs_write_r( struct _reent *r, int fd, const void* ptr, size_t len )
{
  UINT bytesWritten;

  if (f_write(mmcfs_fd_table + fd, ptr, len, &bytesWritten) != FR_OK)
  {
    r->_errno = EIO;
    return -1;
  }

  return (_ssize_t) bytesWritten;
}

static _ssize_t mmcfs_read_r( struct _reent *r, int fd, void* ptr, size_t len )
{
  UINT bytesRead;

  if (f_read(mmcfs_fd_table + fd, ptr, len, &bytesRead) != FR_OK)
  {
    r->_errno = EIO;
    return -1;
  }

  return (_ssize_t) bytesRead;
}

// IOCTL: only fseek
static int mmcfs_ioctl_r( struct _reent *r, int fd, unsigned long request, void *ptr )
{
  struct fd_seek *pseek = ( struct fd_seek* )ptr;
  FIL* pFile = mmcfs_fd_table + fd;
  u16 newpos = 0;

  if (request == FDSEEK)
  {
    switch (pseek->dir)
    {
      case SEEK_SET:
        // seek from beginning of file
        newpos =  pseek->off;
        break;

      case SEEK_CUR:
        // seek from current position
        newpos = pFile->fptr + pseek->off;
        break;

      case SEEK_END:
        // seek from end of file
        newpos = pFile->fsize + pseek->off;
        break;

      default:
        return -1;
    }
    if (f_lseek (pFile, newpos) != FR_OK)
      return -1;
    pseek->off = newpos;
    return 0;
  }
  else
    return -1;
}

// MMC device descriptor structure
static DM_DEVICE mmcfs_device =
{
  "/mmc",
  mmcfs_open_r,
  mmcfs_close_r,
  mmcfs_write_r,
  mmcfs_read_r,
  mmcfs_ioctl_r
};

DM_DEVICE* mmcfs_init()
{
  // Mount the MMC file system using logical disk 0
  if ( f_mount( 0, &mmc_fs ) != FR_OK )
    return NULL;

  return &mmcfs_device;
}

#else // #ifdef BUILD_MMCFS

DM_DEVICE* mmcfs_init()
{
  return NULL;
}

#endif // BUILD_MMCFS
