#include "device.h"

Device::Device(const char *ifname, const uint8_t type)
{
	m_type = type;

	uint8_t i = 0;
	while( ifname[i] != '\0' ){
		m_ifname[i] = ifname[i];
		i++;
	}
	m_ifname[i] = '\0';

	setIfName();



	switch(type) {
		case type_tap:
			del_tap();
			add_tap();
			ifup();
			m_ifreq.ifr_flags = IFF_TAP | IFF_NO_PI;
			m_desc = open(TUNDEV, O_RDWR);
			m_error = ioctl(m_desc, TUNSETIFF, (void*) &m_ifreq);
			if( m_desc >= 0 && m_error >= 0 ) m_init = true;
			printf("dsec: %d	error: %d\n",m_desc,m_error);
		break;
		case type_vlan:
			m_init = true;
		break;
	}



	if( m_init ){
		printf("Device [%s] created\n",m_ifname);
		init();
	}else{
		printf("Device [%s] created error\n",m_ifname);
	}


//	m_sd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
//	if((ioctl(m_sd,SIOCGIFINDEX,&m_ifreq))<0) printf("error in index ioctl reading");

//	printf("Device [%s] created from index = %d\n",m_ifname,m_ifreq.ifr_ifindex);

//	if(ioctl(m_sd,SIOCGIFHWADDR,&m_ifreq)<0) printf("error in SIOCGIFHWADDR ioctl reading");
//	if(ioctl(m_sd,SIOCGIFADDR,&m_ifreq)<0) printf("error in SIOCGIFADDR \n");

//	memset( m_txBuff, 0, sizeof( m_txBuff ) );

//	struct ethhdr *eth = (struct ethhdr *)(sendbuff);

//	eth->h_source[0] = (unsigned char)(m_ifreq.ifr_hwaddr.sa_data[0]);
//	eth->h_source[1] = (unsigned char)(m_ifreq.ifr_hwaddr.sa_data[1]);
//	eth->h_source[2] = (unsigned char)(m_ifreq.ifr_hwaddr.sa_data[2]);
//	eth->h_source[3] = (unsigned char)(m_ifreq.ifr_hwaddr.sa_data[3]);
//	eth->h_source[4] = (unsigned char)(m_ifreq.ifr_hwaddr.sa_data[4]);
//	eth->h_source[5] = (unsigned char)(m_ifreq.ifr_hwaddr.sa_data[5]);

//	/* filling destination mac. DESTMAC0 to DESTMAC5 are macro having octets of mac address. */
//	eth->h_dest[0] = 0xFF;
//	eth->h_dest[1] = 0xFF;
//	eth->h_dest[2] = 0xFF;
//	eth->h_dest[3] = 0xFF;
//	eth->h_dest[4] = 0xFF;
//	eth->h_dest[5] = 0xFF;

//	eth->h_proto = htons(ETH_P_IP);

	//total_len += sizeof(struct ethhdr);

//	struct iphdr *iph = (struct iphdr*)(m_txBuff + sizeof(struct ethhdr));
//	iph->ihl = 5;
//	iph->version = 4;
//	iph->tos = 16;
//	iph->id = htons(10201);
//	iph->ttl = 64;
//	iph->protocol = 17;
//	iph->saddr = inet_addr(inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
//	iph->daddr = inet_addr(destination_ip); // put destination IP address

	//total_len += sizeof(struct iphdr);
}

bool Device::setMode(uint8_t mode)
{
	if( !m_init ){
		printf("Device::init	device is NOT init\n");
		return false;
	}

	bool ret = true;
	switch (mode) {
		case mode_managed: printf("Set managed mode\n"); break;
		case mode_monitor: printf("Set monitor mode\n"); break;
		default:
			printf("Unknow mode\n");
			ret = false;
		break;
	}
	if( !ret ) return false;
	if( !ifdown() ) return false;

	int fd;
	struct iwreq iwr;
	memset(&iwr, 0, sizeof(iwr));
	fd = iw_sockets_open();
	iw_get_ext(fd, m_ifname, SIOCGIWMODE, &iwr);
	iwr.u.mode = mode;
	if(iw_set_ext(fd, m_ifname, SIOCSIWMODE, &iwr) < 0) ret = false;
	iw_sockets_close( fd );

	if( !ifup() ) return false;

	if( !ret ) printf("Change mode error\n");

	return ret;
}

int Device::send(void *__buf, size_t __n)
{
	if( !m_init ) return -1;

	if( m_type == type_tap ) return write( m_desc, __buf, __n ) ;

	return -1;
}

int Device::recv(void *__buf, size_t __n)
{
	int ret = -1;
	if( m_type == type_tap ){
		ret = read( m_desc, __buf, __n );
		return ret;
	}

	return ret;
}

void Device::init()
{
	if( !m_init ){
		printf("Device::init	device is NOT init\n");
		return;
	}

	if( m_type == type_tap ){
		int flags = fcntl( m_desc, F_GETFL, 0 );
		if( flags < 0 ) printf( "Error calling fcntl(F_GETFL)\n" );
		if( fcntl( m_desc, F_SETFL, flags | O_NONBLOCK ) < 0 ) printf( "Error calling fcntl(F_SETFL)\n" );
	}
}

bool Device::ifSetFlag(short flag)
{
	int fd;
	bool ret = true;
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_ifrn.ifrn_name, m_ifname, IFNAMSIZ);
	ioctl(fd, SIOCGIFFLAGS, &ifr);
	strncpy(ifr.ifr_ifrn.ifrn_name, m_ifname, IFNAMSIZ);
	ifr.ifr_flags |= flag;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) ret = false;

	close(fd);

	if( !ret ) printf("ifSetFlag error\n");

	return ret;
}

bool Device::ifClearFlag(short flag)
{
	int fd;
	bool ret = true;
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_ifrn.ifrn_name, m_ifname, IFNAMSIZ);
	ioctl(fd, SIOCGIFFLAGS, &ifr);
	strncpy(ifr.ifr_ifrn.ifrn_name, m_ifname, IFNAMSIZ);
	ifr.ifr_flags &= ~flag;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) ret = false;

	close(fd);

	if( !ret ) printf("ifClearFlag error\n");

	return ret;
}

void Device::setIfName()
{
	memset(&m_iwreq, 0, sizeof(m_ifreq));
	memset(&m_ifreq, 0, sizeof(m_iwreq));
	strncpy(m_iwreq.ifr_name, m_ifname, sizeof(m_ifname));
	strncpy(m_ifreq.ifr_name, m_ifname, sizeof(m_ifname));
}

bool Device::add_tap()
{
	struct ifreq ifr;
	int fd;

#ifdef IFF_TUN_EXCL
	ifr.ifr_flags |= IFF_TUN_EXCL;
#endif

	ifr.ifr_flags |= IFF_NO_PI;
	ifr.ifr_flags |= IFF_TAP;

	fd = open(TUNDEV, O_RDWR);
	if (fd < 0) {
		perror("open");
		return false;
	}
	if (ioctl(fd, TUNSETIFF, ifr)) {
		perror("ioctl(TUNSETIFF)");
		close(fd);
		return false;
	}
//	if (uid != -1 && ioctl(fd, TUNSETOWNER, uid)) {
//		perror("ioctl(TUNSETOWNER)");
//		close(fd);
//		return false;
//	}
//	if (gid != -1 && ioctl(fd, TUNSETGROUP, gid)) {
//		perror("ioctl(TUNSETGROUP)");
//		close(fd);
//		return false;
//	}
	if (ioctl(fd, TUNSETPERSIST, 1)) {
		perror("ioctl(TUNSETPERSIST)");
		close(fd);
		return false;
	}

	close(fd);
	return true;
}

bool Device::del_tap()
{
	struct ifreq ifr;

	int fd = open(TUNDEV, O_RDWR);

	if (fd < 0) {
		perror("open");
		return false;
	}
	if (ioctl(fd, TUNSETIFF, ifr)) {
		perror("ioctl(TUNSETIFF)");
		close(fd);
		return false;
	}
	if (ioctl(fd, TUNSETPERSIST, 0)) {
		perror("ioctl(TUNSETPERSIST)");
		close(fd);
		return false;
	}

	close(fd);
	return true;
}
