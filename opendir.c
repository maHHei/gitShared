#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

int main(int argc,char **argv)
{
	int fd = open("hello world", O_RDWR | O_CREAT, 0664);
	if(fd == -1)
	{
		perrror("open error");
		exit(1);
	}
	opendir();	
	return 0;
}
