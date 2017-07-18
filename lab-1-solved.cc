 // Network topology
//
//       n1 ------ n2------n3
//            p2p
//
// - UDP flows from n1 to n2
 
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

#include <time.h>
#include <stdlib.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab1");

int main(int argc, char *argv[]) {
	double lat = 2.0;
	uint64_t rate = 5000000; // Data rate in bps
	double interval = 0.05;

	CommandLine cmd;
	cmd.AddValue("latency", "P2P link Latency in miliseconds", lat);
	cmd.AddValue("rate", "P2P data rate in bps", rate);
	cmd.AddValue("interval", "UDP client packet interval", interval);

	cmd.Parse(argc, argv);

//
// Enable logging for UdpClient and UdpServer
//
	LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
	LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

//
// Explicitly create the nodes required by the topology (shown above).
//
	NS_LOG_INFO("Create nodes.");
	NodeContainer p2pNodes;
	p2pNodes.Create(3);

	NS_LOG_INFO("Create channels.");
//
// Explicitly create the channels required by the topology (shown above).
//
	PointToPointHelper p2p;
	p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(lat)));
	p2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate(rate)));
	p2p.SetDeviceAttribute("Mtu", UintegerValue(1400));
	NetDeviceContainer dev = p2p.Install(p2pNodes.Get(0), p2pNodes.Get(1));
	NetDeviceContainer dev2 = p2p.Install(p2pNodes.Get(1), p2pNodes.Get(2));

	MobilityHelper mobility;

	mobility.SetPositionAllocator(
			"ns3::GridPositionAllocator", //位置分配器类型：网格位置分配器，分配每个节点的初始位置
			"MinX", DoubleValue(0.0), "MinY", DoubleValue(0.0), "DeltaX",
			DoubleValue(5.0), //x轴的间隔
			"DeltaY", DoubleValue(10.0), "GridWidth", UintegerValue(3), //网格宽度
			"LayoutType", StringValue("RowFirst")); //布局类型

	mobility.SetMobilityModel(
			"ns3::RandomWalk2dMobilityModel", //运动模型：在固定范围内节点以任意速度任意方向移动
			"Bounds", RectangleValue(Rectangle(-200, 200, -200, 200)), "Time",
			TimeValue(Seconds(0.0)), "Speed",
			StringValue("ns3::ConstantRandomVariable[Constant=5.0]")); //移动范围设置
	mobility.Install(p2pNodes.Get(1)); //将移动模型安装在STA节点上
	mobility.Install(p2pNodes.Get(2));

	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); //设置AP节点的移动模型：固定
	mobility.Install(p2pNodes.Get(0)); //在AP节点上安装移动模型

// We've got the "hardware" in place.  Now we need to add IP addresses.

// Install Internet Stack
	InternetStackHelper internet;
	internet.Install(p2pNodes);
	Ipv4AddressHelper ipv4;

	NS_LOG_INFO("Assign IP Addresses.");
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer i = ipv4.Assign(dev);

	ipv4.SetBase("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer i2 = ipv4.Assign(dev2);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	NS_LOG_INFO("Create Applications.");
//
// Create one udpServer application on node one.
//
	uint16_t port1 = 8000; // Need different port numbers to ensure there is no conflict
	uint16_t port2 = 8001;
	UdpServerHelper server1(port1);
	UdpServerHelper server2(port2);
	ApplicationContainer apps;
	apps = server1.Install(p2pNodes.Get(2));
	apps = server2.Install(p2pNodes.Get(2));

	apps.Start(Seconds(1.0));
	apps.Stop(Seconds(20.0));

//
// Create one UdpClient application to send UDP datagrams from node zero to
// node one.
//
	uint32_t MaxPacketSize = 1024;
	Time interPacketInterval = Seconds(interval);
	uint32_t maxPacketCount = 1024;
	UdpClientHelper client1(i2.GetAddress(1), port1);
	UdpClientHelper client2(i2.GetAddress(1), port2);

	client1.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	client1.SetAttribute("Interval", TimeValue(interPacketInterval));
	client1.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));

	client2.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	client2.SetAttribute("Interval", TimeValue(interPacketInterval));
	client2.SetAttribute("PacketSize", UintegerValue(MaxPacketSize));

	apps = client1.Install(p2pNodes.Get(0));
	apps = client2.Install(p2pNodes.Get(0));

	apps.Start(Seconds(1.0));
	apps.Stop(Seconds(20.0));

//
// Tracing
//
	AsciiTraceHelper ascii;
	p2p.EnableAscii(ascii.CreateFileStream("lab-1.tr"), dev);
	p2p.EnablePcap("lab-1", dev, false);

//
// Calculate Throughput using Flowmonitor
//
	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor ;
	/*
	 *　下面三句和这一句效果一样
	 *　monitor=flowmon.InstallAll();//监测所有的节点
	 */
	monitor=flowmon.Install(p2pNodes.Get(0));//监测节点０
	monitor=flowmon.Install(p2pNodes.Get(1));//监测节点１
	monitor=flowmon.Install(p2pNodes.Get(2));//监测节点２

// Now, do the actual simulation.
	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(21.0));
	Simulator::Run();

		monitor->CheckForLostPackets();//监测传输中丢失的包

		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(
				flowmon.GetClassifier());
		std::map<FlowId, FlowMonitor::FlowStats> stats =
				monitor->GetFlowStats();
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
				stats.begin(); i != stats.end(); ++i) {//stats.begin()表示在统计的监测点中开始处；
			//stats.end()表示监测结束处
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);//
			if ((t.sourceAddress == "10.1.1.1"
					&& t.destinationAddress == "10.1.2.2")) {

				std::cout << "Flow " << i->first << " (" << t.sourceAddress
						<< " -> " << t.destinationAddress << ")\n";
				std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
				std::cout << "  Rx Bytes:   " << i->second.rxBytes 
						<< "\n";
				std::cout << "最后接收包的时间："
						<< i->second.timeLastRxPacket.GetSeconds()
						<< " ,开始接收包的时间： "
						<< i->second.timeFirstTxPacket.GetSeconds()
						<< std::endl;
				std::cout << "  Throughput: "
						<< (long double) (i->second.rxBytes * 8.0 
								/ (i->second.timeLastRxPacket.GetSeconds()
										- i->second.timeFirstTxPacket.GetSeconds())
								/ 1024 / 1024) << " Mbps\n";
			}

		}
	monitor->SerializeToXmlFile("lab-1.flowmon", true, true);

	AnimationInterface anim("lab-1-solved.xml");
	anim.EnablePacketMetadata(true); // Optional

	Simulator::Destroy();
	NS_LOG_INFO("Done.");
}
