

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

/*北京地区DNS服务器地址: 211.161.45.222 or 144.144.144.144*/
/*www.baidu.com的IP地址：61.135.169.125*/
struct icmp_echo_msg
{
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t id;
	uint16_t sn;
	uint32_t sec;
	uint32_t usec;
} echo;

uint16_t create_checksum(uint16_t count, uint16_t *addr)
{
	unsigned long sum = 0;

	while(count > 1)
	{
		sum += *addr++;
		count -= 2;
	}
	if(count > 0)
	{
		sum += *(uint8_t*)addr;

	}
	if(sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return (uint16_t)~sum;
}

int main(int argc, char** argv)
{
	int sock;
	socklen_t addr_len; //用于需要socket address结构的长度参数的地方.
	struct sockaddr_in host_addr;	//本机socketaddr结构, 全局使用.
	struct sockaddr_in peer_addr;	//对端socketaddr结构, 全局使用.
	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(0);

	//分析命令行参数,并解析出目标主机的IP地址. -a 表示命令行输入的是FQDN；-n表示命令行输入的是IP地址.

	char c;
	opterr = 0;
	struct hostent *he;
	
	while((c = getopt(argc, argv, "na")) != -1)
	{
		switch(c)
		{
			case 'n':
				inet_aton(argv[2], &peer_addr.sin_addr);
				break;
			case 'a':
				he = gethostbyname(argv[2]);
				/*if(he == 0)
				{
					puts("Domain Name Error");
					return 0;
				}*/
				peer_addr.sin_addr.s_addr = *(long*)(he->h_addr_list[0]);
				//puts("aa");
				//inet_aton(inet_ntoa(*((struct in_addr*)*he->h_addr_list)), &peer_addr.sin_addr);
				break;
			default:
				break;
		}
	}
	
	
	
	struct timeval tval;
	int rcv_buffer_size = 1024;
	int snd_buffer_size = 1024;

	if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
	{
		perror("socket creation error");
	}


	//应用程序并不操作IP header，而是留给内核来处理.
	int off = 0;
	if(setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &off, sizeof(off)) < 0)
	{
		perror("Set socket option error");
	}
	//设置接收缓存大小.
	if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_buffer_size, sizeof(int32_t)) < 0)
	{
		perror("Set socket option error");
	}
	//设置发送缓存大小.
	if(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &snd_buffer_size, sizeof(int32_t)) < 0)
	{
		perror("Set socket option error");
	}
	//设置接收超时.
	tval.tv_sec = 5;
	tval.tv_usec = 0;
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(struct timeval)) < 0)
	{
		perror("Set socket option error");
	}
	//设置发送超时.
	tval.tv_sec = 5;
	tval.tv_usec = 0;
	if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tval, sizeof(struct timeval)) < 0)
	{
		perror("Set socket option error");
	}
	
	/*
	 * ICMP echo/echo reply消息的发送与接收处理.
	 * ?是按照一发一收的方式，还是按照一下子发送4条echo消息的方式.:本程序选择的是一下子发4条的方法.
	 * ?如何设置socket接收队列的大小.:通过SOL_SOCKET层选项来设置.
	 * */
	int attemps = 0;
	int seq_no = 100;
	while(attemps++ < 4) //接连发送4次ICMP echo消息.
	{
		//构造ICMP echo 消息.
		echo.type = 8;
		echo.code = 0;
		echo.id = htons(getpid());
		echo.sn = htons(seq_no++);
		gettimeofday(&tval, NULL); //获取当前精确时间.
		echo.sec = tval.tv_sec;
		echo.usec = tval.tv_usec;
		echo.checksum = 0;
		echo.checksum = create_checksum(sizeof(echo), (uint16_t*)&echo);


		if(sendto(sock, &echo, sizeof(echo), 0, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0)
		{
			perror("Sendto function error");
			return 0;
		}

	}
	
	/*
	 * 并处理显示相应的ICMP echo reply. 
	 *?需要手动设置4次读取循环，还是可以利用事件触发机制? :此处采用循环读取的方式.
	 * 需要设置对于超时的处理.
	 * */
	attemps = 0;
	char buf[200];
	int len = 0;

	/*
	addr_len = sizeof(struct sockaddr_in);
	if(getsockname(sock, (struct sockaddr*)&host_addr, &addr_len) < 0)
	{
		perror("Get Host Socket Address Failed");
		
	}
	printf("%s \n", inet_ntoa(host_addr.sin_addr));  //对原始socket使用getsockname()为何总是返回0.0.0.0??
	*/
	

	
	
	/********数据读取方法一: 直接利用recvfrom自身的阻塞来读取接收到的data: Begin********/
	//while(attemps++ < 4)
	//{
		//if(recvfrom(sock, buf, 200, 0, (struct sockaddr*)&peer_addr, (socklen_t*)&len) < 0)
		//{
			////perror("Recvfrom Function Error");
			//switch(errno)
			//{
				//case EWOULDBLOCK:  //接收超时.
				//puts("Receiving Timeout.");
				//default: break;
			//}
		//}
		//else
		//{	//输出ICMP echo response消息.
			//struct in_addr dest, src;
			//src.s_addr = *(uint32_t*)(buf+12);
			//dest.s_addr = *(uint32_t*)(buf+16);
			
			////printf("%s %s\n", inet_ntoa(src), inet_ntoa(dest));  //这种方式为何总是显式两个相同的IP地址？
			//printf("%s --> ", inet_ntoa(src));
			//printf("%s, ", inet_ntoa(dest));
			////为何输出的源地址和目的地址总是相同？？？
			//printf(" 0x%d Bytes, ID 0x%x, Seq 0x%x \n", len, ntohs(*(uint16_t*)(buf+24)), ntohs(*(uint16_t*)(buf+26)));
			
			
			///*printf("%s --> %s, 0x%d Bytes, ID 0x%x, Seq 0x%x \n",
			//inet_ntoa(peer_addr.sin_addr),inet_ntoa(aa), len, ntohs(*(uint16_t*)(buf+24)), ntohs(*(uint16_t*)(buf+26)));*/
		//}
	//}
	/********直接利用recvfrom自身的阻塞来读取接收到的data: End********/
	
	/********数据读取方法二: 利用select的I/O复用来读取接收到的data: Begin********/
	fd_set rset;
	int result;
	tval.tv_sec = 3;
	tval.tv_usec = 0;
	
	while(attemps++ < 4)
	{
		FD_ZERO(&rset);
		FD_SET(sock, &rset);
		result = select(FD_SETSIZE, &rset, NULL, NULL, &tval);
		if( result< 0)
		{
			perror("Select() error:");
		}
		else if (result == 0)
		{
			puts("Select() Timeout!");
		}
		else
		{
			if(FD_ISSET(sock, &rset))
			{
				recvfrom(sock, buf, 200, 0, (struct sockaddr*)&peer_addr, (socklen_t*)&len);
				//输出ICMP echo response消息.
				struct in_addr dest, src;
				src.s_addr = *(uint32_t*)(buf+12);
				dest.s_addr = *(uint32_t*)(buf+16);
			
				//printf("%s %s\n", inet_ntoa(src), inet_ntoa(dest));  //这种方式为何总是显式两个相同的IP地址？
				printf("%s --> ", inet_ntoa(src));
				printf("%s, ", inet_ntoa(dest));
				printf(" 0x%d Bytes, ID 0x%x, Seq 0x%x \n", len, ntohs(*(uint16_t*)(buf+24)), ntohs(*(uint16_t*)(buf+26)));
			
			}
		}
		
	}
	
	/********利用select的I/O复用来读取接收到的data: End********/
	
	return 0;
}
















