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
    long long output =(long long) arg;
//    printf("arg is %llu\n", output);
    output *= 3;
    output += 4;
//    printf("output is %d\n", output);
    foo2(output);
}

int main()
{
    unsigned long long arg = 12345;
    int err = 0;

    err = init_main(1);
    if (err)
    {
        printf("init main fail.\n");
        return err;
    }
    err = spawn(foo, (void *)arg);
    if (err)
    {
        printf("spawn fail: %d\n", err);
        return 0;
    }

    for(;;);
    return 0;
}
