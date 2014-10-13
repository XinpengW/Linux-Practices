
/*利用ioctl()进行接口相关的操作.*/

#include <stdio.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


int main(int argc, char** argv)
{
	int fd;
	int len;
	char * buf;
	struct ifconf ifc;
	struct ifreq *ifr;
	
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	len = 20*sizeof(struct ifreq);
	buf = (char*)malloc(len);
	ifc.ifc_len = len;
	ifc.ifc_buf = buf;
	
	if(ioctl(fd, SIOCGIFCONF, &ifc) < 0)
	{
		perror("ioctl() error:");
	}

	int num, i;
	num = ifc.ifc_len / sizeof(struct ifreq);  //返回的获取接口个数.
	
	ifr = ifc.ifc_req;
	
	//依次输出每个接口的信息.
	for(i=0; i< num; i++, ifr++)
	{
		

		printf("#Information for Interface %d:\n", i);
		printf("Interface Name: %s\n", ifr->ifr_name);
		
		if(ioctl(fd, SIOCGIFADDR, ifr) < 0) //获取接口的IP地址.
		{
			perror("SIOCGIFADDR Error:");
		}
		printf("IP Address: %s\n", inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr));
		
		if(ioctl(fd, SIOCGIFNETMASK, ifr) < 0) //获取接口的子网掩码.
		{
			perror("SIOCGIFNETMASK Error:");
		}
		printf("Network Mask: %s\n", inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr));
		
		if(ioctl(fd, SIOCGIFBRDADDR, ifr) < 0) //获取接口的广播地址.
		{
			perror("SIOCGIFBRDADDR Error:");
		}
		printf("The Broadcast Address is: %s\n", inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr));
		
		if(ioctl(fd, SIOCGIFHWADDR, ifr) < 0) //获取接口的MAC地址.
		{
			perror("SIOCGIFHWADDR Error:");
		}
		printf("The MAC address is: %02x:%02x:%02x:%02x:%02x:%02x\n", 
				(unsigned char)(ifr->ifr_hwaddr).sa_data[0],
				(unsigned char)(ifr->ifr_hwaddr).sa_data[1],
				(unsigned char)(ifr->ifr_hwaddr).sa_data[2],
				(unsigned char)(ifr->ifr_hwaddr).sa_data[3],
				(unsigned char)(ifr->ifr_hwaddr).sa_data[4],
				(unsigned char)(ifr->ifr_hwaddr).sa_data[5]);
	
		if(ioctl(fd, SIOCGIFMTU, ifr) < 0) //获取接口的MTU.
		{
			perror("SIOCGIFMTU Error:");
		}
		printf("The MTU is: %d\n", ifr->ifr_mtu);
		
		
		putchar('\n');
		putchar('\n');
	}
	
	free(buf);
	
	
	
	return 0;
}
