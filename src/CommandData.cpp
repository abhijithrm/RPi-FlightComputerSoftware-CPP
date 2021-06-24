#include <ProtoData.pb.h>

using namespace protobuf_ProtoData_2eproto;
//CommandData dto for holding proto buf Command obj properties
class CommandData
{
    public:
    int commandCode;
    MissionData data;

};