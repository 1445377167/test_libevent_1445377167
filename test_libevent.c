#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>


void callback_func(evutil_socket_t fd, short event, void *arg)
{
	char buf[256] = {0};
	int len = 0;
	struct event_base *base = (struct event_base *)arg;

	printf("fd = %d, event = %d,", fd, event);

	len = read(fd, buf, sizeof(buf));

	if (len == -1) {
		perror("read");
		return;
	} else if (len == 0) {
		perror("remote close fd or nothing");
		return;
	} else {
		buf[len] = '\0';
		printf("read buf=[%s]\n", buf);

		FILE *fp = fopen("event_base_stat.txt", "a");
		if (fp == NULL) {
			perror("fopen error");
			exit(1);
		}
		event_base_dump_events(base, fp);
		fclose(fp);
	}
	return ;
}

void cb_func(evutil_socket_t fd, short what, void *arg)
{
	const char *data = arg;
	printf("Got an event on socket %d:%s%s%s%s [%s]\n",
		(int)fd,
		(what&EV_TIMEOUT) ? " timeout" : "",
		(what&EV_READ) ? " read" : "",
		(what&EV_WRITE) ? " write" : "",
		(what&EV_SIGNAL) ? " signal" : "",
		data);
}

void main_loop(evutil_socket_t fd1, evutil_socket_t fd2)
{
	struct event *ev1, *ev2;
	struct timeval five_seconds = {5,0};
	struct event_base *base = event_base_new();
	
	ev1 = event_new(base, fd1, EV_TIMEOUT|EV_READ|EV_PERSIST, cb_func, (char*)"Reading event");
	ev2 = event_new(base, fd2, EV_WRITE, cb_func, (char*)"Writing event");
	//ev2 = event_new(base, fd2, EV_WRITE|EV_PERSIST, cb_func, (char*)"Writing event");
	event_add(ev1, &five_seconds);
	event_add(ev2, NULL);
	event_base_dispatch(base);
}

static void
echo_read_cb(struct bufferevent *bev, void *ctx)
{
	struct evbuffer *input = bufferevent_get_input(bev);

	struct evbuffer *output = bufferevent_get_output(bev);

	evbuffer_add_buffer(output, input);
}

static void
echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
	}
}

static void
accept_error_cb(struct evconnlistener *listener, void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
}

static void
accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);

	struct bufferevent *bev = bufferevent_socket_new(
		base, fd, BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);

	bufferevent_enable(bev, EV_READ|EV_WRITE);
}

int main(int argc, char *argv[])
{
	struct event_base *base = NULL;

	struct evconnlistener *listener;
	struct sockaddr_in sin;

	int port = 9876;

	if (argc > 1) {
		port = atoi(argv[1]);
	}

	if (port <= 0 || port > 65535) {
		puts("Invalid port");
		return 1;
	}

	/*
	struct event *evfifo = NULL;

	const char *fifo = "event.fifo";

	int fd1, fd2;

	unlink(fifo);

	if (mkfifo(fifo, 0644)) {
		perror("mkfifo error:");
		exit(1);
	}

	fd1 = open(fifo, O_RDONLY|O_NONBLOCK, 0644);	
	if (fd1 < 0) {
		perror("open fifo error");
		exit(1);
	}
	*/

	/*
	fd2 = open(fifo, O_WRONLY|O_NONBLOCK, 0644);	
	if (fd2 < 0) {
		perror("open fifo error");
		exit(1);
	}

	main_loop(fd1,fd2);

	close(fd1);
	close(fd2);
	*/

	base = event_base_new();
	if (!base) {
		puts("Couldn't open event base");
		return 1;
	}	

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0);
	sin.sin_port = htons(port);

	listener = evconnlistener_new_bind(base, accept_conn_cb, NULL, LEV_OPT_CLOSE_ON_FREE||LEV_OPT_REUSEABLE, -1, (struct sockaddr*)&sin, sizeof(sin));

	if (!listener) {
		perror("Couldn't create listener");
		return 1;
	}

	evconnlistener_set_error_cb(listener, accept_error_cb);

	event_base_dispatch(base);

	/*
	evfifo = event_new(base, fd1, EV_READ|EV_PERSIST, callback_func, base);

	event_add(evfifo, NULL);

	event_base_dispatch(base);

	event_free(evfifo);
	event_base_free(base);
	*/

	event_base_free(base);

	return 0;
}
