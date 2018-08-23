#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "myerror.h"

#define MAXLISTEN 1024 
#define SER_PORT 8888

typedef struct myepoll_s
{
	int fd;//必须有的文件描述符	
	uint32_t events;
	void* ptr;
	void (*cb)(int fd, uint32_t events,  void *ptr);
	int status;//判断是否加入了监听树中
	char buf[BUFSIZ];
}epoll_point_t;

int epfd;//监听树句柄
epoll_point_t epoint[MAXLISTEN+1]; 

void perr_exit(char *buf)
{
	perror(buf);	
	exit(1);
}

void epollSet(int fd, epoll_point_t *ept, void (*cb)(int fd, uint32_t events,void *ptr), void *ptr)
{
	ept->fd = fd;
	ept->events = 0;
	ept->cb = cb;
	ept->ptr = ptr;

	//设置为非阻塞读写
	int flag = fcntl(fd, F_GETFL, 0);
	flag |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flag);
}

void epollAdd(int events, epoll_point_t *ept)
{
	struct epoll_event ev;
	ept->status = 1;
	ev.events = ept->events = events; 
	ev.data.ptr = ept;	
	int flag = epoll_ctl(epfd, EPOLL_CTL_ADD, ept->fd, &ev);
	if(flag == -1)
	{
		perr_exit("epoll_ctl error");
	}
}

void epollDel(int fd, int status, void *ptr)
{
	epoll_point_t *ept = (epoll_point_t *)ptr;		
	struct epoll_event ev;
	ev.events = ept->events; 
	ev.data.ptr = ptr; 
	ept->status = status;
	int flag = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
	if(flag == -1)
	{
		perr_exit("epollDel():epoll_ctl error");
	}
}


void writeData(int fd, uint32_t ev, void *ptr);
void readData(int fd, uint32_t ev, void *ptr)
{
	int size=0;
	int pos=0;
	epoll_point_t *ept=(epoll_point_t *)ptr;
	//非阻塞读取数据
	while((size = read(fd, ept->buf, sizeof(ept->buf))) > 0)
	{
		for(int i=0; i<size; ++i)	
		{
			ept->buf[pos] = toupper(ept->buf[pos]);	
			pos++;
		}
	}
	if(size == 0)
	{
		epollDel(fd, 0, ptr);//摘下监听节点
		close(fd);//关闭连接
		return;
	}
	ept->buf[pos] = 0;
	//如果客户端没有退出
	//摘下读监听,换上写监听,不可进行status的变化,因为status为0时就有可能发生被占位的风险
	epollDel(fd, 1, ptr);
	epollSet(fd, ept, writeData, ptr);
	epollAdd(EPOLLOUT, ptr);
}

void writeData(int fd, uint32_t ev, void *ptr)
{
	epoll_point_t *ept = (epoll_point_t *)ptr;
	int size = 0;
	int max = strlen(ept->buf);
	do
	{
		size = write(fd, ept->buf, max);	
		if(size == -1)
		{
			if(errno == EINTR || errno == EAGAIN)	
			{
				continue;
			}
			else
			{
				perr_exit("write error");
			}
		}
		max -= size;
	}while(max > 0);
	epollDel(fd, 1, ptr);
	epollSet(fd, ept, readData, ptr);
	epollAdd(EPOLLIN, ptr);
}

//cb(int fd, uint32_t events,  void *ptr);
//epollSet(sfd, conDeal, &epoint[MAXLISTEN]);
void conDeal(int fd, uint32_t ev, void *ptr)
{
	printf("condeal");
	struct sockaddr_in cli_addr;
	int i;
	socklen_t addr_len = sizeof(cli_addr);
	//获取链接的对端的文件描述符
	int cfd = Accept(fd, (struct sockaddr *)&cli_addr, &addr_len);
	
	for(i=0; i<=MAXLISTEN; ++i)
	{
		if(epoint[i].status == 0)	
		{
			break;
		}
	}
	if(i == MAXLISTEN+1)
	{
		write(cfd, "server is busy",15);	
		return;
	}
	//加入监听树中
	epollSet(cfd, &epoint[i], readData, &epoint[i]);
	epollAdd(EPOLLIN, (void *)&epoint[i]);
}


void setListen()
{
	//创建监听套接字
	int sfd = socket(AF_INET, SOCK_STREAM, 0);	
	if(sfd == -1)
	{
		perr_exit("socket error");
	}

	//把该节点加入监听树中
	uint32_t events = EPOLLIN|EPOLLET;
	epollSet(sfd, &epoint[MAXLISTEN], conDeal, &epoint[MAXLISTEN]);
	epollAdd(events, &epoint[MAXLISTEN]);

	//绑定监听端口
	struct sockaddr_in ser_addr;
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(SER_PORT);
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	Bind(sfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
	//限制最大的同时3次握手数量,listen最大只能为128
	Listen(sfd, 128);
	
	return;
}

int main(int argc, char **argv)
{
	epoll_point_t *temp = NULL;
	struct epoll_event ev[MAXLISTEN+1];
	//先创建epoll监听树的根节点
	epfd = epoll_create(MAXLISTEN+1);
	if(epfd == -1)	
	{
		perr_exit("epoll_create error");
	}
	bzero(epoint, sizeof(epoint));
	bzero(ev,sizeof(ev));
	setListen();
	while(1)
	{
		int flag = epoll_wait(epfd, ev, MAXLISTEN+1, -1);	
		if(flag == -1)
		{
			perr_exit("epoll_wait error");	
		}
		for(int i=0; i<flag; ++i)	
		{
#if 1
			temp = (epoll_point_t *)ev[i].data.ptr;
			
			if(ev[i].events & EPOLLIN)
			{
				temp->cb(temp->fd, temp->events, (void *)temp);
			}
			if(ev[i].events & EPOLLOUT)
			{
				temp->cb(temp->fd, temp->events, (void *)temp);
			}
#endif
		}
	}
	return 0;
}






















