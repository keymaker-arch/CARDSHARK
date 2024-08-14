#ifndef __LIB_INTERFACE
#define __LIB_INTERFACE

void do_setup();
void setup_one();
void cleanup_one();
void do_cleanup();

int sanity_check();

void sys_racer(void);
void sys_racee(void);
#endif