#ifndef __SPAWN_H__
#define __SPAWN_H__


int spawn(void (*pfn)(void *),void *arg);
int init_main(int worker_num);

int stop_main();

#endif
