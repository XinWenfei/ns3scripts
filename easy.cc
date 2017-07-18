/*
//network topology
//
//               n6                    n7
//               |                    /p2p
//               |                   /
//   n0      n4---------------------n5
// p2p \    /p2p     csma/cd          \
//      \  /                           \
//       n2                             n8
//      /  \p2p
// p2p /    \
//   n1      n
 */
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EasyExample");

int main(int argc,char* argv[])
{
	//Config::SetDefault("ns3::OnOffApplication::PacketSize",UintegerValue(210));
        //Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("448kb/s"));

        //实时模拟
        //GlobalValue::Bind("SimulatorImplementationType",StringValue ("ns3::RealtimeSimulatorImpl"));

	Time::SetResolution(Time::NS);
	LogComponentEnable("UdpEchoClientApplication",LOG_LEVEL_INFO);
	LogComponentEnable("UdpEchoServerApplication",LOG_LEVEL_INFO);

	CommandLine cmd;
	cmd.Parse(argc,argv);

	NS_LOG_INFO("Create some Nodes:");
	NodeContainer nodes;
	nodes.Create(9);

        MobilityHelper mobility;
	/*mobility.SetPositionAllocator("ns3::GridPositionAllocator",
			              "MinX",DoubleValue(0.0),
				      "MinY",DoubleValue(0.0)
				      "DeltaX",DoubleValue(10.0),
				      "DeltaY",DoubleValue(10.0),
				      //"GirdWidth",UintegerValue(10),//这儿出错了！！！
				      "LayoutType",StringValue("RowFirst")
	                              );*/

	mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",//设置除了节点6以外的点为STA节点（在此设想为highway中的汽车）
			                   "Bounds",RectangleValue(Rectangle(-200,200,-200,200)));
	mobility.Install(nodes.Get(0));
	mobility.Install(nodes.Get(1));
	mobility.Install(nodes.Get(2));
	mobility.Install(nodes.Get(3));
	mobility.Install(nodes.Get(4));
	mobility.Install(nodes.Get(5));
	mobility.Install(nodes.Get(7));
	mobility.Install(nodes.Get(8));

	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");//将节点6设置为AP节点，运动模型为固定不动（在highway中视为RSU）
	mobility.Install(nodes.Get(6)); 
   
        for (NodeContainer::Iterator j = nodes.Begin ();
       j != nodes.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Vector pos = position->GetPosition ();
      std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
    }

	NodeContainer n0n2=NodeContainer(nodes.Get(0),nodes.Get(2));
	NodeContainer n1n2=NodeContainer(nodes.Get(1),nodes.Get(2));
	NodeContainer n2n3=NodeContainer(nodes.Get(2),nodes.Get(3));
	NodeContainer n2n4=NodeContainer(nodes.Get(2),nodes.Get(4));

	NodeContainer n5n8=NodeContainer(nodes.Get(5),nodes.Get(8));
	NodeContainer n5n7=NodeContainer(nodes.Get(5),nodes.Get(7));

	NodeContainer n4n6n5=NodeContainer(nodes.Get(4),nodes.Get(6),nodes.Get(5));

	InternetStackHelper stacks;
	stacks.Install(nodes);

	std::cout<<"Create Channel:"<<std::endl;
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate",StringValue("5Mbps"));
	p2p.SetChannelAttribute("Delay",StringValue("5ms"));
	NetDeviceContainer d0d2=p2p.Install(n0n2);
	NetDeviceContainer d1d2=p2p.Install(n1n2);

	p2p.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
	p2p.SetChannelAttribute("Delay",StringValue("2ms"));
	NetDeviceContainer d2d4=p2p.Install(n2n4);
	NetDeviceContainer d2d3=p2p.Install(n2n3);

	p2p.SetDeviceAttribute("DataRate",StringValue("5Mbps"));
	p2p.SetChannelAttribute("Delay",StringValue("2ms"));
	NetDeviceContainer d5d8=p2p.Install(n5n8);
	NetDeviceContainer d5d7=p2p.Install(n5n7);

	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate",StringValue("100Mbps"));
	csma.SetChannelAttribute("Delay",StringValue("5ms"));
	NetDeviceContainer d4d6d5=csma.Install(n4n6n5);
  

	std::cout<<"Staring assign IP address:"<<std::endl;
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0","255.255.255.0");
	Ipv4InterfaceContainer i0i2=ipv4.Assign(d0d2);
	ipv4.SetBase("10.1.2.0","255.255.255.0");
	ipv4.Assign(d1d2);
	ipv4.SetBase("10.1.3.0","255.255.255.0");
	ipv4.Assign(d2d3);
	ipv4.SetBase("10.1.4.0","255.255.255.0");
	ipv4.Assign(d2d4);
	ipv4.SetBase("10.1.5.0","255.255.255.0");
	Ipv4InterfaceContainer i5i7= ipv4.Assign(d5d7);
    ipv4.SetBase("10.1.6.0","255.255.255.0");
	Ipv4InterfaceContainer i5i8=ipv4.Assign(d5d8);
    ipv4.SetBase("10.1.7.0","255.255.255.0");
    ipv4.Assign(d4d6d5);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    NS_LOG_INFO("Creating Application:");

    uint16_t port1 = 9000;   // Discard port (RFC 863)
    OnOffHelper onoff ("ns3::UdpSocketFactory",
	               InetSocketAddress (i0i2.GetAddress (0), port1));
    onoff.SetConstantRate(DataRate("300bps"));
    onoff.SetAttribute("PacketSize",UintegerValue(50));
  
    for(int num=0;num<9;num++)
    {
       if(num!=5||num!=7)
      { 
           ApplicationContainer apps=onoff.Install(nodes.Get(num));
           apps.Start(Seconds(1.0));
           apps.Stop(Seconds(20.0));
      }
    }

    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream=ascii.CreateFileStream("easy.tr");
    p2p.EnableAsciiAll(stream);
    csma.EnableAsciiAll(stream);

    p2p.EnablePcapAll("easy");
    csma.EnablePcapAll("easy",false);

    std::cout<<"Run Simulation:"<<std::endl;
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done!!!");

    return 0;
}


