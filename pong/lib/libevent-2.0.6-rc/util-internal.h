/*
 * Copyright (c) 2007-2010 Niels Provos and Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _EVENT_UTIL_INTERNAL_H
#define _EVENT_UTIL_INTERNAL_H

#include "event2/event-config.h"
#include <errno.h>

/* For EVUTIL_ASSERT */
#include "log-internal.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef _EVENT_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include "event2/util.h"

#ifdef __cplusplus
extern "C" {
#endif

/* If we need magic to say "inline", get it for free internally. */
#ifdef _EVENT_inline
#define inline _EVENT_inline
#endif
#ifdef _EVENT___func__
#define __func__ _EVENT___func__
#endif

/* A good no-op to use in macro definitions. */
#define _EVUTIL_NIL_STMT ((void)0)

/* Internal use only: macros to match patterns of error codes in a
   cross-platform way.  We need these macros because of two historical
   reasons: first, nonblocking IO functions are generally written to give an
   error on the "blocked now, try later" case, so sometimes an error from a
   read, write, connect, or accept means "no error; just wait for more
   data," and we need to look at the error code.  Second, Windows defines
   a different set of error codes for sockets. */

#ifndef WIN32

/* True iff e is an error that means a read/write operation can be retried. */
#define EVUTIL_ERR_RW_RETRIABLE(e)				\
	((e) == EINTR || (e) == EAGAIN)
/* True iff e is an error that means an connect can be retried. */
#define EVUTIL_ERR_CONNECT_RETRIABLE(e)			\
	((e) == EINTR || (e) == EINPROGRESS)
/* True iff e is an error that means a accept can be retried. */
#define EVUTIL_ERR_ACCEPT_RETRIABLE(e)			\
	((e) == EINTR || (e) == EAGAIN || (e) == ECONNABORTED)

/* True iff e is an error that means the connection was refused */
#define EVUTIL_ERR_CONNECT_REFUSED(e)					\
	((e) == ECONNREFUSED)

#else

#define EVUTIL_ERR_RW_RETRIABLE(e)					\
	((e) == WSAEWOULDBLOCK ||					\
	    (e) == WSAEINTR)

#define EVUTIL_ERR_CONNECT_RETRIABLE(e)					\
	((e) == WSAEWOULDBLOCK ||					\
	    (e) == WSAEINTR ||						\
	    (e) == WSAEINPROGRESS ||					\
	    (e) == WSAEINVAL)

#define EVUTIL_ERR_ACCEPT_RETRIABLE(e)			\
	EVUTIL_ERR_RW_RETRIABLE(e)

#define EVUTIL_ERR_CONNECT_REFUSED(e)					\
	((e) == WSAECONNREFUSED)

#endif

#ifdef _EVENT_socklen_t
#define socklen_t _EVENT_socklen_t
#endif

/* Locale-independent replacements for some ctypes functions.  Use these
 * when you care about ASCII's notion of character types, because you are about
 * to send those types onto the wire.
 */
#define DECLARE_CTYPE_FN(name)						\
	static int EVUTIL_##name(char c);				\
	extern const ev_uint32_t EVUTIL_##name##_TABLE[];		\
	static inline int EVUTIL_##name(char c) {			\
		ev_uint8_t u = c;					\
		return !!(EVUTIL_##name##_TABLE[(u >> 5) & 7] & (1 << (u & 31))); \
	}
DECLARE_CTYPE_FN(ISALPHA)
DECLARE_CTYPE_FN(ISALNUM)
DECLARE_CTYPE_FN(ISSPACE)
DECLARE_CTYPE_FN(ISDIGIT)
DECLARE_CTYPE_FN(ISXDIGIT)
DECLARE_CTYPE_FN(ISPRINT)
DECLARE_CTYPE_FN(ISLOWER)
DECLARE_CTYPE_FN(ISUPPER)
extern const unsigned char EVUTIL_TOUPPER_TABLE[];
extern const unsigned char EVUTIL_TOLOWER_TABLE[];
#define EVUTIL_TOLOWER(c) ((char)EVUTIL_TOLOWER_TABLE[(ev_uint8_t)c])
#define EVUTIL_TOUPPER(c) ((char)EVUTIL_TOUPPER_TABLE[(ev_uint8_t)c])

/** Helper macro.  If we know that a given pointer points to a field in a
    structure, return a pointer to the structure itself.  Used to implement
    our half-baked C OO.  Example:

    struct subtype {
	int x;
	struct supertype common;
	int y;
    };
    ...
    void fn(struct supertype *super) {
	struct subtype *sub = EVUTIL_UPCAST(super, struct subtype, common);
	...
    }
 */
#define EVUTIL_UPCAST(ptr, type, field)				\
	((type *)(((char*)(ptr)) - evutil_offsetof(type, field)))

int evutil_read_file(const char *filename, char **content_out, size_t *len_out,
    int is_binary);

int evutil_socket_connect(evutil_socket_t *fd_ptr, struct sockaddr *sa, int socklen);

int evutil_socket_finished_connecting(evutil_socket_t fd);

int evutil_ersatz_socketpair(int, int , int, evutil_socket_t[]);

int evutil_resolve(int family, const char *hostname, struct sockaddr *sa,
    ev_socklen_t *socklen, int port);

const char *evutil_getenv(const char *name);

long _evutil_weakrand(void);

/* Evaluates to the same boolean value as 'p', and hints to the compiler that
 * we expect this value to be false. */
#ifdef __GNUC__
#define EVUTIL_UNLIKELY(p) __builtin_expect(!!(p),0)
#else
#define EVUTIL_UNLIKELY(p) (p)
#endif

/* Replacement for assert() that calls event_errx on failure. */
#define EVUTIL_ASSERT(cond)						\
	do {								\
		if (EVUTIL_UNLIKELY(!(cond))) {				\
			event_errx(_EVENT_ERR_ABORT,			\
			    "%s:%d: Assertion %s failed in %s",		\
			    __FILE__,__LINE__,#cond,__func__);		\
			/* In case a user-supplied handler tries to */	\
			/* return control to us, log and abort here. */	\
			(void)fprintf(stderr,				\
			    "%s:%d: Assertion %s failed in %s",		\
			    __FILE__,__LINE__,#cond,__func__);		\
			abort();					\
		}							\
	} while (0)

/* Internal addrinfo error code.  This one is returned from only from
 * evutil_getaddrinfo_common, when we are sure that we'll have to hit a DNS
 * server. */
#define EVUTIL_EAI_NEED_RESOLVE      -90002

struct evdns_base;
struct evdns_getaddrinfo_request;
typedef struct evdns_getaddrinfo_request* (*evdns_getaddrinfo_fn)(
    struct evdns_base *base,
    const char *nodename, const char *servname,
    const struct evutil_addrinfo *hints_in,
    void (*cb)(int, struct evutil_addrinfo *, void *), void *arg);

void evutil_set_evdns_getaddrinfo_fn(evdns_getaddrinfo_fn fn);

struct evutil_addrinfo *evutil_new_addrinfo(struct sockaddr *sa,
    ev_socklen_t socklen, const struct evutil_addrinfo *hints);
struct evutil_addrinfo *evutil_addrinfo_append(struct evutil_addrinfo *first,
    struct evutil_addrinfo *append);
void evutil_adjust_hints_for_addrconfig(struct evutil_addrinfo *hints);
int evutil_getaddrinfo_common(const char *nodename, const char *servname,
    struct evutil_addrinfo *hints, struct evutil_addrinfo **res, int *portnum);

int evutil_getaddrinfo_async(struct evdns_base *dns_base,
    const char *nodename, const char *servname,
    const struct evutil_addrinfo *hints_in,
    void (*cb)(int, struct evutil_addrinfo *, void *), void *arg);

/** Return true iff sa is a looback address. (That is, it is 127.0.0.1/8, or
 * ::1). */
int evutil_sockaddr_is_loopback(const struct sockaddr *sa);


/**
    Formats a sockaddr sa into a string buffer of size outlen stored in out.
    Returns a pointer to out.  Always writes something into out, so it's safe
    to use the output of this function without checking it for NULL.
 */
const char *evutil_format_sockaddr_port(const struct sockaddr *sa, char *out, size_t outlen);

long evutil_tv_to_msec(const struct timeval *tv);

int evutil_hex_char_to_int(char c);

#ifdef __cplusplus
}
#endif

#endif
