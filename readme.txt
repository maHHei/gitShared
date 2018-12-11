#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
#ifdef WIN32
    printf("windows\n");
#endif
    printf("hello world");
    return 0;
}
