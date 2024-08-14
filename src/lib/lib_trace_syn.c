/*
[COMMIT]
fix: 4ccf11c4e8a8e
parent: 7e364e56293bb98

[COMPILE]
compiled with GCC 10.4, defconfig, with KASAN enabled, require: CONFIG_SYNTH_EVENTS
run the following command to setup: mount -t tracefs none /sys/kernel/tracing
*/

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

unsigned long racee_delay=0, racer_delay=0;

#define TRACE_PATH "/sys/kernel/tracing/synthetic_events"

static int fd1, fd2;
static int _init_flag;

void do_setup(void)
{
  if (_init_flag == 0) {
    system("mount -t tracefs none /sys/kernel/tracing");
    _init_flag = 1;
  }
  return;
}

void setup_one(void)
{
  fd1 = open(TRACE_PATH, O_RDWR);
  if (fd1 < 0) {
    perror("[-] open trace interface failed");
    exit(-1);
  }

  fd2 = open(TRACE_PATH, O_RDWR);
  if (fd2 < 0) {
    perror("[-] open trace interface failed");
    exit(-1);
  }
}

void sys_racer(void)
{
  write(fd1, "\x88", 1);
}

void sys_racee(void)
{
  write(fd2, "\x88", 1);
}

void cleanup_one(void)
{
  close(fd1);
  close(fd2);
}

void do_cleanup(void)
{
  return;
}

int sanity_check(void)
{
  int fd;

  if (_init_flag == 0) {
    system("mount -t tracefs none /sys/kernel/tracing");
    _init_flag = 1;
  }

  fd = open(TRACE_PATH, O_RDWR);
  if (fd < 0) {
    perror("[-] open trace interface failed");
    return -1;
  }

  write(fd, "\x88", 1);

  close(fd);
  return 0;
}
