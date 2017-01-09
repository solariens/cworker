#include <iostream>
#include <string>
#include <set>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
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

void WebServer::runWorker() {
	std::cout << serverfd << std::endl;
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
	
	/*if (setsockopt(serverfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) == -1) {
		std::cout << "socket set tcp nodelay failed" << std::endl;
		exit(0);
	}*/

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
	std::cout << "hello world!" << std::endl;
	WebServer webserver;
	webserver.runAll();
	return 0;
}
