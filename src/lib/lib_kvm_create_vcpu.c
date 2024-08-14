/*
[COMMIT]
fix: 423ecfe
parent: 7c69661

[COMPILE]
compiled with GCC 10.4, defconfig, with KASAN
the following kernel config needed: CONFIG_KVM, CONFIG_KVM_INTEL

[COMMENT]
the host machine must enable APICv feature
when run on QEMU, the following commands are needed:
  -cpu host,+vmx in QEMU boot line
*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/kvm.h>
#include <string.h>
// #include <pthread.h>
// #include <signal.h>
// #include <sched.h>
// #include <time.h>
// #include <x86intrin.h>

unsigned long racee_delay=1000000, racer_delay=0;

static int fd_kvm, fd_vm, fd_vcpu1, fd_vcpu2;
static struct kvm_enable_cap cap, cap_race, dbg;
static struct timespec ts;

void do_setup(){
  memset(&dbg, 0, sizeof(dbg));
  memset(&cap, 0, sizeof(cap));
  memset(&cap_race, 0, sizeof(cap_race));
  memset(&ts, 0, sizeof(ts));
  *(unsigned long*)&dbg = 0x5dda9c14aa95f5c5;
  cap.cap = KVM_CAP_SPLIT_IRQCHIP;
  cap_race.cap = 212;  // KVM_CAP_PMU_CAPABILITY
  ts.tv_nsec = 1;

  fd_kvm = open("/dev/kvm", O_RDWR);
  if(fd_kvm == -1) {
    perror("[-] open /dev/kvm failed");
    exit(-1);
  }
}

void sys_racee(void)
{
  fd_vcpu2 = ioctl(fd_vm, KVM_CREATE_VCPU, 1);
}

void sys_racer(void)
{
  ioctl(fd_vcpu1, KVM_SET_GUEST_DEBUG, &dbg);
}

// static void* runner_fn(void* arg)
// {
//   runner_killable = 1;
//   if (ioctl(fd_vcpu2, KVM_RUN, 0) < 0)
//     perror("[-] KVM_RUN failed");

//   puts("[-] should not reach");
//   sleep(-1);
// }

void cleanup_one()
{
  if (ioctl(fd_vcpu2, KVM_RUN, 0) < 0) {
    perror("[-] KVM_RUN failed");
  }
  
  close(fd_vcpu1);
  close(fd_vcpu2);
  close(fd_vm);
}

void setup_one()
{
  // runner_killable = 0;

  fd_vm = ioctl(fd_kvm, KVM_CREATE_VM, 0);
  if (fd_vm < -1) {
    perror("[-] kvm ioctl KVM_CREATE_VM failed");
    exit(-1);
  }

  if (ioctl(fd_vm, KVM_ENABLE_CAP, &cap) != 0) {
    perror("[-] kvm ioctl KVM_ENABLE_CAP failed");
    exit(-1);
  }

  if (ioctl(fd_vm, KVM_SET_BOOT_CPU_ID, 1) != 0) {
    perror("[-] kvm ioctl KVM_SET_BOOT_CPU_ID failed 1");
    exit(-1);
  }

  fd_vcpu1 = ioctl(fd_vm, KVM_CREATE_VCPU, 0);
  if (fd_vcpu1 < 0) {
    perror("[-] kvm ioctl KVM_CREATE_VCPU failed 1");
    exit(-1);
  }
}


void do_cleanup()
{
  close(fd_kvm);
}

int sanity_check(void)
{
  do_setup();
  setup_one();
  fd_vcpu2 = ioctl(fd_vm, KVM_CREATE_VCPU, 1);
  if (fd_vcpu2 < 0) {
    perror("[-] KVM_CREATE_VCPU failed");
    return -1;
  }
  if (ioctl(fd_vcpu1, KVM_SET_GUEST_DEBUG, &dbg) < 0) {
    perror("[-] KVM_SET_GUEST_DEBUG failed");
    return -1;
  }

  if (ioctl(fd_vcpu2, KVM_RUN, 0) < 0) {
    perror("[-] KVM_RUN failed");
    return -1;
  }
  close(fd_vcpu1);
  close(fd_vcpu2);
  close(fd_vm);
  
  do_cleanup();

  return 0;
}
