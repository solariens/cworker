#ifndef WEBSERVER_H_

#define WEBSERVER_H_

#include <set>

class WebServer {
private:
	int serverfd;
	std::set<pid_t> pids;
	int workerProcess;
	pid_t masterPid;
	void monitorWorker();
	void setMasterPid();
	void forkWorker();
	void runWorker();
	void setServerFd();
	void setSignal();
	void setDeamon();
	static void signalHandler(int);
	void setWorkerProcess(int);
	void setNonblock();
    static void onAccept(int fd, short event, void *arg);
    static void onRead(int fd, short event, void *arg);
public:
	WebServer();
	void runAll();
	~WebServer();
};

#endif
