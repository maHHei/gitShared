#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
#ifdef centos
    pirntf("centos\n");
#endif
    printf("hello world");
    return 0;
}
