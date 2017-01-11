#include <iostream>
#include <string>
#include <set>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <event2/event.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "webserver.h"

WebServer::WebServer() {
	masterPid = 0;
	workerProcess = 4;
	serverfd = 0;
}

WebServer::~WebServer() {
	
}

void WebServer::runAll() {
	setServerFd();
	//setDeamon();
	setMasterPid();
	//setSignal();
	forkWorker();
	monitorWorker();
}

void WebServer::setDeamon() {
	pid_t pid = fork();
	if (pid < 0) {
		std::cout << "fork process failed" << std::endl;
		exit(0);
	} else if (pid > 0) {
		exit(0);
	} else {
		chdir("/");
		umask(0);
	}
}

void WebServer::setNonblock(int fd) {
	int flags;
	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		std::cout << "get serverfd flags failed" << std::endl;
		exit(0);
	}
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1) {
		std::cout << "set serverfd flags failed" << std::endl;
		exit(0);
	}	
}

void WebServer::setSignal() {
	signal(SIGINT, WebServer::signalHandler);
	signal(SIGUSR1, WebServer::signalHandler);
	signal(SIGUSR2, WebServer::signalHandler);
}

void WebServer::setWorkerProcess(int processNum) {
	workerProcess = processNum > 0 ? processNum : 1;
}

void WebServer::forkWorker() {
	for (int i=0; i<workerProcess; ++i) {
		pid_t pid = fork();
		if (pid < 0) {
			std::cout << "fork process failed, please try again" << std::endl;
			exit(0);
		} else if (pid > 0) {
			pids.insert(pid);
		} else {
			runWorker();
			exit(100);
		}
	}	
}

void WebServer::onAccept(int fd, short event, void *arg) {
    struct event_base *base = (struct event_base *)arg;
    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(clientaddr));
    socklen_t len = sizeof(clientaddr);
    int clientfd = accept(fd, (struct sockaddr *)&clientaddr, &len);
    if (clientfd < 0) {
        std::cout << "accept failed" << std::endl;
        return;
    }
    setNonblock(clientfd);
    struct event_state *state = allocateEventState(base, clientfd);
    event_add(state->read_ev, NULL);
}

struct event_state *WebServer::allocateEventState(struct event_base *base, int fd) {
    struct event_state *state = new struct event_state();
    if (!state) {
        return NULL;
    }
    state->read_ev = event_new(base, fd, EV_READ|EV_PERSIST, WebServer::onRead, state);
    if (!state->read_ev) {
        delete state;
        return NULL;
    }
    state->write_ev = event_new(base, fd, EV_WRITE, WebServer::onWrite, state);
    if (!state->write_ev) {
        event_free(state->read_ev);
        delete state;
        return NULL;
    }
    return state;
}

void WebServer::deleteEventState(struct event_state *state){
    event_del(state->read_ev);
    event_free(state->read_ev);
    event_free(state->write_ev);
    delete state;
}

void WebServer::onWrite(int fd, short event, void *arg) {
    struct event_state *state = (struct event_state *)arg;
    send(fd, "hello world!", strlen("hello world!"), 0);
    delete state;
    close(fd);
}

void WebServer::onRead(int fd, short event, void *arg) {
    struct event_state * state = (struct event_state *)arg;
    int size = 0;
    char buffer[65535];
    memset(buffer, 0, sizeof(buffer));
    int datasize = 0;
    while (1) {
        char *tmp = buffer;
        size = recv(fd, tmp, 256, 0);
        if (size <= 0) {
            break;
        }
        datasize += size;
        tmp += size;
        std::cout << datasize << std::endl;
        if (buffer[datasize - 1] == ' ') {
            std::cout << buffer << std::endl;
            event_add(state->write_ev, NULL);
            memset(buffer, 0, sizeof(buffer));
        }
    }
    return;
}

void WebServer::runWorker() {
    struct event_base *base = event_base_new();
    struct event *listen_ev = event_new(base, serverfd, EV_READ|EV_PERSIST, WebServer::onAccept, base);
    event_add(listen_ev, NULL);
    event_base_dispatch(base);
}

void WebServer::monitorWorker() {
	while (true) {
		if (pids.empty()) {
			break;
		}
		int status;
		pid_t pid = wait(&status);
		status = WEXITSTATUS(status);
		if (status == 100) {
			std::set<int>::iterator it = pids.find(pid);
			pids.erase(it);
		}
	}
}

void WebServer::signalHandler(int sigo) {
	switch (sigo) {
		case SIGINT:
			std::cout << "sigint" << std::endl;
		break;
		case SIGUSR1:
			std::cout << "sigusr1" << std::endl;
		break;
		case SIGUSR2:
			std::cout << "sigusr2" << std::endl;
		break;
	}
}

void WebServer::setMasterPid() {
	masterPid = getpid();
} 

void WebServer::setServerFd() {
	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverfd == -1) {
		std::cout << "socket create failed, please try again" << std::endl;
		exit(0);
	}
	int flag = 1;
	if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
		std::cout << "socket option set failed" << std::endl;
		exit(0);
	}
	
	if (setsockopt(serverfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) == -1) {
		std::cout << "socket set tcp nodelay failed" << std::endl;
		exit(0);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	server_addr.sin_port = htons(8888);
	if (bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		std::cout << "socket bid failed, please try again" << std::endl;
		exit(0);
	}

	if (listen(serverfd, 512) == -1) {
		std::cout << "socket listen failed, please try again" << std::endl;
		exit(0);
	}
	setNonblock(serverfd);
}

int main(int argc, char *argv[]) {
	WebServer webserver;
	webserver.runAll();
	return 0;
}
