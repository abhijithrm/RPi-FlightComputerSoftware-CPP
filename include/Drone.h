//TODO: VERIFY rASPPI iP, 
#ifndef DRONE_H
#define DRONE_H

#include <ProtoData.pb.h>
#include <Logger.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <string.h>
#include <numeric>
#include <iostream>
#include <chrono>
#include <mavsdk.h>
#include <telemetry.h>
#include <vector>
#include <thread>
#include <future>
#include <CommandData.h>
//#include <wiringPi.h>
#include<action.h>
#include<offboard.h>
#include <mavsdk/plugins/info/info.h>

//int DROP_PACKAGE_PIN = 21;
//int IGNITE_LED_PIN = 2;
using namespace mavsdk;
//TODO: GPIO CODE, cancel, activate missions, camera control,dtor
using namespace protobuf_ProtoData_2eproto;      
using namespace std;
using namespace mavsdk;
using namespace CPlusPlusLogging;


class CommandHandler;

class Drone
{

    private:
    int droneId;
    bool useSimulator;
    const char * linuxDevicePort;
    int simulatorPort;
    int takeoffAltitude;
    int returnAltitude;
    char * state;
    string hostServerIP;

    public:
    Mavsdk *mavlinkConnectionObject;
    shared_ptr<System> system;//Mavlink system object
    DroneData *droneData;
    Telemetry *telemetryData;
    bool isActive;
    CommandHandler *controltab;
    Logger* pLogger;
    

    Drone(int droneid, bool usesimulator, const char* linuxDeviceSerialPort, int simport, int takeoffalt, int returnaltitude, Logger* pLoggerInstance, const char * hostip);
   // void GetMavlinkSystemObjectForTheVehicle(Mavsdk *mavsdk);
    /*Method takes video udp port and a buffer array as arguments. It makes use of MAVSDK Telemtery 
    object APIs to fetch realtime telemetry data of the drone from flight controller and stores it 
    in DroneData protobuf object, serializes object into a byte array and stores it in the buffer 
    array passed as argument. */
    int getDroneDataSerialized(int videoPort, unsigned char* byteArray);//serialize drone data protobuf obj to byte array and returns the size of the byte array
    void WaitUntilDiscoverSystem(Mavsdk *mavsdk);
    string getRaspPiIP(); 
    void freeze();//freeze in air
    void returnToBase();
    void toggleLights();
    void executeCommand(CommandData commandData);
    //call destructor
    void close();
    ~Drone();

};

#endif