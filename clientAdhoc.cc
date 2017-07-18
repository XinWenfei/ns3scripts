#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/olsr-helper.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <sstream>
#include <unistd.h>

// Default Network Topology
//
//                          
// -------------------------|----------------------------
//  +----------+
//  | external |
//  |  Linux   |
//  |   Host   |
//  |          |
//  | "ns3tap" |
//  +----------+
//       |
//       |                  
//       |   +----------+   
//       +---|tap bridge|      Wifi 192.168.1.0        
//           +----------+            Adhoc               
//           | station  |    *    *    *    *    *    * 
//           +----------+    |    |    |    |    |    |
//                n0        n1   n2   n3   n4   n5   n6
//                   

using namespace ns3;
using namespace std;
#define SOCK_COUNT 6
#define PORT       5000

NS_LOG_COMPONENT_DEFINE ("socketClient");

static void ConnectServer(Ptr<Socket> sockArray[], uint32_t sockCount, InetSocketAddress remote){
    NS_LOG_INFO("Beginning connect");
    int ret;
    cout<<"sizeof sock fd:"<<sizeof(sockArray[0])<<endl;
    cout<<"sock memory address:"<<sockArray[0]<<endl;
    for(uint32_t i=0; i<sockCount; i++){
        ret = sockArray[i]->Connect(remote);
        cout<<"Ret is:"<<ret<<endl;
    }
    // if(ret == -1)
    //     exit(0);
}

static void GenerateTraffic (Ptr<Socket> sockArray[], uint32_t sockCount, 
    uint32_t interval, uint32_t start, uint32_t end)
{
    for(uint32_t i=0; i < sockCount; i ++){
        srand((unsigned)time(NULL));
        uint32_t randNum = rand()%(end-start-SOCK_COUNT+1)+start+i;
        uint32_t nodeId  = i+1;
        stringstream sstr;
        sstr<<nodeId;
        sstr<<randNum;
        string str;
        sstr>>str;
        // uint32_t charNum = str.length();
        uint8_t *asciiData = new uint8_t[str.length()];
        for(uint8_t i=0; i < str.length(); i++)
            asciiData[i] = (int)str[i];
        uint8_t const *ptrdata = asciiData;
        uint32_t datasize = sizeof(*ptrdata);
        // cout<<ptrdata<<endl;
        int sendSize = sockArray[i]->Send(ptrdata, datasize, 0);
        cout<<"Send size: "<<sendSize<<endl;
        // cout <<"Rand num is: "<<randNum<<endl;
        // cout <<"string is :"<<str<<endl;
        // cout <<"assic code is: "<<(int)str[0]<<(int)str[1]<<endl;
        // cout <<"num of characters: "<<charNum<<endl;
        // data[0] = nodeId;
        // data[1] = randNum;
        // const uint32_t
        // sockArray[i]->Send()
        delete asciiData;
    }
}

int main(int argc, char *argv[])
{
    uint32_t nodeCount = 7;
    uint32_t sinkNodeId = 0;
    uint32_t start = 10;
    uint32_t end   = 99;
    bool tracing = true;
    bool verbose = true;
    uint32_t simTime = 60; //s
    uint32_t interval = 5; //s
    std::string phyMode = "OfdmRate54Mbps";
    std::string tapMode = "ConfigureLocal";
    std::string tapName = "ns3tap";

    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    
    if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("socketClient", LOG_LEVEL_INFO);
    }

    NS_LOG_INFO("Create Nodes");
    NodeContainer nc;
    nc.Create(nodeCount);
    
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel( channel.Create() );
    phy.Set("ChannelNumber", UintegerValue(1));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
            "DataMode", StringValue(phyMode),
            "ControlMode", StringValue(phyMode));

    WifiMacHelper mac;

    NS_LOG_INFO("Set wireless network mac info");
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer ndc;
    ndc = wifi.Install(phy, mac, nc);

    InternetStackHelper stack;
    OlsrHelper olsr;
    stack.SetRoutingHelper(olsr);
    stack.Install (nc);

    NS_LOG_INFO("Set all nodes' ip address");
    
    Ipv4AddressHelper addr;

    addr.SetBase("10.1.1.240", "255.255.255.240");
    Ipv4InterfaceContainer intf;
    intf =  addr.Assign(ndc) ;

    NS_LOG_INFO("Output ip address");

    uint32_t i;
    for(i=0; i<nc.GetN(); i++){
        Ptr<Ipv4> ptr_ipv4 = nc.Get(i)->GetObject<Ipv4>();
        std::cout<<ptr_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
    }

    NS_LOG_INFO("Set mobility model and position allocator");
    MobilityHelper mobility;

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    for(i=0; i<nc.GetN(); i++)
        positionAlloc->Add (Vector (i*10, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(nc);

    
    NS_LOG_INFO("Create tap bridge");
    TapBridgeHelper tapBridge;
    tapBridge.SetAttribute ("Mode", StringValue(tapMode));
    tapBridge.SetAttribute ("DeviceName", StringValue (tapName));
    tapBridge.Install (nc.Get (0), ndc.Get (0));

    NS_LOG_INFO("Create socket array");
    TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");
    Ptr<Socket> source[SOCK_COUNT];
    // cout<<intf.GetAddress(sinkNodeId, 0)<<endl;
    InetSocketAddress remote = InetSocketAddress(intf.GetAddress(sinkNodeId, 0), PORT);
    for(i=0; i<SOCK_COUNT; i++){
       source[i] = Socket::CreateSocket( nc.Get(i+1), tid);
       // cout<<staIntf.GetAddress(i, 0)<<endl;
       // source[i]->Connect(remote);
    }
    

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    if(tracing == true){
        phy.EnablePcapAll("pcap/socketClient");
    }

    AnimationInterface anim("xml/socketClient");

    Simulator::Schedule(Seconds(15.0), &ConnectServer, source, SOCK_COUNT, remote);
    // Simulator::Schedule(Seconds(22.0), &GenerateTraffic, source, SOCK_COUNT, interval, start, end);

    Simulator::Stop (Seconds(simTime));
    NS_LOG_INFO("Simulation begins");
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
