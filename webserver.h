#ifndef WEBSERVER_H_

#define WEBSERVER_H_

#include <set>

#define WORKER_IS_RUNNING 1
#define WORKER_IS_SHUTDOWN 0
#define MAX_CONN_BACKLOG 512

struct event_state{
    char buffer[65535];
    int cur_size;
    struct event *read_ev;
    struct event *write_ev;
};

class WebServer {
private:
	int serverfd;
	int workerProcess;
    const char *addr;
    int port;
	static pid_t masterPid;
    static int workerStatus;
    static char * (*dataHandler)(void *buffer);
    static char pidSavePath[128];
	static std::set<pid_t> pids;
    static char logFile[128];
	void monitorWorker();
    void init();
	void setMasterPid();
	void forkWorker();
	void runWorker();
	void setServerFd();
	void setSignal();
	void setDeamon();
    void saveMasterPid2File();
    static void log(const char *);
    static void stopWorker();
	static void signalHandler(int);
    static void onRead(int fd, short event, void *arg);
    static void onWrite(int fd, short event, void *arg);
    static struct event_state *allocateEventState(struct event_base *, int);
    static void deleteEventState(struct event_state *);
	static void setNonblock(int);
    static void onAccept(int fd, short event, void *arg);
public:
	WebServer(char *, int);
	void setWorkerProcess(int);
    void setDataHandler(char *(*callback)(void *));
	void runAll();
	~WebServer();
};

#endif
