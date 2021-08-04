#ifndef CONNECTIONMONITOR_H
#define CONNECTIONMONITOR_H
#include <stdio.h>
#include<Drone.h>
#include <pthread.h>
#include <thread>
#include <chrono>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <AppUtils.h>

using namespace std;
using namespace CPlusPlusLogging;


class ConnectionMonitor
{
    private:
    pthread_t thread;
    Logger* pLogger = NULL;

    public:
    Drone *droneInstance;
    const char * hostIp;
    int maxConnectionAttempts;
    int connectionAttempts;
    bool netStatus;

    ConnectionMonitor(Drone *drn, const char* ip, int maxConn , Logger* logger);
    void start();
    void restart();
    static void* staticConnectionMonitorTask(void* cm);
    //Pings the host server ip to check for internet connection. Returns false if RaspberryPi loses network connection or Raspberry Pi cannot connect to host because host is down. It runs continuosly and helps to stop the control socket connection and DataReciever thread once network is down, so that the application doesn't crash with segmentation fault due to issues like writing to a closed socket etc.
    bool isInternetOn();
    void tokenize(std::string const &str, const char* delim, std::vector<std::string> &out);
    int parsePingCmdOutput(const char *cmd);
    void connectionMonitorTask();
    ~ConnectionMonitor();

};
#endif