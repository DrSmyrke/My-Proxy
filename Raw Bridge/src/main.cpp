#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#define MTU			1500
#define IFACE		"eth0"
//./tap_bridge -i wlan0 -t tap1 [-v] [-vv] и т.д.

#include "device.h"


int main(int argc, char *argv[])
{
	Device devWlan("wlxd46e0e115a54",Device::type_vlan);
	devWlan.setMode( Device::mode_monitor );
	Device devTap("tap0",Device::type_tap);
	devTap.setMode( Device::mode_monitor );



	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	int one = 0;
	const int *val = &one;

	int sdRx = socket( PF_PACKET, SOCK_RAW, htons(ETH_P_ALL) );
	if (sdRx < 0) {
		perror("socket() error");
		return 2;
	}
	printf("OK: a raw socket is created.\n");

	fcntl(sdRx, F_SETFL, O_NONBLOCK);
	//int ret = bind( sdRx, &addr, &len);

	// сообщаем ядру не создавать структуру пакетов
//	if(setsockopt(sdRx, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
//		perror("setsockopt() error");
//		return 2;
//	}
//	if(setsockopt(sdTx, IPPROTO_RAW, IP_HDRINCL, val, sizeof(one)) < 0) {
//		perror("setsockopt() error");
//		return 2;
//	}
//	printf("OK: socket option IP_HDRINCL is set.\n");



//	ifr.ifr_flags |= IFF_PROMISC;
//		if (ioctl(s_sock, SIOCGIFFLAGS, &ifr) < 0) {
//		  perror("Unable to set promiscious mode for device");
//		  close(s_sock);
//		  exit(-1);
//		}

//	struct ifreq ifr;
//	memset(&ifr, 0, sizeof(ifr));
//	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
//	strncpy(ifr.ifr_name, "tap0", IFNAMSIZ);
//	int fd = open("/dev/net/tun", O_RDWR);
//	int err = ioctl(fd, TUNSETIFF, (void*) &ifr);

	fd_set rfds;
	FD_ZERO( &rfds );
	FD_SET( sdRx, &rfds );
	struct timeval tv;


	char buf[MTU];
	memset(buf, 0, sizeof(buf));
	struct sockaddr_ll sll;
	socklen_t size = sizeof(struct sockaddr_ll);

	char interface[15];

	while(1){
		fd_set rfds;

		FD_ZERO(&rfds);
		FD_SET(sdRx, &rfds);
		FD_SET(devTap.getDescriptor(), &rfds);

		tv.tv_sec = 0;
		tv.tv_usec = 10000;

		int recVal = select(sdRx + 1, &rfds, NULL, NULL, &tv);

		switch(recVal){
			case(0):		break;		//Timeout
			case(-1):		break;		//Error
			default:
			{
				if( FD_ISSET(sdRx, &rfds) ){		//RAW SOCKET
					int rx = recvfrom(sdRx, buf, sizeof(buf), 0, (struct sockaddr *)&sll, &size);
					if( rx >= 0 && sll.sll_ifindex == devWlan.getIFindex() ){
						if_indextoname(sll.sll_ifindex, interface);
						printf("recvfrom [%s] >: %d bytes\n", interface, rx);

						int tx = devTap.send(buf, rx);
						if( tx >= 0 ){
							printf("sendto <: %d bytes\n", tx);
						}
					}
					break;
				}
				if(  FD_ISSET(devTap.getDescriptor(), &rfds)  ){
					int rx = devTap.recv(buf, sizeof(buf));
					if( rx >= 0 ){
						printf("recvfrom [%s] >: %d bytes\n", devTap.getName(), rx);
					}
				}
				break;
			}
		}



//
//


//
//
//		usleep(10);
//		//if( sll.sll_ifindex == dev.getIFindex() ) continue;
//		//if( sll.sll_ifindex == if_nametoindex("enp3s0") ) continue;
//
//		if( tx >= 0 ){
//			char interface[15];
//

//			printf("recvfrom [%s][%d] >: %d bytes\n", interface, sll.sll_ifindex, rx);
//			printf("sendto <: %d bytes\n", tx);
//		}
	}

	return 0;
}
