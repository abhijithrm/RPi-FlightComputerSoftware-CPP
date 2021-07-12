#include <Drone.h>
#include <CommandHandler.h>

Drone::Drone(int droneid, bool usesimulator, const char* linuxDeviceSerialPort, int simport, int takeoffalt, int returnaltitude, CPlusPlusLogging::Logger* pLoggerInstance)
    {      
        cout<<droneid<<usesimulator<<linuxDeviceSerialPort<<simport<<takeoffalt<<returnaltitude<<endl;

        this->droneId = droneid;//drone identifier
        this->useSimulator = usesimulator;//use simulator or connect to flight controller
        this->linuxDevicePort = linuxDeviceSerialPort;//COM/UART port for serial comm b/w Pi and flight ctrlr
        this->simulatorPort = simport;//simulator udp port
        this->takeoffAltitude = takeoffalt;//default alt on takeoff
        this->returnAltitude = returnaltitude;//alt from ground
        this->pLogger = pLoggerInstance;
        this->mavlinkConnectionObject = new Mavsdk();//This is the main class of MAVSDK (a MAVLink API Library). Used to discover vehicles and manage active connections.
        this->droneData = new DroneData();//protobuf object
        this->state = "DISARMED";
        this->isActive = true;
        
        //*****************Connect to simulator through udp or flight controller through serial port*************//
        if(this->useSimulator)
        {
         string raspPiIp = this->getRaspPiIP();//get the ethernet ip address of rasp pi
         cout<<raspPiIp<<endl;
         ConnectionResult result = mavlinkConnectionObject->add_any_connection("udp://:14540");
         //debugging
       
         cout<<mavlinkConnectionObject->is_connected()<<endl;
         cout<<mavlinkConnectionObject->systems().size()<<endl;
         //cout<<mavlinkConnectionObject->system_uuids().at(0)<<endl;
         //debugging

          if (result != ConnectionResult::Success) 
          {
              pLogger->error("MAVLINK connection to simulator through specified UDP port and IP failed. See below for local ip info:");
              pLogger->info(raspPiIp.c_str());
          }
          else if(result == ConnectionResult::Success)
          pLogger->info("MAVLINK: Connected to simulator via UDP port.");

        }
        else
        {
        ConnectionResult result = mavlinkConnectionObject->add_serial_connection(string(linuxDevicePort), mavlinkConnectionObject->DEFAULT_SERIAL_BAUDRATE, false);
         if(result != ConnectionResult::Success)
          pLogger->error("MAVLINK connection attempt to Flight Controller via serial port failed. Please troubleshoot the issue to enable communication with flight controller.");
          
          pLogger->info("MAVLINK: Connected to Flight Controller device via serial port.");
        }
      //*****************Connect to simulator through udp port or flight controller through serial port*************//

        //*******************Get mavlink system object**********************//
         try
        {
           // this->GetMavlinkSystemObjectForTheVehicle(mavlinkConnectionObject);//subscribe to connect to new systems available
            int count =0;
             while(!(mavlinkConnectionObject->systems().size()>0))
            {

                if(count ==0)
                {
                pLogger->alarm("DRONE:MAVLINK: Connecting to vehicle and obtaining system object...");
                count++;
                }
                
            }
            // Wait for the system to connect via heartbeat
            //this->WaitUntilDiscoverSystem(this->mavlinkConnectionObject);
            this->system = mavlinkConnectionObject->systems().at(0);
           
            //debugging
            cout<<"Mavlink System ID of connected system: "<<this->system->get_system_id()<<endl;
            cout<<"UUID of the connected system: "<<this->system->get_uuid()<<endl;
            cout<<"Connection status: "<<this->system->is_connected()<<endl;
            //Info *info = new Info(this->system);

            if((this->system->is_connected()))
            pLogger->info("DRONE:MAVLINK: MAVSDK System object for the drone obtained and connection established with the drone.");

        }
        catch(const std::exception& e)
        {
            this->pLogger->error(e.what());
            this->pLogger->info("MAVLINK: No Vehicle/Flight Controller device found. Failed to connect and obtain MAVSDK System object for the drone.");
        }
      //*******************Get mavlink system object**********************//

      this->telemetryData = new Telemetry(this->system);//initialize Telemetry object for vehicle status update messages.
      //Since all Drone object properties are initialized and connection to FC through mavlink enabled, create CommandHandler obj and pass the pointer to this instance
      
        while (!telemetryData->health_all_ok()) {
            this->pLogger->info("Waiting for system to be ready");
            this_thread::sleep_for(chrono::seconds(1));
        }

      this->controltab = new CommandHandler(this);
      
      this->pLogger->info("DRONE: Successfully initiated drone and control objects");

    }

   void Drone::WaitUntilDiscoverSystem(Mavsdk *mavsdk)
{
   
    std::cout << "Waiting to discover system..." << std::endl;
    std::promise<void> discover_promise;
    auto discover_future = discover_promise.get_future();

    mavsdk->subscribe_on_new_system([mavsdk, &discover_promise]() {
        const auto system = mavsdk->systems().at(0);

        if (system->is_connected()) {
            std::cout << "Discovered system" << std::endl;
            discover_promise.set_value();
        }
    });

    discover_future.wait();
}

//Fetches the telemtry data from flight contrller and store data in DroneData protobuf object, serialize it and return size of resulting byte array
int Drone::getDroneDataSerialized(int videoPort, unsigned char* droneDataByteArray)
{
    this->droneData->set_drone_id(std::to_string(this->droneId));

    Telemetry::Position position= telemetryData->position();//get a single Position structure containing position data
    if(position.relative_altitude_m!=NULL)
    {
     float alt;
     if(position.relative_altitude_m<0)
     alt = 0;
     else
     alt = position.relative_altitude_m;

     droneData->set_altitude(alt);
    }
    if(position.latitude_deg != NULL)
    droneData->set_latitude(position.latitude_deg);
    if(position.longitude_deg != NULL)
    droneData->set_longitude(position.longitude_deg);
    
    Telemetry::Battery batteryData = telemetryData->battery();
    if(batteryData.remaining_percent != NULL)
    droneData->set_voltage(batteryData.remaining_percent);

    Telemetry::VelocityNed velocityData= telemetryData->velocity_ned();
    if(velocityData.north_m_s != NULL)
    droneData->set_speed(velocityData.north_m_s);

    droneData->set_state(string(this->state));

    droneData->set_video_port(videoPort);
    
    //debug code
    cout<<"Drone data protobuf obj set values|DEBUG "<<"vol: "<<droneData->voltage()<<"alt: "<<droneData->altitude()<<"lat: "<<droneData->latitude()<<"lon: "<<droneData->longitude()<<"speed: "<<droneData->speed()<<"state: "<<droneData->state()<<"drn id: "<<droneData->drone_id()<<endl;
    //string droneDataSerialized;
    int bsize = droneData->ByteSize();//get the byte size of protobuf drone data object
    
    unsigned char * tempByteArray =  new unsigned char[bsize];
    bool serializationStatus = droneData->SerializeToArray(tempByteArray, bsize);//serialize to byte array
    cout<<"Drone data serialization status: "<<serializationStatus<<endl;
    
    for(int i =0; i<bsize; i++)
    droneDataByteArray[i] = tempByteArray[i];

    cout<<"Serialized dronedata proto obj:---> "<<droneDataByteArray<<endl;
    return bsize;

}


//need to verify in raspi
string Drone::getRaspPiIP() 
{
           string raspIP;
           struct ifaddrs *ifaddr;
           int family, s;
           char host[NI_MAXHOST];

           if (getifaddrs(&ifaddr) == -1) {
               perror("getifaddrs");
               exit(EXIT_FAILURE);
           }

           /* Walk through linked list, maintaining head pointer so we
              can free list later. */

           for (struct ifaddrs *ifa = ifaddr; ifa != NULL;
                    ifa = ifa->ifa_next) {
               if (ifa->ifa_addr == NULL)
                   continue;
                 
               family = ifa->ifa_addr->sa_family;

               /* For an AF_INET* interface address, display the address. */
                string str1="enp0s25";
                string str2 = "eth0";
                string interfacename =string(ifa->ifa_name);

               if (family == AF_INET && ((str1.compare(interfacename)==0) || (str2.compare(interfacename))==0)) {
                   s = getnameinfo(ifa->ifa_addr,
                            sizeof(struct sockaddr_in),               
                           host, NI_MAXHOST,
                           NULL, 0, NI_NUMERICHOST);
                   if (s != 0) {
                       printf("getnameinfo() failed: %s\n", gai_strerror(s));
                       exit(EXIT_FAILURE);
                   }
                 raspIP= string(host);
                  //cout<<host1<<endl;
               } 
           }
                    
           freeifaddrs(ifaddr);
           return raspIP;        
}

void Drone::freeze()//freeze in air
{
    this->pLogger->info("DRONE: Freezing the drone at current position.");
    this->controltab->stopMovement();
}

void Drone::returnToBase()
{
    this->controltab->goHome(this->returnAltitude);
}

void Drone::toggleLights()
{
    this->controltab->toggleLights();
}


//call destructor
void Drone::close()
{
this->~Drone();
}

void Drone::executeCommand(CommandData commandData)
{
if(commandData.commandCode == 7)
{
    this->controltab->goHome(this->returnAltitude);
    this->pLogger->info("Executing command: 'Go Home', command code: 7");
    return;
}

if(commandData.commandCode == 8){
    this->toggleLights();
    this->pLogger->info("Executing command : 'Toggle Lights/Drop Package', command code : 8");
    return;
}

if(commandData.commandCode == 9)
{
    this->state = "ARMING";
    this->controltab->armAndTakeoff(this->takeoffAltitude);
    this->state="READY";
    this->pLogger->info("Executing command: 'Arm and TakeOff', command code: 9");
    return;
}

if(commandData.commandCode == 1)
{
    this->controltab->increaseSpeedZ();
    this->pLogger->info("Executing command: 'Increase Vertival Speed', command code: 1");
    return;
}

if(commandData.commandCode == 5)
{
    this->controltab->decreaseSpeedZ();
    this->pLogger->info("Executing command: 'Decrease Vertival Speed', command code: 5");
    return;
}

if(commandData.commandCode == 2)
{
    this->controltab->rotateLeft(10);
    this->pLogger->info("Executing command: 'Rotate Left 10 Deg', command code: 2");
    return;
}
if(commandData.commandCode == 3)
{
    this->controltab->rotateRight(10);
    this->pLogger->info("Executing command: 'Rotate Right 10 Deg', command code: 3");
    return;
}

if(commandData.commandCode == 18)
{
    this->controltab->rotateLeft(45);
    this->pLogger->info("Executing command: 'Rotate Left 45 Deg', command code:18");
    return;
}

if(commandData.commandCode == 19)
{
    this->controltab->rotateLeft(90);
    this->pLogger->info("Executing command: 'Rotate Left 90 Deg', command code: 19");
    return;
}

if(commandData.commandCode == 20)
{
    this->controltab->rotateRight(45);
    this->pLogger->info("Executing command: 'Rotate Right 45 Deg', command code: 20");
    return;
}

if(commandData.commandCode == 21)
{
    this->controltab->rotateRight(90);
    this->pLogger->info("Executing command: 'Rotate Right 90 Deg', command code: 21");
    return;
}

if(commandData.commandCode == 22)
{
    this->controltab->cameraUP();
    this->pLogger->info("Executing command: 'Move Camera Up', command code: 22");
    return;
}

if(commandData.commandCode == 23)
{
    this->controltab->cameraDown();
    this->pLogger->info("Executing command: 'Move Camera Down', command code: 23");
    return;
}

if(commandData.commandCode == 10)
{
    this->controltab->land();
    this->pLogger->info("Executing command: 'Landing', command code: 10");
    return;
}
if(commandData.commandCode == 11)
{
    this->controltab->increaseSpeedX();
    this->pLogger->info("Executing command: 'Increase Speed X', command code: 11");
    return;
}

if(commandData.commandCode == 4)
{
    this->controltab->decreaseSpeedX();
    this->pLogger->info("Executing command: 'Decrease Speed X', command code: 4");
    return;
}

if(commandData.commandCode == 16)
{
    this->controltab->rightSpeedY();
    this->pLogger->info("Executing command: 'Increase Speed Y', command code: 16");
    return;
}

if(commandData.commandCode == 15)
{
    this->controltab->leftSpeedY();
    this->pLogger->info("Executing command: 'Decrease Speed Y', command code: 15");
    return;
}

if(commandData.commandCode == 12)
{
    this->controltab->stopSpeedXY();
    this->pLogger->info("Executing command: 'Stop Horizontal Movement', command code: 12");
    return;
}

if(commandData.commandCode == 13)
{
    this->controltab->stopSpeedZ();
    this->pLogger->info("Executing command: 'Stop Vertical Movement', command code: 13");
    return;
}

if(commandData.commandCode == 14)
{
    this->state = "ON MISSION";
    this->controltab->activateMission(commandData.data);
    this->pLogger->info("Executing command: 'Activate Mission', command code: 14");
    return;
}

if(commandData.commandCode == 6)
{
    this->state = "MISSION CANCEL";
    this->controltab->cancelMission();
    this->pLogger->info("Executing command: 'Cancel Mission', command code: 6");
    this->freeze();
    return;
}

if(commandData.commandCode == 17)
{
    this->state = "MOTORS KILL";
    this->controltab->killMotorsNow();
    this->pLogger->info("Executing command: 'Emergency Motor Kill', command code: 17");
    this->isActive = false;
    return;
}


}

Drone::~Drone()
{
    delete this->mavlinkConnectionObject;
    delete this->droneData;
    delete this->telemetryData;
    delete this->controltab;
    delete this;

}