#ifndef WEBSERVER_H_

#define WEBSERVER_H_

#include <set>

struct event_state{
    struct event *read_ev;
    struct event *write_ev;
};

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
public:
	WebServer();
	void runAll();
    static void onRead(int fd, short event, void *arg);
    static void onWrite(int fd, short event, void *arg);
    static struct event_state *allocateEventState(struct event_base *, int);
    static void deleteEventState(struct event_state *);
	static void setNonblock(int);
    static void onAccept(int fd, short event, void *arg);
	~WebServer();
};

#endif
