#ifndef __SPAWN_H__
#define __SPAWN_H__

struct light_thread
{
    long long main_rsp;
    long long rsp;
    void *arg;
    void (*pfn)(void *);
};

int spawn(void *arg, void (*pfn)(void *));

struct light_thread *get_one_thread();

void run_thread(struct light_thread *lt);

void back_to_main();


#endif
