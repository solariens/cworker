#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <event2/event.h>
#include <arpa/inet.h>
#include <sys/socket.h>

struct event_pair{
    struct event *read_ev;
    struct event *write_ev;
};

void setNonblock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flag);
}

void delete_event_state(struct event_pair *event) {
    event_free(event->read_ev);
    event_free(event->write_ev);
    delete event;
}

void onWrite(int fd, short event, void *arg) {
    struct event_pair *state = (struct event_pair *)arg;
    char buffer[1024] = "hello world!";
        int size = send(fd, buffer, strlen(buffer), 0);
            delete_event_state(state);
            close(fd);
}

void onRead(int fd, short event, void *arg) {
    struct event_pair *state = (struct event_pair *)arg;
    char buffer[1024];
    while (1) {
        int size = recv(fd, buffer, sizeof(buffer), 0);
        if (size <= 0) {
            break;
        }
        std::cout << buffer << std::endl;
        event_add(state->write_ev, NULL);
    }
}

struct event_pair * allocate_event_state(struct event_base *base, int fd) {
    struct event_pair *event = new struct event_pair();
    if (!event) {
        std::cout << "event init failed" << std::endl;
        return NULL;
    }
    event->read_ev = event_new(base, fd, EV_READ|EV_PERSIST, onRead, event);
    if (!event->read_ev) {
        delete event;
        std::cout << "allocate read event failed" << std::endl;
        return NULL;
    }
    event->write_ev = event_new(base, fd, EV_WRITE, onWrite, event);
    if (!event->write_ev) {
        event_free(event->read_ev);
        delete event;
        return NULL;
    }
    return event;
}

void onAccept(int fd, short event, void *arg) {
    struct event_base *base = (struct event_base *)arg;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(struct sockaddr_in);
    int clientfd = accept(fd, (struct sockaddr *)&addr, &len);
    setNonblock(clientfd);
    struct event_pair *state = allocate_event_state(base, clientfd);
    event_add(state->read_ev, NULL);
}


int main(int argc, char *argv[]) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(8888);
    bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));

    listen(fd, 16);
    
    setNonblock(fd);
    
    struct event_base *base = event_base_new();
    struct event *listen_ev = event_new(base, fd, EV_READ|EV_PERSIST, onAccept, base);
    event_add(listen_ev, NULL);
    event_base_dispatch(base);
    return 0;
}
