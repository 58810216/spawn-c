#include <stdio.h>
#include <stdlib.h>

#include "spawn.h"

#define STACK_SIZE (8 * 1024)
#define TOP_POS (8 * 2)

void *g_thread_addr[10] = {NULL};
struct light_thread *g_thread = NULL;

void do_run(struct light_thread *lt)
{
    lt->pfn(lt->arg); 
}

int spawn(void *arg, void (*pfn)(void *))
{
    struct light_thread *lt = NULL;
    long long *stack = NULL;
    long long old_rbp = 0;

    lt = malloc(STACK_SIZE);

    lt->main_rsp = 0;
    lt->rsp = 0;
    lt->pfn = pfn;
    lt->arg = arg;

    stack = (long long *)((char *)lt + STACK_SIZE);
    

    /* top rbp */
    stack -= (TOP_POS)/sizeof(*stack);
    *stack = (long long)stack;

    old_rbp = stack;
    /* arg: lt */
    stack--;
    *stack = (long long)lt;

    /* ip */
    stack--;
    *stack = (long long)do_run;

    stack--;
    *stack = old_rbp;

    /* rbp  */
    stack--;
    *stack =  old_rbp;

    /* rsp */
    lt->rsp = stack;

    g_thread = lt;

    return 0;
}

#define SAVE_REG(name, dst) \
    asm volatile("movq %%" #name ", %0\n\t":"=r"(dst):);

struct light_thread *get_one_thread()
{
    struct light_thread *lt = NULL;
    long long *reg = NULL; 

    lt = g_thread;
    g_thread_addr[0] = lt;

    return lt;
}



#define offsetof(p, v) \
    (long long)(&((typeof(p) *)0)->v)

void run_thread(struct light_thread *lt)
{
    asm volatile("push %%rbp\n\t"
                 "movq %%rsp, %P1(%0)\n\t"
                 "movq %%rbp, %P3(%0)\n\t"
                 "movq %P2(%0), %%rsp\n\t"
                 "popq %%rbp\n\t"
                 :
                 :"a"(lt),
                  "i"(offsetof(struct light_thread, main_rsp)),
                  "i"(offsetof(struct light_thread, rsp)),
                  "i"(STACK_SIZE - TOP_POS)
                 :"memory");
}

void back_to_main()
{
    struct light_thread *lt = NULL;
    long long rsp;

    SAVE_REG(rsp, rsp);
    
    if (rsp)
    {
        lt = g_thread_addr[0];
    }

    asm volatile("push %%rbp\n\t"
                 "movq %%rsp, %P2(%0)\n\t"
                 "movq %P1(%0), %%rsp\n\t"
                 "popq %%rbp\n\t"
                 :
                 :"r"(lt),
                  "i"(offsetof(struct light_thread, main_rsp)),
                  "i"(offsetof(struct light_thread, rsp))
                 );
}


