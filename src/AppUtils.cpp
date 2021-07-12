#include <AppUtils.h>

//takes std::string message body as arg
size_t AppUtils::createNetworkMessage(string msg_body, unsigned char* resultMessage)
{
    int msg_length = msg_body.length();
    cout<<"Network message length(int): "<<msg_length<<endl;

    unsigned char byteArray[4];
    AppUtils::intToByte(msg_length, byteArray);//int to bytearray

    unsigned char networkMessage[4+msg_length];//full msg body with encoded length

    for(int i=0; i<4; i++)
    {
        resultMessage[i] = byteArray[i];
    }

    for(int i=4, j=0; j< msg_length; j++)
    {
        resultMessage[i] = msg_body[j];
    }
    cout<<"AppUtils: Create net mssgee: total length of byte array:"<<4+msg_length<<endl;
    return 4+msg_length;
}

//takes byte array of message body as argument
size_t AppUtils::createNetworkMessage(unsigned char* messageBodyByteArray, int messageByteSize, unsigned char* resultMessage)
{
    
    cout<<"Network message length(int): "<<messageByteSize<<endl;

    unsigned char byteArray[4];
    AppUtils::intToByte(messageByteSize, byteArray);//int to bytearray

    unsigned char networkMessage[4+messageByteSize];//full msg body with encoded length

    for(int i=0; i<4; i++)
    {
        resultMessage[i] = byteArray[i];
    }

    for(int i=4, j=0; j< messageByteSize; i++, j++)
    {
        resultMessage[i] = messageBodyByteArray[j];
    }
    cout<<"AppUtils: Created network message. Total length of result byte array:"<<4+messageByteSize<<endl;
    return 4+messageByteSize;
}

 size_t AppUtils::readNetworkMessage(int sockfd, unsigned char* resultMessageArray)
{

    unsigned char byteArray[4];
    int msgLengthRead = read(sockfd, byteArray, 4 );//read first four bytes from socket stream to get length of message body to read.
    int msgbody_length = AppUtils::byteToInt(byteArray);
    cout<<"Size of incoming message body read from socket input stream: "<<msgbody_length<<endl;

    //unsigned char messageBody[msgbody_length];
    int msgbody_read = read(sockfd, resultMessageArray, msgbody_length);//read actual mssge body from socket i/p stream

    return msgbody_length;
}

void AppUtils::intToByte(int n, unsigned char *byteArray) {//pass a byte array and fill it

  for(int i =3;i>=0;i--)    
  {
      byteArray[i] = (unsigned char) (n & 0xff);//0xff is hexadecimal representation of mumber 255 which in binary is 00000000000000000000000011111111, in 32 bits. So last 8 bits are all 1s. So on &ing with a no., we get the last 8 bits of that no. Then we shift 8 bits to right and again &, so we get the next set i.e, from bit 16 to 24
      n=n>>8;//>> is the binary right shift operator. n>>8 means to shift 8 bits to the right of the numbers binary
  }

    /* byteArray[0] = n & 0xff000000 >> 24;
     byteArray[1] = n & 0x00ff0000 >> 16;
     byteArray[2] = n & 0x0000ff00 >> 8;
     byteArray[3] = n & 0x000000ff; */

}

int AppUtils::byteToInt(unsigned char* byte) {

    int n = 0;
    n= ((byte[0]<<24) + (byte[1]<<16) + (byte[2]<<8) + (byte[3]<<0));

    /*n = n + (byte[0] & 0x000000ff);
    n = n + ((byte[1] & 0x000000ff) << 8);
    n = n + ((byte[2] & 0x000000ff) << 16);
    n = n + ((byte[3] & 0x000000ff) << 24);*/
    return n;
}