#include <AppUtils.h>

size_t AppUtils::createNetworkMessage(string msg_body, unsigned char* resultMessage)
{
    int msg_length = msg_body.length();

    unsigned char byteArray[4];
    AppUtils::intToByte(msg_length, byteArray);//int to bytearray

    unsigned char networkMessage[4+msg_length];//full msg body with encoded length

    for(int i=0; i<4; i++)
    {
        resultMessage[i] = byteArray[i];
    }

    for(int i=4; i< msg_length; i++)
    {
        resultMessage[i] = msg_body[i];
    }
    
    return 4+msg_length;
}

 size_t AppUtils::readNetworkMessage(int sockfd, unsigned char* resultMessageArray)
{

    unsigned char byteArray[4];
    int msgLengthRead = read(sockfd, byteArray, 4 );//read first four bytes from socket stream to get length of message body to read.
    int msgbody_length = AppUtils::byteToInt(byteArray);

    //unsigned char messageBody[msgbody_length];
    int msgbody_read = read(sockfd, resultMessageArray, msgbody_length);//read actual mssge body from socket i/p stream

    return msgbody_length;
}

void AppUtils::intToByte(int n, unsigned char *byteArray) {//pass a byte array and fill it

    

     byteArray[0] = n & 0x000000ff;
     byteArray[1] = n & 0x0000ff00 >> 8;
     byteArray[2] = n & 0x00ff0000 >> 16;
     byteArray[3] = n & 0xff000000 >> 24; 

}

int AppUtils::byteToInt(unsigned char* byte) {

    int n = 0;

    n = n + (byte[0] & 0x000000ff);
    n = n + ((byte[1] & 0x000000ff) << 8);
    n = n + ((byte[2] & 0x000000ff) << 16);
    n = n + ((byte[3] & 0x000000ff) << 24);
    return n;
}