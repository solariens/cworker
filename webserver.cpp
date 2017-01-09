#include <iostream>
#include <string>
#include <set>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <event.h>
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
	setDeamon();
	setMasterPid();
	setSignal();
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

void WebServer::setNonblock() {
	int flags;
	if ((flags = fcntl(serverfd, F_GETFL)) == -1) {
		std::cout << "get serverfd flags failed" << std::endl;
		exit(0);
	}
	flags |= O_NONBLOCK;
	if (fcntl(serverfd, F_SETFL, flags) == -1) {
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
    struct event *read_ev = new struct event();
    event_set(read_ev, clientfd, EV_READ|EV_PERSIST, WebServer::onRead, read_ev);
    event_base_set(base, read_ev);
    event_add(read_ev, NULL);
}

void WebServer::onRead(int fd, short event, void *arg) {
    struct event *read_ev = (struct event *)arg;
    char *buffer = new char[1024];
    memset(buffer, 0, sizeof(buffer));
    int size = 0;
    while (true) {
        size = recv(fd, buffer+size, sizeof(buffer) - size, 0);
        if (size <= 0) {
            break;
        }
    }
    std::cout << buffer << std::endl;
    event_del(read_ev);
    delete read_ev;
    delete [] buffer;
    close(fd);
    return;
}

void WebServer::runWorker() {
    struct event_base *base = event_base_new();
    struct event *listen_ev = new struct event();
    event_set(listen_ev, serverfd, EV_READ|EV_PERSIST, WebServer::onAccept, base);
    event_base_set(base, listen_ev);
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
	setNonblock();
}

int main(int argc, char *argv[]) {
	WebServer webserver;
	webserver.runAll();
	return 0;
}
