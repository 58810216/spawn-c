#include <stdio.h>
#include <stdlib.h>

#include "spawn.h"

void foo(void *arg)
{
    long long output =(long long) arg;
    printf("arg is %llu\n", output);
    back_to_main(0);
    output++;
    printf("output is %llu\n", output);
}

int main()
{
    unsigned long long arg = 0;
    int err = 0;
    int i = 0;

    err = init_main(2);
    if (err)
    {
        printf("init main fail.\n");
        return err;
    }
    for (i = 0; i < 10000000; i++)
    {
        arg+= 100000;
        err = spawn(foo, (void *)arg);
    }
    if (err)
    {
        printf("spawn fail: %d\n", err);
        return 0;
    }

    for(;;);
    return 0;
}
