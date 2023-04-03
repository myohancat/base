/**
 * My simple network util source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __NET_UTIL_H_
#define __NET_UTIL_H_

#include <stdint.h>
#include <poll.h>

typedef struct MacAddr_s
{
    char    str[32];
    uint8_t raw[6];
} MacAddr_t;

namespace NetUtil
{

int poll2(struct pollfd *fds, nfds_t nfds, int timeout);

#define POLL_SUCCESS     (1)
#define POLL_INTERRUPTED (2)
#define POLL_TIMEOUT     (0)

#define POLL_REQ_IN      (POLLIN)
#define POLL_REQ_OUT     (POLLOUT)
int fd_poll(int fd, int req, int timeout, int fd_int = -1);

int connect2(int sock, const char* ipaddr, int port, int timeout, int fd_int = -1);
int bind2(int sock, const char* ipaddr, int port);

/* Socket Option */
int socket_set_blocking(int sock, bool block);
int socket_set_reuseaddr(int sock);
int socket_set_buffer_size(int sock, int rcvBufSize, int sndBufSize);

/* network information */
int get_link_state(const char* ifname, bool* isUP);
int get_mac_addr(const char* ifname, MacAddr_t* mac);
int get_ip_addr(const char* ifname, char* ip);
int get_netmask(const char* ifname, char* netmask);
int get_default_gateway(char* default_gw, char *interface);

} // namespace NetUtil


#endif /* __NET_UTIL_H_ */
