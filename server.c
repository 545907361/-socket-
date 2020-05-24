#include  <stdio.h>
#include  <sys/types.h>
#include  <sys/socket.h>   
#include  <unistd.h>  
#include <sys/ioctl.h> 
#include  <netinet/in.h>   
#include  <arpa/inet.h>
#include  <stdlib.h>   
#include  <time.h> 
#include  <string.h>
#include  <strings.h>  
#include <pthread.h>  
#include <fcntl.h>
#include <termios.h>

#define  SERVER_PORT 20000          //define the defualt connect port id   
#define  LENGTH_OF_LISTEN_QUEUE 10  //length of listen queue in server   
#define  BUFFER_SIZE 256
#define COM0        0
#define BLOCK_MODE  1
#define NONBLK_MODE 0   

static struct termios g_newtio,g_oldtio;
int  servfd,clifd,fd,i;  
struct  sockaddr_in servaddr,cliaddr; 
char  sendline[BUFFER_SIZE],rebuf[BUFFER_SIZE],temp[1],para;  
long  timestamp; 
pthread_t trecvid,tsendid;
int recv_arg=100,send_arg=200;

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
void *thread_recv(void *arg);
void *thread_send(void *arg);
void RestoreComConfiguration(int fd,struct termios *ptios)
{
	if( tcsetattr(fd, TCSANOW, ptios) != 0 )	/*success*/
	{
		printf("Restore Com Parameter Error!\n");
	}
}
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
  			printf("Unsupported Parity\r\n");    
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
	
	g_newtio.c_cc[VTIME] = 0;       /* unit: 1/10 second. */ 
	g_newtio.c_cc[VMIN]  = 1;       /* minimal characters for reading */
	
  g_newtio.c_iflag &= ~INPCK;     /* Disable parity checking */ 
	g_newtio.c_cflag &= ~CRTSCTS;   //NO flow control
	g_newtio.c_cflag &= ~CSIZE;
	g_newtio.c_cflag |= CS8;        //databit=8
	g_newtio.c_cflag |= CLOCAL;     //本地连接
	g_newtio.c_cflag |= CREAD;      //允许接收数据
	
	if( tcsetattr(ret, TCSANOW, &g_newtio) != 0 )	/*success*/
	{
		printf("Set Com Parameter Error!\r\n");
		return -1;
	}
	
	tcflush (ret, TCIOFLUSH); //清空输入输出队列

	return ret;
}
int main(int argc, char** argv)  
{  
	
  	if((servfd = socket(AF_INET,SOCK_STREAM, 0 )) < 0) 
  	{  
		printf("create socket error!\n");  
		exit(1);  
	}   
	bzero(&servaddr, sizeof(servaddr));  
	servaddr.sin_family  =  AF_INET;  
	servaddr.sin_port    =  htons(SERVER_PORT);  
	servaddr.sin_addr.s_addr  =  htons(INADDR_ANY);  
	if(bind(servfd,(struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)  
	{  
		printf( "bind to port %d failure!\n" ,SERVER_PORT);  
		exit(1);  
	} 
	if(listen(servfd,LENGTH_OF_LISTEN_QUEUE) < 0)  //chuangjian dui lie
	{  
		printf( "call listen failure!\n" );  
		exit(1);  
	}  
	
	fd = Init_COM(COM0,115200,'N',1,BLOCK_MODE);
	if(fd<0)
	{
		printf("Init_com fail!");
	}
	while( 1 )  
	{ // server loop will nerver exit unless any body kill the process   
		 
		socklen_t length = sizeof(cliaddr);  
		clifd = accept(servfd,( struct sockaddr *) &cliaddr, &length);  
    		if  (clifd < 0)  
		{  
			printf( " error comes when call accept!\n " );  
			break ;  
		}
		else
		{
			printf("from client,IP:%s,Port:%d\r\n" ,inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
			if(pthread_create(&trecvid,NULL,thread_recv,&recv_arg))
				printf("Fail to create thread recv!\n");
			sleep(1);
			if(pthread_create(&tsendid,NULL,thread_send,&send_arg))
				printf("Fail to create thread send!\n");
			sleep(1);
			
			
		}
		 if(para=='3')break;
			
		//close(clifd);             
	} // exit   
	
	close(servfd);  
	return   0;  
} 
void *thread_recv(void *arg)
{
	int *val = arg,length = 0;
	if(arg!=NULL)
	{
	  
		while(1)
		{
			length=recv(clifd,rebuf,BUFFER_SIZE,0);
			if(length<0)
			{
				printf("recv error!\n");
			}
			else if(length>0)
			{	/*
				if(fd>=0)
				{
				//printf("from client msg:");
				//strcat(rebuf,"\r\n");
				
					write(fd,rebuf,strlen(rebuf));
					usleep(5000);
				}*/
				printf("from client msg:%s\r\n",rebuf);
				printf("0:send msg,1:No reception,2:close,3:quit\r\n");
			}
			length=0;
		}
	}
	
}
void *thread_send(void *arg)
{
	
	if(arg!=NULL)
	{
			while(1)
			{ 
				printf("0:send msg,1:No reception,2:close,3:quit\r\n");
				//scanf("%d",&val);
				//memset(temp,0,1);
				para = 0;		
				read(fd,temp,1);
				write(fd,temp,1);
				getchar();	
				para = temp[0];
				printf("\r\n");
				if(para=='2'||para=='3')break;
				switch(para)
				{
					case '0':
						printf("input msg:\r\n");
						//fgets(sendline,BUFFER_SIZE,stdin);
						//printf("Serial port-->network send msg:");
						memset(sendline,0,BUFFER_SIZE);
						i=0;
						temp[0]=0;
						while(1)
						{
							read(fd,temp,1);
							if(temp[0]==13||temp[0]==10)
							{
							   break;
							}
							sendline[i++]=temp[0];
							write(fd,temp,1);
						}
						//printf("$$$$$%s$$$$",sendline);
						timestamp  =  time(NULL);  
						strcat(sendline, "---timestamp in server:" );  
						strcat(sendline,ctime(&timestamp));
						printf("\r\n");
						if(send(clifd,sendline,strlen(sendline),0)<0)
						{
							printf("send msg error!\r\n");
							
						}
						else
						{
							printf("send msg succeed!\r\n");
						}
					break;
					case '1':
						printf("No reception msg!\n");
						pthread_cancel(trecvid);
					break;
					case '2': 
						pthread_cancel(trecvid);
						pthread_cancel(tsendid); 
						close(clifd);
					break;
					case '3': 
						pthread_cancel(trecvid);
						pthread_cancel(tsendid); 
						close(clifd);
						close(servfd);
						exit(1);
					break;
					default:
						printf("input error!\r\n");

				} 
			}
		pthread_cancel(trecvid);
		pthread_cancel(tsendid);
		close(clifd);
	}
}
