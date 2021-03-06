#include "ns3/core-module.h"
#include <sys/time.h>
#include <ctime>

using namespace ns3;

long getCurrentTime(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + tv.tv_usec/1000;
}

int main(int argc, char *argv[])
{
    RngSeedManager::SetSeed(getCurrentTime());
        RngSeedManager::SetRun(std::time(0));
        //RngSeedManager::SetRun(i);
        Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();   
        double Min = 0.0;
        double Max = 100.0;
        x->SetAttribute("Min", DoubleValue(Min));
        x->SetAttribute("Max", DoubleValue(Max));
    for(uint32_t i=0; i<10; i++){
        std::cout<< x->GetInteger()<<"  "<<x->GetValue()<<std::endl;
        //std::cout<< x->GetValue()<<std::endl;
        //std::cout<< x->GetStream()<<std::endl;
    }
    return 0;
}
