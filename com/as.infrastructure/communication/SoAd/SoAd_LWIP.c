/**
 * AS - the open source Automotive Software on https://github.com/parai
 *
 * Copyright (C) 2013, ArcCore AB, Sweden, www.arccore.com.
 * Copyright (C) 2018  AS <parai@foxmail.com>
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation; See <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */
#if defined(USE_SOAD) && defined(USE_LWIP)
/* ============================ [ INCLUDES  ] ====================================================== */
#include "Bsd.h"
/* ============================ [ MACROS    ] ====================================================== */
/* ============================ [ TYPES     ] ====================================================== */
/* ============================ [ DECLARES  ] ====================================================== */
/* ============================ [ DATAS     ] ====================================================== */
/* ============================ [ LOCALS    ] ====================================================== */
/* ============================ [ FUNCTIONS ] ====================================================== */
int SoAd_SocketCloseImpl(int s)
{
	return lwip_close(s);
}

int SoAd_SocketStatusCheckImpl(int s)
{
	int r = 0;
	int sockErr;
	socklen_t sockErrLen = sizeof(sockErr);

	lwip_getsockopt(s, SOL_SOCKET, SO_ERROR, &sockErr, &sockErrLen);
	if ((sockErr != 0) && (sockErr != EWOULDBLOCK)) {
		r = -1;
	}

	return 0;
}

int SoAd_SendImpl(int s, const void *data, size_t size, int flags)
{
	return lwip_send(s, data, size, flags);
}

int SoAd_SendToImpl(int s, const void *data, size_t size, uint32 RemoteIpAddress, uint16 RemotePort)
{
	struct sockaddr_in toAddr;
	socklen_t toAddrLen = sizeof(toAddr);
	toAddr.sin_family = AF_INET;
	toAddr.sin_len = sizeof(toAddr);

	toAddr.sin_addr.s_addr = RemoteIpAddress;
	toAddr.sin_port = RemotePort;
	return lwip_sendto(s, data, size, 0, (struct sockaddr *)&toAddr, toAddrLen);
}

int SoAd_CreateSocketImpl(int domain, int type, int protocol)
{
	return lwip_socket(domain, type, protocol);
}

uint32 SoAd_GetLocalIp(void)
{
	extern struct netif* sys_get_netif(void);
	struct netif* netif = sys_get_netif();

	return netif->ip_addr.addr;
}

int SoAd_BindImpl(int s, uint16 SocketLocalPort, char* SocketLocalIpAddress)
{
	int r;
	ip_addr_t ipaddr;
	ip_mreq mreq;
	struct sockaddr_in sLocalAddr;

	int on = 1;
	lwip_ioctl(s, FIONBIO, &on);

	lwip_setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(int));	/* Set socket to no delay */

	/*Source*/
	memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));
	sLocalAddr.sin_family = AF_INET;
	sLocalAddr.sin_len = sizeof(sLocalAddr);

	sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	sLocalAddr.sin_port = htons(SocketLocalPort);

	r = lwip_bind(s, (struct sockaddr *)&sLocalAddr, sizeof(sLocalAddr));

	if((0 == r) && (SocketLocalIpAddress != NULL))
	{
		ipaddr.addr = ipaddr_addr(SocketLocalIpAddress);

		if(ip_addr_ismulticast(&ipaddr))
		{
			mreq.imr_multiaddr.s_addr = ipaddr.addr;
			mreq.imr_interface.s_addr = SoAd_GetLocalIp();;
			r = lwip_setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
		}
	}

	return r;
}

int SoAd_ListenImpl(int s, int backlog)
{
	return lwip_listen(s, backlog);
}

int SoAd_AcceptImpl(int s, uint32 *RemoteIpAddress, uint16 *RemotePort)
{
	int clientFd;
	struct sockaddr_in client_addr;
	int addrlen = sizeof(client_addr);

	clientFd = lwip_accept(s, (struct sockaddr*)&client_addr, (socklen_t *)&addrlen);

	if( clientFd != (-1) )
	{
		/* Check that remote port and ip match
		 * NOTE: Check remote port and ip with SocketAdminList and select first matching
		 */

		/* New connection established */
		int on = 1;
		lwip_ioctl(clientFd, FIONBIO, &on);	/* Set socket to non block mode */

		lwip_setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(int));	/* Set socket to no delay */

		*RemotePort = client_addr.sin_port;
		*RemoteIpAddress = client_addr.sin_addr.s_addr;

	}

	return clientFd;
}

int SoAd_RecvImpl(int s, void *mem, size_t len, int flags)
{
	return lwip_recv(s, mem, len, flags);
}

int SoAd_RecvFromImpl(int s, void *mem, size_t len, int flags,
					  uint32 *RemoteIpAddress, uint16 *RemotePort)
{
	struct sockaddr_in fromAddr;
	socklen_t fromAddrLen = sizeof(fromAddr);
	int nbytes;

	nbytes = lwip_recvfrom(s, mem, len, flags, (struct sockaddr*)&fromAddr, &fromAddrLen);

	if(nbytes > 0)
	{
		*RemotePort = fromAddr.sin_port;
		*RemoteIpAddress = fromAddr.sin_addr.s_addr;
	}

	return nbytes;
}


#endif
