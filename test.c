#include <stdio.h>
#include <stdlib.h>

#include "spawn.h"

void foo2(long long output)
{
    int out = (int )output;
    printf("%d\n", out);
}


void foo(void *arg)
{
    long long output = arg;
//    printf("arg is %llu\n", output);
    output *= 3;
    output += 4;
//    printf("output is %d\n", output);
    foo2(output);

    back_to_main();
}

int worker_run()
{
    struct light_thread *pw;

    pw = get_one_thread();
    if (pw)
    {
        run_thread(pw);
        return 1;
    }

    return 0;
}

int main()
{
    unsigned long long arg = 12345;
    int err = 0;

    err = spawn((void*)arg, foo);
    if (err)
    {
        printf("spawn fail: %d\n", err);
        return 0;
    }

    err = worker_run();
    printf("run set %d\n", err);

    return 0;
}
