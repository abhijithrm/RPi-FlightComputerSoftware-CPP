#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H
#include <Drone.h>


using namespace protobuf_ProtoData_2eproto;      
using namespace std;
using namespace mavsdk;
using namespace CPlusPlusLogging;

class Engine;
class CommandHandler 
{
    private:
        Mavsdk *mavlink;
        shared_ptr<System> vehicle;//Mavlink system object
        Drone* droneReference = nullptr;
        
        Telemetry *telemetryInstance;
        int lightState;
        int ignitorState;

        int cameraAngle;
        //ServoController* cameraGimbalServo;

    public:
        float speedX;
        float speedY;
        float speedZ;
        float speedIncrementValueX;
        float speedIncrementValueY;
        float speedIncrementValueZ;
        float rotationAngle;
        Engine *engine = nullptr;
        Logger *logger;
        mavsdk::Action *mavsdkActionPluginObject;

    CommandHandler (Drone* drone);
    ~CommandHandler();
    void stopMovement();
    void rotateLeft(int angle);
    void rotateRight(int angle);
    void increaseSpeedX();
    void decreaseSpeedX();
    void leftSpeedY();
    void rightSpeedY();
    void stopSpeedXY();
    void increaseSpeedZ();//note that we are actualling decrementing z axis speed
    void decreaseSpeedZ();
    void stopSpeedZ();
    void killMotorsNow();
    void armAndTakeoff(int takeOffAlt);
    void goHome(float returnToLaunchAltitude);
    void toggleLights();
    void cameraUP();
    void cameraDown();
    void cancelMission();
    void activateMission(MissionData missionPointsData);
    void land();
};
#endif