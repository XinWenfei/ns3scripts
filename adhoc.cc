#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-routing-protocol.h"
#include "ns3/aodv-helper.h"
#include <iostream>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("AdHocExample");
void CourseChange(std::string context, Ptr<const MobilityModel> model)
{
    Vector position = model->GetPosition();
    
    NS_LOG_UNCOND (context << " X = " << position.x << ", Y = " << position.y 
                         << " Z = " << position.z);
}

int
main(int argc,char*argv[])
{
	Time::SetResolution (Time::NS);
	LogComponentEnable ("AdHocExample", LOG_LEVEL_INFO);
	LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
	LogComponentEnable ("TcpL4Protocol",LOG_LEVEL_INFO);	

	uint16_t nAdHoc=16;
	CommandLine cmd;
	cmd.AddValue ("nAdHoc", "Number of wifi ad devices", nAdHoc);
	cmd.Parse (argc,argv);
	
	NodeContainer AdHocNode;
	AdHocNode.Create(nAdHoc);

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.SetChannel (channel.Create());

	WifiHelper wifi;
	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager("ns3::AarfWifiManager");
	
	NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
	mac.SetType ("ns3::AdhocWifiMac", "Slot", StringValue("16us"));
	NqosWifiMacHelper mac_ap = NqosWifiMacHelper::Default ();
	mac.SetType ("ns3::ApWifiMac", "Slot", StringValue("16us"));
	
	
	NetDeviceContainer AdHocDevices;
	AdHocDevices = wifi.Install(phy,mac,AdHocNode);
	NetDeviceContainer ApDevices;
	ApDevices = wifi.Install(phy,mac_ap,AdHocNode);

	MobilityHelper mobility;
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
				"MinX", DoubleValue (0.0),
				"MinY",  DoubleValue (0.0),
				"DeltaX", DoubleValue (100.0),
				"DeltaY", DoubleValue (100.0),
				"GridWidth", UintegerValue (4),
				"LayoutType", StringValue ("RowFirst"));

	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel","Bounds", RectangleValue (Rectangle(-15000, 15000, -15000, 15000)));
	mobility.Install(AdHocNode);

	//InternetStackHelper Internet;
	//Internet.Install(AdHocNode);

	NS_LOG_INFO ("Enablig AODV routing on all adHoc nodes");
         
///
	AodvHelper aodv;
  	// you can configure AODV attributes here using aodv.Set(name, value)
  	InternetStackHelper stack;
  	stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  	stack.Install (AdHocNode);

	
///
	
	Ipv4AddressHelper address;
	address.SetBase("196.6.1.0","255.255.255.0");
	Ipv4InterfaceContainer AdHocIp;
	AdHocIp = address.Assign(AdHocDevices);



//应用程序  创建TCP接收机
	NS_LOG_INFO("Create Applications.");
	uint16_t port=9;
	OnOffHelper onOff("ns3::TcpSocketFactory",Address(InetSocketAddress(AdHocIp.GetAddress(15),port)));
	onOff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	onOff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	onOff.SetAttribute("PacketSize", UintegerValue(1000));
    onOff.SetAttribute("DataRate", StringValue("5kbps"));
    onOff.SetAttribute("MaxBytes", UintegerValue(100000));


	ApplicationContainer apps1= onOff.Install(AdHocNode);
	apps1.Start(Seconds(1.0));
	apps1.Stop(Seconds(5.0));

//采用PacketSink 拓扑器
	PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",Address(InetSocketAddress(Ipv4Address::GetAny(),port)));
	ApplicationContainer apps2 = sinkHelper.Install(AdHocNode.Get(15));

	apps2.Start(Seconds(0.0));
	apps2.Stop(Seconds(5.0));
	//Ipv4GlobalRoutingHelper::PopulateRoutingTables();

/*
// 生成 trace 文件 输出为ASCII的 trace 文件
	NS_LOG_INFO("Configure Tracing.");
	std::ofstream ascii;
	ascii.open("adHoc_30node_test.tr");
	YansWifiPhyHelper::EnableAsciiAll(ascii);
	YansWifiPhyHelper::EnablePcapAll ("adHoc_30node_test");
*/

	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(5.0));
	AnimationInterface anim("xml/adhoc.xml");

    std::ostringstream oss;
    oss << "/NodeList/" << AdHocNode.Get(nAdHoc - 5)->GetId()<<
           "/$ns3::MobilityModel/CourseChange";
    // Config::Connect(oss.str(), MakeCallback(&CourseChange));

	Simulator::Run();
	Simulator::Destroy();
return 0; 

}
