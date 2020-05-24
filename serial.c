#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <linux/rtc.h>

#define COM0        0
#define BLOCK_MODE  1
#define NONBLK_MODE 0

static struct termios g_newtio,g_oldtio;

static int speed_arr[] = 
{
 B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300,
 B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300, 
};

static int name_arr[] = 
{
 115200, 57600, 38400,  19200,  9600,  4800,  2400,  1200,  300, 
 115200, 57600, 38400,  19200,  9600,  4800,  2400,  1200,  300, 
};


/**********************************************************************
* 函数名称： // 函数名称
* 功能描述： // 函数功能、性能等的描述
* 输入参数： // 输入参数说明，包括每个参数的作用、取值说明及参数间关系
* 输出参数： // 对输出参数的说明。
* 返 回 值：  -1：Failed     
* 其它说明： // 其它说明
* 修改日期        版本号     修改人	      修改内容
* -------------------------------------------------------------
* 2007/10/10	     V1.0	    XXXX	      XXXX
***********************************************************************/
int Init_COM(int Comm,int Baudrate,int Parity,int Stopbit,int Flagblock)
{
	int ret,i;
	char dev_buf[16];
		
	if(Comm > 3)
	{
		printf("Com%d not exist\n",Comm);	
		return -1;
	}
	
	memset(dev_buf,0x00,sizeof(dev_buf));
	sprintf(dev_buf,"/dev/ttyS%d",Comm);
	
 	if(Flagblock)
 	{
		ret = open(dev_buf, O_RDWR | O_NOCTTY );            // 以默认阻塞方式打开;
 	}
 	else
 	{
 		ret = open(dev_buf, O_RDWR | O_NOCTTY | O_NONBLOCK); //以非阻塞方式打开;
 	}
 	
 	if(ret < 0)
 	{
 		printf("Open ttyS%d failed\n",Comm);
 		return -1;	
 	}
 		
	if( tcgetattr(ret, &g_oldtio) < 0 )
 	{
 		printf("Get Com Parameter Error.\n");	
 		return -1;
 	}
 	
 	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) 
 	{ 
  		if(Baudrate == name_arr[i]) 
  		{     
   			cfsetispeed(&g_newtio,speed_arr[i]);
   			cfsetospeed(&g_newtio,speed_arr[i]);
   			break;
  		}  
 	}
 	
 	if(i>=sizeof(speed_arr) / sizeof(int))
 	{
 			printf("Unsupported Speed!\n");
  		return -1;
 	}
 	 	
 	switch (Parity) 
 	{ // 设置奇偶校验位数
 		case 'n':
		case 'N':    
  			g_newtio.c_cflag &= ~PARODD;  //very important; zwh
  			g_newtio.c_cflag &= ~PARENB;  /* Clear parity enable */
  		break;  
 		case 'o':   
 		case 'O':     
 				g_newtio.c_cflag |= PARENB;    /* Enable parity */ 
  			g_newtio.c_cflag |= PARODD;    /* 设置为奇效验*/  
  		break;  
 		case 'e':  
 		case 'E':   
  			g_newtio.c_cflag |= PARENB;     /* Enable parity */    
  			g_newtio.c_cflag &= ~PARODD;    /* 转换为偶效验*/     
  		break;
 	
 		default:   
  			printf("Unsupported Parity\n");    
  		return -1;  
 	}	
 	
 	/*stop bit */
	switch(Stopbit)
	{
		case 1:
			g_newtio.c_cflag &= ~CSTOPB;
		break;
		case 2:
			g_newtio.c_cflag |= CSTOPB;
		break;
		default:
			printf("Unsupported Stopbit!\n");	
			return -1;
	}
	g_newtio.c_iflag = 0;           //输入无校验
	g_newtio.c_oflag = 0;           //输出不处理
	g_newtio.c_lflag  = 0;          //RAW模式
	
	g_newtio.c_cc[VTIME] = 10;       /* unit: 1/10 second. */ 
	g_newtio.c_cc[VMIN]  = 5;       /* minimal characters for reading */
	
  g_newtio.c_iflag &= ~INPCK;     /* Disable parity checking */ 
	g_newtio.c_cflag &= ~CRTSCTS;   //NO flow control
	g_newtio.c_cflag &= ~CSIZE;
	g_newtio.c_cflag |= CS8;        //databit=8
	g_newtio.c_cflag |= CLOCAL;     //本地连接
	g_newtio.c_cflag |= CREAD;      //允许接收数据
	
	if( tcsetattr(ret, TCSANOW, &g_newtio) != 0 )	/*success*/
	{
		printf("Set Com Parameter Error!\n");
		return -1;
	}
	
	tcflush (ret, TCIOFLUSH); //清空输入输出队列

	return ret;
}


void RestoreComConfiguration(int fd,struct termios *ptios)
{
	if( tcsetattr(fd, TCSANOW, ptios) != 0 )	/*success*/
	{
		printf("Restore Com Parameter Error!\n");
	}
}

int main(int argc,char *argv[])
{
	char info[]={"please input:\r\n"};
	char buf[64];
	int num;int i;
	int fd;
	fd = Init_COM(COM0,115200,'N',1,BLOCK_MODE);
	if(fd >= 0)
	{
		write(fd,info,strlen(info));
		usleep(5000);
		memset(buf,0,64);
		num=read(fd,buf,8);
		if(num>0)
		{
			for(i=0;i<num;i++)
			{printf("0x%02x",buf[i]);}
			printf("\r\n");
		}
		RestoreComConfiguration(fd,&g_oldtio);
		close(fd);
	}
	return 0;
}
/*
int main(int argc,char *argv[])
{
	int fd;
	fd = Init_COM(COM0,115200,'N',1,BLOCK_MODE);
	if(fd >= 0)
	{
		write(fd,"Hello...\n",0);
		usleep(5000);
		RestoreComConfiguration(fd,&g_oldtio);
		close(fd);
	}
	return 0;
}
*/

