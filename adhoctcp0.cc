#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/gnuplot.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/aodv-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/udp-client.h"
#include "ns3/seq-ts-header.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

//Network topology
//
//   S     *     *     *     *
//
//
//   *     *     *     *     *
//
//
//   *     *   	 *     *     *
//
//
//   *     *   	 *     *     *
//
//
//   *     *   	 *     *     D

using namespace ns3;
using namespace std;

int counter = 0;
int m_sent = 0;

NS_LOG_COMPONENT_DEFINE("SimpleWirelessTcp");

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
{
	double localThrou=0;
	std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
	Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
	{
		//Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
		//std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
		//std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
		//std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
		//std::cout<<"Duration		: "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
		//std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
		//std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
		localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
		// updata gnuplot data
		DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
		//std::cout<<"---------------------------------------------------------------------------"<<std::endl;
	}
	
	Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon,DataSet);
	
	//if(flowToXml)
	//{
	flowMon->SerializeToXmlFile ("./adhoctcp/ThroughputMonitor.xml", true, true);
	//}

}

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      //NS_LOG_UNCOND ("Received one packet!");
      //std::cout << Simulator::Now().GetSeconds() << " Received packet: " << ++counter << std::endl;
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval )
{
    //std::cout<<"Generate traffic at:"<<Simulator::Now()<<std::endl;
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  Ptr<Packet> p = Create<Packet> (pktSize);
  p->AddHeader (seqTs);
  
  if (pktCount > 0)
    {
      //socket->Send (Create<Packet> (pktSize));
      socket->Send (p);
      ++m_sent;
      Simulator::Schedule (pktInterval, &GenerateTraffic, 
                           socket, pktSize,pktCount-1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

int main(int argc, char* argv[])
{
	//Set all the default time unit is nanosecond
	Time::SetResolution(Time::NS);

	string phyMode("ErpOfdmRate6Mbps");
	uint32_t packetSize = 1024;
	uint32_t numPackets = 200;
	uint32_t numNodes = 25;
    //waiting time before sending next packet
	double interval = 0.10;   
	bool TRACING_TR = false;
	bool TRACING_PCAP = true;
	bool GENERATE_XML = true;
	bool ENABLE_LOG_COMPONENT = true;
	bool ENABLE_LOG_INFO = true;
	bool ENABLE_ALL = false;
    bool TRACING_ROUTE = false;

    string chnDelayModel("ns3::ConstantSpeedPropagationDelayModel");
    string chnLossModel("ns3::FriisPropagationLossModel");

	CommandLine cmd;

	cmd.AddValue ("phyMode", "Wifi phy mode, format: string", phyMode);
    cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue ("numPackets", "number of packets generated", numPackets);
    cmd.AddValue ("interval", "interval (seconds) between packets", interval);
    cmd.AddValue ("enable_log_component", "turn on all log components", ENABLE_LOG_COMPONENT);
    cmd.AddValue ("enable_log_info", "turn on all log components", ENABLE_LOG_INFO);
    cmd.AddValue ("tracing_tr", "turn on ascii tracing", TRACING_TR);
    cmd.AddValue ("tracing_pcap", "turn on pcap tracing", TRACING_PCAP);
    cmd.AddValue ("generate_xml", "turn on xml generator", GENERATE_XML);
    cmd.AddValue ("enable_all", "turn on all asistant output", ENABLE_ALL);
    cmd.AddValue ("numNodes", "number of nodes", numNodes);
    cmd.AddValue ("tracing_route", "turn on route tracing", TRACING_ROUTE);

    cmd.Parse (argc, argv);

    if(ENABLE_LOG_COMPONENT){
        LogComponentEnable("SimpleWirelessTcp", LOG_LEVEL_INFO);
    }

    
    // disable fragmentation for frames below 2200 bytes
    //Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    //// turn off RTS/CTS for frames below 2200 bytes
    //Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set physical channel and phy layer ...");
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.Set("TxPowerStart", DoubleValue(5));  //Minimum available transmission level (dbm)
    phy.Set("TxPowerEnd", DoubleValue(5));  //Maximum available transmission level (dbm)
    phy.Set("EnergyDetectionThreshold", DoubleValue(-83.0) ); //Receive Sensivity: -85/-83 dbm
    //ns-3 supports RadioTap and Prism tracing extensions for 802.11b
    //phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 
    phy.SetChannel(channel.Create());

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set wifi mac ...");
    NqosWifiMacHelper mac = NqosWifiMacHelper::Default();
    mac.SetType("ns3::AdhocWifiMac");

    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211g);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
            "DataMode", StringValue(phyMode),
            "ControlMode",StringValue(phyMode),
            "RtsCtsThreshold", UintegerValue(2200),
            "FragmentationThreshold",UintegerValue(2200),
            "NonUnicastMode", StringValue(phyMode) );
    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Creating nodes and install wifi netdevice ...");
    NodeContainer nodesContainer;
    nodesContainer.Create(numNodes);
    NetDeviceContainer netDeviceContainer = wifi.Install(phy, mac, nodesContainer);

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set position allocator and mobility model ...");
    MobilityHelper mobility;
    ///////////////////////////////////////
    	Ptr<UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable>();
	  randomizer->SetAttribute("Min", DoubleValue(0.0));
	  randomizer->SetAttribute("Max", DoubleValue(500.0));
	  
	  mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator",
                                "X", PointerValue(randomizer),
                                "Y", PointerValue(randomizer));
	  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("300s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|500|0|500"));
	///////////////////////////////////////
    // mobility.SetPositionAllocator("ns3::GridPositionAllocator",
    //         "MinX", DoubleValue(0),
    //         "MinY", DoubleValue(0),
    //         "DeltaX", DoubleValue(20),
    //         "DeltaY", DoubleValue(20),
    //         "GridWidth", UintegerValue(5),
    //         "LayoutType", StringValue("RowFirst")
    //         );
    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodesContainer);

    // Enable OLSR
    OlsrHelper olsr;
    AodvHelper aodv;
    Ipv4StaticRoutingHelper staticRouting;

    Ipv4ListRoutingHelper listRouting;
    //The second parameter indicates the priority of routing protocol.
    //the first protocol(index 0) the highest priority, the next one
    //(index 1) the second highest priority, and so on.
    listRouting.Add(staticRouting, 0);
    listRouting.Add(aodv, 10);

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Install routing protocol ...");
    InternetStackHelper internetStack;
    internetStack.SetRoutingHelper(listRouting);
    internetStack.Install(nodesContainer);

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Assign ip address ...");
    Ipv4AddressHelper ipv4Address;
    ipv4Address.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ipv4IntfContainer;
    ipv4IntfContainer = ipv4Address.Assign(netDeviceContainer);
    //cout<<"Number of ipv4 interface: "<<ipv4IntfContainer.GetN()<<endl;

    // Convert to time object
    Time interPacketInterval = Seconds (interval);

	uint32_t sourceNodeId = 0;
	uint32_t sinkNodeId = numNodes - 1;
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Creating udp server and binding with local netdevices ...");
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket (nodesContainer.Get(sinkNodeId), tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny(), 80);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Creating udp client and connecting to server ...");
    Ptr<Socket> source = Socket::CreateSocket(nodesContainer.Get(sourceNodeId), tid);
    InetSocketAddress remote = InetSocketAddress(ipv4IntfContainer.GetAddress(sinkNodeId, 0), 80);
    source->Connect(remote);

    string fileNameRoot("adhoctcp");
    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set pcap trace nodes ... ");
    if(TRACING_PCAP){
        phy.EnablePcap("./pcap/"+fileNameRoot, netDeviceContainer.Get(sourceNodeId));
        phy.EnablePcap("./pcap/"+fileNameRoot, netDeviceContainer.Get(sinkNodeId));
    }

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set route trace ...");
    if(TRACING_ROUTE){
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("./routes/"+fileNameRoot+".routes", ios::out);
        olsr.PrintRoutingTableAllEvery (Seconds(2), routingStream);
        Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper>("./routes/"+fileNameRoot+".neighbors", ios::out);
        olsr.PrintRoutingTableAllEvery (Seconds(2), neighborStream);
    }

    AnimationInterface anim("xml/adhoctcp.xml");
    //anim.UpdateNodeColor()
    //for(int i=0; i<5; i++){
    //    for(int j = 0; j<5; j++){
    //        AnimationInterface::SetConstantPosition(nodesContainer.Get(i*5+j), j*45, i*45);
    //    }
    //}

    //Give OLSR time to converge -- 30 seconds perhaps
    Simulator::Schedule(Seconds(30.0), &GenerateTraffic,
            source, packetSize, numPackets, interPacketInterval);
// Gnuplot parameters      
  std::string fileNameWithNoExtension = "./adhoctcp/FlowVSThroughput_";
  std::string graphicsFileName        = fileNameWithNoExtension + ".png";
  std::string plotFileName            = fileNameWithNoExtension + ".plt";
  std::string plotTitle               = "Flow vs Throughput";
  std::string dataTitle               = "Throughput";

  // Instantiate the plot and set its title.
  Gnuplot gnuplot (graphicsFileName);
  gnuplot.SetTitle (plotTitle);

  // Make the graphics file, which the plot file will be when it
  // is used with Gnuplot, be a PNG file.
  gnuplot.SetTerminal ("png");

  // Set the labels for each axis.
  gnuplot.SetLegend ("Flow", "Throughput");
     
  Gnuplot2dDataset dataset;
  dataset.SetTitle (dataTitle);
  dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);
  
  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  
  // call the flow monitor function
  ThroughputMonitor(&flowHelper, flowMonitor, dataset);

  //Simulator::Stop (Seconds(4000.0));
  Simulator::Stop (Seconds(50.0)); // for testing/debugging only
  Simulator::Run ();
  
  //Gnuplot ...continued
  gnuplot.AddDataset (dataset);
  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str());
  // Write the plot file.
  gnuplot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();
  
  // Print per flow statistics
  flowMonitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();
  /*
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
  {
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);
	  NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
	  NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
	  NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
	  NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
  }       
  */
  flowMonitor->SerializeToXmlFile("./adhoctcp/myManet.xml", true, true);
  
  Simulator::Destroy ();

    return 0;
} 
