#include <assert.h>
#include <event.h>
#include <evhttp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <sstream>

#include "util.hpp"

void error(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    exit(EXIT_FAILURE);
}

// A generic callback for evhttp. Turns on UTF-8 and sends 200 BUTTS
// lovingly.
void route(struct evhttp_request *request, void *arg) {
    int ret;

    struct evbuffer *buf = evbuffer_new();
    struct evkeyvalq *headers = evhttp_request_get_output_headers(request);
    ret = evhttp_add_header(headers, "Content-Type", "text/html; charset=utf-8");
    if (ret != 0) {
        error("Unable to add Content-Type header\n");
    }

    evbuffer_add_printf(buf, "<3");
    evhttp_send_reply(request, HTTP_OK, "BUTTS", buf);
    evbuffer_free(buf);
}

int main(int argc, char **argv) {
    int ret;

    assert(argc == 4 && "Invalid arguments");
    drop_privileges(argv[1]);

    ev_uint16_t port;
    std::stringstream port_stream(argv[3]);
    port_stream >> port;
    if (port_stream.fail() || !port_stream.eof()) {
        error("Invalid port number: %s\n", argv[3]);
    }

    const char *version = event_get_version();
    printf("[log] libevent: %s\n", version);
    printf("[log] port: %d\n", port);

    struct event_base *base = event_base_new();
    struct evhttp *http = evhttp_new(base);
    ret = evhttp_bind_socket(http, argv[2], port);
    if (ret != 0) {
        evhttp_free(http);
        event_base_free(base);
        error("Unable to bind to %s:%d\n", argv[2], port);
    }

    evhttp_set_gencb(http, route, NULL);
    event_base_dispatch(base);
    evhttp_free(http);
    event_base_free(base);
    return 0;
}
