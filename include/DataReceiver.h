#ifndef DATARECEIVER_H
#define DATARECEIVER_H

#include <ProtoData.pb.h>
#include <pthread.h>
#include <Logger.h>
#include <stdio.h>
#include <Drone.h>
#include <AppUtils.h>

using namespace std;
using namespace CPlusPlusLogging;
using namespace protobuf_ProtoData_2eproto;

class DataReceiver
{
    private:
    int socketFileDescriptor;
    Drone* droneInstance;
    Logger* pLogger = NULL; 
    pthread_t dataReceiverThread;
        
    public:
    bool isActive;

    DataReceiver( int sockfd, Drone* drn, Logger* logger);
    ~DataReceiver();
    static void* staticDataReceiverTask(void *dr);
    void dataReceiverTask();
    void start();
    void stop();

};
#endif