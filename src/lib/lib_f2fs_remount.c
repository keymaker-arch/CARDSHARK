/*
[COMMIT]
fix :458c15dfbce6
parent: bfd47662399

[COMPILE]
build with GCC 10.4, defconfig, with KASAN
the following kernel config is required: CONFIG_F2FS_FS

[COMMENT]
need a diskfs.img in the root directory, which can be created by
  dd if=/dev/zero of=diskfs.img bs=1M count=100
  mkfs.f2fs diskfs.img
*/

#define _GNU_SOURCE
#include <sys/mount.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/loop.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

unsigned long racee_delay=0, racer_delay=0;

#define DISKFS_PATH "/diskfs.img"
#define MOUNT_POINT "/mnt"
#define FSTYPE_F2FS "f2fs"
#define LOOPDEV_PATH "/dev/loop0"
#define DUMMYF_PATH MOUNT_POINT"/dummy"
#define MOUNT_OPT "noextent_cache,lazytime,background_gc=sync"

static int fd_loopdev, fd_img, fd_dummy;

void do_setup(void)
{
  int ret;

  fd_img = open(DISKFS_PATH, O_RDWR);
  if (fd_img < 0) {
    return;
    // perror("[-] open disk image file failed");
    // exit(-1);
  }

  fd_loopdev = open(LOOPDEV_PATH, O_RDWR);
  if (fd_loopdev < 0) {
    return;
    // perror("[-] open loop device failed");
    // exit(-1);
  }

  ret = ioctl(fd_loopdev, LOOP_SET_FD, fd_img);
  if (ret < 0) {
    return;
    // perror("[-] set loop device failed");
    // exit(-1);
  }

  ret = mount(LOOPDEV_PATH, MOUNT_POINT, FSTYPE_F2FS, 0, MOUNT_OPT);
  if (ret < 0) {
    perror("[-] mount failed");
    exit(-1);
  }
}

void setup_one(void)
{
  fd_dummy = open(DUMMYF_PATH, O_RDWR|O_CREAT, 0666);
  if (fd_dummy < 0) {
    perror("[-] open dummy file failed");
  }

  // write(fd_dummy, "ests", 4);
  if (ftruncate(fd_dummy, 0x100000) < 0) {
    perror("[-] ftruncate failed");
  }
}

void sys_racer(void)
{
  mount(LOOPDEV_PATH, MOUNT_POINT, FSTYPE_F2FS, MS_REMOUNT, MOUNT_OPT);
}

void sys_racee(void)
{
  fallocate(fd_dummy, FALLOC_FL_INSERT_RANGE, 0, 0x10000);
}

void cleanup_one(void)
{
  close(fd_dummy);
  unlink(DUMMYF_PATH);
  return;
}

void do_cleanup(void)
{
  close(fd_img);
  umount(MOUNT_POINT);
  ioctl(fd_loopdev, LOOP_CLR_FD);
  close(fd_loopdev);
}

int sanity_check(void)
{
  return 0;
  int ret, fd_loopdev, fd_img, fd_dummy;

  fd_img = open(DISKFS_PATH, O_RDWR);
  if (fd_img < 0) {
    perror("[-] open disk image file failed");
    return -1;
  }

  fd_loopdev = open(LOOPDEV_PATH, O_RDWR);
  if (fd_loopdev < 0) {
    perror("[-] open loop device failed");
    return -1;
  }

  ret = ioctl(fd_loopdev, LOOP_SET_FD, fd_img);
  if (ret < 0) {
    perror("[-] set loop device failed");
    return -1;
  }

  ret = mount(LOOPDEV_PATH, MOUNT_POINT, FSTYPE_F2FS, 0, MOUNT_OPT);
  if (ret < 0) {
    perror("[-] mount failed");
    return -1;
  }

  fd_dummy = open(DUMMYF_PATH, O_RDWR|O_CREAT, 0666);
  if (fd_dummy < 0) {
    perror("[-] open dummy file failed");
    return -1;
  }

  if (ftruncate(fd_dummy, 0x100000) < 0) {
    perror("[-] ftruncate failed");
    return -1;
  }

  if (mount(LOOPDEV_PATH, MOUNT_POINT, FSTYPE_F2FS, MS_REMOUNT, MOUNT_OPT) < 0) {
    perror("[-] remount failed");
    return -1;
  }

  if (fallocate(fd_dummy, FALLOC_FL_INSERT_RANGE, 0, 0x10000) < 0) {
    perror("[-] fallocate failed");
    return -1;
  }

  close(fd_dummy);
  unlink(DUMMYF_PATH);
  close(fd_img);
  if (umount(MOUNT_POINT) < 0) {
    perror("[-] unmount failed");
    return -1;
  }
  if (ioctl(fd_loopdev, LOOP_CLR_FD) < 0) {
    perror("[-] un-associate loop dev failed");
    return -1;
  }
  close(fd_loopdev);

  return 0;
}
