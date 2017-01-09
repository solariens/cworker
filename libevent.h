#ifndef LIBEVENT_H_

#define LIBEVENT_H_

class Event {
private:
	struct event_base *base;
public:
	Event();
	void addEvent(int serverfd, int flag, void (*func)(int sock, short event, void *args), void *args=NULL);
	void loop();
	~Event();
};

#endif
