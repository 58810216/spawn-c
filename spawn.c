#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "list.h"
#include "spawn.h"

#define STACK_SIZE (8 * 1024)

struct light_thread
{
    long main_rsp;
    long rsp;
    void *arg;
    void (*pfn)(void *);
    struct list_head list;
    int stop;
};

struct frame_base
{
    long saved_rbp;
    long rbp;
    long rip;
    long main_rip;
    long main_rbp;
    long pad;
};

struct worker_info
{
    int stop;
    int num;
    pthread_mutex_t mutex;
    pthread_t *workers;
    struct light_thread **running_threads;
    struct list_head waiting_list;
};

struct worker_info g_workers = {.stop = 0, .num = 0 };

void init_workinfo(struct worker_info *workers)
{
    workers->stop = 0;
    workers->num = 0;
    pthread_mutex_init(&workers->mutex, NULL);
    workers->workers = NULL;
    workers->running_threads = NULL;
    INIT_LIST_HEAD(&workers->waiting_list);

    return ;
    
}

void run_light_thread(struct light_thread *lt)
{
    int i = 0;
    int found = 0;

    pthread_mutex_lock(&g_workers.mutex);

    for (i = 0; i < g_workers.num; i++)
    {
        if (!g_workers.running_threads[i])
        {
            g_workers.running_threads[i] = lt;
            found = 1;
            break;
        }
    }

    if (!found)
    {
        printf("cannot add \n");
    }

    pthread_mutex_unlock(&g_workers.mutex);

    asm volatile("push %%rbp\n\t"
                 "movq %%rsp, %P1(%0)\n\t"
                 "movq %P2(%0), %%rsp\n\t"
                 "popq %%rbp\n\t"
                 "leave; ret\n\t"
                 :
                 :"D"(lt),
                  "i"(offsetof(struct light_thread, main_rsp)),
                  "i"(offsetof(struct light_thread, rsp))
                 :"memory", "cc");
}

void back_to_main(int finish)
{
    struct light_thread *lt = NULL;
    long rsp = 0;
    int i;

    asm volatile("movq %%rsp, %0\n\t":"=r"(rsp)::"memory");
    
    for (i = 0; i < g_workers.num; i++)
    {
        if (!g_workers.running_threads[i])
        {
            continue;
        }
        if (rsp - (long)g_workers.running_threads[i] <= STACK_SIZE)
        {
            lt = g_workers.running_threads[i];
            g_workers.running_threads[i] = NULL;
            break;
        }
    }

    if (!lt)
    {
        int *p = 0;
        printf("cannot find  %p\n", (void *)rsp);
        *p = 100;
        return ;
    }

    lt->stop = finish;

    asm volatile("movq %3, %%rsp\n\t"
                 "push %%rbp\n\t"
                 "movq %%rsp, %P2(%0)\n\t"
                 "movq %P1(%0), %%rsp\n\t"
                 "popq %%rbp\n\t"
                 "leave; ret\n\t"
                 :
                 :"D"(lt),
                  "i"(offsetof(struct light_thread, main_rsp)),
                  "i"(offsetof(struct light_thread, rsp)),
                  "a"(rsp)
                 :"memory");
}

void light_thread_main(struct light_thread *lt)
{
    lt->pfn(lt->arg); 

    back_to_main(1);
}

void back_to_main_and_stop()
{
    back_to_main(1);
}

void add_lt_to_waiting(struct light_thread *lt)
{
    pthread_mutex_lock(&g_workers.mutex);
    list_add(&lt->list, &g_workers.waiting_list);
    pthread_mutex_unlock(&g_workers.mutex);
    printf("add lt %p\n", lt);
}

void add_lt_to_waiting_head(struct light_thread *lt)
{
    pthread_mutex_lock(&g_workers.mutex);
    list_add(&lt->list, &g_workers.waiting_list);
    pthread_mutex_unlock(&g_workers.mutex);
    printf("add lt %p\n", lt);
}

struct light_thread *new_lt()
{
    struct light_thread *lt = NULL;
    struct frame_base *bf = NULL;
    long old_rbp = 0;

    lt = malloc(STACK_SIZE);
    if (!lt)
    {
        return NULL;
    }

    bf = (struct frame_base *)((char *)lt + STACK_SIZE);
    bf--;

    bf->main_rbp = (long)&bf->main_rbp;
    bf->main_rip = (long)back_to_main_and_stop;
    bf->rip = (long)light_thread_main;
    bf->rbp = (long)&bf->main_rbp;
    bf->saved_rbp = (long)&bf->rbp;

    lt->main_rsp = 0;
    lt->rsp = (long)&bf->saved_rbp;

    INIT_LIST_HEAD(&lt->list);

    lt->stop = 0;
    return lt;
}

void free_lt(struct light_thread *lt)
{
    free(lt); 
}

int spawn(void (*pfn)(void *),void *arg)
{
    struct light_thread *lt = NULL;

    lt = new_lt();
    if (!lt)
    {
        printf("new lt fail\n");
        return -1;
    }
    
    lt->pfn = pfn;
    lt->arg = arg;
    add_lt_to_waiting(lt);

    return 0;
}

struct light_thread *get_one_thread()
{
    struct light_thread *lt = NULL;
    long long *reg = NULL; 
    int i = 0;

    pthread_mutex_lock(&g_workers.mutex);
    if (!list_empty(&g_workers.waiting_list))
    {
        lt = list_first_entry(&g_workers.waiting_list, struct light_thread, list);
        list_del_init(&lt->list);
    }
    pthread_mutex_unlock(&g_workers.mutex);

    return lt;
}

void *worker_run(void *arg)
{
    struct light_thread *pw;

    while (!g_workers.stop)
    {
        pw = get_one_thread();
        if (pw)
        {
            run_light_thread(pw);
            if (!pw->stop)
            {
                add_lt_to_waiting_head(pw);
            }
            else
            {
                printf("finish %p\n", pw);
                free_lt(pw);
            }
        }
        else
        {
            sleep(0);
        }
    }

    return (void *)0;
}


int init_main(int worker_num)
{
    int i;
    int err;
     
    if (g_workers.num)
    {
        printf("already inited \n");
        return -1;
    }

    init_workinfo(&g_workers);
    g_workers.num = worker_num;
    g_workers.workers = malloc(sizeof(*g_workers.workers) * worker_num);
    if (!g_workers.workers)
    {
        printf("no memory for workers\n");
        return -1;
    }

    g_workers.running_threads = malloc(sizeof(*g_workers.running_threads) * worker_num);

    for (i = 0; i < worker_num; i++)
    {
        g_workers.running_threads[i] = NULL;
    }

    for (i = 0; i < worker_num; i++)
    {
        err = pthread_create(&g_workers.workers[i], NULL, worker_run , NULL);
        if (err)
        {
            printf("start worker %d fail\n", i);
            break;
        }
    }

    if (err)
    {
        void *retval = NULL;
        g_workers.stop = 1; 
        for (i = i - 1; i > 0; i--)
        {
            pthread_join(g_workers.workers[i], &retval);
        }
        free(g_workers.workers);
        free(g_workers.running_threads);
        g_workers.num = 0;
        return err; 
    }
    

    return 0;
}
