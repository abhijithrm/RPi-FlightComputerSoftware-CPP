#ifndef COMMANDDATA_H
#define COMMANDDATA_H
#include <ProtoData.pb.h>


using namespace protobuf_ProtoData_2eproto;
class CommandData
{
    public:
    int commandCode;
    MissionData data;

};
#endif