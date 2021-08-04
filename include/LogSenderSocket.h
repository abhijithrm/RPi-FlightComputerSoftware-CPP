#ifndef LOGSENDERSOCKET_H
#define LOGSENDERSOCKET_H

#include <pthread.h>
#include <Logger.h>
#include <stdio.h>
#include <Drone.h>
#include <AppUtils.h>
#include<ConnectionMonitor.h>
#include <sys/socket.h>
#include <netdb.h>


#define SA struct sockaddr
using namespace std;
using namespace CPlusPlusLogging;


class LogSender
{
    private:
    int socketFileDescriptor;
    Drone* droneInstance;
    Logger* pLogger = NULL; 
    pthread_t LogSenderThread;
    string hostServerIp;
    int droneID;
        
    public:
    bool isActive;
    ConnectionMonitor *networkMonitor;

    LogSender( Drone* drn, Logger* logger, ConnectionMonitor* nm, string hostServerIP, int droneId);
    ~LogSender();
    static void* staticLogSenderTask(void *ls);
    void LogSenderTask();
    void start();
    void stop();
    void connectToServerSocket();

};
#endif