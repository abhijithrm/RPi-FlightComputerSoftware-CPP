#ifndef ENGINE_H
#define ENGINE_H
#include <CommandHandler.h>
#include <offboard.h>


using namespace protobuf_ProtoData_2eproto;      
using namespace std;
using namespace mavsdk;
using namespace CPlusPlusLogging;

class Engine
{
    private:
    Drone* droneReference;
    CommandHandler *comHandlerReferenceObject;
    int lastMissionCommandIndex;
    Logger *logger;
    pthread_t constantSpeedAndDirectionMaintainingThread;
    enum frame{
       ned,
       body
     };
    
    public:
    //Flag is true while rotating drone, used to prevent engine speed thread from interrupting drone rotation. Until this flag is true, engine speed thread will not execute
    bool rotationControlFlag;
    //Flag true while executing updated 3D speed values, used to prevent engine speed thread from interrupting the process.
    bool executingChangesFlag;
    Offboard *offboardObjectForVelocityControl;
    Engine(Drone *drone, CommandHandler * comhandler, Logger *logger );
    ~Engine();
    //Sends the updated 3D speed values to the flight controller.
    void executeChangesNow();
    static void* staticConstantSpeedAndDirectionThreadTask(void *eng);
    void killMotorsNow();
    //Starts engine speed maintaining thread. Every stated seconds, the thread sends the current value of 3D speed to the flight controller.
    void start();
    //Thread task
    void constantSpeedAndDirectionMaintainingThreadTask();
    //Rotates drone cw or ccw according to the direction parameter
    void rotate(int direction, float rotationAngle);
    //Sets an initial velocity yaw setpoint in the choosen frame before starting offboard control, otherwise the subsequent velocity yaw setpoints sent to FC will be rejected.
    bool setOffboardInitialSetpointAndStartOffboardVelocityControl(frame);
    //Set the velocity-yaw structure accodring to the frame type and send the setpoint to flighcontoller.
    mavsdk::Offboard::Result setAndSendVelocityYawValuesForOffboardControl(float speedx, float speedy, float speedz, float rotationangle, frame);
    //Stop the engine speed maintaining thread.
    void stopEngineConstantSpeedThread();
};
#endif