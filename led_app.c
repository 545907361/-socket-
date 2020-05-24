#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define LED_DEVICE            "/dev/led_cugb"

#define LED_ON                1
#define LED_OFF               0

static unsigned char buf_wr[10]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99};
static unsigned char buf_rd[10];

int main()
{
        int fd;
        int val=-1;
	int i,ret;

       
        if( (fd = open(LED_DEVICE,O_RDWR)) < 0 )
        {
                printf("open device error!\r\n");
                exit(1);
        }
       
        while(1)
        {
                printf("0:LED OFF,1:LED ON,2:Write test,3:Read test,4:quit\r\n");
                scanf("%d",&val);
	            	
		switch(val)
		{
			case 0:
				ioctl(fd,LED_ON,0);
			break;
			case 1:
				ioctl(fd,LED_OFF,0);
			break;
			case 2:
				ret = write(fd,buf_wr,sizeof(buf_wr));
				if(ret < 0)
				{
					printf("Write error!\r\n");
				}
			break;
			case 3:
				ret = read(fd,buf_rd,sizeof(buf_rd));
				if(ret >= 0)
				{	
					printf("app: read %d bytes:",ret);
					for(i=0;i<ret;i++)
					{
						printf("0x%02x ",buf_rd[i]);
					}
					printf("\r\n");
				}
				else
				{
					printf("Read error!\r\n");
				}
			break;
			case 4:
				close(fd);
                        	exit(1);
			break;
			default:
				printf("input error!\r\n");

		}
             
        }
}

