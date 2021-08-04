#ifndef APPUTILS_H
#define APPUTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <Logger.h>


using namespace std;
using namespace CPlusPlusLogging;

class AppUtils
{
public:
static size_t createNetworkMessage(string msg_body, unsigned char* finalResultMessage);
//Creates the full network message as a byte array by using the size of the message body passed as parameter
static size_t createNetworkMessage(unsigned char* messageBodyByteArray, int messageByteSize, unsigned char* resultMessage);
//Reads the incoming message body from the socket i/p stream and returns the size in bytes of the read message body
static size_t readNetworkMessage(int sockfd, unsigned char* readBytesArray);
//Stores the 32 bit integer in a byte array of size 4.
static void intToByte(int n, unsigned char *byteArray);
//Converts the byte array obtained from  the head of the incoming network message and converts it back to int which is the size of the message body
static int byteToInt(unsigned char* byte) ;
//Simple console bar animation for implying that the program is waiting or stuck at something.
static int waitingConsoleAnimation(int counter, string consoleMessage, Logger* logger);

};
#endif