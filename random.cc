#include "ns3/core-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    //RngSeedManager::SetSeed(3);
    //RngSeedManager::SetRun(7);
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();   
    double Min = 0.0;
    double Max = 10.0;
    x->SetAttribute("Min", DoubleValue(Min));
    x->SetAttribute("Max", DoubleValue(Max));
    std::cout<< x->GetInteger()<<std::endl;
    std::cout<< x->GetValue()<<std::endl;
    std::cout<< x->GetStream()<<std::endl;
    return 0;
}
