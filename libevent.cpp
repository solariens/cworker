#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <event.h>
#include "libevent.h"

Event::Event() {
	base = event_base_new();
}

Event::~Event() {

}

void Event::addEvent(int serverfd, int flag, void (*func)(int sock, short event, void *args), void *args) {
	switch (flag) {
		case EV_READ:
			flag = EV_READ | EV_PERSIST;
		break;
	}	
	struct event listen_ev;
	event_set(&listen_ev, serverfd, flag, func, args);
	event_base_set(base, &listen_ev);
	event_add(&listen_ev, NULL);
}

void Event::loop() {
	event_base_dispatch(base);
}
