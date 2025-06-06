/**
 * @file addons/http.c
 * @brief HTTP addon.
 *
 * This is a heavily modified version of the EmbeddableWebServer (see copyright
 * below). This version has been stripped from everything not strictly necessary
 * for receiving/replying to simple HTTP requests, and has been modified to use
 * the Flecs OS API.
 *
 * EmbeddableWebServer Copyright (c) 2016, 2019, 2020 Forrest Heller, and 
 * CONTRIBUTORS (see below) - All rights reserved.
 *
 * CONTRIBUTORS:
 * Martin Pulec - bug fixes, warning fixes, IPv6 support
 * Daniel Barry - bug fix (ifa_addr != NULL)
 * 
 * Released under the BSD 2-clause license:
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. THIS SOFTWARE IS 
 * PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS 
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN 
 * NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "../private_api.h"
#include <errno.h>

#ifdef FLECS_HTTP

#ifdef ECS_TARGET_MSVC
#pragma comment(lib, "Ws2_32.lib")
#endif

#if defined(ECS_TARGET_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET ecs_http_socket_t;
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <strings.h>
#include <signal.h>
#include <fcntl.h>
#ifdef __FreeBSD__
#include <netinet/in.h>
#endif
typedef int ecs_http_socket_t;

#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL (0)
#endif

#endif

/* Max length of request method */
#define ECS_HTTP_METHOD_LEN_MAX (8) 

/* Timeout (s) before connection purge */
#define ECS_HTTP_CONNECTION_PURGE_TIMEOUT (1.0)

/* Number of dequeues before purging */
#define ECS_HTTP_CONNECTION_PURGE_RETRY_COUNT (5)

/* Number of retries receiving request */
#define ECS_HTTP_REQUEST_RECV_RETRY (10)

/* Minimum interval between dequeueing requests (ms) */
#define ECS_HTTP_MIN_DEQUEUE_INTERVAL (50)

/* Minimum interval between printing statistics (ms) */
#define ECS_HTTP_MIN_STATS_INTERVAL (10 * 1000)

/* Receive buffer size */
#define ECS_HTTP_SEND_RECV_BUFFER_SIZE (64 * 1024)

/* Max length of request (path + query + headers + body) */
#define ECS_HTTP_REQUEST_LEN_MAX (10 * 1024 * 1024)

/* Total number of outstanding send requests */
#define ECS_HTTP_SEND_QUEUE_MAX (256)

/* Global statistics */
int64_t ecs_http_request_received_count = 0;
int64_t ecs_http_request_invalid_count = 0;
int64_t ecs_http_request_handled_ok_count = 0;
int64_t ecs_http_request_handled_error_count = 0;
int64_t ecs_http_request_not_handled_count = 0;
int64_t ecs_http_request_preflight_count = 0;
int64_t ecs_http_send_ok_count = 0;
int64_t ecs_http_send_error_count = 0;
int64_t ecs_http_busy_count = 0;

/* Send request queue */
typedef struct ecs_http_send_request_t {
    ecs_http_socket_t sock;
    char *headers;
    int32_t header_length;
    char *content;
    int32_t content_length;
} ecs_http_send_request_t;

typedef struct ecs_http_send_queue_t {
    ecs_http_send_request_t requests[ECS_HTTP_SEND_QUEUE_MAX];
    int32_t head;
    int32_t tail;
    ecs_os_thread_t thread;
    int32_t wait_ms;
} ecs_http_send_queue_t;

typedef struct ecs_http_request_key_t {
    const char *array;
    ecs_size_t count;
} ecs_http_request_key_t;

typedef struct ecs_http_request_entry_t {
    char *content;
    int32_t content_length;
    int code;
    double time;
} ecs_http_request_entry_t;

/* HTTP server struct */
struct ecs_http_server_t {
    bool should_run;
    bool running;

    ecs_http_socket_t sock;
    ecs_os_mutex_t lock;
    ecs_os_thread_t thread;

    ecs_http_reply_action_t callback;
    void *ctx;

    double cache_timeout;
    double cache_purge_timeout;

    ecs_sparse_t connections; /* sparse<http_connection_t> */
    ecs_sparse_t requests; /* sparse<http_request_t> */

    bool initialized;

    uint16_t port;
    const char *ipaddr;

    double dequeue_timeout; /* used to not lock request queue too often */
    double stats_timeout; /* used for periodic reporting of statistics */

    double request_time; /* time spent on requests in last stats interval */
    double request_time_total; /* total time spent on requests */
    int32_t requests_processed; /* requests processed in last stats interval */
    int32_t requests_processed_total; /* total requests processed */
    int32_t dequeue_count; /* number of dequeues in last stats interval */ 
    ecs_http_send_queue_t send_queue;

    ecs_hashmap_t request_cache;
};

/** Fragment state, used by HTTP request parser */
typedef enum  {
    HttpFragStateBegin,
    HttpFragStateMethod,
    HttpFragStatePath,
    HttpFragStateVersion,
    HttpFragStateHeaderStart,
    HttpFragStateHeaderName,
    HttpFragStateHeaderValueStart,
    HttpFragStateHeaderValue,
    HttpFragStateCR,
    HttpFragStateCRLF,
    HttpFragStateCRLFCR,
    HttpFragStateBody,
    HttpFragStateDone
} HttpFragState;

/** A fragment is a partially received HTTP request */
typedef struct {
    HttpFragState state;
    ecs_strbuf_t buf;
    ecs_http_method_t method;
    int32_t body_offset;
    int32_t query_offset;
    int32_t header_offsets[ECS_HTTP_HEADER_COUNT_MAX];
    int32_t header_value_offsets[ECS_HTTP_HEADER_COUNT_MAX];
    int32_t header_count;
    int32_t param_offsets[ECS_HTTP_QUERY_PARAM_COUNT_MAX];
    int32_t param_value_offsets[ECS_HTTP_QUERY_PARAM_COUNT_MAX];
    int32_t param_count;
    int32_t content_length;
    char *header_buf_ptr;
    char header_buf[32];
    bool parse_content_length;
    bool invalid;
} ecs_http_fragment_t;

/** Extend public connection type with fragment data */
typedef struct {
    ecs_http_connection_t pub;
    ecs_http_socket_t sock;

    /* Connection is purged after both timeout expires and connection has
     * exceeded retry count. This ensures that a connection does not immediately
     * timeout when a frame takes longer than usual */
    double dequeue_timeout;
    int32_t dequeue_retries;    
} ecs_http_connection_impl_t;

typedef struct {
    ecs_http_request_t pub;
    uint64_t conn_id; /* for sanity check */
    char *res;
    int32_t req_len;
} ecs_http_request_impl_t;

static
ecs_size_t http_send(
    ecs_http_socket_t sock, 
    const void *buf, 
    ecs_size_t size, 
    int flags)
{
    ecs_assert(size >= 0, ECS_INTERNAL_ERROR, NULL);
#ifdef ECS_TARGET_POSIX
    ssize_t send_bytes = send(sock, buf, flecs_itosize(size), 
        flags | MSG_NOSIGNAL);
    return flecs_itoi32(send_bytes);
#else
    int send_bytes = send(sock, buf, size, flags);
    return flecs_itoi32(send_bytes);
#endif
}

static
ecs_size_t http_recv(
    ecs_http_socket_t sock,
    void *buf,
    ecs_size_t size,
    int flags)
{
    ecs_size_t ret;
#ifdef ECS_TARGET_POSIX
    ssize_t recv_bytes = recv(sock, buf, flecs_itosize(size), flags);
    ret = flecs_itoi32(recv_bytes);
#else
    int recv_bytes = recv(sock, buf, size, flags);
    ret = flecs_itoi32(recv_bytes);
#endif
    if (ret == -1) {
        ecs_dbg("recv failed: %s (sock = %d)", ecs_os_strerror(errno), sock);
    } else if (ret == 0) {
        ecs_dbg("recv: received 0 bytes (sock = %d)", sock);
    }

    return ret;
}

static
void http_sock_set_timeout(
    ecs_http_socket_t sock,
    int32_t timeout_ms)
{
    int r;
#ifdef ECS_TARGET_POSIX
    struct timeval tv;
    tv.tv_sec = timeout_ms * 1000;
    tv.tv_usec = 0;
    r = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
#else
    DWORD t = (DWORD)timeout_ms;
    r = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t);
#endif
    if (r) {
        ecs_warn("http: failed to set socket timeout: %s", 
            ecs_os_strerror(errno));
    }
}

static
void http_sock_keep_alive(
    ecs_http_socket_t sock)
{
    int v = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&v, sizeof v)) {
        ecs_warn("http: failed to set socket KEEPALIVE: %s",
            ecs_os_strerror(errno));
    }
}

static
void http_sock_nonblock(ecs_http_socket_t sock, bool enable) {
    (void)sock;
    (void)enable;
#ifdef ECS_TARGET_POSIX
    int flags;
    flags = fcntl(sock,F_GETFL,0);
    if (flags == -1) {
        ecs_warn("http: failed to set socket NONBLOCK: %s",
            ecs_os_strerror(errno));
        return;
    }
    if (enable) {
        flags = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    } else {
        flags = fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
    }
    if (flags == -1) {
        ecs_warn("http: failed to set socket NONBLOCK: %s",
            ecs_os_strerror(errno));
        return;
    }
#endif
}

static
int http_getnameinfo(
    const struct sockaddr* addr,
    ecs_size_t addr_len,
    char *host,
    ecs_size_t host_len,
    char *port,
    ecs_size_t port_len,
    int flags)
{
    ecs_assert(addr_len > 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(host_len > 0, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(port_len > 0, ECS_INTERNAL_ERROR, NULL);
#if defined(ECS_TARGET_WINDOWS)
    return getnameinfo(addr, addr_len, host, 
        flecs_ito(uint32_t, host_len), port, flecs_ito(uint32_t, port_len), 
        flags);
#else
    return getnameinfo(addr, flecs_ito(uint32_t, addr_len), host, 
        flecs_ito(uint32_t, host_len), port, flecs_ito(uint32_t, port_len), 
        flags);
#endif
}

static
int http_bind(
    ecs_http_socket_t sock,
    const struct sockaddr* addr,
    ecs_size_t addr_len)
{
    ecs_assert(addr_len > 0, ECS_INTERNAL_ERROR, NULL);
#if defined(ECS_TARGET_WINDOWS)
    return bind(sock, addr, addr_len);
#else
    return bind(sock, addr, flecs_ito(uint32_t, addr_len));
#endif
}

static
bool http_socket_is_valid(
    ecs_http_socket_t sock)
{
#if defined(ECS_TARGET_WINDOWS)
    return sock != INVALID_SOCKET;
#else
    return sock >= 0;
#endif
}

#if defined(ECS_TARGET_WINDOWS)
#define HTTP_SOCKET_INVALID INVALID_SOCKET
#else
#define HTTP_SOCKET_INVALID (-1)
#endif

static
void http_close(
    ecs_http_socket_t *sock)
{
    ecs_assert(sock != NULL, ECS_INTERNAL_ERROR, NULL);

#if defined(ECS_TARGET_WINDOWS)
    closesocket(*sock);
#else
    ecs_dbg_2("http: closing socket %u", *sock);
    shutdown(*sock, SHUT_RDWR);
    close(*sock);
#endif
    *sock = HTTP_SOCKET_INVALID;
}

static
ecs_http_socket_t http_accept(
    ecs_http_socket_t sock,
    struct sockaddr* addr,
    ecs_size_t *addr_len)
{
    socklen_t len = (socklen_t)addr_len[0];
    ecs_http_socket_t result = accept(sock, addr, &len);
    addr_len[0] = (ecs_size_t)len;
    return result;
}

static
void http_reply_fini(ecs_http_reply_t* reply) {
    ecs_assert(reply != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_os_free(reply->body.content);
}

static
void http_request_fini(ecs_http_request_impl_t *req) {
    ecs_assert(req != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(req->pub.conn != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(req->pub.conn->server != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(req->pub.conn->id == req->conn_id, ECS_INTERNAL_ERROR, NULL);
    ecs_os_free(req->res);
    flecs_sparse_remove_t(&req->pub.conn->server->requests, 
        ecs_http_request_impl_t, req->pub.id);
}

static
void http_connection_free(ecs_http_connection_impl_t *conn) {
    ecs_assert(conn != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(conn->pub.id != 0, ECS_INTERNAL_ERROR, NULL);
    uint64_t conn_id = conn->pub.id;

    if (http_socket_is_valid(conn->sock)) {
        http_close(&conn->sock);
    }

    flecs_sparse_remove_t(&conn->pub.server->connections, 
        ecs_http_connection_impl_t, conn_id);
}

// https://stackoverflow.com/questions/10156409/convert-hex-string-char-to-int
static
char http_hex_2_int(char a, char b){
    a = (a <= '9') ? (char)(a - '0') : (char)((a & 0x7) + 9);
    b = (b <= '9') ? (char)(b - '0') : (char)((b & 0x7) + 9);
    return (char)((a << 4) + b);
}

static
void http_decode_url_str(
    char *str) 
{
    char ch, *ptr, *dst = str;
    for (ptr = str; (ch = *ptr); ptr++) {
        if (ch == '%') {
            dst[0] = http_hex_2_int(ptr[1], ptr[2]);
            dst ++;
            ptr += 2;
        } else {
            dst[0] = ptr[0];
            dst ++;
        }
    }
    dst[0] = '\0';
}

static
void http_parse_method(
    ecs_http_fragment_t *frag)
{
    char *method = ecs_strbuf_get_small(&frag->buf);
    if (!ecs_os_strcmp(method, "GET")) frag->method = EcsHttpGet;
    else if (!ecs_os_strcmp(method, "POST")) frag->method = EcsHttpPost;
    else if (!ecs_os_strcmp(method, "PUT")) frag->method = EcsHttpPut;
    else if (!ecs_os_strcmp(method, "DELETE")) frag->method = EcsHttpDelete;
    else if (!ecs_os_strcmp(method, "OPTIONS")) frag->method = EcsHttpOptions;
    else {
        frag->method = EcsHttpMethodUnsupported;
        frag->invalid = true;
    }
    ecs_strbuf_reset(&frag->buf);
}

static
bool http_header_writable(
    ecs_http_fragment_t *frag)
{
    return frag->header_count < ECS_HTTP_HEADER_COUNT_MAX;
}

static
void http_header_buf_reset(
    ecs_http_fragment_t *frag)
{
    frag->header_buf[0] = '\0';
    frag->header_buf_ptr = frag->header_buf;
}

static
void http_header_buf_append(
    ecs_http_fragment_t *frag,
    char ch)
{
    if ((frag->header_buf_ptr - frag->header_buf) < 
        ECS_SIZEOF(frag->header_buf)) 
    {
        frag->header_buf_ptr[0] = ch;
        frag->header_buf_ptr ++;
    } else {
        frag->header_buf_ptr[0] = '\0';
    }
}

static
uint64_t http_request_key_hash(const void *ptr) {
    const ecs_http_request_key_t *key = ptr;
    const char *array = key->array;
    int32_t count = key->count;
    return flecs_hash(array, count * ECS_SIZEOF(char));
}

static
int http_request_key_compare(const void *ptr_1, const void *ptr_2) {
    const ecs_http_request_key_t *type_1 = ptr_1;
    const ecs_http_request_key_t *type_2 = ptr_2;

    int32_t count_1 = type_1->count;
    int32_t count_2 = type_2->count;

    if (count_1 != count_2) {
        return (count_1 > count_2) - (count_1 < count_2);
    }

    return ecs_os_memcmp(type_1->array, type_2->array, count_1);
}

static
ecs_http_request_entry_t* http_find_request_entry(
    ecs_http_server_t *srv,
    const char *array,
    int32_t count)
{
    ecs_http_request_key_t key;
    key.array = array;
    key.count = count;

    ecs_time_t t = {0, 0};
    ecs_http_request_entry_t *entry = flecs_hashmap_get(
        &srv->request_cache, &key, ecs_http_request_entry_t);

    if (entry) {
        double tf = ecs_time_measure(&t);
        if ((tf - entry->time) < srv->cache_timeout) {
            return entry;
        }
    }
    return NULL;
}

static
void http_insert_request_entry(
    ecs_http_server_t *srv,
    ecs_http_request_impl_t *req,
    ecs_http_reply_t *reply)
{
    int32_t content_length = ecs_strbuf_written(&reply->body);
    if (!content_length) {
        return;
    }

    ecs_http_request_key_t key;
    key.array = req->res;
    key.count = req->req_len;
    ecs_http_request_entry_t *entry = flecs_hashmap_get(
        &srv->request_cache, &key, ecs_http_request_entry_t);
    if (!entry) {
        flecs_hashmap_result_t elem = flecs_hashmap_ensure(
            &srv->request_cache, &key, ecs_http_request_entry_t);
        ecs_http_request_key_t *elem_key = elem.key;
        elem_key->array = ecs_os_memdup_n(key.array, char, key.count);
        entry = elem.value;
    } else {
        ecs_os_free(entry->content);
    }

    ecs_time_t t = {0, 0};
    entry->time = ecs_time_measure(&t);
    entry->content_length = ecs_strbuf_written(&reply->body);
    entry->content = ecs_strbuf_get(&reply->body);
    entry->code = reply->code;
    ecs_strbuf_appendstrn(&reply->body, 
            entry->content, entry->content_length);
}

static
char* http_decode_request(
    ecs_http_request_impl_t *req,
    ecs_http_fragment_t *frag)
{
    ecs_os_zeromem(req);

    ecs_size_t req_len = frag->buf.length;
    char *res = ecs_strbuf_get(&frag->buf);
    if (!res) {
        return NULL;
    }

    req->pub.method = frag->method;
    req->pub.path = res + 1;
    http_decode_url_str(req->pub.path);

    if (frag->body_offset) {
        req->pub.body = &res[frag->body_offset];
    }
    int32_t i, count = frag->header_count;
    for (i = 0; i < count; i ++) {
        req->pub.headers[i].key = &res[frag->header_offsets[i]];
        req->pub.headers[i].value = &res[frag->header_value_offsets[i]];
    }
    count = frag->param_count;
    for (i = 0; i < count; i ++) {
        req->pub.params[i].key = &res[frag->param_offsets[i]];
        req->pub.params[i].value = &res[frag->param_value_offsets[i]];
        /* Safe, member is only const so that end-user can't change it */
        http_decode_url_str(ECS_CONST_CAST(char*, req->pub.params[i].value));
    }

    req->pub.header_count = frag->header_count;
    req->pub.param_count = frag->param_count;
    req->res = res;
    req->req_len = frag->header_offsets[0];
    if (!req->req_len) {
        req->req_len = req_len;
    }

    return res;
}

static
ecs_http_request_entry_t* http_enqueue_request(
    ecs_http_connection_impl_t *conn,
    uint64_t conn_id,
    ecs_http_fragment_t *frag)
{
    ecs_http_server_t *srv = conn->pub.server;

    ecs_os_mutex_lock(srv->lock);
    bool is_alive = conn->pub.id == conn_id;

    if (!is_alive || frag->invalid) { 
        /* Don't enqueue invalid requests or requests for purged connections */
        ecs_strbuf_reset(&frag->buf);
    } else {
        ecs_http_request_impl_t req;
        char *res = http_decode_request(&req, frag);
        if (res) {
            req.pub.conn = (ecs_http_connection_t*)conn;

            /* Check cache for GET requests */
            if (frag->method == EcsHttpGet) {
                ecs_http_request_entry_t *entry = 
                    http_find_request_entry(srv, res, frag->header_offsets[0]);
                if (entry) {
                    /* If an entry is found, don't enqueue a request. Instead
                     * return the cached response immediately. */
                    ecs_os_free(res);
                    return entry;
                }
            }

            ecs_http_request_impl_t *req_ptr = flecs_sparse_add_t(
                &srv->requests, ecs_http_request_impl_t);
            *req_ptr = req;
            req_ptr->pub.id = flecs_sparse_last_id(&srv->requests);
            req_ptr->conn_id = conn->pub.id;
            ecs_os_linc(&ecs_http_request_received_count);
        }
    }

    ecs_os_mutex_unlock(srv->lock);
    return NULL;
}

static
bool http_parse_request(
    ecs_http_fragment_t *frag,
    const char* req_frag, 
    ecs_size_t req_frag_len) 
{
    int32_t i;
    for (i = 0; i < req_frag_len; i++) {
        char c = req_frag[i];
        switch (frag->state) {
        case HttpFragStateBegin:
            ecs_os_memset_t(frag, 0, ecs_http_fragment_t);
            frag->state = HttpFragStateMethod;
            frag->header_buf_ptr = frag->header_buf;

            /* fall through */
        case HttpFragStateMethod:
            if (c == ' ') {
                http_parse_method(frag);
                ecs_strbuf_reset(&frag->buf);
                frag->state = HttpFragStatePath;
                frag->buf.content = NULL;
            } else {
                ecs_strbuf_appendch(&frag->buf, c);
            }
            break;
        case HttpFragStatePath:
            if (c == ' ') {
                frag->state = HttpFragStateVersion;
                ecs_strbuf_appendch(&frag->buf, '\0');
            } else {
                if (c == '?' || c == '=' || c == '&') {
                    ecs_strbuf_appendch(&frag->buf, '\0');
                    int32_t offset = ecs_strbuf_written(&frag->buf);
                    if (c == '?' || c == '&') {
                        frag->param_offsets[frag->param_count] = offset;
                    } else {
                        frag->param_value_offsets[frag->param_count] = offset;
                        frag->param_count ++;
                    }
                } else {
                    ecs_strbuf_appendch(&frag->buf, c);
                }
            }
            break;
        case HttpFragStateVersion:
            if (c == '\r') {
                frag->state = HttpFragStateCR;
            } /* version is not stored */
            break;
        case HttpFragStateHeaderStart:
            if (http_header_writable(frag)) {
                frag->header_offsets[frag->header_count] = 
                    ecs_strbuf_written(&frag->buf);
            }
            http_header_buf_reset(frag);
            frag->state = HttpFragStateHeaderName;

            /* fall through */
        case HttpFragStateHeaderName:
            if (c == ':') {
                frag->state = HttpFragStateHeaderValueStart;
                http_header_buf_append(frag, '\0');
                frag->parse_content_length = !ecs_os_strcmp(
                    frag->header_buf, "Content-Length");

                if (http_header_writable(frag)) {
                    ecs_strbuf_appendch(&frag->buf, '\0');
                    frag->header_value_offsets[frag->header_count] =
                        ecs_strbuf_written(&frag->buf);
                }
            } else if (c == '\r') {
                frag->state = HttpFragStateCR;
            } else  {
                http_header_buf_append(frag, c);
                if (http_header_writable(frag)) {
                    ecs_strbuf_appendch(&frag->buf, c);
                }
            }
            break;
        case HttpFragStateHeaderValueStart:
            http_header_buf_reset(frag);
            frag->state = HttpFragStateHeaderValue;
            if (c == ' ') { /* skip first space */
                break;
            }

            /* fall through */
        case HttpFragStateHeaderValue:
            if (c == '\r') {
                if (frag->parse_content_length) {
                    http_header_buf_append(frag, '\0');
                    int32_t len = atoi(frag->header_buf);
                    if (len < 0) {
                        frag->invalid = true;
                    } else {
                        frag->content_length = len;
                    }
                    frag->parse_content_length = false;
                }
                if (http_header_writable(frag)) {
                    int32_t cur = ecs_strbuf_written(&frag->buf);
                    if (frag->header_offsets[frag->header_count] < cur &&
                        frag->header_value_offsets[frag->header_count] < cur)
                    {
                        ecs_strbuf_appendch(&frag->buf, '\0');
                        frag->header_count ++;
                    }
                }
                frag->state = HttpFragStateCR;
            } else {
                if (frag->parse_content_length) {
                    http_header_buf_append(frag, c);
                }
                if (http_header_writable(frag)) {
                    ecs_strbuf_appendch(&frag->buf, c);
                }
            }
            break;
        case HttpFragStateCR:
            if (c == '\n') {
                frag->state = HttpFragStateCRLF;
            } else {
                frag->state = HttpFragStateHeaderStart;
            } 
            break;
        case HttpFragStateCRLF:
            if (c == '\r') {
                frag->state = HttpFragStateCRLFCR;
            } else {
                frag->state = HttpFragStateHeaderStart;
                i--;
            }
            break;
        case HttpFragStateCRLFCR:
            if (c == '\n') {
                if (frag->content_length != 0) {
                    frag->body_offset = ecs_strbuf_written(&frag->buf);
                    frag->state = HttpFragStateBody;
                } else {
                    frag->state = HttpFragStateDone;
                }
            } else {
                frag->state = HttpFragStateHeaderStart;
            }
            break;
        case HttpFragStateBody: {
                ecs_strbuf_appendch(&frag->buf, c);
                if ((ecs_strbuf_written(&frag->buf) - frag->body_offset) == 
                    frag->content_length) 
                {
                    frag->state = HttpFragStateDone;
                }
            }
            break;
        case HttpFragStateDone:
            break;
        }
    }

    if (frag->state == HttpFragStateDone) {
        return true;
    } else {
        return false;
    }
}

static
ecs_http_send_request_t* http_send_queue_post(
    ecs_http_server_t *srv)
{
    /* This function should only be called while the server is locked. Before 
     * the lock is released, the returned element should be populated. */
    ecs_http_send_queue_t *sq = &srv->send_queue;
    int32_t next = (sq->head + 1) % ECS_HTTP_SEND_QUEUE_MAX;
    if (next == sq->tail) {
        return NULL;
    }

    /* Don't enqueue new requests if server is shutting down */
    if (!srv->should_run) {
        return NULL;
    }

    /* Return element at end of the queue */
    ecs_http_send_request_t *result = &sq->requests[sq->head];
    sq->head = next;
    return result;
}

static
ecs_http_send_request_t* http_send_queue_get(
    ecs_http_server_t *srv)
{
    ecs_os_mutex_lock(srv->lock);
    ecs_http_send_queue_t *sq = &srv->send_queue;
    if (sq->tail == sq->head) {
        return NULL;
    }

    int32_t next = (sq->tail + 1) % ECS_HTTP_SEND_QUEUE_MAX;
    ecs_http_send_request_t *result = &sq->requests[sq->tail];
    sq->tail = next;
    return result;
}

static
void* http_server_send_queue(void* arg) {
    ecs_http_server_t *srv = arg;
    int32_t wait_ms = srv->send_queue.wait_ms;

    /* Run for as long as the server is running or there are messages. When the
     * server is stopping, no new messages will be enqueued */
    while (srv->should_run || (srv->send_queue.head != srv->send_queue.tail)) {
        ecs_http_send_request_t* r = http_send_queue_get(srv);
        if (!r) {
            ecs_os_mutex_unlock(srv->lock);
            /* If the queue is empty, wait so we don't run too fast */
            if (srv->should_run) {
                ecs_os_sleep(0, wait_ms * 1000 * 1000);
            }
        } else {
            ecs_http_socket_t sock = r->sock;
            char *headers = r->headers;
            int32_t headers_length = r->header_length;
            char *content = r->content;
            int32_t content_length = r->content_length;
            ecs_os_mutex_unlock(srv->lock);

            if (http_socket_is_valid(sock)) {
                bool error = false;

                http_sock_nonblock(sock, false);

                /* Write headers */
                ecs_size_t written = http_send(sock, headers, headers_length, 0);
                if (written != headers_length) {
                    ecs_err("http: failed to write HTTP response headers: %s",
                        ecs_os_strerror(errno));
                    ecs_os_linc(&ecs_http_send_error_count);
                    error = true;
                } else if (content_length >= 0) {
                    /* Write content */
                    written = http_send(sock, content, content_length, 0);
                    if (written != content_length) {
                        ecs_err("http: failed to write HTTP response body: %s",
                            ecs_os_strerror(errno));
                        ecs_os_linc(&ecs_http_send_error_count);
                        error = true;
                    }
                }
                if (!error) {
                    ecs_os_linc(&ecs_http_send_ok_count);
                }

                http_close(&sock);
            } else {
                ecs_err("http: invalid socket\n");
            }

            ecs_os_free(content);
            ecs_os_free(headers);
        }
    }
    return NULL;
}

static
void http_append_send_headers(
    ecs_strbuf_t *hdrs,
    int code, 
    const char* status, 
    const char* content_type,  
    ecs_strbuf_t *extra_headers,
    ecs_size_t content_len,
    bool preflight)
{
    ecs_strbuf_appendlit(hdrs, "HTTP/1.1 ");
    ecs_strbuf_appendint(hdrs, code);
    ecs_strbuf_appendch(hdrs, ' ');
    ecs_strbuf_appendstr(hdrs, status);
    ecs_strbuf_appendlit(hdrs, "\r\n");

    if (content_type) {
        ecs_strbuf_appendlit(hdrs, "Content-Type: ");
        ecs_strbuf_appendstr(hdrs, content_type);
        ecs_strbuf_appendlit(hdrs, "\r\n");
    }

    if (content_len >= 0) {
        ecs_strbuf_appendlit(hdrs, "Content-Length: ");
        ecs_strbuf_append(hdrs, "%d", content_len);
        ecs_strbuf_appendlit(hdrs, "\r\n");
    }

    ecs_strbuf_appendlit(hdrs, "Access-Control-Allow-Origin: *\r\n");
    if (preflight) {
        ecs_strbuf_appendlit(hdrs, "Access-Control-Allow-Private-Network: true\r\n");
        ecs_strbuf_appendlit(hdrs, "Access-Control-Allow-Methods: GET, PUT, DELETE, OPTIONS\r\n");
        ecs_strbuf_appendlit(hdrs, "Access-Control-Max-Age: 600\r\n");
    }

    ecs_strbuf_mergebuff(hdrs, extra_headers);

    ecs_strbuf_appendlit(hdrs, "\r\n");
}

static
void http_send_reply(
    ecs_http_connection_impl_t* conn, 
    ecs_http_reply_t* reply,
    bool preflight) 
{
    ecs_strbuf_t hdrs = ECS_STRBUF_INIT;
    int32_t content_length = reply->body.length;
    char *content = ecs_strbuf_get(&reply->body);

    /* Use asynchronous send queue for outgoing data so send operations won't
     * hold up main thread */
    ecs_http_send_request_t *req = NULL;

    if (!preflight) {
        req = http_send_queue_post(conn->pub.server);
        if (!req) {
            reply->code = 503; /* queue full, server is busy */
            ecs_os_linc(&ecs_http_busy_count);
        }
    }

    http_append_send_headers(&hdrs, reply->code, reply->status, 
        reply->content_type, &reply->headers, content_length, preflight);
    ecs_size_t headers_length = ecs_strbuf_written(&hdrs);
    char *headers = ecs_strbuf_get(&hdrs);

    if (!req) {
        ecs_size_t written = http_send(conn->sock, headers, headers_length, 0);
        if (written != headers_length) {
            ecs_err("http: failed to send reply to '%s:%s': %s",
                conn->pub.host, conn->pub.port, ecs_os_strerror(errno));
            ecs_os_linc(&ecs_http_send_error_count);
        }
        ecs_os_free(content);
        ecs_os_free(headers);
        http_close(&conn->sock);
        return;
    }

    /* Second, enqueue send request for response body */
    req->sock = conn->sock;
    req->headers = headers;
    req->header_length = headers_length;
    req->content = content;
    req->content_length = content_length;

    /* Take ownership of values */
    reply->body.content = NULL;
    conn->sock = HTTP_SOCKET_INVALID;
}

static
void http_recv_connection(
    ecs_http_server_t *srv,
    ecs_http_connection_impl_t *conn, 
    uint64_t conn_id,
    ecs_http_socket_t sock)
{
    ecs_size_t bytes_read;
    char *recv_buf = ecs_os_malloc(ECS_HTTP_SEND_RECV_BUFFER_SIZE);
    ecs_http_fragment_t frag = {0};
    int32_t retries = 0;

    ecs_os_sleep(0, 10 * 1000 * 1000);

    do {
        if ((bytes_read = http_recv(
            sock, recv_buf, ECS_HTTP_SEND_RECV_BUFFER_SIZE, 0)) > 0) 
        {
            bool is_alive = conn->pub.id == conn_id;
            if (!is_alive) {
                /* Connection has been purged by main thread */
                goto done;
            }

            if (http_parse_request(&frag, recv_buf, bytes_read)) {
                if (frag.method == EcsHttpOptions) {
                    ecs_http_reply_t reply;
                    reply.body = ECS_STRBUF_INIT;
                    reply.code = 200;
                    reply.content_type = NULL;
                    reply.headers = ECS_STRBUF_INIT;
                    reply.status = "OK";
                    http_send_reply(conn, &reply, true);
                    ecs_os_linc(&ecs_http_request_preflight_count);
                } else {
                    ecs_http_request_entry_t *entry =
                        http_enqueue_request(conn, conn_id, &frag);
                    if (entry) {
                        ecs_http_reply_t reply;
                        reply.body = ECS_STRBUF_INIT;
                        reply.code = entry->code;
                        reply.content_type = "application/json";
                        reply.headers = ECS_STRBUF_INIT;
                        reply.status = "OK";
                        ecs_strbuf_appendstrn(&reply.body, 
                            entry->content, entry->content_length);
                        http_send_reply(conn, &reply, false);
                        http_connection_free(conn);

                        /* Lock was transferred from enqueue_request */
                        ecs_os_mutex_unlock(srv->lock);
                    }
                }
            } else {
                ecs_os_linc(&ecs_http_request_invalid_count);
            }
        }

        ecs_os_sleep(0, 10 * 1000 * 1000);
    } while ((bytes_read == -1) && (++retries < ECS_HTTP_REQUEST_RECV_RETRY));

    if (bytes_read == ECS_HTTP_SEND_RECV_BUFFER_SIZE) {
        ecs_warn("request exceeded receive buffer size (%d)",
            ECS_HTTP_SEND_RECV_BUFFER_SIZE);
    }

    if (retries == ECS_HTTP_REQUEST_RECV_RETRY) {
        http_close(&sock);
    }

done:
    ecs_os_free(recv_buf);
    ecs_strbuf_reset(&frag.buf);
}

typedef struct {
    ecs_http_connection_impl_t *conn;
    uint64_t id;
} http_conn_res_t;

static
http_conn_res_t http_init_connection(
    ecs_http_server_t *srv, 
    ecs_http_socket_t sock_conn,
    struct sockaddr_storage *remote_addr, 
    ecs_size_t remote_addr_len) 
{
    http_sock_set_timeout(sock_conn, 100);
    http_sock_keep_alive(sock_conn);
    http_sock_nonblock(sock_conn, true);

    /* Create new connection */
    ecs_os_mutex_lock(srv->lock);
    ecs_http_connection_impl_t *conn = flecs_sparse_add_t(
        &srv->connections, ecs_http_connection_impl_t);
    uint64_t conn_id = conn->pub.id = flecs_sparse_last_id(&srv->connections);
    conn->pub.server = srv;
    conn->sock = sock_conn;
    ecs_os_mutex_unlock(srv->lock);

    char *remote_host = conn->pub.host;
    char *remote_port = conn->pub.port;

    /* Fetch name & port info */
    if (http_getnameinfo((struct sockaddr*) remote_addr, remote_addr_len,
        remote_host, ECS_SIZEOF(conn->pub.host),
        remote_port, ECS_SIZEOF(conn->pub.port),
            NI_NUMERICHOST | NI_NUMERICSERV))
    {
        ecs_os_strcpy(remote_host, "unknown");
        ecs_os_strcpy(remote_port, "unknown");
    }

    ecs_dbg_2("http: connection established from '%s:%s' (socket %u)", 
        remote_host, remote_port, sock_conn);
    
    return (http_conn_res_t){ .conn = conn, .id = conn_id };
}

static
int http_accept_connections(
    ecs_http_server_t* srv, 
    const struct sockaddr* addr, 
    ecs_size_t addr_len) 
{
#ifdef ECS_TARGET_WINDOWS
    /* If on Windows, test if winsock needs to be initialized */
    SOCKET testsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == testsocket && WSANOTINITIALISED == WSAGetLastError()){
        WSADATA data = { 0 };
        int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result) {
            ecs_warn("http: WSAStartup failed with GetLastError = %d\n", 
                GetLastError());
            return 0;
        }
    } else {
        http_close(&testsocket);
    }
#endif

    /* Resolve name + port (used for logging) */
    char addr_host[256];
    char addr_port[20];

    int ret = 0; /* 0 = ok, 1 = port occupied */

    ecs_http_socket_t sock = HTTP_SOCKET_INVALID;
    ecs_assert(srv->sock == HTTP_SOCKET_INVALID, ECS_INTERNAL_ERROR, NULL);

    if (http_getnameinfo(
        addr, addr_len, addr_host, ECS_SIZEOF(addr_host), addr_port, 
        ECS_SIZEOF(addr_port), NI_NUMERICHOST | NI_NUMERICSERV))
    {
        ecs_os_strcpy(addr_host, "unknown");
        ecs_os_strcpy(addr_port, "unknown");
    }

    ecs_os_mutex_lock(srv->lock);
    if (srv->should_run) {
        ecs_dbg_2("http: initializing connection socket");

        sock = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
        if (!http_socket_is_valid(sock)) {
            ecs_err("http: unable to create new connection socket: %s", 
                ecs_os_strerror(errno));
            ecs_os_mutex_unlock(srv->lock);
            goto done;
        }

        int reuse = 1, result;
        result = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
            (char*)&reuse, ECS_SIZEOF(reuse)); 
        if (result) {
            ecs_warn("http: failed to setsockopt: %s", ecs_os_strerror(errno));
        }

        if (addr->sa_family == AF_INET6) {
            int ipv6only = 0;
            if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, 
                (char*)&ipv6only, ECS_SIZEOF(ipv6only)))
            {
                ecs_warn("http: failed to setsockopt: %s", 
                    ecs_os_strerror(errno));
            }
        }

        result = http_bind(sock, addr, addr_len);
        if (result) {
            if (errno == EADDRINUSE) {
                ret = 1;
                ecs_warn("http: address '%s:%s' in use, retrying with port %u", 
                    addr_host, addr_port, srv->port + 1);
            } else {
                ecs_err("http: failed to bind to '%s:%s': %s", 
                    addr_host, addr_port, ecs_os_strerror(errno));
            }

            ecs_os_mutex_unlock(srv->lock);
            goto done;
        }

        http_sock_set_timeout(sock, 1000);

        srv->sock = sock;

        result = listen(srv->sock, SOMAXCONN);
        if (result) {
            ecs_warn("http: could not listen for SOMAXCONN (%d) connections: %s", 
                SOMAXCONN, ecs_os_strerror(errno));
        }

        ecs_trace("http: listening for incoming connections on '%s:%s'",
            addr_host, addr_port);
    } else {
        ecs_dbg_2("http: server shut down while initializing");
    }
    ecs_os_mutex_unlock(srv->lock);

    struct sockaddr_storage remote_addr;
    ecs_size_t remote_addr_len = 0;

    while (srv->should_run) {
        remote_addr_len = ECS_SIZEOF(remote_addr);
        ecs_http_socket_t sock_conn = http_accept(srv->sock, (struct sockaddr*) &remote_addr, 
            &remote_addr_len);

        if (!http_socket_is_valid(sock_conn)) {
            if (srv->should_run) {
                ecs_dbg("http: connection attempt failed: %s", 
                    ecs_os_strerror(errno));
            }
            continue;
        }

        http_conn_res_t conn = http_init_connection(srv, sock_conn, &remote_addr, remote_addr_len);
        http_recv_connection(srv, conn.conn, conn.id, sock_conn);
    }

done:
    ecs_os_mutex_lock(srv->lock);
    if (http_socket_is_valid(sock) && errno != EBADF) {
        http_close(&sock);
        srv->sock = sock;
    }
    ecs_os_mutex_unlock(srv->lock);

    ecs_trace("http: no longer accepting connections on '%s:%s'",
        addr_host, addr_port);

    return ret;
}

static
void* http_server_thread(void* arg) {
    ecs_http_server_t *srv = arg;
    struct sockaddr_in addr;
    ecs_os_zeromem(&addr);
    addr.sin_family = AF_INET;

    int retries = 0;
retry:
    addr.sin_port = htons(srv->port);

    if (!srv->ipaddr) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        inet_pton(AF_INET, srv->ipaddr, &(addr.sin_addr));
    }

    if (http_accept_connections(
        srv, (struct sockaddr*)&addr, ECS_SIZEOF(addr)) == 1) 
    {
        srv->port ++;
        retries ++;
        if (retries < 10) {
            goto retry;
        } else {
            ecs_err("http: failed to connect (retried 10 times)");
        }
    }

    return NULL;
}

static
void http_do_request(
    ecs_http_server_t *srv,
    ecs_http_reply_t *reply,
    const ecs_http_request_impl_t *req)
{
    ecs_check(srv != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(srv->callback != NULL, ECS_INVALID_OPERATION, 
        "missing request handler for server");

    if (srv->callback(ECS_CONST_CAST(ecs_http_request_t*, req), reply, 
        srv->ctx) == false) 
    {
        reply->status = "Resource not found";
        ecs_os_linc(&ecs_http_request_not_handled_count);
    } else {
        if (reply->code >= 400) {
            ecs_os_linc(&ecs_http_request_handled_error_count);
        } else {
            ecs_os_linc(&ecs_http_request_handled_ok_count);
        }
    }
error:
    return;
}

static
void http_handle_request(
    ecs_http_server_t *srv,
    ecs_http_request_impl_t *req)
{
    ecs_http_reply_t reply = ECS_HTTP_REPLY_INIT;
    ecs_http_connection_impl_t *conn = 
        (ecs_http_connection_impl_t*)req->pub.conn;

    if (req->pub.method != EcsHttpOptions) {
        if (srv->callback((ecs_http_request_t*)req, &reply, srv->ctx) == false) {
            reply.code = 404;
            reply.status = "Resource not found";
            ecs_os_linc(&ecs_http_request_not_handled_count);
        } else {
            if (reply.code >= 400) {
                ecs_os_linc(&ecs_http_request_handled_error_count);
            } else {
                ecs_os_linc(&ecs_http_request_handled_ok_count);
            }
        }

        if (req->pub.method == EcsHttpGet) {
            http_insert_request_entry(srv, req, &reply);
        }

        http_send_reply(conn, &reply, false);
        ecs_dbg_2("http: reply sent to '%s:%s'", conn->pub.host, conn->pub.port);
    } else {
        /* Already taken care of */
    }

    http_reply_fini(&reply);
    http_request_fini(req);
    http_connection_free(conn);
}

static
void http_purge_request_cache(
    ecs_http_server_t *srv,
    bool fini)
{
    ecs_time_t t = {0, 0};
    double time = ecs_time_measure(&t);
    ecs_map_iter_t it = ecs_map_iter(&srv->request_cache.impl);
    while (ecs_map_next(&it)) {
        ecs_hm_bucket_t *bucket = ecs_map_ptr(&it);
        int32_t i, count = ecs_vec_count(&bucket->values);
        ecs_http_request_key_t *keys = ecs_vec_first(&bucket->keys);
        ecs_http_request_entry_t *entries = ecs_vec_first(&bucket->values);
        for (i = count - 1; i >= 0; i --) {
            ecs_http_request_entry_t *entry = &entries[i];
            if (fini || ((time - entry->time) > srv->cache_purge_timeout)) {
                ecs_http_request_key_t *key = &keys[i];
                /* Safe, code owns the value */
                ecs_os_free(ECS_CONST_CAST(char*, key->array));
                ecs_os_free(entry->content);
                flecs_hm_bucket_remove(&srv->request_cache, bucket, 
                    ecs_map_key(&it), i);
            }
        }
    }

    if (fini) {
        flecs_hashmap_fini(&srv->request_cache);
    }
}

static
int32_t http_dequeue_requests(
    ecs_http_server_t *srv,
    double delta_time)
{
    ecs_os_mutex_lock(srv->lock);

    int32_t i, request_count = flecs_sparse_count(&srv->requests);
    for (i = request_count - 1; i >= 1; i --) {
        ecs_http_request_impl_t *req = flecs_sparse_get_dense_t(
            &srv->requests, ecs_http_request_impl_t, i);
        http_handle_request(srv, req);
    }

    int32_t connections_count = flecs_sparse_count(&srv->connections);
    for (i = connections_count - 1; i >= 1; i --) {
        ecs_http_connection_impl_t *conn = flecs_sparse_get_dense_t(
            &srv->connections, ecs_http_connection_impl_t, i);

        conn->dequeue_timeout += delta_time;
        conn->dequeue_retries ++;
        
        if ((conn->dequeue_timeout > 
            (double)ECS_HTTP_CONNECTION_PURGE_TIMEOUT) &&
             (conn->dequeue_retries > ECS_HTTP_CONNECTION_PURGE_RETRY_COUNT)) 
        {
            ecs_dbg("http: purging connection '%s:%s' (sock = %d)", 
                conn->pub.host, conn->pub.port, conn->sock);
            http_connection_free(conn);
        }
    }

    http_purge_request_cache(srv, false);
    ecs_os_mutex_unlock(srv->lock);

    return request_count - 1;
}

const char* ecs_http_get_header(
    const ecs_http_request_t* req,
    const char* name) 
{
    for (ecs_size_t i = 0; i < req->header_count; i++) {
        if (!ecs_os_strcmp(req->headers[i].key, name)) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

const char* ecs_http_get_param(
    const ecs_http_request_t* req,
    const char* name) 
{
    for (ecs_size_t i = 0; i < req->param_count; i++) {
        if (!ecs_os_strcmp(req->params[i].key, name)) {
            return req->params[i].value;
        }
    }
    return NULL;
}

ecs_http_server_t* ecs_http_server_init(
    const ecs_http_server_desc_t *desc) 
{
    ecs_http_server_t* srv = ecs_os_calloc_t(ecs_http_server_t);
    if (ecs_os_has_threading()) {
        srv->lock = ecs_os_mutex_new();
    }
    srv->sock = HTTP_SOCKET_INVALID;

    srv->should_run = false;
    srv->initialized = true;

    srv->cache_timeout = desc->cache_timeout;
    srv->cache_purge_timeout = desc->cache_purge_timeout;

    if (!ECS_EQZERO(srv->cache_timeout) && 
         ECS_EQZERO(srv->cache_purge_timeout)) 
    {
        srv->cache_purge_timeout = srv->cache_timeout * 10;
    }

    srv->callback = desc->callback;
    srv->ctx = desc->ctx;
    srv->port = desc->port;
    srv->ipaddr = desc->ipaddr;
    srv->send_queue.wait_ms = desc->send_queue_wait_ms;
    if (!srv->send_queue.wait_ms) {
        srv->send_queue.wait_ms = 1;
    }

    flecs_sparse_init_t(&srv->connections, NULL, NULL, ecs_http_connection_impl_t);
    flecs_sparse_init_t(&srv->requests, NULL, NULL, ecs_http_request_impl_t);

    /* Start at id 1 */
    flecs_sparse_new_id(&srv->connections);
    flecs_sparse_new_id(&srv->requests);

    /* Initialize request cache */
    flecs_hashmap_init(&srv->request_cache, 
        ecs_http_request_key_t, ecs_http_request_entry_t,
        http_request_key_hash, http_request_key_compare, NULL);

#ifndef ECS_TARGET_WINDOWS
    /* Ignore pipe signal. SIGPIPE can occur when a message is sent to a client
     * but te client already disconnected. */
    signal(SIGPIPE, SIG_IGN);
#endif

    return srv;
}

void ecs_http_server_fini(
    ecs_http_server_t* srv) 
{
    if (srv->should_run) {
        ecs_http_server_stop(srv);
    }
    if (ecs_os_has_threading()) {
        ecs_os_mutex_free(srv->lock);
    }
    http_purge_request_cache(srv, true);
    flecs_sparse_fini(&srv->requests);
    flecs_sparse_fini(&srv->connections);
    ecs_os_free(srv);
}

int ecs_http_server_start(
    ecs_http_server_t *srv)
{
    ecs_check(srv != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(srv->initialized, ECS_INVALID_PARAMETER, NULL);
    ecs_check(!srv->should_run, ECS_INVALID_PARAMETER, NULL);
    ecs_check(!srv->thread, ECS_INVALID_PARAMETER, NULL);
    ecs_check(ecs_os_has_threading(), ECS_UNSUPPORTED,
        "missing OS API implementation");

    srv->should_run = true;

    ecs_dbg("http: starting server thread");

    srv->thread = ecs_os_thread_new(http_server_thread, srv);
    if (!srv->thread) {
        goto error;
    }

    srv->send_queue.thread = ecs_os_thread_new(http_server_send_queue, srv);
    if (!srv->send_queue.thread) {
        goto error;
    }

    return 0;
error:
    return -1;
}

void ecs_http_server_stop(
    ecs_http_server_t* srv) 
{
    ecs_check(srv != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(srv->initialized, ECS_INVALID_OPERATION, 
        "cannot stop HTTP server: not initialized");
    ecs_check(srv->should_run, ECS_INVALID_PARAMETER, 
        "cannot stop HTTP server: already stopped/stopping");
    ecs_check(ecs_os_has_threading(), ECS_UNSUPPORTED,
        "missing OS API implementation");

    /* Stop server thread */
    ecs_dbg("http: shutting down server thread");

    ecs_os_mutex_lock(srv->lock);
    srv->should_run = false;
    if (http_socket_is_valid(srv->sock)) {
        http_close(&srv->sock);
    }
    ecs_os_mutex_unlock(srv->lock);

    ecs_os_thread_join(srv->thread);
    ecs_os_thread_join(srv->send_queue.thread);
    ecs_trace("http: server threads shut down");

    /* Cleanup all outstanding requests */
    int i, count = flecs_sparse_count(&srv->requests);
    for (i = count - 1; i >= 1; i --) {
        http_request_fini(flecs_sparse_get_dense_t(
            &srv->requests, ecs_http_request_impl_t, i));
    }

    /* Close all connections */
    count = flecs_sparse_count(&srv->connections);
    for (i = count - 1; i >= 1; i --) {
        http_connection_free(flecs_sparse_get_dense_t(
            &srv->connections, ecs_http_connection_impl_t, i));
    }

    ecs_assert(flecs_sparse_count(&srv->connections) == 1, 
        ECS_INTERNAL_ERROR, NULL);
    ecs_assert(flecs_sparse_count(&srv->requests) == 1,
        ECS_INTERNAL_ERROR, NULL);

    srv->thread = 0;
error:
    return;
}

void ecs_http_server_dequeue(
    ecs_http_server_t* srv,
    ecs_ftime_t delta_time)
{
    ecs_check(srv != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(srv->initialized, ECS_INVALID_PARAMETER, NULL);
    ecs_check(srv->should_run, ECS_INVALID_PARAMETER, NULL);
    
    srv->dequeue_timeout += (double)delta_time;
    srv->stats_timeout += (double)delta_time;

    if ((1000 * srv->dequeue_timeout) > (double)ECS_HTTP_MIN_DEQUEUE_INTERVAL) {
        srv->dequeue_timeout = 0;

        ecs_time_t t = {0};
        ecs_time_measure(&t);
        int32_t request_count = http_dequeue_requests(srv, srv->dequeue_timeout);
        srv->requests_processed += request_count;
        srv->requests_processed_total += request_count;
        double time_spent = ecs_time_measure(&t);
        srv->request_time += time_spent;
        srv->request_time_total += time_spent;
        srv->dequeue_count ++;
    }

    if ((1000 * srv->stats_timeout) > (double)ECS_HTTP_MIN_STATS_INTERVAL) {
        srv->stats_timeout = 0;
        ecs_dbg("http: processed %d requests in %.3fs (avg %.3fs / dequeue)",
            srv->requests_processed, srv->request_time, 
            (srv->request_time / (double)srv->dequeue_count));
        srv->requests_processed = 0;
        srv->request_time = 0;
        srv->dequeue_count = 0;
    }

error:
    return;
}

int ecs_http_server_http_request(
    ecs_http_server_t* srv,
    const char *req,
    ecs_size_t len,
    ecs_http_reply_t *reply_out)
{
    if (!len) {
        len = ecs_os_strlen(req);
    }

    ecs_http_fragment_t frag = {0};
    if (!http_parse_request(&frag, req, len)) {
        ecs_strbuf_reset(&frag.buf);
        reply_out->code = 400;
        return -1;
    }

    ecs_http_request_impl_t request;
    char *res = http_decode_request(&request, &frag);
    if (!res) {
        reply_out->code = 400;
        return -1;
    }

    ecs_http_request_entry_t *entry = 
        http_find_request_entry(srv, request.res, request.req_len);
    if (entry) {
        reply_out->body = ECS_STRBUF_INIT;
        reply_out->code = entry->code;
        reply_out->content_type = "application/json";
        reply_out->headers = ECS_STRBUF_INIT;
        reply_out->status = "OK";
        ecs_strbuf_appendstrn(&reply_out->body, 
            entry->content, entry->content_length);
    } else {
        http_do_request(srv, reply_out, &request);

        if (request.pub.method == EcsHttpGet) {
            http_insert_request_entry(srv, &request, reply_out);
        }
    }

    ecs_os_free(res);

    http_purge_request_cache(srv, false);

    return (reply_out->code >= 400) ? -1 : 0;
}

int ecs_http_server_request(
    ecs_http_server_t* srv,
    const char *method,
    const char *req,
    const char *body,
    ecs_http_reply_t *reply_out)
{
    ecs_check(srv != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(method != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(req != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(reply_out != NULL, ECS_INVALID_PARAMETER, NULL);

    const char *http_ver = " HTTP/1.1\r\n";
    int32_t method_len = ecs_os_strlen(method);
    int32_t req_len = ecs_os_strlen(req);
    int32_t body_len = body ? ecs_os_strlen(body) : 0;
    int32_t http_ver_len = ecs_os_strlen(http_ver);
    char reqbuf[1024], *reqstr = reqbuf;
    char content_length[32] = {0};

    if (body_len) {
        ecs_os_snprintf(content_length, 32, 
            "Content-Length: %d\r\n\r\n", body_len);
    }

    int32_t content_length_len = ecs_os_strlen(content_length);

    int32_t len = method_len + req_len + content_length_len + body_len + 
        http_ver_len;

    len += 3;

    if (len >= 1024) {
        reqstr = ecs_os_malloc(len);
    }

    char *ptr = reqstr;
    ecs_os_memcpy(ptr, method, method_len); ptr += method_len;
    ptr[0] = ' '; ptr ++;
    ecs_os_memcpy(ptr, req, req_len); ptr += req_len;
    ecs_os_memcpy(ptr, http_ver, http_ver_len); ptr += http_ver_len;

    if (body) {
        ecs_os_memcpy(ptr, content_length, content_length_len);
        ptr += content_length_len;
        ecs_os_memcpy(ptr, body, body_len);
        ptr += body_len;
    }

    ptr[0] = '\r';
    ptr[1] = '\n';

    int result = ecs_http_server_http_request(srv, reqstr, len, reply_out);
    if (reqbuf != reqstr) {
        ecs_os_free(reqstr);
    }

    return result;
error:
    return -1;
}

void* ecs_http_server_ctx(
    ecs_http_server_t* srv)
{
    return srv->ctx;
}

#endif
