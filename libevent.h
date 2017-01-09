#ifndef LIBEVENT_H_

#define LIBEVENT_H_

class Event {
private:
	struct event_base *base;
    struct event_pair {
        int fd;
        struct event read_ev;
        struct event write_ev;
    };
public:
	Event();
    struct event_pair * event_pair_new(int fd);
	void addEvent(int serverfd, int flag, void (*func)(int sock, short event, void *args), void *args=NULL);
	void loop();
	~Event();
};

#endif
