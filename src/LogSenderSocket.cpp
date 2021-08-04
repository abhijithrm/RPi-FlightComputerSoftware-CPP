#include <LogSenderSocket.h>

//ctor
LogSender:: LogSender( Drone* drn, Logger* logger, ConnectionMonitor* nm, string hostip, int droneID)
    {
        //socket stream file descriptor...
        this->droneInstance = drn;
        this->isActive = true;
        this->pLogger = logger;
        this->networkMonitor = nm;
        this->hostServerIp = hostip;
        this->droneID = droneID;
        cout<<"Current size of log queue: "<<pLogger->threadSafeQueue->getCount()<<endl;
    }

//dtor
LogSender::~LogSender()
{
   pthread_cancel(LogSenderThread);
   pLogger->info("[Log Sender Thread]: LogSender thread stopped....");
   close(this->socketFileDescriptor);
   delete this;

}

//static thread task function wrapper...passed to pthread
void* LogSender::staticLogSenderTask(void *dr)
{
  ((LogSender*)dr)->LogSenderTask();
}

//Class member function holding thread body
void LogSender::LogSenderTask()
{
    this->connectToServerSocket();
    while(this->isActive && this->networkMonitor->netStatus && this->droneInstance->isActive)
    {
        try
        { 
            string consoleLogMessage = this->pLogger->threadSafeQueue->dequeue();
            unsigned char networkMessageByteArray[consoleLogMessage.size()+4];
            size_t sizeOfMessage = AppUtils::createNetworkMessage(consoleLogMessage, networkMessageByteArray);
            /*unsigned char networkMessageByteArray[sizeOfMessage];//byte array to store network message
            for(int i=0; i<sizeOfMessage; i++)
            {
            networkMessageByteArray[i] = message[i];
            }*/
            
            //serialization debug code-uncomment below portion to see the serialized byte info of protobuf DroneData object in terminal
            /*unsigned char test[sizeOfMessage-4];
            for(int i=0, j=4;j<sizeOfMessage;j++,i++)
            {
            test[i] = networkMessageByteArray[j];
            cout<<"Byte no: "<<i<<"="<<test[i]<<endl;
            }

            DroneData droneData;
            droneData.ParseFromArray(test, sizeOfMessage-4);
            cout<<droneData.altitude()<<droneData.latitude()<<droneData.longitude()<<droneData.voltage()<<endl;*/
            //test code
           
            int writeRes =  write(this->socketFileDescriptor, networkMessageByteArray, sizeOfMessage);
            if(writeRes == -1)
            {
                pLogger->error("[Log Sender Thread]: SOCKET: Failed to send console log data through socket connection. Re-attempting...");
                while(writeRes != -1)
                {
                    writeRes =  write(this->socketFileDescriptor, networkMessageByteArray, sizeOfMessage);
                    pLogger->error("[Log Sender Thread]: SOCKET: Sending console log data through socket stream failed. Re-attempting.");
                }
            }
            else if (writeRes < sizeOfMessage)
            {
               unsigned char temp[sizeOfMessage-writeRes];
               for(int i=0,j= writeRes; j<sizeOfMessage; i++, j++)
               temp[i] = networkMessageByteArray[j];

               writeRes =  write(this->socketFileDescriptor, temp, sizeOfMessage-writeRes);

            }
            //cout<<"Drone status data: Actual bytes written to socket o/p stream: "<< writeRes<<". Expected bytes needed to be sent: "<<sizeOfMessage<<endl;
            this_thread::sleep_for(chrono::seconds(1));
         }
        catch(const std::exception& e)
        {
            pLogger->info("LogSender error. See below for more info :");
            pLogger->error(e.what());
        } 
    
}
}

//create and start thread
void LogSender::start()
{
      int rc;
      pLogger->info("[Log Sender Thread]: Starting LogSender thread...");
      rc = pthread_create(&LogSenderThread, NULL, staticLogSenderTask, this);
      if (rc) 
      {
        pLogger->info("[Log Sender Thread]: Failed to start LogSender thread.");
        exit(-1);
      }

}

void LogSender::stop()
{
    this->isActive = false;
    pthread_cancel(LogSenderThread);
    close(this->socketFileDescriptor);
}

 void LogSender::connectToServerSocket()
 {
    int connfd;
    struct sockaddr_in servaddr, cli;
    // socket create and verification
    this->socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if ( this->socketFileDescriptor == -1) 
    {
        pLogger->info("[Log Sender Thread]: Client socket creation failed...");
        exit(0);
    }
    else
        pLogger->info("[Log Sender Thread]: Client socket successfully created..");
        bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(this->hostServerIp.c_str());
    servaddr.sin_port = htons(1315);

    // connect the client socket to server socket
    int animCounter = 0;
    while(connect(this->socketFileDescriptor, (SA*)&servaddr, sizeof(servaddr)) != 0)//RETURNS 0 IF SUCCESS
    {
        string sout = "[Log Sender Thread] [SOCKET]: Attempting TCP socket connection with server. Server IP: "+ string(this->hostServerIp) + ". Port: " + std::to_string(1315);
        animCounter = AppUtils::waitingConsoleAnimation(animCounter, sout, pLogger);
        this_thread::sleep_for(chrono::seconds(1));
    }
    string sout = "[Log Sender Thread]: TCP socket connection accepeted by server with IP: "+ string(this->hostServerIp) + " at port: " + std::to_string(1315);
    this->pLogger->info(sout);

    //write drone id to socket o/p stream.
    unsigned char buff[10];
    bzero(buff, sizeof(buff));//fill zeros
    string droneid = to_string(this->droneID);
    size_t sizeOfDroneIdMessage  = AppUtils::createNetworkMessage(droneid, buff);//drone id to bytes
    unsigned char droneIdNetworkMessageBytes[sizeOfDroneIdMessage];
    for(int i=0; i<sizeOfDroneIdMessage; i++)
    {
        droneIdNetworkMessageBytes[i] = buff[i];
    }//sizeofDroneIdMessage bytes will be written to sockt o/p stream from droneIdNetworkMessageBytes array
    int wres = write(this->socketFileDescriptor, droneIdNetworkMessageBytes, sizeOfDroneIdMessage);//write to socket stream the bytes in char buffer.
    //cout<<"Sending drone id: Actual bytes written to socket stream: "<<wres<<"Expected bytes to be written: "<<sizeOfDroneIdMessage<<endl;
    //cout<<"Drone with ID: "<<this->droneID<<" connected to control server socket endpoint: "<<dronecloudapp_ip<<": "<<dronecloudapp_control_port<<endl; 

 }