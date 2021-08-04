#include <sstream>
#include <config4cpp/Configuration.h>
#include<stdlib.h>
#include <memory>
#include <stdexcept>
#include <array>
#include <vector>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ConnectionMonitor.h>
#include <DataReceiver.h>
#include <unistd.h>
#include<LogSenderSocket.h>

#define SIGKILL 9
#define SA struct sockaddr

using namespace std;
using namespace config4cpp;
using namespace CPlusPlusLogging;




void tokenize(std::string const &str, const char* delim, std::vector<std::string> &out) 
{ 
    char *token = strtok(const_cast<char*>(str.c_str()), delim); 
    while (token != nullptr) 
    { 
        out.push_back(std::string(token)); 
        token = strtok(nullptr, delim); 
    } 
}  
   
void killProcesses(const char* cmd) 
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    const char* delim = " "; 
    std::vector<std::string> out; 
    tokenize(result, delim, out); 
 
 for(int i=0; i<out.size(); i++)
 {
        try
        {
            int pid = atoi(out[i].c_str());
            int killStatus = kill(pid, SIGKILL); 
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            cout<<" Killing process "<<out[i]<<" failed"<<endl;
        }
 }
//cout<<out[1]<<endl;
return;
}

    
int main(int argc, char * argv[])
{   
    //*************************Initialize Logger class instance for Logging*********************//
    // Log message C++ Interface
    Logger* pLogger = NULL; // Create the object pointer for Logger Class
    pLogger = Logger::getInstance();
    //*************************Initialize Logger class instance for Logging*********************//

pLogger->always("Drone RaspPi C++ application started....");
    
    //*************************Get the directory path of the executable(app)********************//
    
    char * str = argv[0];
    char str2[strlen(argv[0])-2];
    int i;
    for(i=0;i<strlen(argv[0])-3;i++)
    {
        str2[i]=str[i];
    }
    str2[i]='\0';
    string appDirectory = str2;
cout<<appDirectory<<(appDirectory+ "configuration.cfg").c_str()<<endl;
  
    //*************************Get the directory path of the executable(app)********************//



/*Logging Reference:
pLogger->error("Message Logged using C++ Interface, Log level: LOG_ERROR");
pLogger->alarm("Message Logged using C++ Interface, Log level: LOG_ALARM");
pLogger->always("Message Logged using C++ Interface, Log level: LOG_ALWAYS");
pLogger->buffer("Message Logged using C++ Interface, Log level: LOG_INFO");
pLogger->info("Message Logged using C++ Interface, Log level: LOG_BUFFER");
pLogger->trace("Message Logged using C++ Interface, Log level: LOG_TRACE");
pLogger->debug("Message Logged using C++ Interface, Log level: LOG_DEBUG");*/


//*************************Initialize config parsing from configuration.cfg file*******************************//
 string configurationFileFullName = appDirectory+ "configuration.cfg";
 Configuration *  cfg = Configuration::create();
    const char *     scope = "";
    const char *     configFile = configurationFileFullName.c_str();//Get const char* of std::string
    int    drone_id;
    const char *     drone_linux_device;
    int              drone_rtl_alt;
    int              drone_takeoff_alt;
    bool             drone_use_simulator ;
    int              drone_simulator_port;
    const char *     dronecloudapp_ip;
    int              dronecloudapp_control_port;
    int              dronecloudapp_video_port;
    int              dronecloudapp_max_reconnection_attempts;
    const char *     video_fps ;
    const char *     video_quality ;
    const char *     video_width ;
    bool             video_grayscale ;
    const char *     video_height ;

    try {
        cfg->parse(configFile);
        drone_id        = cfg->lookupInt(scope, "drone_id");
        drone_linux_device        = cfg->lookupString(scope, "drone_linux_device");
        drone_rtl_alt       = cfg->lookupInt(scope, "drone_rtl_alt");
        drone_takeoff_alt = cfg->lookupInt(scope, "drone_takeoff_alt");
        drone_use_simulator       = cfg->lookupBoolean(scope, "drone_use_simulator");
        drone_simulator_port = cfg->lookupInt(scope, "drone_simulator_port");
        dronecloudapp_ip       = cfg->lookupString(scope, "dronecloudapp_ip");
        dronecloudapp_control_port = cfg->lookupInt(scope, "dronecloudapp_control_port");
        dronecloudapp_video_port       = cfg->lookupInt(scope, "dronecloudapp_video_port");
        dronecloudapp_max_reconnection_attempts = cfg->lookupInt(scope, "dronecloudapp_max_reconnection_attempts");
        video_fps       = cfg->lookupString(scope, "video_fps");
        video_quality = cfg->lookupString(scope, "video_quality");
        video_width       = cfg->lookupString(scope, "video_width");
        video_grayscale = cfg->lookupBoolean(scope, "video_grayscale");
        video_height       = cfg->lookupString(scope, "video_height");
    
    } catch(const ConfigurationException & ex) {
        cerr << ex.c_str() << endl;
        cfg->destroy();
        return 1;
    }
    //debugger
    cout << "drone_linux_device=" << drone_linux_device << "; video_grayscale=" << video_grayscale
        << "; video_height=" << video_height<<drone_id<<dronecloudapp_ip
         << endl;
//*************************Initialize config parsing from configuration.cfg file*******************************//

cout<<drone_id<<endl;
if(drone_id != 1)//ensure whether config file was properly parsed and values read by checking drone id value
{
    pLogger->error("Unable to succesfully read configuration file. See below INFO message for fullname of the configuration file.");
    pLogger->info(configurationFileFullName.c_str());
    exit(0);
}   

Drone *drone;
while(true)
{
    try
    {    
      //remember to delete the pointer
      drone = new Drone(drone_id, drone_use_simulator, drone_linux_device, drone_simulator_port, drone_takeoff_alt, drone_rtl_alt, pLogger, dronecloudapp_ip);
      break;
    
    }
    catch(const std::exception& e)
    {
        pLogger->error(e.what());

        std::cerr << e.what() << '\n';
        this_thread::sleep_for(chrono::seconds(2));
    }
}    



ConnectionMonitor *watchdog = new ConnectionMonitor(drone, dronecloudapp_ip, dronecloudapp_max_reconnection_attempts, pLogger);//remember to delete the pointer
watchdog->start();//internet connection monitor. Separate daemon thread.


int videoStreamerProcessReturnStatus;
int sockfd, connfd;
struct sockaddr_in servaddr, cli;
DataReceiver* serverMessageReciever = nullptr;
LogSender *realTimeLogStreamer = nullptr;

while(drone->isActive)
{
    try
    {
        while(!watchdog->netStatus)
        {
            this_thread::sleep_for(chrono::seconds(1));//run the loop until connected to internet
        }

        this_thread::sleep_for(chrono::seconds(3));

//****************Creating client socket and connecting to host server******************//
    
    
        // socket create and verification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) 
        {
            pLogger->info("[APP] [MAIN THREAD]: Client socket creation failed...");
            exit(0);
        }
        else
            pLogger->info("[APP] [MAIN THREAD]: Client socket successfully created..");
            bzero(&servaddr, sizeof(servaddr));
    
        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(dronecloudapp_ip);
        servaddr.sin_port = htons(dronecloudapp_control_port);
    
        // connect the client socket to server socket
        int animCounter = 0;
        while(connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0)//RETURNS 0 IF SUCCESS
        {
            string sout = "[APP] [MAIN THREAD] [CONTROL SOCKET]: Attempting TCP socket connection with server. Server IP: "+ string(dronecloudapp_ip) + ". Port: " + std::to_string(dronecloudapp_control_port);
            animCounter = AppUtils::waitingConsoleAnimation(animCounter, sout, pLogger);
            this_thread::sleep_for(chrono::seconds(1));
        }
        string sout = "[APP] [MAIN THREAD]: TCP socket connection accepeted by server with IP: "+ string(dronecloudapp_ip) + " at port: " + std::to_string(dronecloudapp_control_port);
        pLogger->info(sout);
        
        //write drone id to socket o/p stream.
        unsigned char buff[10];
        bzero(buff, sizeof(buff));//fill zeros
        string droneid = to_string(drone_id);
        size_t sizeOfDroneIdMessage  = AppUtils::createNetworkMessage(droneid, buff);//drone id to bytes
        unsigned char droneIdNetworkMessageBytes[sizeOfDroneIdMessage];
        for(int i=0; i<sizeOfDroneIdMessage; i++)
        {
         droneIdNetworkMessageBytes[i] = buff[i];
        }//sizeofDroneIdMessage bytes will be written to sockt o/p stream from droneIdNetworkMessageBytes array
        int wres = write(sockfd, droneIdNetworkMessageBytes, sizeOfDroneIdMessage);//write to socket stream the bytes in char buffer.
        //cout<<"Sending drone id: Actual bytes written to socket stream: "<<wres<<"Expected bytes to be written: "<<sizeOfDroneIdMessage<<endl;
        cout<<"Drone with ID: "<<drone_id<<" connected to control server socket endpoint: "<<dronecloudapp_ip<<": "<<dronecloudapp_control_port<<endl; 
       
       //****************Creating client socket and connecting to host server******************//
        
        //*******************Start video streaming*********************//
         /*try
         {  //separte process for video streaming
            string command = "/usr/bin/python3 "+appDirectory+"video_streamer.py";//cmd line command for starting video streamer python process
            videoStreamerProcessReturnStatus = system(command.c_str());
         }
         catch(const std::exception& e)
         {
            std::cerr << e.what() << '\n';
            pLogger->error("[APP] [VIDEO STREAMER]: Failed to start video_streamer python process");
        }*/
        //*******************Start video streaming*********************//
         cout<<"Current size of log queue: "<<pLogger->threadSafeQueue->getCount()<<endl;
         serverMessageReciever = new DataReceiver(sockfd, drone, pLogger);//remember to delete the object at the right time
         serverMessageReciever->start();//separate thread dealing with recieving data from the server.

         realTimeLogStreamer = new LogSender(drone, pLogger, watchdog, dronecloudapp_ip, drone_id);
         realTimeLogStreamer->start();

        //Keep sending drone status(seriliazed DroneData protobuf object) to the ground server through TCP socket connection...
         while(watchdog->netStatus && drone->isActive)
         
         {  
            unsigned char message[100];//temp buffer to store full network message with encode message body length 
            unsigned char droneDataMessageBodyByteArray[100];//temp buffer to store serialized DroneData object
            int byteArraySize = drone->getDroneDataSerialized(dronecloudapp_video_port, droneDataMessageBodyByteArray);
            //cout<<droneDataMessageBodyByteArray[0]<<endl;
            size_t sizeOfMessage = AppUtils::createNetworkMessage(droneDataMessageBodyByteArray, byteArraySize, message);
            unsigned char networkMessageByteArray[sizeOfMessage];//byte array to store network message
            for(int i=0; i<sizeOfMessage; i++)
            {
            networkMessageByteArray[i] = message[i];
            }
            
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
           
            int writeRes =  write(sockfd, networkMessageByteArray, sizeOfMessage);
            if(writeRes == -1)
            {
                pLogger->error("APP: MAIN THREAD: SOCKET: Failed to send drone status data through socket connection. Re-attempting...");
                while(writeRes != -1)
                {
                    writeRes =  write(sockfd, networkMessageByteArray, sizeOfMessage);
                    pLogger->error("APP: MAIN THREAD: SOCKET: Sending drone status data through socket stream failed. Re-attempting.");
                }
            }
            else if (writeRes < sizeOfMessage)
            {
               unsigned char temp[sizeOfMessage-writeRes];
               for(int i=0,j= writeRes; j<sizeOfMessage; i++, j++)
               temp[i] = networkMessageByteArray[j];

               writeRes =  write(sockfd, temp, sizeOfMessage-writeRes);

            }
            //cout<<"Drone status data: Actual bytes written to socket o/p stream: "<< writeRes<<". Expected bytes needed to be sent: "<<sizeOfMessage<<endl;
            this_thread::sleep_for(chrono::seconds(1));
         }
        pLogger->alarm("APP: MAIN THREAD: Closing socket connection and stopping data reciever thread as internet is unavailable or drone is not active.");
        serverMessageReciever->stop();//stop data receiver thread before closing socket, or else we get segmentation fault while the thread tries to write to a closed sock stream
        close(sockfd);
        realTimeLogStreamer->stop();
    }
    catch(const std::exception& e)
    {
        pLogger->error(e.what());
        std::cerr << e.what() << '\n';
        drone->freeze();
        //***********Kill process and close socket********//
           /* try
            {
                killProcesses("pidof video_streamer");
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                pLogger->alarm("Failed killing video_streamer python process or its child processes");
            }
            pLogger->info("Successfully killed video_streamer python process");*/
           
            if(serverMessageReciever!=nullptr)
              serverMessageReciever->stop();
            close(sockfd);
            if(realTimeLogStreamer != nullptr)
             realTimeLogStreamer->stop();
        //***********Kill process and close socket********//

    }  
}
//***********Kill process and close socket********//
    /*try
    {
        killProcesses("pidof video_streamer");
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        pLogger->alarm("Failed killing video_streamer python process or its child processes");
    }
    pLogger->info("Successfully killed video_streamer python process");*/
    if(serverMessageReciever!=nullptr)
      serverMessageReciever->stop();
    close(sockfd);
    if(realTimeLogStreamer != nullptr)
      realTimeLogStreamer->stop();
   
//***********Kill process and close socket********//


serverMessageReciever->~DataReceiver();
watchdog->~ConnectionMonitor();
drone->close();
cfg->destroy();
realTimeLogStreamer->~LogSender();

return 0;
}


