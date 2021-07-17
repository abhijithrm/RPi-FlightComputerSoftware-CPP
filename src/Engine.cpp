#include <Engine.h>


 Engine::Engine(Drone *drone, CommandHandler * comhandler, Logger *logger )
{
    this->droneReference = drone;
    this->comHandlerReferenceObject = comhandler;
    this->lastMissionCommandIndex = -1;
    this->logger = logger;
    this->rotationControlFlag = false;
    this->executingChangesFlag = false;
    
    //The Offboard MAVSDK plugin provides a simple API for controlling the vehicle using velocity and yaw setpoints. It is useful for tasks requiring direct control from a companion computer; for example to implement collision avoidance.
    this->offboardObjectForVelocityControl = new Offboard(this->droneReference->system);
    
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
       this->executingChangesFlag = true;
       if(!this->offboardObjectForVelocityControl->is_active())
       {
        //start offboard control
        if(this->setOffboardInitialSetpointAndStartOffboardVelocityControl(frame::body))
        this->logger->info("DRONE: ENGINE: OFFBOARD: Success.");
        else
        this->logger->error("DRONE: ENGINE: OFFBOARD: Failed.");
       }
       
       //set the speed values in x,y,z as is in the command handler object speed properties
       mavsdk::Offboard::Result setVelocityYawSpeed = this->setAndSendVelocityYawValuesForOffboardControl(this->comHandlerReferenceObject->speedX, this->comHandlerReferenceObject->speedY, this->comHandlerReferenceObject->speedZ, 0.0f, frame::body);
       this_thread::sleep_for(chrono::milliseconds(15));

       if(setVelocityYawSpeed != mavsdk::Offboard::Result::Success)
        this->logger->error("DRONE:ENGINE: Sending command to set velocity/yaw values to drone failed");
       else if(setVelocityYawSpeed == mavsdk::Offboard::Result::Success)
        this->logger->info("DRONE:ENGINE: Command to set updated velocity values send successfully to drone");

       this->executingChangesFlag = false; 
}

void Engine::killMotorsNow()
{  
 mavsdk:Action::Result killEngine = this->comHandlerReferenceObject->mavsdkActionPluginObject->kill();
 if(killEngine != mavsdk::Action::Result::Success)
  this->logger->info("DRONE:ENGINE: Killing engine failed.");
 else if(killEngine == mavsdk::Action::Result::Success)
  this->logger->info("DRONE:ENGINE: Engine killed. Drone will fall off sky if in air");
}

//if direction is -1 rotate ccw and rotate cw if 1
void Engine::rotate(int direction, float rotationAngle)
{
    this->rotationControlFlag = true;//when this flag is true, constant speed thread won't run, thus not changing the frame type abruptly and interrupting drone rotation
    mavsdk::Offboard::Result setYawAngleResult;//success flag
    do
    {       
        if(!this->offboardObjectForVelocityControl->is_active())   
        { 
          //start offboard control
          if(this->setOffboardInitialSetpointAndStartOffboardVelocityControl(frame::body))
          this->logger->info("DRONE: ENGINE: OFFBOARD: Success. Frame - Body");
          else
          this->logger->error("DRONE: ENGINE: OFFBOARD: Failed. Frame- Body");
        }

        if(direction == 1)//cw roation
        {
            //set yaw angle/s for cw rotation. Returns Success if command sent successfully
            setYawAngleResult = this->setAndSendVelocityYawValuesForOffboardControl(0.0f, 0.0f, 0.0f, rotationAngle, frame::body);//cw

            this_thread::sleep_for(chrono::milliseconds(1100));//As we are setting the yaw degree per second value, we allow sleep for 1 sec to complete the given degree rotation and then stop offboard mode
            mavsdk::Offboard::Result stopOffboardControl = this->offboardObjectForVelocityControl->stop();//stop offboard control mode after rotation complete. Engine speed thread will start offboard mode for BODY frame every 5 seconds, or executechangesnow()

        }
        else if(direction == -1)//ccw
        {
            //set yaw angle/s for ccw rotation. Returns Success if command sent successfully
            setYawAngleResult = this->setAndSendVelocityYawValuesForOffboardControl(0.0f, 0.0f, 0.0f, -rotationAngle, frame::body);

            this_thread::sleep_for(chrono::milliseconds(1100));//Let yaw settle
            mavsdk::Offboard::Result stopOffboardControl = this->offboardObjectForVelocityControl->stop();//stop offboard control mode

        }

    }while(setYawAngleResult != mavsdk::Offboard::Result::Success); 

    this->rotationControlFlag = false;

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
           //speed maintainence safety check: every 5 seconds, keep sending latest speed values to the vehicle, only if offboard control is inactive
           if(this->comHandlerReferenceObject->speedX !=0 || this->comHandlerReferenceObject->speedY !=0 || this->comHandlerReferenceObject->speedZ !=0)
           {
               //initiate offboard control if not enabled
              if(!this->offboardObjectForVelocityControl->is_active() && !this->rotationControlFlag && !this->executingChangesFlag) 
              {
                 //start offboard control
                if(this->setOffboardInitialSetpointAndStartOffboardVelocityControl(frame::body))
                this->logger->info("DRONE: ENGINE: OFFBOARD: Success.");
                else
                this->logger->error("DRONE: ENGINE: OFFBOARD: Failed.");
              
              //set the speed values in x,y,z as is in the command handler object speed properties
              //no change in yaw angle..no rotation, only speed maintaining
              //send values to flight controller
              mavsdk::Offboard::Result setVelocityYaw = this->setAndSendVelocityYawValuesForOffboardControl(this->comHandlerReferenceObject->speedX, this->comHandlerReferenceObject->speedY, this->comHandlerReferenceObject->speedZ, 0.0f, frame::body);
              if(setVelocityYaw != mavsdk::Offboard::Result::Success)
                this->logger->error("DRONE: ENGINE: CONSTANT SPEED THREAD: Sending command to set velocity/yaw values to drone failed");
              else if(setVelocityYaw == mavsdk::Offboard::Result::Success) 
                this->logger->info("DRONE: ENGINE: CONSTANT SPEED THREAD: Command to set velocity values sent successfully to drone");
              
              }
              this_thread::sleep_for(chrono::seconds(1));
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
      this->logger->info("DRONE: ENGINE: Starting constant speed maintaining thread.");
      rc = pthread_create(&constantSpeedAndDirectionMaintainingThread, NULL, staticConstantSpeedAndDirectionThreadTask, this);
      if (rc) 
      {
        this->logger->info("");
        exit(-1);
      }

}

bool Engine::setOffboardInitialSetpointAndStartOffboardVelocityControl(frame frameType)
{
     mavsdk::Offboard::VelocityNedYaw velocityYawNEDValuesInitialSetpoint;
     mavsdk::Offboard::VelocityBodyYawspeed velocityYawSpeedBodyInitialSetpoint;
     mavsdk::Offboard::Result initialOffboardSetpointSet;
     
     switch(frameType)
     {
            case frame::ned:
            {
                //set the values for the structure members and set velocity yaw degree ned setpoint
                //Send one velocity setpoint before starting offboard(start()), otherwise succeeding setpoints will be rejected.
                velocityYawNEDValuesInitialSetpoint.down_m_s=  0.0f;
                velocityYawNEDValuesInitialSetpoint.east_m_s=  0.0f;
                velocityYawNEDValuesInitialSetpoint.north_m_s= 0.0f;
                velocityYawNEDValuesInitialSetpoint.yaw_deg =  0.0f;//deg
              
                //Stop offboard mode and set body setpoint and start offboard again for velocity body frame based control for rotation
                this->offboardObjectForVelocityControl->stop();
                initialOffboardSetpointSet = this->offboardObjectForVelocityControl->set_velocity_ned( velocityYawNEDValuesInitialSetpoint);
              
                if(initialOffboardSetpointSet == mavsdk::Offboard::Result::Success)
                this->logger->info("DRONE: ENGINE: Initial OFFBOARD velocity NED frame setpoint set");
                break;
            }
            case frame::body:
            {
                //set the values for the structure members and set velocity yaw-speed body setpoint
                //Send one velocity setpoint before starting offboard(start()), otherwise succeeding setpoints will be rejected.
                velocityYawSpeedBodyInitialSetpoint.down_m_s=       0.0f;
                velocityYawSpeedBodyInitialSetpoint.forward_m_s=    0.0f;
                velocityYawSpeedBodyInitialSetpoint.right_m_s=      0.0f;
                velocityYawSpeedBodyInitialSetpoint.yawspeed_deg_s= 0.0f;//deg/s
                
                //Stop offboard mode and set body setpoint and start offboard again for velocity body frame based control for rotation
                this->offboardObjectForVelocityControl->stop();
                initialOffboardSetpointSet = this->offboardObjectForVelocityControl->set_velocity_body(velocityYawSpeedBodyInitialSetpoint);
                
                if(initialOffboardSetpointSet == mavsdk::Offboard::Result::Success)
                this->logger->info("DRONE: ENGINE: Initial OFFBOARD velocity yaw-speed BODY frame setpoint set");
                break;
            }
      }
      
      //start offboard control for whichever frame chosen
      mavsdk::Offboard::Result offboardControlStart =  this->offboardObjectForVelocityControl->start();
      if(offboardControlStart != mavsdk::Offboard::Result::Success)
        this->logger->error("DRONE: ENGINE: Setting MAVSDK OFFBOARD control for vehicle failed.");
      else if(offboardControlStart == mavsdk::Offboard::Result::Success)
        this->logger->info("DRONE: ENGINE: OFFBOARD control enabled successfully for drone.");
      
      cout<<offboardControlStart<<endl;

      return (initialOffboardSetpointSet == mavsdk::Offboard::Result::Success) && (offboardControlStart == mavsdk::Offboard::Result::Success);

}

 mavsdk::Offboard::Result Engine::setAndSendVelocityYawValuesForOffboardControl(float speedx, float speedy, float speedz, float rotationangle, frame frametype)
{
    mavsdk::Offboard::VelocityNedYaw velocityYawNEDSetpoint;
    mavsdk::Offboard::VelocityBodyYawspeed velocityYawSpeedBodySetpoint;
    mavsdk::Offboard::Result OffboardSetpointSetStatus;
     
     switch(frametype)
     {
       case frame::body:
       {
         velocityYawSpeedBodySetpoint.down_m_s=       speedz;
         velocityYawSpeedBodySetpoint.forward_m_s=    speedx;
         velocityYawSpeedBodySetpoint.right_m_s=      speedy;
         velocityYawSpeedBodySetpoint.yawspeed_deg_s= rotationangle;//deg/s
         OffboardSetpointSetStatus = this->offboardObjectForVelocityControl->set_velocity_body(velocityYawSpeedBodySetpoint);
         break;
       }

       case frame::ned:
       {
          velocityYawNEDSetpoint.down_m_s=  speedz;
          velocityYawNEDSetpoint.east_m_s=  speedy;
          velocityYawNEDSetpoint.north_m_s= speedx;
          velocityYawNEDSetpoint.yaw_deg =  rotationangle;//deg
          OffboardSetpointSetStatus = this->offboardObjectForVelocityControl->set_velocity_ned(velocityYawNEDSetpoint);
          break;
       }
     }
     return OffboardSetpointSetStatus;
}

void Engine::stopEngineConstantSpeedThread()
{
    pthread_cancel(constantSpeedAndDirectionMaintainingThread);
    this->logger->info("DRONE:ENGINE: Engine constant speed maintaining thread closed.");

}

