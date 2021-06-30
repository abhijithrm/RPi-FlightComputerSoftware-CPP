#include<CommandHandler.h>
#include<Engine.h>

CommandHandler::CommandHandler (Drone* drone)
{
cout<<"commhand block1"<<endl;
this->logger = drone->pLogger;
 cout<<"commhand block2"<<endl;
this->mavlink = drone->mavlinkConnectionObject;
 cout<<"commhand block3"<<endl;
this->vehicle = drone->system;
 cout<<"commhand block4"<<endl;
this->droneReference = drone;
  cout<<drone->system->is_connected()<<endl;
 cout<<"commhand block5"<<endl;
    cout<<this->vehicle->is_connected()<<endl;
this->mavsdkActionPluginObject = new Action(this->vehicle);
 cout<<"commhand block6"<<endl;
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
 cout<<"commhand block7"<<endl;
this->engine = new Engine(drone, this, this->logger);
 cout<<"commhand block8"<<endl;
this->engine->start();


}

 CommandHandler::~CommandHandler()
{//todo
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
    return;
}

void CommandHandler::goHome(float returnToLaunchAltitude)
{
    this->logger->info("DRONE: Returning home.");

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
            this->logger->info("DRONE: Vehicle still in air. On return to launch location...");
        }
        break;
    }
        std::this_thread::sleep_for(std::chrono::seconds(1));
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
    mavsdk::Action::Result land = this->mavsdkActionPluginObject->land();
    if(land != mavsdk::Action::Result::Success)
    {
        this->logger->error("DRONE: Land operation failed...");
    }
}
