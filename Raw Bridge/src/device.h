#ifndef DEVICE_H
#define DEVICE_H

#include <iwlib.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/wireless.h>
#include <linux/if_vlan.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>

#define TUNDEV "/dev/net/tun"

class Device
{
public:
	enum{
		mode_IBSS			= 1,
		mode_managed		= 2,
		mode_monitor		= 6,
	};
	enum{
		type_tap,
		type_eth,
		type_vlan,
	};
	Device( const char* ifname, const uint8_t type );
	bool setMode( uint8_t mode );
	int getDescriptor(){ return m_desc; }
	int send(void *__restrict __buf, size_t __n);
	int getIFindex(){ return if_nametoindex(m_ifname); }
	int recv(void *__buf, size_t __n);
	char* getName(){ return m_ifname; }
private:
	char m_ifname[16];
	struct iwreq m_iwreq;
	struct ifreq m_ifreq;
	int m_desc = -1;
	int m_error = -1;
	bool m_init = false;
	uint8_t m_txBuff[64];
	uint8_t m_type;

	void init();
	bool ifdown(){ return ifClearFlag(IFF_UP); }
	bool ifup(){ return ifSetFlag(IFF_UP | IFF_RUNNING); }
	bool ifSetFlag( short flag );
	bool ifClearFlag( short flag );
	void setIfName();
	bool add_tap();
	bool del_tap();
};

#endif // DEVICE_H
