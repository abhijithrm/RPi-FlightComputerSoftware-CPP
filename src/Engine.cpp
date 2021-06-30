#include<Engine.h>


 Engine::Engine(Drone *drone, CommandHandler * comhandler, Logger *logger )
{
    this->droneReference = drone;
    this->comHandlerReferenceObject = comhandler;
    this->lastMissionCommandIndex = -1;
    this->logger = logger;
    this->offboardObjectForVelocityControl = new Offboard(this->droneReference->system);

    this->start();
    this->logger->info("DRONE:ENGINE: Engine started.");
}

 Engine::~Engine()
{
    pthread_cancel(constantSpeedAndDirectionMaintainingThread);
    this->logger->info("DRONE:ENGINE: Engine constant speed maintaining thread closed.");
    delete offboardObjectForVelocityControl;
    delete this;
}


void Engine::executeChangesNow()
{
       if(!this->offboardObjectForVelocityControl->is_active())
       mavsdk::Offboard::Result offboardControlStart =  this->offboardObjectForVelocityControl->start();
       
       //set the speed values in x,y,z as is in the command handler object speed properties
       mavsdk::Offboard::VelocityNedYaw velocityYawValues ;
       //set values in the structure according to as in the current speed values from Command Handler object.
       velocityYawValues.north_m_s = this->comHandlerReferenceObject->speedX;//set speed in compass north direction
       velocityYawValues.east_m_s = this->comHandlerReferenceObject->speedY;//east
       velocityYawValues.down_m_s = this->comHandlerReferenceObject->speedZ;
       velocityYawValues.yaw_deg = 0;
      
       mavsdk::Offboard::Result setVelocityYaw = this->offboardObjectForVelocityControl->set_velocity_ned(velocityYawValues);

}

void Engine::killMotorsNow()
{  
 mavsdk:Action::Result killEngine = this->comHandlerReferenceObject->mavsdkActionPluginObject->kill();
 if(killEngine != mavsdk::Action::Result::Success)
  this->logger->info("DRONE:ENGINE: Killing engine failed.");
 this->logger->info("DRONE:ENGINE: Engine killed. Drone will fall off sky if in air");
}

//if direction is -1 rotate left and rotate right if 1
void Engine::rotate(int direction, float rotationAngle)
{
    mavsdk::Offboard::Result setYawAngleResult;//success flag
    do
    {
    if(!this->offboardObjectForVelocityControl->is_active())    
    mavsdk::Offboard::Result offboardControlStart =  this->offboardObjectForVelocityControl->start();//start offboard control

    if(direction == 1)
    {
       mavsdk::Offboard::VelocityNedYaw yawValueRight;
       yawValueRight.down_m_s = 0;
       yawValueRight.east_m_s = 0;
       yawValueRight.north_m_s = 0;
       yawValueRight.yaw_deg =  rotationAngle;
     
     
        setYawAngleResult = this->offboardObjectForVelocityControl->set_velocity_ned(yawValueRight);//set yaw angle. Returns Success if command sent successfully
    }
    else if(direction == -1)//left
    {
        mavsdk::Offboard::VelocityNedYaw yawValueLeft;
        //set the values for the structure members
        yawValueLeft.down_m_s = 0;
        yawValueLeft.east_m_s=0;
        yawValueLeft.north_m_s = 0;
        yawValueLeft.yaw_deg =  360-rotationAngle;
        setYawAngleResult = this->offboardObjectForVelocityControl->set_velocity_ned( yawValueLeft);
    }

    }while(setYawAngleResult != mavsdk::Offboard::Result::Success); 

    if(this->offboardObjectForVelocityControl->is_active())
    mavsdk::Offboard::Result stopOffboardControl = this->offboardObjectForVelocityControl->stop();//stop offboard control mode

}

 void* Engine::staticConstantSpeedAndDirectionThreadTask(void *eng)
{
  ((Engine*)eng)->constantSpeedAndDirectionMaintainingThreadTask();
}

//Class member function holding thread body
void Engine::constantSpeedAndDirectionMaintainingThreadTask()
{
    while(true)
    {
        try
        {
            //todo- any mission related condition checks, change drone state to MISSION OVER etc
           if(this->comHandlerReferenceObject->speedX !=0 || this->comHandlerReferenceObject->speedY !=0 || this->comHandlerReferenceObject->speedZ !=0)
           {
               //initiate offboard control
              if(!this->offboardObjectForVelocityControl->is_active()) 
              mavsdk::Offboard::Result offboardControlStart =  this->offboardObjectForVelocityControl->start();
             //set the speed values in x,y,z as is in the command handler object speed properties
              mavsdk::Offboard::VelocityNedYaw velocityYawValues;
              velocityYawValues.north_m_s = this->comHandlerReferenceObject->speedX;
              velocityYawValues.east_m_s = this->comHandlerReferenceObject->speedY;
              velocityYawValues.down_m_s =  this->comHandlerReferenceObject->speedZ;
              velocityYawValues.yaw_deg = 0;//no change in yaw angle..no rotation
                  
              mavsdk::Offboard::Result setVelocityYaw = this->offboardObjectForVelocityControl->set_velocity_ned(velocityYawValues);//send values to flight controller
              this_thread::sleep_for(chrono::milliseconds(1500));

           }

        }
        catch(const std::exception& e)
        {
            this->logger->error("DRONE:ENGINE: Engine killed..");
            this->logger->error(e.what());
        }
    }
}

//create and start thread
void Engine::start()
{
      int rc;
      this->logger->info("");
      rc = pthread_create(&constantSpeedAndDirectionMaintainingThread, NULL, staticConstantSpeedAndDirectionThreadTask, this);
      if (rc) 
      {
        this->logger->info("");
        exit(-1);
      }

}

