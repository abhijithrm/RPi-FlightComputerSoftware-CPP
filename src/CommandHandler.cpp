#include<CommandHandler.h>
#include<Engine.h>
using namespace std;
CommandHandler::CommandHandler (Drone* drone)
{
this->logger = drone->pLogger;
this->mavlink = drone->mavlinkConnectionObject;
this->vehicle = drone->system;
this->droneReference = drone;
this->telemetryInstance = drone->telemetryData;
this->mavsdkActionPluginObject = new Action(this->vehicle);
//this->lightState = GPIO.HIGH;
//this->ignitorState = GPIO.HIGH;

this->cameraAngle = 140;
//this->cameraGimbalServo = new ServoController(this->cameraAngle);
//this->cameraGimbalServo->start();

this->speedX = 0;
this->speedY = 0;
this->speedZ = 0;
this->speedIncrementValueX = 0.5;
this->speedIncrementValueY = 0.5;
this->speedIncrementValueZ = 0.5;
this->rotationAngle = 10;
this->engine = new Engine(drone, this, this->logger);
this->engine->start();
this->logger->info("DRONE:ENGINE: Engine started.");
 //this->armAndTakeoff(8);//test code

////////test code//////

   /* this->armAndTakeoff(8);//test code

    std::cout<<"Increasing speed..."<<endl;
        this->increaseSpeedX();
    std::this_thread::sleep_for(std::chrono::seconds(15));

    cout<<"Decreasing speed..."<<endl;
    this->decreaseSpeedX();

    cout<<"Increase speed left Y..."<<endl;
    this->leftSpeedY();
    std::this_thread::sleep_for(std::chrono::seconds(15));
         this->rightSpeedY();

   


    
    this->decreaseSpeedX();
    std::this_thread::sleep_for(std::chrono::seconds(15));
    this->increaseSpeedX();

     this->rightSpeedY();
     std::this_thread::sleep_for(std::chrono::seconds(15));
         this->leftSpeedY();





for(int i=0; i<50;i++)
{
cout<<"Rotating ccw"<<endl;
this->rotateLeft(360);

cout<<"Rotating cw..."<<endl;
this->rotateRight(360);

}

this->stopMovement();
cout<<"Stopping drone"<<endl;


this->goHome(12);
cout<<"going home"<<endl;*/

////////test code//////
}

 CommandHandler::~CommandHandler()
{
    this->engine->~Engine();
    delete this;
}

void CommandHandler::stopMovement()
{
    this->speedX = 0;
    this->speedY = 0;
    this->speedZ = 0;
    this->engine->executeChangesNow();
    return;
}

void CommandHandler::rotateLeft(int angle)
{
this->engine->rotate(-1, angle);
}

void CommandHandler::rotateRight(int angle)
{
    this->engine->rotate(1, angle);
}

void CommandHandler::increaseSpeedX()
{
    this->speedX = this->speedX + this->speedIncrementValueX;
    this->engine->executeChangesNow();
}

void CommandHandler::decreaseSpeedX()
{
    this->speedX = this->speedX - this->speedIncrementValueX;
    this->engine->executeChangesNow();
}

void CommandHandler::leftSpeedY()
{
    this->speedY = this->speedY - this->speedIncrementValueY;
    this->engine->executeChangesNow();
}

void CommandHandler::rightSpeedY()
{
    this->speedY = this->speedY + this->speedIncrementValueY;
    this->engine->executeChangesNow();
}

void CommandHandler::stopSpeedXY()
{
    this->speedX = 0;
    this->speedY = 0;
    this->engine->executeChangesNow();
}

void CommandHandler::increaseSpeedZ()//note that we are actualling decrementing z axis speed
{
    this->speedZ = this->speedZ - this->speedIncrementValueZ;
    this->engine->executeChangesNow();
}

void CommandHandler::decreaseSpeedZ()
{
    this->speedZ = this->speedZ + this->speedIncrementValueZ;
    this->engine->executeChangesNow();

}

void CommandHandler::stopSpeedZ()
{
    this->speedZ = 0;
    this->engine->executeChangesNow();
}

void CommandHandler::killMotorsNow()
{
    this->engine->killMotorsNow();
}

void CommandHandler::armAndTakeoff(int takeOffAlt)
{
    this->engine->stopEngineConstantSpeedThread();//stop engine speed thread.
    this->engine->offboardObjectForVelocityControl->stop();//stop offboard velocity control mode

    this->logger->info("DRONE : ARMING");

    //ARMING
    mavsdk::Action::Result armStatus = this->mavsdkActionPluginObject->arm();//returns success if arming drone successfull
    if(armStatus != mavsdk::Action::Result::Success)
    {
        this->logger->alarm("DRONE: Arming failed.");
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    while(armStatus != mavsdk::Action::Result::Success)
    {
        this->logger->info("DRONE: Retrying arming vehicle.");
        armStatus = this->mavsdkActionPluginObject->arm();//returns success if arming drone successfull
        std::this_thread::sleep_for(std::chrono::seconds(1));

    }
    this->logger->alarm("DRONE: Successfully Armed! Please stay a safe distance away from the vehicle.");

    //TAKEOFF..
    this->logger->info("DRONE: Preparing for takeoff");
    //SET TAKEOFF ALT
    mavsdk::Action::Result setTakeoffAlt = this->mavsdkActionPluginObject->set_takeoff_altitude((float)takeOffAlt);
    if(setTakeoffAlt != mavsdk::Action::Result::Success)
    {
        this->logger->error("DRONE: Setting takeoff alt failed.");
    }
    this->logger->info("DRONE: Takeoff altitude set.");

    mavsdk::Action::Result droneTakeOff = this->mavsdkActionPluginObject->takeoff();
    if(droneTakeOff != mavsdk::Action::Result::Success)
    {
        this->logger->error("DRONE: Vehicle takeoff operation failed.");
    }
    this->logger->info("DRONE: Vehicle takeoff operation successfull.");

    while(true)
    {
        Telemetry::Position position = this->droneReference->telemetryData->position();
            if(position.relative_altitude_m >= (float)(takeOffAlt*.95))
            {
                this->logger->info("DRONE: Altitude reached");
                //commanding movement to the same location to unlock Yaw
                mavsdk::Action::Result gotoLocation = this->mavsdkActionPluginObject->goto_location(position.latitude_deg, position.longitude_deg, position.absolute_altitude_m, 0);
                break;
            }
        std::this_thread::sleep_for(std::chrono::seconds(1));

    }
    this->engine->start();//start engine speed thread
    return;
}

void CommandHandler::goHome(float returnToLaunchAltitude)
{

    this->logger->info("DRONE: COMMAND HANDLER: Shutting Engine thread down and returning home.");

    this->engine->stopEngineConstantSpeedThread();

    for(int i=0; i<10; i++)
    {
        this->increaseSpeedZ();//icreasing z axis speed 10 times
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    while(true)
    {
        Telemetry::Position position = this->droneReference->telemetryData->position();
        //set rtl altitude
        mavsdk::Action::Result setReturnToLaunchAlt = this->mavsdkActionPluginObject->set_return_to_launch_altitude(returnToLaunchAltitude);
        if(setReturnToLaunchAlt !=  mavsdk::Action::Result::Success)
        {
            this->logger->error("DRONE: Failed to set RTL aLtitude");
        }

        //execute rtl mavsdk api
        mavsdk::Action::Result returnToLaunch = this->mavsdkActionPluginObject->return_to_launch();
        if(returnToLaunch !=  mavsdk::Action::Result::Success)
        {
            this->logger->error("Drone: Return to Launch operation failed.");
        }
        while(this->telemetryInstance->in_air())
        {
            this->logger->info("DRONE: LANDING IN PROGRESS: Vehicle still in air. On return to launch location...");
            std::this_thread::sleep_for(std::chrono::seconds(5));

        }
        this->logger->info("DRONE: LANDED: Vehicle reached launch location and landed successfully");
        break;
    }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        this->engine->start();
        return;
}

void CommandHandler::toggleLights()
{
    //todo
    return;
}

void CommandHandler::cameraUP()
{
    //todo
    return;
}

void CommandHandler::cameraDown()
{
    //todo
    return;
}

void CommandHandler::cancelMission()
{
return;
}

void CommandHandler::activateMission(MissionData missionPointsData)
{
return;
}

void CommandHandler::land()
{
    this->logger->info("DRONE: Landing...");
    this->engine->stopEngineConstantSpeedThread();
    this->engine->offboardObjectForVelocityControl->stop();

    mavsdk::Action::Result land = this->mavsdkActionPluginObject->land();
    if(land != mavsdk::Action::Result::Success)
    {
        this->logger->error("DRONE: Land operation failed...");
    }
    else if(land == mavsdk::Action::Result::Success)
    this->logger->error("DRONE: Land command sent successfully...");
    while(this->telemetryInstance->in_air())
    {
     this->logger->info("DRONE: LANDING PROGRESS: In air.");
    }
    this->engine->start();
}
