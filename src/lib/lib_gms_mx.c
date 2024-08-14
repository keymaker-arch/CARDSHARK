/*
[COMMIT]
fix: 3c4f8333b582487
parent: a4a79e03bab57729bd

[COMMENT]
compiled with GCC 10.4, defconfig, with KASAN enabled
the following kernel config is required: CONFIG_N_GSM
*/

#include <stdio.h>
#include <stdint.h>
#include <linux/gsmmux.h>
#include <linux/tty.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

unsigned long racee_delay=0, racer_delay=0;

#define SERIAL_PORT "/dev/ttyS0"

static struct gsm_config c, c1, c2;
static int fd, ldisc = N_GSM0710, ldisc_default;

void sys_racer(void)
{
  ioctl(fd, GSMIOC_SETCONF, &c1);
}

void sys_racee(void)
{
  ioctl(fd, GSMIOC_SETCONF, &c2);
}

void do_setup(void)
{
  if (ioctl(fd, TIOCGETD, &ldisc_default) < 0) {
    perror("[-] get default line discpline failed");
    exit(-1);
  }
  return;
}

void setup_one(void)
{
  fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0) {
    perror("[-] open "SERIAL_PORT" failed");
    exit(-1);    
  }

  if (write(fd, "AT+CMUX=0\r", 10) != 10) {
    perror("[-] write to fd failed");
    exit(-1);
  }

  if (write(fd, "AT+CMUX=0\r", 10) != 10) {
    perror("[-] write to fd failed");
    exit(-1);
  }

  if (ioctl(fd, TIOCSETD, &ldisc) < 0) {
    perror("[-] ioctl TIOCSETD failed");
    exit(-1);
  }

  if (ioctl(fd, GSMIOC_GETCONF, &c) < 0) {
    perror("[-] ioctl GSMIOC_GETCONF failed");
    exit(-1);
  }

  c.mru++;
  ioctl(fd, GSMIOC_SETCONF, &c);
  c.mru++;
  memcpy(&c1, &c, sizeof(c));
  c.mru++;
  memcpy(&c2, &c, sizeof(c));
}

void cleanup_one(void)
{
  if (ioctl(fd, TIOCSETD, &ldisc_default) < 0) {
    perror("[-] ioctl TIOCSETD failed");
    exit(-1);
  }

  if (write(fd, "ATZ\r", 4) != 4) {
    perror("[-] reset modem failed");
    exit(-1);
  }

  close(fd);
}

void do_cleanup(void)
{
  return;
}

int sanity_check(void)
{
  int fd;
  struct gsm_config c;

  return 0;

  fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0) {
    return -1;    
  }
  
  if (write(fd, "AT+CMUX=0\r", 10) != 10) {
    return -1;
  }

  if (ioctl(fd, TIOCSETD, &ldisc) < 0) {
    return -1;
  }

  if (ioctl(fd, GSMIOC_GETCONF, &c) < 0) {
    return -1;
  }

  c.mru++;
  if (ioctl(fd, GSMIOC_SETCONF, &c) < 0) {
    return -1;
  }

  close(fd);
  return 0;
}