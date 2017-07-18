#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/mesh-l2-routing-protocol.h"

#include <iostream>
#include <sstream>
#include <fstream>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("Subnet2Mesh");

int main()
{
    uint16_t nodesCount = 3;
    uint16_t distance = 50; //meter
    double   totalTime= 20; //seconds
    double   pktInterval = 0.1;//second
    uint16_t pktSize  = 1024;
    uint16_t intfCount = 1;
    bool     pcap    = true;
    bool     log     = true;
    double   randomStart = 0.1;

    if(log){
        LogComponentEnable("Subnet2Mesh", LOG_LEVEL_INFO);
	    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());

    MeshHelper mesh = MeshHelper::Default();
    std::string meshStack = "ns3::Dot11sStack";
    std::string root  = "ff:ff:ff:ff:ff:ff";
    mesh.SetStackInstaller(meshStack);
    mesh.SetSpreadInterfaceChannels(MeshHelper::SPREAD_CHANNELS);
    mesh.SetMacType("RandomStart", TimeValue(Seconds(randomStart)));
    mesh.SetNumberOfInterfaces(intfCount);

    NodeContainer nc;
    nc.Create(nodesCount);
    NodeContainer subnet1;
    subnet1.Add(nc.Get(0));
    subnet1.Add(nc.Get(1));
    NodeContainer subnet2;
    subnet2.Add(nc.Get(1));
    subnet2.Add(nc.Get(2));

    NetDeviceContainer subnet1MeshDevs = mesh.Install(phy, subnet1);
    NetDeviceContainer subnet2MeshDevs = mesh.Install(phy, subnet2);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
            "MinX", DoubleValue(0.0),
            "MinY", DoubleValue(0.0),
            "DeltaX", DoubleValue(distance),
            "DeltaY", DoubleValue(distance),
            "GridWidth", UintegerValue(nodesCount),
            "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nc);
    if(pcap)
        phy.EnablePcapAll(std::string("pcap/subnet2mesh"));

    Ipv4StaticRoutingHelper staticRoutingHelper;
    Ipv4ListRoutingHelper listRouting;
    AodvHelper aodv;
    listRouting.Add(aodv, 0);

    InternetStackHelper ipstack;
    //ipstack.SetRoutingHelper(aodv);
    Config::SetDefault("ns3::MeshPointDevice::RoutingProtocol", StringValue("ns3::dot11s::HwmpProtocol"));
    ipstack.Install(nc);
    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer subnet1Intf = addr.Assign(subnet1MeshDevs);
    addr.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer subnet2Intf = addr.Assign(subnet2MeshDevs);

    //Ptr<Ipv4> ptr_ipv4N2 = nc.Get(1)->GetObject<Ipv4>();
    //Ptr<Ipv4StaticRouting> ptr_staticRouting = staticRoutingHelper.GetStaticRouting(ptr_ipv4N2);
    ////Remove the default route
    //for(uint16_t i = 0; i<= ptr_staticRouting->GetNRoutes(); i++){
    //    ptr_staticRouting->RemoveRoute(i);
    //}
    ////Add route to node2, interface zero is the lookback
    //ptr_staticRouting->AddHostRouteTo(Ipv4Address("10.0.2.2"), 2);
    //ptr_staticRouting->AddHostRouteTo(Ipv4Address("10.0.1.1"), 1);
    //for(uint16_t i=0; i < ptr_staticRouting->GetNRoutes(); i++){
    //      Ipv4RoutingTableEntry routeTable = ptr_staticRouting->GetRoute(i);
    //      std::cout<<"N2 dest: "<<routeTable.GetDest()<<", Gateway: "<<routeTable.GetGateway()<<std::endl;
    //      std::cout<<"N2 dest network: "<<routeTable.GetDestNetwork()<<", Interface: "<<routeTable.GetInterface()<<std::endl<<std::endl;
    //  }

    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApp = echoServer.Install(nc.Get(2));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(totalTime));

    UdpEchoClientHelper echoClient(subnet2Intf.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(20));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(pktInterval)));
    echoClient.SetAttribute("PacketSize", UintegerValue(pktSize));
    ApplicationContainer clientApp = echoClient.Install(nc.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(totalTime));

    AnimationInterface anim("xml/subnet2Mesh.xml");
    Simulator::Stop(Seconds(totalTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
