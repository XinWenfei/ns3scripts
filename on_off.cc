#include"ns3/core-module.h"
#include"ns3/point-to-point-module.h"
#include"ns3/network-module.h"
#include"ns3/applications-module.h"
#include"ns3/netanim-module.h"
#include"ns3/internet-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OnOffAppExp");

int
main(int argc, char *argv[])
{
    Time::SetResolution (Time::NS);
    LogComponentEnable("OnOffAppExp", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    //LogComponentEnable("DataRate", LOG_LEVEL_INFO);

    uint32_t nNodes = 2;
    NodeContainer p2pNodes;
    p2pNodes.Create(nNodes);

    InternetStackHelper internet;
    internet.Install(p2pNodes);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("512Kbps"));
    p2p.SetChannelAttribute("Delay", StringValue("0.5ms"));

    NetDeviceContainer netdev;
    netdev = p2p.Install(p2pNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0","255.255.255.0");

    uint16_t port = 4000;
    Ipv4InterfaceContainer intf;
    intf = address.Assign(netdev);

    MobilityHelper mobility;
    //mobility.SetPositionAllocator("ns3::GridPositionAllocator",
    //        "MinX", DoubleValue(0.0),
    //        "MinY", DoubleValue(0.0),
    //        "DeltaX", DoubleValue(10.0),
    //        "DeltaY", DoubleValue(10.0),
    //        "GridWidth", UintegerValue(20),
    //        "LayoutType", StringValue("RowFirst"));

    ConstantPositionMobilityModel cpmm = ConstantPositionMobilityModel();
    ConstantPositionMobilityModel cpmm2 = ConstantPositionMobilityModel();
    cpmm.SetPosition(Vector(0.0, 0.0, 0.0));
    cpmm2.SetPosition(Vector(2.5, 2.5, 0.0));

    //mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel", "Position", Vector3DValue(Vector3D(0.0, 0.0, 0)));
    //mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.PushReferenceMobilityModel(&cpmm);
    mobility.Install(p2pNodes.Get(0));

    //mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel", "Position", Vector3DValue(Vector3D(0.0, 5.0, 0)));
    //mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    //cpmm.SetPosition(Vector(2.5, 2.5, 0.0));
    mobility.PushReferenceMobilityModel(&cpmm2);
    mobility.Install(p2pNodes.Get(1));

    OnOffHelper onOff("ns3::TcpSocketFactory", Address(InetSocketAddress(intf.GetAddress(1),port)));
    onOff.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onOff.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    onOff.SetAttribute("PacketSize", UintegerValue(1000));
    onOff.SetAttribute("DataRate", StringValue("5kbps"));
    onOff.SetAttribute("MaxBytes", UintegerValue(100000));
    

    double simTime = 25.0;
    ApplicationContainer app1 = onOff.Install(p2pNodes);
    app1.Start(Seconds(1.0));
    app1.Stop(Seconds(simTime));

    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    ApplicationContainer app2 = sinkHelper.Install(p2pNodes.Get(1));

    app2.Start(Seconds(0.0));
    app2.Stop(Seconds(simTime));

    Simulator::Stop(Seconds(simTime));
    AnimationInterface anim("on_off.xml");
    p2p.EnablePcapAll("./pcap/on_off");

    NS_LOG_INFO("Begin simulation...");
    //Ptr<Node> ptr_node = p2pNodes.Get(0);
    //Ptr<PointToPointNetDevice> ptr_netdev = ptr_node->GetObject<PointToPointNetDevice>();
    //Ptr<PointToPointChannel> ptr_channel = ptr_netdev->GetObject<PointToPointChannel>();
    //Time delay = ptr_channel->GetDelay();
    //std::cout<<"The point to point channel's delay is:"<<delay<<std::endl;

    Simulator::Run();
    Simulator::Destroy();
}
