#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

/* firstdrvtest on
  * firstdrvtest off
  */
int main(int argc, char **argv)
{
	int fd;
	unsigned char key_val[6];
	fd = open("/dev/buttons", O_RDWR);
	if (fd < 0)
	{
		printf("can't open!\n");
	}
	while(1)
	{
		read(fd,key_val,sizeof(key_val));
		if(!key_val[0]||!key_val[1]||!key_val[2]||!key_val[3]||!key_val[4]||!key_val[5])
		{
			printf("buttons :%d %d %d %d %d %d",key_val[0],key_val[1],key_val[2],key_val[3],key_val[4],key_val[5]);
			}	
	}
	return 0;
}
