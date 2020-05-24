#include <stdio.h>  
#include <sys/socket.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <stdlib.h>   
#include <strings.h>
#include <string.h>
#include <pthread.h>

#define  SERVER_PORT 20000  //  define the defualt connect port id   
#define  CLIENT_PORT ((20001+rand())%65536)  //  define the defualt client port as a random port   
#define  BUFFER_SIZE 256   

int  servfd,clifd,sockfd,val=-1;  
struct  sockaddr_in servaddr,cliaddr;   
char  buf[BUFFER_SIZE],sendline[BUFFER_SIZE];
char *arg_temp;
void usage(char* name)  
{  
	printf( "usage: %s IpAddr\n " ,name);  
}  
void *thread_func(void *arg);
int main(int argc, char** argv)  
{  
	
	socklen_t socklen = sizeof (servaddr); 
  	pthread_t tid;
  	int t_arg=100;
	arg_temp=argv[1];
	if(argc < 2 )  
	{  
		usage(argv[0]);  
		exit( 1 );  
	}   
	if((clifd = socket(AF_INET,SOCK_STREAM,0))  <   0 )  //tcp
	{
		printf( " create socket error!\n " );  
		exit( 1 );  
	}

	srand(time(NULL)); // initialize random generator   
	
	bzero( & cliaddr, sizeof (cliaddr));  
	cliaddr.sin_family = AF_INET;  
	cliaddr.sin_port   = htons(CLIENT_PORT);  
	cliaddr.sin_addr.s_addr = htons(INADDR_ANY); 
	
	if(bind(clifd,(struct sockaddr* )&cliaddr,sizeof(cliaddr)) < 0 )  
	{
		printf( "bind to port %d failure!\n " ,CLIENT_PORT);  
		exit( 1 );  
	}//绑定的目的是让其端口是随机的，否则端口是自增1 //一般情况下client端不用绑定

	bzero(&servaddr, sizeof(servaddr)); 	 
	servaddr.sin_family = AF_INET;  
	inet_aton(argv[1], &servaddr.sin_addr);  
	servaddr.sin_port = htons(SERVER_PORT);  
	   
	if(connect(clifd,( struct  sockaddr *)&servaddr, socklen) < 0)  
	{  
		printf( "can't connect to %s!\n", argv[1]);  
		exit(1);  
	} 
	if(pthread_create(&tid,NULL,thread_func,&t_arg))
		printf("Fail to create thread!\n");
	sleep(1);
	while(1)
	{
		printf("0:send msg,1:No reception,2:quit\r\n");
		scanf("%d",&val);
		getchar();
		switch(val)
		{
			case 0:
				printf("input msg:\r\n");
				fgets(sendline,BUFFER_SIZE,stdin);
				if(send(clifd,sendline,strlen(sendline),0)<0)
				{
					printf("send msg error!\n");
					exit(1);
				}
				else
				{
					printf("send msg succeed!\n");
				}
			break;
			case 1:
				printf("No reception msg!\n");
				pthread_cancel(tid);
			break;
			case 2:  
				close(clifd);
                exit(1);
			break;
			default:
				printf("input error!\r\n");

		}
	}    
	return 0;  
} 
void *thread_func(void *arg)
{
	int *val = arg,length = 0;
	if(arg!=NULL)
	{
	        while(1)
		{
			length = recv(clifd, buf,BUFFER_SIZE, 0);
			if(length < 0)  
			{  
				printf("error comes when recieve data from server %s!", arg_temp);  
				exit(1);  
			}
			else if(length>0)
			{
				printf("from server %s msg:\n%s", arg_temp,buf);
				printf("0:send msg,1:No reception,2:quit\r\n");
			}
			length=0;
		}
	}

}
