#include<stdio.h>

int print(int a);

main()
{
    int a = 7;
hhh:
    print(a);
    if (a == 0)
        return;
    else
        --a;
    return;
}

int print(int a)
{
    printf("%d\n", a);
}