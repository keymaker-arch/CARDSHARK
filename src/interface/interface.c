#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

#include "interface.h"

#ifdef BOOST_CPU_FREQ
#define CPU_FREQ_BOOST_MAX_TRY 20000
#define CPU_FREQ_BOOST_LOOPNR 5000
static unsigned long long cpu_freq_boost_threshold;
#endif

static char racee_spin, racer_spin, spin_flag;
static pthread_t thrs[2];
#define DEFAULT_DELAY_STEP 100
static unsigned long long racee_len, racer_len, larger_len;

extern unsigned long long racee_delay, racer_delay;

static unsigned long long my_rdtsc(void)
{
  unsigned long low, high;

  asm volatile(
    "lfence;"
    "sfence;"
    "rdtsc;"
    "sfence;"
    "lfence;"
    : "=a" (low), "=d" (high)
    :
    : "ecx"
  );

  return (low) | (high << 32);
}

static void pin_task_to(int cpu)
{
  cpu_set_t cset;

  CPU_ZERO(&cset);
  CPU_SET(cpu, &cset);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &cset))
    perror("[-] set affninity error"), exit(-1);
}

static inline void noop_nomem_loop1(unsigned long dummy1, unsigned long dummy2, unsigned long nr)
{
  asm volatile(
    "movq %0, %%rax;"
    "movq %1, %%rbx;"
    "movq %2, %%rcx;"
    "loop_mark1:"
    "addq %%rbx, %%rax;"
    "loop loop_mark1;"
    :
    : "r"(dummy1), "r"(dummy2), "r"(nr)
    :
  );
}

static void busy_sleep(unsigned long long interval)
{
  unsigned long long end;

  if (interval == 0)
    return;

  end = my_rdtsc() + interval;
  while (1) {
    if (my_rdtsc() > end)
      return;
  }
}

#ifdef BOOST_CPU_FREQ
static void cpu_freq_boost_init(float threshold_scale)
{
  unsigned long enter, leave, min;
  unsigned long i;

  if (cpu_freq_boost_threshold != 0) {
    printf("[-] cpu freq boost already initialized\n");
    return;
  }

  for (i=0; i<CPU_FREQ_BOOST_MAX_TRY; i++) {
    enter = my_rdtsc();
    noop_nomem_loop1(100, 200, CPU_FREQ_BOOST_LOOPNR);
    leave = my_rdtsc();
    if (i == 0 || leave - enter < min)
      min = leave - enter;
  }

  cpu_freq_boost_threshold = min * threshold_scale;
}

static void* cpu_freq_boost_fn(void* arg)
{
  unsigned long long enter, leave;
  while (1) {
    enter = my_rdtsc();
    noop_nomem_loop1(100, 200, CPU_FREQ_BOOST_LOOPNR);
    leave = my_rdtsc();
    if (leave - enter > cpu_freq_boost_threshold)
      break;
  }
}

static void cpu_freq_boost(void)
{
  pthread_t thrs[2];

  pin_task_to(1);
  pthread_create(&thrs[0], NULL, cpu_freq_boost_fn, NULL);
  pin_task_to(2);
  pthread_create(&thrs[1], NULL, cpu_freq_boost_fn, NULL);
  pin_task_to(0);

  cpu_freq_boost_fn(NULL);
  pthread_join(thrs[0], NULL);
  pthread_join(thrs[1], NULL);
}
#endif

void print_up_time(void)
{
  int fd;
  char buf[100];

  fd = open("/proc/uptime", O_RDONLY);
  if (fd < 0) {
    puts("[-] open /proc/uptime failed");
    exit(-1);
  }
  if (read(fd, buf, sizeof(buf)) < 0) {
    puts("[-] read /proc/uptime failed");
    exit(-1);
  }
  close(fd);
  printf("[+] uptime: %s", buf);
}

void* align_racer_fn(void* arg)
{
  racer_spin = 0;

  while (spin_flag);
  busy_sleep(racer_delay);
  sys_racer();
}

void* align_racee_fn(void* arg)
{
  racee_spin = 0;

  while(spin_flag);
  busy_sleep(racee_delay);
  sys_racee();
}

void* unalign_racer_fn(void* arg)
{
  sys_racer();
}

void* unalign_racee_fn(void* arg)
{
  sys_racee();
}


static void cardshark(unsigned try_nr)
{
  unsigned i;

  for (i=0; i<try_nr; i++) {
    setup_one();

    racee_spin = 1;
    racer_spin = 1;
    spin_flag = 1;

    pin_task_to(1);
    pthread_create(&thrs[0], NULL, align_racer_fn, NULL);
    pin_task_to(2);
    pthread_create(&thrs[1], NULL, align_racee_fn, NULL);
    pin_task_to(0);

    while (racee_spin || racer_spin);
    spin_flag = 0;

    pthread_join(thrs[0], NULL);
    pthread_join(thrs[1], NULL);

    cleanup_one();
  }
}

void unalign(unsigned nr)
{
  unsigned i;

  print_up_time();

  do_setup();
  for (i=0; i<nr; i++) {
    setup_one();
    pthread_create(&thrs[1], NULL, unalign_racer_fn, NULL);
    pthread_create(&thrs[0], NULL, unalign_racee_fn, NULL);
    pthread_join(thrs[1], NULL);
    pthread_join(thrs[0], NULL);
    cleanup_one();
  }

  do_cleanup();
  puts("[-] failed to trigger");
}

#ifdef MORE_UNALIGNS
void unalign2(unsigned nr)
{
  unsigned i;

  print_up_time();

  do_setup();
  for (i=0; i<nr; i++) {
    setup_one();
    pthread_create(&thrs[0], NULL, unalign_racee_fn, NULL);
    pthread_create(&thrs[1], NULL, unalign_racer_fn, NULL);
    pthread_join(thrs[0], NULL);
    pthread_join(thrs[1], NULL);
    cleanup_one();
  }

  do_cleanup();
  puts("[-] failed to trigger");
}

void unalign3(unsigned nr)
{
  unsigned i;

  print_up_time();

  do_setup();
  for (i=0; i<nr; i++) {
    setup_one();
    pthread_create(&thrs[0], NULL, unalign_racee_fn, NULL);
    unalign_racer_fn(NULL);
    pthread_join(thrs[0], NULL);
    cleanup_one();
  }

  do_cleanup();
  puts("[-] failed to trigger");
}

void unalign4(unsigned nr)
{
  unsigned i;

  print_up_time();

  do_setup();
  for (i=0; i<nr; i++) {
    setup_one();
    pthread_create(&thrs[0], NULL, unalign_racer_fn, NULL);
    unalign_racee_fn(NULL);
    pthread_join(thrs[0], NULL);
    cleanup_one();
  }

  do_cleanup();
  puts("[-] failed to trigger");
}
#endif

void measure_syscall_time(void)
{
  unsigned long long enter, leave;

  setup_one();

  enter = my_rdtsc();
  sys_racee();
  leave = my_rdtsc();
  racee_len = (leave - enter) * 1.2;

  enter = my_rdtsc();
  sys_racer();
  leave = my_rdtsc();
  racer_len = (leave - enter) * 1.2;

  larger_len = racee_len > racer_len ? racee_len : racer_len;
  cleanup_one();
}

void blindshark()
{
  unsigned long long _delay;;

  do_setup();

#ifdef BOOST_CPU_FREQ
  pin_task_to(0);
  cpu_freq_boost_init(1.2);
  cpu_freq_boost();
#endif

  measure_syscall_time();
  print_up_time();
  for (_delay=0; _delay<racer_len; _delay+=DEFAULT_DELAY_STEP) {
    racee_delay = _delay;
    racer_delay = 0;
    cardshark(10000);
  }

  for (_delay=0; _delay<racee_len; _delay+=DEFAULT_DELAY_STEP) {
    racee_delay = 0;
    racer_delay = _delay;
    cardshark(10000);
  }

  for (_delay=0; _delay<racer_len; _delay+=DEFAULT_DELAY_STEP) {
    racee_delay = _delay;
    racer_delay = 0;
    cardshark(1000000);
  }

  for (_delay=0; _delay<racee_len; _delay+=DEFAULT_DELAY_STEP) {
    racee_delay = 0;
    racer_delay = _delay;
    cardshark(1000000);
  }

  do_cleanup();
}

int main(int argc, char** argv)
{
  if (strcmp(argv[1], "unalign") == 0) {
#ifdef MORE_UNALIGNS
  switch (atoi(argv[2])) {
    case 1:
      unalign(atoi(argv[3]));
      break;
    case 2:
      unalign2(atoi(argv[3]));
      break;
    case 3:
      unalign3(atoi(argv[3]));
      break;
    case 4:
      unalign4(atoi(argv[3]));
      break;
    default:
      puts("[-] wrong arguments!");
      exit(0);
  }
#endif

    unalign(atoi(argv[2]));
    return 0;
  }

  if (strcmp(argv[1], "blindshark") == 0) {
    blindshark();
    return 0;
  }

  if (strcmp(argv[1], "cardshark") == 0) {
    racee_delay = atol(argv[2]);
    racer_delay = atol(argv[3]);
    do_setup();
    print_up_time();
    cardshark(atoi(argv[4]));
    do_cleanup();
    return 0;
  }

  puts("[-] wrong argument");
  return 0;
}