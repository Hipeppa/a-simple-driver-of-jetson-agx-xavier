#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(void){
        int fd1 = open("/dev/mygpio0", O_RDWR);
		int fd2 = open("/dev/mygpio1", O_RDWR);
		int flag = 10;
        if(fd1 < 0)
        {
        	printf("open pin29 fail\n");
        }
		if(fd2 < 0){
			printf("open pin31 fail\n");

		}
        while(flag){
	        char cmd1 = '1';
			char cmd2 = '0';
			int ret;
			if(flag%2 == 0){
				ret = write(fd1,&cmd1,1);
				if (ret < 0)
				{
					printf("write fail\n");
				}else{
					printf("pin29输出%c", cmd);
				}
				ret= write(fd2,&cmd2,1);
	        	if (ret < 0)
	        	{
	        		printf("write fail\n");
	        	}else{
					printf("pin31输出%c", cmd);
				}

			}
	        else{
				ret = write(fd1,&cmd2,1);
				if (ret < 0)
				{
					printf("write fail\n");
				}else{
					printf("pin29输出%c", cmd);
				}
				ret= write(fd2,&cmd1,1);
	        	if (ret < 0)
	        	{
	        		printf("write fail\n");
	        	}else{
					printf("pin31输出%c", cmd);
				}
			
			}
			sleep(2);
			flag--;
	        
        }
		int ret = 0;
		ret = close(fd1);
		if(ret < 0){
			printf("mygpio0关闭失败！\n");
		}
		ret = close(fd2);
		if(ret < 0){
			printf("mygpio1关闭失败！\n");
		}
        return 0;
 
}

