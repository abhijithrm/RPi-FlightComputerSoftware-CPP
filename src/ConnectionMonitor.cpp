#include <ConnectionMonitor.h>

ConnectionMonitor:: ConnectionMonitor(Drone *drn, const char* ip, int maxConn , Logger* logger)
{
 this->droneInstance = drn;
 this->hostIp = ip;
 this->maxConnectionAttempts = maxConn;
 this->connectionAttempts = 0;
 this->netStatus = false;
 this->pLogger = logger;

}

ConnectionMonitor::~ConnectionMonitor()
{
   pthread_cancel(thread);
   pLogger->info("Connection Monitor thread closed.");
}


void * ConnectionMonitor::staticConnectionMonitorTask(void *cm)//static class function is passed as thread func with the pointer to obkect as argument. Needs this for pthread impl
{
   ((ConnectionMonitor*)cm)->connectionMonitorTask();
}

void ConnectionMonitor::connectionMonitorTask()//actual class member function which is the thread body/task
{
   this_thread::sleep_for(chrono::seconds(5));
   
   while(true)
   {
      try
      {
         if(isInternetOn())
         {
            this->connectionAttempts = 0;
            this_thread::sleep_for(chrono::seconds(1));
         }
         else
         {
            this->droneInstance->freeze();
            this->connectionAttempts = this->connectionAttempts + 1;
            string connectionMessage = "Connection attempts: "+to_string(this->connectionAttempts)+" , max connection attempts: "+to_string(this->maxConnectionAttempts);
            pLogger->info(connectionMessage.c_str());
            this_thread::sleep_for(chrono::seconds(1));
            
            if(this->connectionAttempts == this->maxConnectionAttempts)
            {
               this->droneInstance->returnToBase();
               break;
            }
         }
      }
      catch(const std::exception& e)
      {  
         pLogger->info("Connection Monitor task failed. See below for error details");
         pLogger->error(e.what());
         this_thread::sleep_for(chrono::seconds(2));
      }
   }
}

void ConnectionMonitor::start()
{
      int rc;
      pLogger->info("Starting Connection Monitor thread...");
      rc = pthread_create(&thread, NULL, staticConnectionMonitorTask, this);
      if (rc) 
      {
        pLogger->info("Failed to start Connection Monitor thread.");
        exit(-1);
      }
}

//Checks for internet connection by pinging 1 request to server ip address..
bool ConnectionMonitor::isInternetOn()
{
   try
   {
      string hostip = string(this->hostIp);
      string pingCommand = "ping -c 1 -w 2 "+hostip;//cmd command for a single ping request. timeout-2s.

      int packetRecievedOnPing = parsePingCmdOutput(pingCommand.c_str());
      if(packetRecievedOnPing == 1)//ping succesfull
      {
      this->netStatus = true;
      return true;
      }
      else if(packetRecievedOnPing == 0)
      {
      this->netStatus = false;
      return false;
      }
   }
   catch(const std::exception& e)
   {
      this->netStatus = false;
      return false;
      pLogger->error("Connection Monitor: Network is unreachable");
      pLogger->info(e.what());

   }
}
   


//below code tested and gives correct o/p -0 or 1.
void ConnectionMonitor::tokenize(std::string const &str, const char* delim, std::vector<std::string> &out) 
{ 
    char *token = strtok(const_cast<char*>(str.c_str()), delim); 
    while (token != nullptr) 
    { 
        out.push_back(std::string(token)); 
        token = strtok(nullptr, delim); 
    } 
}

//returns 0 if no packets recieved on ping, 1 if received
int ConnectionMonitor::parsePingCmdOutput(const char *cmd)
{
      //ping ip address and parse the output to get the number of packets recieved.
   
      std::array<char, 128> buffer;
      std::string result;
      std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
      if (!pipe) {
         throw std::runtime_error("popen() failed!");
      }
      while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
         result += buffer.data();
      }
      //Split cmd output on ','.
      const char* delim = ","; 
      std::vector<std::string> out; 
      tokenize(result, delim, out);

      const char* delim2 = " "; 
      std::vector<std::string> out2; 
      tokenize(out[1], delim2, out2);   

      return atoi(out2[0].c_str()); //1 or 0 

}