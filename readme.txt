#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
<<<<<<< HEAD
#ifdef WIN32
    printf("windows\n");
#endif
=======
#ifdef centos
    pirntf("centos\n");
>>>>>>> 50e352d71c340820892705b49c18b6d4bf6e814f
#endif
    printf("hello world");
    return 0;
}
