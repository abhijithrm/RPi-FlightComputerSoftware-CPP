#include <DataReceiver.h>

//ctor
DataReceiver:: DataReceiver(int socket, Drone* drn, Logger* logger)
    {
    this->socketFileDescriptor = socket;//socket stream file descriptor...
    this->droneInstance = drn;
    this->isActive = true;
    this->pLogger = logger;
    }

//dtor
DataReceiver::~DataReceiver()
{
   pthread_cancel(dataReceiverThread);
   pLogger->info("DataReciever thread stopped....");
   delete this;

}

//static thread task function wrapper...passed to pthread
void* DataReceiver::staticDataReceiverTask(void *dr)
{
  ((DataReceiver*)dr)->dataReceiverTask();
}

//Class member function holding thread body
void DataReceiver::dataReceiverTask()
{
    while(this->isActive)
    {
        try
        {
            //read from socket input stream data from server..
            unsigned char messageFromServer[1000]; 
            int messageSize = AppUtils::readNetworkMessage(this->socketFileDescriptor, messageFromServer);
            ///unsigned char receievedMessageByteArray[messageSize];
            unsigned char receievedMessageByteArray[messageSize];
            for(int i =0; i<messageSize; i++)
            {
             receievedMessageByteArray[i] = messageFromServer[i];
            }
           
            Command command = Command();//ptotobuf command object according to prototype specified in .proto file
            command.ParseFromArray(receievedMessageByteArray, messageSize);//parse byte array to protobuf command object

            CommandData commandData = CommandData();//command data dto
            commandData.commandCode = command.code();
            cout<<"Incoming deciphered command code is: "<<commandData.commandCode<<endl;

            if(command.code() == 14)//if code is 14, then payload of Command object will be list of datapoints(mission data) objects from server
            {
                MissionData missionData = MissionData();
                missionData.ParseFromString(command.payload());
                commandData.data = missionData;
            }
            this->droneInstance->executeCommand(commandData);
            
        }
        catch(const std::exception& e)
        {
            pLogger->info("DataReceiver error. See below for more info :");
            pLogger->error(e.what());
        }
    }
}

//create and start thread
void DataReceiver::start()
{
      int rc;
      pLogger->info("Starting Data Receiver thread...");
      rc = pthread_create(&dataReceiverThread, NULL, staticDataReceiverTask, this);
      if (rc) 
      {
        pLogger->info("Failed to start Data Receiver thread.");
        exit(-1);
      }

}

void DataReceiver::stop()
{
    this->isActive = false;
    pthread_cancel(dataReceiverThread);
}