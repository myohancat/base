/**
 * My simple network util source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "NetUtil.h"

#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>

#include "trim.h"
#include "Log.h"

namespace NetUtil
{

int poll2(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int ret = 0;
    sigset_t org_mask;
    sigset_t new_mask;

    memset(&org_mask, 0, sizeof(org_mask));
    memset(&new_mask, 0, sizeof(new_mask));

    sigaddset(&new_mask, SIGHUP);
    sigaddset(&new_mask, SIGALRM);
    sigaddset(&new_mask, SIGUSR1);
    sigaddset(&new_mask, SIGUSR2);

    sigprocmask(SIG_SETMASK, &new_mask, &org_mask);
    ret = poll(fds, nfds, timeout);
    sigprocmask(SIG_SETMASK, &org_mask, NULL);

    return ret;
}

int fd_poll(int fd, int req, int timeout, int fd_int)
{
    int ret = 0;
    struct pollfd fds[2];
    nfds_t nfds = 1;

    fds[0].fd = fd;
    fds[0].events = (short)req | POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;
    fds[0].revents = 0;

    if (fd_int >= 0)
    {
        fds[1].fd = fd_int;
        fds[1].events = POLLIN;
        fds[1].revents = 0;
        nfds = 2;
    }

    ret = poll2(fds, nfds, timeout);
    if ( ret == 0 )
        return POLL_TIMEOUT;    /* poll timeout */

    if ( ret < 0 )
    {
        LOGE("poll failed. ret=%d, errno=%d.\n", ret, errno);
        return -errno;    /* poll failed */
    }

    if (nfds == 2 && fds[1].revents & POLLIN)
    {
        /* W/A code for flush pipe */
#if 0
        LOGW("!!!! INTERRUPTED !!!!\n");
#endif
        char buf[32];
        (void) read(fd_int, buf, sizeof(buf));
        return POLL_INTERRUPTED;
    }

    if ( fds[0].revents & (POLLRDHUP | POLLERR | POLLHUP | POLLNVAL) )
    {
#if 0
        if ( fds[0].revents & POLLRDHUP )
            LOGE("POLLRDHUP.\n");
        if ( fds[0].revents & POLLERR )
            LOGE("POLLERR.\n");
        if ( fds[0].revents & POLLHUP )
            LOGE("POLLHUP.\n");
        if ( fds[0].revents & POLLNVAL )
            LOGE("POLLNVAL.\n");
#endif
        return -1;    /* fd error detected */
    }

    if (fds[0].revents & req)
    {
        return POLL_SUCCESS;
    }

    LOGE("XXXXXXXX What Case XXXXXXX\n");
    return -1;
}

int connect2(int sock, const char* ipaddr, int port, int timeout, int fd_int)
{
    int ret;
    struct sockaddr_in serv_addr;

    socket_set_blocking(sock, false);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    LOGI("connect to %s:%d\n", ipaddr, port);

    if (inet_pton(AF_INET, ipaddr, &serv_addr.sin_addr) <= 0)
    {
        LOGE("Invalid address/ Address not supported \n");
        return -1;
    }

    ret = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (errno > 0 && errno != EAGAIN && errno != EINPROGRESS)
    {
        LOGE("Cannot connect !, errno = %d\n", errno);
        ret = -1;
    }

    ret = fd_poll(sock, POLLOUT, timeout, fd_int);
    if (ret == POLL_SUCCESS)
    {
        LOGI("Connection is established.\n");
        ret = 0;
    }
    else if (ret == POLL_TIMEOUT) // TIMEOUT
    {
        LOGE("Connecton timeout ... !\n");
        ret = -1;
    }
    else if (ret == POLL_INTERRUPTED)
    {
        LOGE("!!! INTERUPTED !!!!\n");
        ret = -1;
    }
    else
    {
        LOGE("Poll error : %d\n", ret);
        ret = -1;
    }

    socket_set_blocking(sock, true);

    return ret;
}


int bind2(int sock, const char* ipaddr, int port)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons (port);

    if (!ipaddr
      || strcmp(ipaddr, "127.0.0.1") == 0
      || strcmp(ipaddr, "localhost") == 0)
    {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if (inet_pton(AF_INET, ipaddr, &addr.sin_addr) <= 0)
        {
            LOGE("Invalid address. %s\n", ipaddr);
            return -1;
        }
    }

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        LOGE("bind failed. errno : %d(%s)\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}


int socket_set_blocking(int sock, bool block)
{
    int flags = fcntl(sock, F_GETFL, 0);
    if(block)
        flags = flags & (O_NONBLOCK ^ 0xFFFFFFFF);
    else
        flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) < 0)
    {
        LOGE("Cannot set socket blocking\n");
        return -1;
    }

    return 0;
}


int socket_set_reuseaddr(int sock)
{
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
    {
        LOGE("Cannot set reuseaddr failed. \n");
        return -1;
    }

    return 0;
}


int socket_set_buffer_size(int sock, int rcvBufSize, int sndBufSize)
{
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&rcvBufSize, sizeof(rcvBufSize)) != 0)
    {
        LOGW("Cannot set Receive Buffer size \n");
    }

    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize)) != 0)
    {
        LOGW("Cannot set Receive Buffer size \n");
    }

    return 0;
}

int get_link_state(const char* ifname, bool* isUP)
{
    FILE* file = NULL;
    char  path[1024];
    char  data[1024];

    sprintf(path, "/sys/class/net/%s/operstate", ifname);
    file = fopen(path, "r");
    if(!file)
        return -1;

    memset(data, 0x00, sizeof(data));
    if(fgets(data, sizeof(data), file) == NULL)
    {
        fclose(file);
        return -2;
    }

    trim(data);
    if(strcmp(data, "up") == 0)
        *isUP = true;
    else if(strcmp(data, "down") == 0)
        *isUP = false;
    else
    {
        LOGE("Unknown state : %s\n", data);
        fclose(file);
        return -3;
    }

    fclose(file);

    return 0;
}

int get_mac_addr(const char* ifname, MacAddr_t* mac)
{
    int sock;
    struct ifreq req;

    memset(mac, 0x00, sizeof(MacAddr_t));

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        LOGE("cannot open socket !\n");
        return -1;
    }

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if(ioctl(sock, SIOCGIFHWADDR, &req) < 0)
    {
        LOGE("cannot get mac address\n");
        close(sock);
        return -2;
    }

    memcpy(mac->raw, req.ifr_hwaddr.sa_data, 6);
    sprintf(mac->str, "%02X:%02X:%02X:%02X:%02X:%02X", 
                    mac->raw[0], mac->raw[1], mac->raw[2], mac->raw[3], mac->raw[4], mac->raw[5]);

    close(sock);

    return 0;
}

int get_ip_addr(const char* ifname, char* ipaddr)
{
    int fd, ret = 0;
    struct ifreq req;
    struct sockaddr_in* sin;

    ipaddr[0] = 0;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1)
    {
        LOGE("cannot oepn socket. !\n");
        return -1;
    }

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if(ioctl(fd, SIOCGIFADDR, &req) == 0)
    {
        sin = (struct sockaddr_in*)&req.ifr_addr;
        strcpy(ipaddr, inet_ntoa(sin->sin_addr));
    }
    else
    {
        LOGE("cannot found address : %s\n", ifname);
        ret = -2;
    }

    close(fd);
    return ret;
}

int get_netmask(const char* ifname, char* netmask)
{
    int fd, ret = 0;
    struct ifreq req;
    struct sockaddr_in* sin;

    netmask[0] = 0;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1)
    {
        LOGE("cannot oepn socket. !\n");
        return -1;
    }

    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if(ioctl(fd, SIOCGIFNETMASK, &req) == 0)
    {
        sin = (struct sockaddr_in*)&req.ifr_netmask;
        strcpy(netmask, inet_ntoa(sin->sin_addr));
    }
    else
    {
        LOGE("cannot found address : %s\n", ifname);
        ret = -2;
    }

    close(fd);
    return ret;
}

int get_default_gateway(char* default_gw, char *interface)
{
    long destination, gateway;
    char iface[1024];
    char buf[2*1024];
    FILE * file;

    memset(iface, 0, sizeof(iface));
    memset(buf, 0, sizeof(buf));

    file = fopen("/proc/net/route", "r");
    if(!file)
        return -1;

    while(fgets(buf, sizeof(buf), file))
    {
        if(sscanf(buf, "%s %lx %lx", iface, &destination, &gateway) == 3)
        {
            if(destination == 0) /* default */
            {
                struct in_addr addr;
                addr.s_addr = gateway;
                strcpy(default_gw, inet_ntoa(addr));
                if(interface)
                    strcpy(interface, iface);

                fclose(file);
                return 0;
            }
        }
    }

    if (file)
        fclose(file);

    return -1;
}

} // namespace NetUtil
