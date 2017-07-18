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
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/udp-client.h"
#include "ns3/seq-ts-header.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"

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

void ReceivePacket(Ptr<Socket> socket)
{
    while(socket->Recv()){
      //NS_LOG_UNCOND ("Received one packet!");
      std::cout << Simulator::Now().GetSeconds() << " Received packet: " << ++counter << std::endl;

    }
}

int main(int argc, char* argv[])
{
	//Set all the default time unit is nanosecond
	Time::SetResolution(Time::NS);

	string phyMode("HtMcs0");
	uint32_t packetSize = 1024;
	uint32_t numPackets = 20;
	uint32_t numNodes = 25;
    //waiting time before sending next packet
	double interval = 0.10;   
	bool TRACING_TR = false;
	bool TRACING_PCAP = true;
	bool GENERATE_XML = true;
	bool ENABLE_LOG_COMPONENT = true;
	bool ENABLE_LOG_INFO = true;
	bool ENABLE_ALL = false;
    bool TRACING_ROUTE = true;

    string chnDelayModel("ConstantSpeedPropagationDelayModel");
    string chnLossModel("FriisPropagationLossModel");

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
        NS_LOG_INFO("Set physical channel ...");
    Ptr<YansWifiChannel> ptr_yansWifiChannel = CreateObject<YansWifiChannel>();
    //Config::SetDefault("ns3::YansWifiChannel::PropagationLossModel",StringValue(chnLossModel));
    //Config::SetDefault("ns3::YansWifiChannel::PropagationDelayModel",StringValue(chnDelayModel));
    Ptr<ConstantSpeedPropagationDelayModel> ptr_ppgdelayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    Ptr<FriisPropagationLossModel> ptr_ppglossModel = CreateObject<FriisPropagationLossModel>();
    ptr_yansWifiChannel->SetPropagationDelayModel(ptr_ppgdelayModel);
    ptr_yansWifiChannel->SetPropagationLossModel(ptr_ppglossModel);

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set phy layer ...");
    Ptr<YansWifiPhy> ptr_yansWifiPhy = CreateObject<YansWifiPhy>();
    ptr_yansWifiPhy->SetChannel(ptr_yansWifiChannel);
    ptr_yansWifiPhy->SetTxPowerStart(double(5));
    ptr_yansWifiPhy->SetTxPowerEnd(double(5));
    ptr_yansWifiPhy->SetEdThreshold(double(-83.0));
    ptr_yansWifiPhy->ConfigureStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set wifi remote manager ...");
    Ptr<ConstantRateWifiManager> ptr_crwm = CreateObject<ConstantRateWifiManager>();
    ptr_crwm->SetFragmentationThreshold(2200);
    ptr_crwm->SetRtsCtsThreshold(2200);
    ptr_crwm->SetHtSupported(true);
    Config::SetDefault("ns3::ConstantRateWifiManager::DataMode",StringValue(phyMode));
    Config::SetDefault("ns3::ConstantRateWifiManager::ControlMode",StringValue(phyMode));
    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set wifi mac ...");
    Ptr<AdhocWifiMac> ptr_wifiMac = CreateObject<AdhocWifiMac>();
    ptr_wifiMac->SetWifiPhy(ptr_yansWifiPhy);
    ptr_wifiMac->SetWifiRemoteStationManager(ptr_crwm);

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set wifi netdevice ...");
    Ptr<WifiNetDevice> ptr_wifiNetDevice = CreateObject<WifiNetDevice>();
    ptr_wifiNetDevice->SetMac(ptr_wifiMac);

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Creating nodes and install netdevice.");
    NodeContainer nodesContainer;
    nodesContainer.Create(numNodes);
    NetDeviceContainer netdeviceCtn;
    for(NodeContainer::Iterator i=nodesContainer.Begin(); i != nodesContainer.End(); ++i){
        (*i)->AddDevice(ptr_wifiNetDevice);
        netdeviceCtn.Add( (*i)->GetDevice(0) );
    }
    //cout<<"Number of netdevices: "<<netdeviceCtn.GetN()<<endl;

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set position allocator and mobility model ...");
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
            "MinX", DoubleValue(0),
            "MinY", DoubleValue(0),
            "DeltaX", DoubleValue(100),
            "DeltaY", DoubleValue(100),
            "GridWidth", UintegerValue(10),
            "LayoutType", StringValue("RowFirst")
            );
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodesContainer);

    // Enable OLSR
    OlsrHelper olsr;
    Ipv4StaticRoutingHelper staticRouting;

    Ipv4ListRoutingHelper listRouting;
    //The second parameter indicates the priority of routing protocol.
    //the first protocol(index 0) the highest priority, the next one
    //(index 1) the second highest priority, and so on.
    listRouting.Add(staticRouting, 0);
    listRouting.Add(olsr, 10);

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Install routing protocol ...");
    InternetStackHelper internetStack;
    internetStack.SetRoutingHelper(listRouting);
    internetStack.Install(nodesContainer);

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Assign ip address ...");
    Ipv4AddressHelper ipv4Address;
    ipv4Address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipv4IntfCtn;
    ipv4IntfCtn = ipv4Address.Assign(netdeviceCtn);
    //cout<<"Number of ipv4 interface: "<<ipv4IntfCtn.GetN()<<endl;

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
    InetSocketAddress remote = InetSocketAddress(ipv4IntfCtn.GetAddress(sinkNodeId, 0), 80);
    source->Connect(remote);

    string fileNameRoot("simpleWirelessTcp");
    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set pcap trace nodes ... ");
    if(TRACING_PCAP){
        //PcapHelperForDevice pcapHelperForDevice;
        //Ptr<WifiNetDevice> ptr_wifiNetDev = DynamicCast<WifiNetDevice>( nodesContainer.Get(sourceNodeId)->GetDevice(0) );
        //pcapHelperForDevice.EnablePcap(fileNameRoot, ptr_wifiNetDev );
        //ptr_wifiNetDev = DynamicCast<WifiNetDevice>( nodesContainer.Get(sinkNodeId)->GetDevice(0) );
        //pcapHelperForDevice.EnablePcap(fileNameRoot, ptr_wifiNetDev );
        //PcapHelper pcapHelper;
        //vector<Ptr<WifiPhy>> v_wifiPhy;
        //v_wifiPhy.push_back(ptr_wifiNetDev->GetPhy());
        //v_wifiPhy.push_back(ptr_wifiNetDev->GetPhy());
        //vector<Ptr<WifiPhy>>::iterator i;
        //Ptr<PcapFileWrapper> file = pcapHelper.CreateFile(fileNameRoot, ios::out, PcapHelper::DLT_IEEE802_11_RADIO);
        //for(i = v_wifiPhy.begin(); i != v_wifiPhy.end(); ++i){
        //    (*i)->TraceConnectWithoutContext("MonitorSinfferTx", MakeBoundCallBack(&YansWifiPhyHelper::PcapSniffTxEvent, fileNameRoot));
        //}
    }

    if(ENABLE_LOG_INFO)
        NS_LOG_INFO("Set route trace ...");
    if(TRACING_ROUTE){
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("./routes/"+fileNameRoot+".routes", ios::out);
        olsr.PrintRoutingTableAllEvery (Seconds(2), routingStream);
        Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper>("./routes/"+fileNameRoot+".neighbors", ios::out);
        olsr.PrintRoutingTableAllEvery (Seconds(2), neighborStream);
    }


    return 0;
} 
