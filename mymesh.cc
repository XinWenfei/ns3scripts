#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <sstream>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MeshScript");

int main(int argc, char* argv[])
{
    uint16_t rowNodes = 3;
    uint16_t colNodes = 3;
    double   distance = 50;// meter
    double   totalTime = 100;//seconds
    double   pktInterval = 0.1; //second
    uint16_t pktSize  = 1024; 
    uint32_t nIntf    = 1;
    bool     pcap     = true;
    bool     log      = true;
    double   randomStart = 0.1;

    CommandLine cmd;
    cmd.AddValue("rowN", "Number of nodes in a row", rowNodes);
    cmd.AddValue("colN", "Number of nodes in a column", colNodes);
    cmd.AddValue("dis",  "Distance between two nodes", distance);
    cmd.AddValue("time", "Sum simulation time", totalTime);
    cmd.AddValue("interval", "Interval between sending two pakcets", pktInterval);
    cmd.AddValue("intfN","Number of radio interfaces used by each mesh point.[0.001s]", nIntf);
    cmd.AddValue("pcap", "Enable pcap trace on interfaces.[true]", pcap);
    cmd.AddValue("log",  "Enable log info when running", log);

    if(log){
        LogComponentEnable("MeshScript", LOG_LEVEL_INFO);
	    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
    NS_LOG_INFO("Grid: "<<rowNodes<<"*"<<colNodes);
    NS_LOG_INFO("Simulation time: "<<totalTime<<" s.");

    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel (channel.Create());

    MeshHelper mesh = MeshHelper::Default();
    std::string stack = "ns3::Dot11sStack";
    std::string root  = "ff:ff:ff:ff:ff:ff";
    mesh.SetStackInstaller(stack);
    mesh.SetSpreadInterfaceChannels(MeshHelper::SPREAD_CHANNELS);
    mesh.SetMacType("RandomStart", TimeValue(Seconds(randomStart)));
    mesh.SetNumberOfInterfaces(nIntf);

    NodeContainer nc;
    nc.Create(rowNodes*colNodes);

    NetDeviceContainer meshDevices = mesh.Install(phy, nc);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
            "MinX", DoubleValue(0.0),
            "MinY", DoubleValue(0.0),
            "DeltaX", DoubleValue(distance),
            "DeltaY", DoubleValue(distance),
            "GridWidth", UintegerValue(rowNodes),
            "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nc);
    if(pcap)
        phy.EnablePcapAll(std::string("pcap/mymesh"));

    InternetStackHelper istack;
    istack.Install(nc);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer intf = address.Assign(meshDevices);

    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApp = echoServer.Install(nc.Get(rowNodes*colNodes -1));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(totalTime));
    UdpEchoClientHelper echoClient(intf.GetAddress(rowNodes*colNodes -1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(pktInterval)));
    echoClient.SetAttribute("PacketSize", UintegerValue(pktSize));
    ApplicationContainer clientApp = echoClient.Install(nc.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(totalTime));

    AnimationInterface anim("xml/mymesh.xml");
    anim.UpdateNodeColor(nc.Get(0), 0, 255, 0);
    anim.UpdateNodeSize(nc.Get(0)->GetId(), 5, 5);
    anim.UpdateNodeColor(nc.Get(rowNodes*colNodes - 1), 255, 0, 0);
    anim.UpdateNodeSize(nc.Get(rowNodes*colNodes - 1)->GetId(), 5, 5);
    for(int i = 1; i < rowNodes*colNodes - 1; i++){
        anim.UpdateNodeColor(nc.Get(i), 0, 0, 255);
        anim.UpdateNodeSize(nc.Get(i)->GetId(), 5, 5);
    }

    Simulator::Stop(Seconds(totalTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;

}
