/*
 * TCP protocol
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "avformat.h"
#include "libavutil/avassert.h"
#include "libavutil/parseutils.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "uni_dns_parse.h"
#include "pthread.h"
#include <sys/time.h>

#include "internal.h"
#include "network.h"
#include "os_support.h"
#include "url.h"
#if HAVE_POLL_H
#include <poll.h>
#endif

typedef struct TCPContext {
    const AVClass *class;
    int fd;
    int listen;
    int open_timeout;
    int rw_timeout;
    int listen_timeout;
    int recv_buffer_size;
    int send_buffer_size;
} TCPContext;

#define OFFSET(x) offsetof(TCPContext, x)
#define D AV_OPT_FLAG_DECODING_PARAM
#define E AV_OPT_FLAG_ENCODING_PARAM
static const AVOption options[] = {
    { "listen",          "Listen for incoming connections",  OFFSET(listen),         AV_OPT_TYPE_INT, { .i64 = 0 },     0,       2,       .flags = D|E },
    { "timeout",     "set timeout (in microseconds) of socket I/O operations", OFFSET(rw_timeout),     AV_OPT_TYPE_INT, { .i64 = -1 },         -1, INT_MAX, .flags = D|E },
    { "listen_timeout",  "Connection awaiting timeout (in milliseconds)",      OFFSET(listen_timeout), AV_OPT_TYPE_INT, { .i64 = -1 },         -1, INT_MAX, .flags = D|E },
    { "send_buffer_size", "Socket send buffer size (in bytes)",                OFFSET(send_buffer_size), AV_OPT_TYPE_INT, { .i64 = -1 },         -1, INT_MAX, .flags = D|E },
    { "recv_buffer_size", "Socket receive buffer size (in bytes)",             OFFSET(recv_buffer_size), AV_OPT_TYPE_INT, { .i64 = -1 },         -1, INT_MAX, .flags = D|E },
    { NULL }
};

static const AVClass tcp_class = {
    .class_name = "tcp",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

#define ADDRINFO_ASYNC_ERRNO     (0x09abcdef)

typedef struct {
  char            *hostname;
  char            *portstr;
  struct addrinfo *hints;
  struct addrinfo *ai;
  int             is_session_stopped;
  int             errcode;
  unsigned int    id;
  int             is_async_thread_running;
  pthread_mutex_t lock;
} AddrinfoAsync;

static void _get_now_str(char *buf, int len) {
  struct timeval tv;
  time_t s;
  struct tm local;
  gettimeofday(&tv, NULL);
  s = tv.tv_sec;
  localtime_r(&s, &local);
  snprintf(buf, len, "%02d:%02d:%02d.%06"PRId64" ", local.tm_hour,
           local.tm_min, local.tm_sec, (int64_t)tv.tv_usec);
}

static void _get_thread_id_str(char *buf, int len) {
  snprintf(buf, len, "%x", pthread_self());
}

static void __ivm_crow_log_write(const char *function, int line,
                                 char *fmt, ...) {
  char buf[1024];
  char now[64];
  char thread_id[32];
  va_list args;
  snprintf(buf, sizeof(buf), "\033[0m\033[41;33m%s\033[0m ", "[W]");
  _get_now_str(now, sizeof(now));
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", now);
  _get_thread_id_str(thread_id, sizeof(thread_id));
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", thread_id);
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "<%s>", "ffmpeg");
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s:%d->", function, line);
  va_start(args, fmt);
  vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, args);
  va_end(args);
  printf("%s\n", buf);
  return 0;
}

static void *__get_addrinfo_tsk(void *args) {
  AddrinfoAsync *addrinfo = (AddrinfoAsync *)args;
  struct addrinfo *ai;
  unsigned int id = addrinfo->id;
  struct addrinfo hints = {0};
  memcpy(&hints, addrinfo->hints, sizeof(struct addrinfo));
  addrinfo->is_async_thread_running = 1;
  __ivm_crow_log_write(__FUNCTION__, __LINE__, "task thread running");
  int ret = getaddrinfo(addrinfo->hostname, addrinfo->portstr, &hints, &ai);
  pthread_mutex_lock(&addrinfo->lock);
  if (!addrinfo->is_session_stopped && id == addrinfo->id) {
    addrinfo->errcode = ret;
    addrinfo->ai = ai;
    __ivm_crow_log_write(__FUNCTION__, __LINE__,
                         "getaddrinfo set errcode=%d", addrinfo->errcode);
    goto L_END;
  }
  __ivm_crow_log_write(__FUNCTION__, __LINE__, "timeout, session is stopped");
  if (0 == ret) {
    freeaddrinfo(ai);
  }
L_END:
  pthread_mutex_unlock(&addrinfo->lock);
  return NULL;
}

static void __set_addrinfo(AddrinfoAsync *addrinfo, char *hostname,
                           struct addrinfo *hints, char *portstr) {
  addrinfo->hostname = hostname;
  addrinfo->portstr = portstr;
  addrinfo->hints = hints;
  addrinfo->ai = NULL;
  addrinfo->is_session_stopped = 0;
  addrinfo->errcode = ADDRINFO_ASYNC_ERRNO;
  addrinfo->id++;
  __ivm_crow_log_write(__FUNCTION__, __LINE__,
                       "hostname=%s, portstr=%s, ai=%p, "
                       "is_session_stopped=%d, errcode=%d, id=%d",
                       addrinfo->hostname, addrinfo->portstr,
                       addrinfo->ai, addrinfo->is_session_stopped,
                       addrinfo->errcode, addrinfo->id);
}

static int __get_addrinfo_async(char *hostname, char *portstr,
                                struct addrinfo *hints, struct addrinfo **ai,
                                int timeout_msec) {
  static AddrinfoAsync addrinfo = {NULL, NULL, NULL, NULL, 0, 0, 0, 0,
                                   PTHREAD_MUTEX_INITIALIZER};
  pthread_t pid;
  pthread_mutex_lock(&addrinfo.lock);
  __set_addrinfo(&addrinfo, hostname, hints, portstr);
  pthread_create(&pid, NULL, __get_addrinfo_tsk, &addrinfo);
  while (!addrinfo.is_async_thread_running) {usleep(2000);};
  addrinfo.is_async_thread_running = 0;
  pthread_mutex_unlock(&addrinfo.lock);
  pthread_detach(pid);
  while (addrinfo.errcode == ADDRINFO_ASYNC_ERRNO) {
    timeout_msec -= 10;
    if (timeout_msec <= 0) break;
    usleep(1000 * 10);
  }
  pthread_mutex_lock(&addrinfo.lock);
  addrinfo.is_session_stopped = 1;
  *ai = addrinfo.ai;
  pthread_mutex_unlock(&addrinfo.lock);
  return addrinfo.errcode;
}

/* return non zero if error */
static int tcp_open(URLContext *h, const char *uri, int flags)
{
    struct addrinfo hints = { 0 }, *ai, *cur_ai;
    int port, fd = -1;
    TCPContext *s = h->priv_data;
    const char *p;
    char buf[256];
    int ret;
    char hostname[1024],proto[1024],path[1024];
    char portstr[10];
    s->open_timeout = 5000000;

    av_url_split(proto, sizeof(proto), NULL, 0, hostname, sizeof(hostname),
        &port, path, sizeof(path), uri);
    if (strcmp(proto, "tcp"))
        return AVERROR(EINVAL);
    if (port <= 0 || port >= 65536) {
        av_log(h, AV_LOG_ERROR, "Port missing in uri\n");
        return AVERROR(EINVAL);
    }
    p = strchr(uri, '?');
    if (p) {
        if (av_find_info_tag(buf, sizeof(buf), "listen", p)) {
            char *endptr = NULL;
            s->listen = strtol(buf, &endptr, 10);
            /* assume if no digits were found it is a request to enable it */
            if (buf == endptr)
                s->listen = 1;
        }
        if (av_find_info_tag(buf, sizeof(buf), "timeout", p)) {
            s->rw_timeout = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "listen_timeout", p)) {
            s->listen_timeout = strtol(buf, NULL, 10);
        }
    }
    if (s->rw_timeout >= 0) {
        s->open_timeout =
        h->rw_timeout   = s->rw_timeout;
    }
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    if (s->listen)
        hints.ai_flags |= AI_PASSIVE;
    if (!hostname[0]) {
        //ret = getaddrinfo(NULL, portstr, &hints, &ai);
        __ivm_crow_log_write(__FUNCTION__, __LINE__, "__get_addrinfo_async start");
        //ret = __get_addrinfo_async(NULL, portstr, &hints, &ai, 50000);
        ret = dns_parse_by_domain(NULL, portstr, &hints, &ai);
        __ivm_crow_log_write(__FUNCTION__, __LINE__, "__get_addrinfo_async done");
    } else {
        //ret = getaddrinfo(hostname, portstr, &hints, &ai);
        __ivm_crow_log_write(__FUNCTION__, __LINE__, "__get_addrinfo_async start");
        //ret = __get_addrinfo_async(hostname, portstr, &hints, &ai, 50000);
        ret = dns_parse_by_domain(hostname, portstr, &hints, &ai);
        __ivm_crow_log_write(__FUNCTION__, __LINE__, "__get_addrinfo_async done");
    }
    if (ret) {
        av_log(h, AV_LOG_ERROR,
               "Failed to resolve hostname %s: %s\n",
               hostname, gai_strerror(ret));
        dns_parse_by_domain_done();
        return AVERROR(EIO);
    }

    cur_ai = ai;

 restart:
#if HAVE_STRUCT_SOCKADDR_IN6
    // workaround for IOS9 getaddrinfo in IPv6 only network use hardcode IPv4 address can not resolve port number.
    if (cur_ai->ai_family == AF_INET6){
        struct sockaddr_in6 * sockaddr_v6 = (struct sockaddr_in6 *)cur_ai->ai_addr;
        if (!sockaddr_v6->sin6_port){
            sockaddr_v6->sin6_port = htons(port);
        }
    }
#endif

    fd = ff_socket(cur_ai->ai_family,
                   cur_ai->ai_socktype,
                   cur_ai->ai_protocol);
    if (fd < 0) {
        ret = ff_neterrno();
        goto fail;
    }

    /* Set the socket's send or receive buffer sizes, if specified.
       If unspecified or setting fails, system default is used. */
    if (s->recv_buffer_size > 0) {
        setsockopt (fd, SOL_SOCKET, SO_RCVBUF, &s->recv_buffer_size, sizeof (s->recv_buffer_size));
    }
    if (s->send_buffer_size > 0) {
        setsockopt (fd, SOL_SOCKET, SO_SNDBUF, &s->send_buffer_size, sizeof (s->send_buffer_size));
    }

    if (s->listen == 2) {
        // multi-client
        if ((ret = ff_listen(fd, cur_ai->ai_addr, cur_ai->ai_addrlen)) < 0)
            goto fail1;
    } else if (s->listen == 1) {
        // single client
        if ((ret = ff_listen_bind(fd, cur_ai->ai_addr, cur_ai->ai_addrlen,
                                  s->listen_timeout, h)) < 0)
            goto fail1;
        // Socket descriptor already closed here. Safe to overwrite to client one.
        fd = ret;
    } else {
        if ((ret = ff_listen_connect(fd, cur_ai->ai_addr, cur_ai->ai_addrlen,
                                     s->open_timeout / 1000, h, !!cur_ai->ai_next)) < 0) {

            if (ret == AVERROR_EXIT)
                goto fail1;
            else
                goto fail;
        }
    }

    h->is_streamed = 1;
    s->fd = fd;
    dns_parse_by_domain_done();
    //freeaddrinfo(ai);
    return 0;

 fail:
    if (cur_ai->ai_next) {
        /* Retry with the next sockaddr */
        cur_ai = cur_ai->ai_next;
        if (fd >= 0)
            closesocket(fd);
        ret = 0;
        goto restart;
    }
 fail1:
    if (fd >= 0)
        closesocket(fd);
    dns_parse_by_domain_done();
    //freeaddrinfo(ai);
    return ret;
}

static int tcp_accept(URLContext *s, URLContext **c)
{
    TCPContext *sc = s->priv_data;
    TCPContext *cc;
    int ret;
    av_assert0(sc->listen);
    if ((ret = ffurl_alloc(c, s->filename, s->flags, &s->interrupt_callback)) < 0)
        return ret;
    cc = (*c)->priv_data;
    ret = ff_accept(sc->fd, sc->listen_timeout, s);
    if (ret < 0)
        return ret;
    cc->fd = ret;
    return 0;
}

static int tcp_read(URLContext *h, uint8_t *buf, int size)
{
    TCPContext *s = h->priv_data;
    int ret;

    if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
        ret = ff_network_wait_fd_timeout(s->fd, 0, h->rw_timeout, &h->interrupt_callback);
        if (ret)
            return ret;
    }
    ret = recv(s->fd, buf, size, 0);
    return ret < 0 ? ff_neterrno() : ret;
}

static int tcp_write(URLContext *h, const uint8_t *buf, int size)
{
    TCPContext *s = h->priv_data;
    int ret;

    if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
        ret = ff_network_wait_fd_timeout(s->fd, 1, h->rw_timeout, &h->interrupt_callback);
        if (ret)
            return ret;
    }
    ret = send(s->fd, buf, size, MSG_NOSIGNAL);
    return ret < 0 ? ff_neterrno() : ret;
}

static int tcp_shutdown(URLContext *h, int flags)
{
    TCPContext *s = h->priv_data;
    int how;

    if (flags & AVIO_FLAG_WRITE && flags & AVIO_FLAG_READ) {
        how = SHUT_RDWR;
    } else if (flags & AVIO_FLAG_WRITE) {
        how = SHUT_WR;
    } else {
        how = SHUT_RD;
    }

    return shutdown(s->fd, how);
}

static int tcp_close(URLContext *h)
{
    TCPContext *s = h->priv_data;
    closesocket(s->fd);
    return 0;
}

static int tcp_get_file_handle(URLContext *h)
{
    TCPContext *s = h->priv_data;
    return s->fd;
}

static int tcp_get_window_size(URLContext *h)
{
    TCPContext *s = h->priv_data;
    int avail;
    int avail_len = sizeof(avail);

#if HAVE_WINSOCK2_H
    /* SO_RCVBUF with winsock only reports the actual TCP window size when
    auto-tuning has been disabled via setting SO_RCVBUF */
    if (s->recv_buffer_size < 0) {
        return AVERROR(ENOSYS);
    }
#endif

    if (getsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, &avail, &avail_len)) {
        return ff_neterrno();
    }
    return avail;
}

const URLProtocol ff_tcp_protocol = {
    .name                = "tcp",
    .url_open            = tcp_open,
    .url_accept          = tcp_accept,
    .url_read            = tcp_read,
    .url_write           = tcp_write,
    .url_close           = tcp_close,
    .url_get_file_handle = tcp_get_file_handle,
    .url_get_short_seek  = tcp_get_window_size,
    .url_shutdown        = tcp_shutdown,
    .priv_data_size      = sizeof(TCPContext),
    .flags               = URL_PROTOCOL_FLAG_NETWORK,
    .priv_data_class     = &tcp_class,
};
