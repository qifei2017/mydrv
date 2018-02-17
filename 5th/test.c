#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

int fd;
void signalfun(int signum)
{
	unsigned char keyval;
	read(fd,&keyval,1);
	printf("key_val: 0x%x\n", keyval);
}
int main(int argc, char **argv)
{
	unsigned char key_val;
	int Oflags ;
	signal(SIGIO, signalfun);
	fd = open("/dev/buttons", O_RDWR);
	
	if (fd < 0)
	{
		printf("can't open!\n");
	}
	fcntl(fd, F_SETOWN, getpid());
	
	Oflags = fcntl(fd, F_GETFL); 
	
	fcntl(fd, F_SETFL, Oflags | FASYNC);
	while (1)
	{
		sleep(1000);
	}
	return 0;
}
