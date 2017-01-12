#include <iostream>
#include <cstdio>
#include <fstream>
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
#include <signal.h>
#include "webserver.h"

pid_t WebServer::masterPid;
std::set<pid_t> WebServer::pids;
int WebServer::workerStatus;
int (*WebServer::dataHandler)(char *) = NULL;
char WebServer::pidSavePath[128];

WebServer::WebServer(char *ip, int p) {
	masterPid = 0;
	workerProcess = 4;
	serverfd = 0;
    addr = ip;
    port = p;
    sprintf(pidSavePath, "%s/%s", getcwd(NULL, 0), "server.pid");
}

WebServer::~WebServer() {
    std::cout << getpid() << " killed" << std::endl;	
}

void WebServer::runAll() {
	setServerFd();
	setDeamon();
	setMasterPid();
    saveMasterPid2File();
	setSignal();
	forkWorker();
	monitorWorker();
}

void WebServer::saveMasterPid2File() {
    char buffer[10];
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "%d", masterPid);
    std::ofstream file;
    file.open(pidSavePath);
    if (file.is_open()) {
        file << buffer;
        file.close();
    } else {
        std::cout << "open the server.pid file failed" << std::endl;
        exit(0);
    }
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
    while (pids.size() < workerProcess) {
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
    state->read_ev = event_new(base, fd, EV_READ, WebServer::onRead, state);
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
    memset(state->buffer, 0, sizeof(state->buffer));
    state->cur_size = 0;
    return state;
}

void WebServer::deleteEventState(struct event_state *state){
    event_free(state->read_ev);
    event_free(state->write_ev);
    delete state;
}

void WebServer::onWrite(int fd, short event, void *arg) {
    struct event_state *state = (struct event_state *)arg;
    if (dataHandler) {
        int res = dataHandler(state->buffer); 
        if (res == 0) {
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            const char *strStatusCode = "Status Code: 200 OK\n";
            const char *content = "\nI am get it\n";
            strcpy(buffer, strStatusCode);
            strcpy(buffer+strlen(strStatusCode), content);
            send(fd, buffer, strlen(buffer), 0);
        }
    }
    /*int size = 0;
    while (1) {
        size = send(fd, state->buffer + size, strlen(state->buffer) - size, 0);
        if (size <= 0) {
            break;
        }
        if (size == strlen(state->buffer)) {
            break;
        }
    }*/
    delete state;
    close(fd);
}

void WebServer::setDataHandler(int (*callback)(char *buffer)) {
    dataHandler = callback;
}

void WebServer::onRead(int fd, short event, void *arg) {
    struct event_state * state = (struct event_state *)arg;
    int size = 0;
    char *tmp = state->buffer;
    while (1) {
        size = recv(fd, tmp, 1, 0);
        if (size <= 0) {
            break;
        }
        state->cur_size += size;
        tmp += size;
        if (state->buffer[state->cur_size - 1] == '\n') {
            event_add(state->write_ev, NULL);
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
		if (pid > 0) {
			std::set<int>::iterator it = pids.find(pid);
			pids.erase(it);
		}
        if (workerStatus == WORKER_IS_RUNNING) {
            forkWorker();            
        }
	}
}

void WebServer::signalHandler(int sigo) {
	switch (sigo) {
		case SIGINT:
            stopWorker();
		break;
		case SIGUSR1:
			std::cout << "sigusr1" << std::endl;
		break;
		case SIGUSR2:
			std::cout << "sigusr2" << std::endl;
		break;
	}
}

void WebServer::stopWorker() {
    if (masterPid == getpid()) {
        if (remove(pidSavePath) == -1) {
            std::cout << "remove server.pid failed" << std::endl;
            exit(0);
        }
        workerStatus = WORKER_IS_SHUTDOWN;
        std::set<pid_t>::iterator it;
        for (it=pids.begin(); it!=pids.end(); ++it) {
            kill(*it, SIGKILL);
            pids.erase(it);
        }
        kill(masterPid, SIGKILL);
    }
}

void WebServer::setMasterPid() {
	masterPid = getpid();
    workerStatus = WORKER_IS_RUNNING;
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
	server_addr.sin_addr.s_addr = inet_addr(addr);
	server_addr.sin_port = htons(port);
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

int echoData(char *data) {
    std::cout << data << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    char addr[] = "0.0.0.0";
	WebServer webserver(addr, 8888);
    webserver.setWorkerProcess(4);
    webserver.setDataHandler(echoData);
	webserver.runAll();
	return 0;
}
