
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
struct reg {
  unsigned int addr;
  unsigned int data;
};

int main(int argc, char **argv)
{
	int fd;
    struct reg val;
	fd = open("/dev/my_reg", O_RDWR);
	if (fd < 0)
	{
		printf("can't open /dev/my_reg !\n");
	}
	if(argc == 2){
	sscanf(argv[1],"%x",&val.addr);
	read(fd, &val, sizeof(val));
	printf("0x%x: 0x%x\n",val.addr,val.data);
	}
	else if(argc == 3){
	sscanf(argv[1],"%x",&val.addr);
	sscanf(argv[2],"%x",&val.data);
	write(fd, &val, sizeof(val));
	read(fd, &val, sizeof(val));
	printf("0x%x: 0x%x\n",val.addr,val.data);
	}
	else{
		printf("Usage :\n");
		printf("<./my_reg addr> or <./my_reg addr data>\n");
		return 0;
	}
	return 0;
}
