#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

// libevent.
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

// libme.
#include "pong.hpp"
#include "util.hpp"

enum {
    PORT = 40713,
    MAX_LINE = 16384
};

typedef struct {
    struct bufferevent *bev;
    pong_t pong;
    bool close;
} state_t;

static void readcb(struct bufferevent *bev, void *ctx);
static void writecb(struct bufferevent *bev, void *ctx);
static void errorcb(struct bufferevent *bev, short error, void *ctx);

static void prompt(struct bufferevent *bev) {
    bufferevent_write(bev, "\n> ", 3);
}

static state_t *state_new(struct event_base *base, evutil_socket_t fd) {
    state_t *state = (state_t *) malloc(sizeof(state_t));
    assert(state && "state_new: malloc");
    state->bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(state->bev, readcb, writecb, errorcb, state);
    bufferevent_setwatermark(state->bev, EV_READ, 0, MAX_LINE);
    bufferevent_enable(state->bev, EV_READ | EV_WRITE);

    struct timeval time;
    time.tv_sec = 1;
    time.tv_usec = 0;
    bufferevent_set_timeouts(state->bev, &time, NULL);

    pong_new(&state->pong);
    state->close = false;

    size_t m;
    char *message = pong_hello(&state->pong, &m);
    bufferevent_write(state->bev, message, m);
    free(message);
    prompt(state->bev);
    return state;
}

static void state_free(state_t *state) {
    bufferevent_free(state->bev);
    pong_free(&state->pong);
    free(state);
}

// Read data available on a file descriptor. Read the data into a
// buffer and pass it to pong.cpp where its state machine will spit
// back a nice message for the nice people.
static void readcb(struct bufferevent *bev, void *ctx) {
    state_t *state = (state_t *) ctx;
    struct evbuffer *input = bufferevent_get_input(bev);

    while (true) {
        size_t n, m;
        char *line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF);
        if (!line) {
            break;
        }
        if (state->close) {
            free(line);
            continue;
        }

        char *message = pong_feed(&state->pong, line, n, &m);
        free(line);

        if (message) {
            bufferevent_write(bev, message, m);
            prompt(bev);
            free(message);
        }

        if (state->pong.state == PONG_END) {
            state->close = true;
        }
    }

    size_t n = evbuffer_get_length(input);
    if (n >= MAX_LINE) {
        static char error[] = "ERROR: too much, too soon\n";
        evbuffer_drain(input, n);
        bufferevent_write(bev, error, sizeof(error));
    }
}

static void writecb(struct bufferevent *bev, void *ctx) {
    state_t *state = (state_t *) ctx;
    if (state->close) {
        state_free(state);
    }
}

// Handle errors. If it's a timeout, renable read/write and try again.
static void errorcb(struct bufferevent *bev, short error, void *ctx) {
    state_t *state = (state_t *) ctx;
    if (error & BEV_EVENT_EOF) {
        /* Connection closed */
        state_free(state);
    }
    else if (error & BEV_EVENT_ERROR) {
        perror("errorcb: ?");
        state_free(state);
    }
    else if (error & BEV_EVENT_TIMEOUT) {
        bufferevent_enable(bev, EV_READ | EV_WRITE);
        size_t m;
        char *message = pong_reset(&state->pong, &m);
        bufferevent_write(bev, message, m);
        free(message);
        prompt(bev);
    }
    else {
        assert(false);
    }
}

// Accept the new file descriptor if valid and create a new state
// object for it.
static void do_accept(evutil_socket_t server, short event, void *arg) {
    struct event_base *base = (struct event_base *) arg;
    struct sockaddr_storage st;
    socklen_t len = sizeof(st);
    int fd = accept(server, (struct sockaddr *) &st, &len);
    if (fd < 0) {
        perror("do_accept: accept");
        return;
    }

    if (fd > FD_SETSIZE) {
        close(fd);
        return;
    }

    evutil_make_socket_nonblocking(fd);
    state_new(base, fd);
}

int main(int argc, char **argv) {
    assert(argc == 2 && "Invalid arguments");
    drop_privileges(argv[1]);

    const char *version = event_get_version();
    printf("libevent: %s\n", version);

    // Our listening socket.
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(fd);
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // Bind to localhost on port PORT.
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(PORT);
    if (bind(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        perror("main: bind");
        exit(EXIT_FAILURE);
    }

    // And listen.
    if (listen(fd, 16) < 0) {
        perror("main: listen");
        exit(EXIT_FAILURE);
    }

    // Start libevent loop.
    struct event_base *base = event_base_new();
    if (!base) {
        fputs("main: event_base_new: null returned", stderr);
    }

    struct event *event = event_new(base, fd, EV_READ | EV_PERSIST,
                                    do_accept, (void *) base);
    pong_init();
    event_add(event, NULL);
    event_base_dispatch(base);
    event_free(event);
    pong_close();
    return 0;
}
