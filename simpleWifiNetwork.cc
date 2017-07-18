#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/config-store.h"
#include "ns3/propagation-delay-model.h"
#include <string.h>

// Default Network Topology

// There are only 3 wifi nodes in this simple network.

// The program is to realize the function that sending 
// data from n1 to n2, transmiting by ap, three nodes are
// in the same network, their ip address are assigned 
// as following.

//    n1             ap            n2
//     *              *             *
//    1.1            1.2          1.3
//
//              192.168.1.0
using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("SimpleWifiNetwork");

void Sta0DevTxTrace(std::string context, Ptr<const Packet> p)
{
    std::cout<<Simulator::Now().As(Time::S)<<std::endl;
    std::cout<<context<<", TX p:"<<*p<<std::endl;
}


int main(int argc, char* argv[])
{
	Time::SetResolution(Time::NS);

	bool verbose = true;
    uint32_t nWifi = 3;
    bool tracing = true;

    CommandLine cmd;
    cmd.AddValue("verbose","Boolean value, enable log component when it is true", verbose);
    cmd.AddValue("nWifi", "Number of wifi nodes", nWifi);
    cmd.AddValue("tracing", "Boolean value, enable tracing function when it is true",tracing);

    if(verbose){
        LogComponentEnable("SimpleWifiNetwork", LOG_LEVEL_INFO);
        //LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
        //LogComponentEnable("ArfWifiManager", LOG_LEVEL_INFO);
        //LogComponentEnable("Address", LOG_LEVEL_INFO);
        //LogComponentEnable("AnimationInterface", LOG_LEVEL_INFO);
        //LogComponentEnable("ApWifiMac", LOG_LEVEL_INFO);
        //LogComponentEnable("GlobalRouter", LOG_LEVEL_INFO);
        //LogComponentEnable("InetSocketAddress", LOG_LEVEL_INFO);
        //LogComponentEnable("IpLProtocol", LOG_LEVEL_INFO);
        //LogComponentEnable("Ipv4Address", LOG_LEVEL_INFO);
        //LogComponentEnable("Ipv4GlobalRouting", LOG_LEVEL_INFO);
    }
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue(true));

    NS_LOG_INFO("Create nodes ...");
    NodeContainer nodes_container;
    nodes_container.Create(3);

	NS_LOG_INFO("Set mobility model ...");
	MobilityHelper mobility;
	//mobility.SetPositionAllocator("ns3::GridPositionAllocator",
	//        "MinX", DoubleValue(10.0),
	//        "MinY", DoubleValue(10.0),
	//        "DeltaX", DoubleValue(40.0),
	//        "DeltaY", DoubleValue(10.0),
	//        "GridWidth", UintegerValue(40),
	//        "LayoutType", StringValue("RowFirst"));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(nodes_container);
	Ptr<ConstantPositionMobilityModel> ptr_cpmm = CreateObject<
			ConstantPositionMobilityModel>();
//	string ptr_mm = mobility.GetMobilityModelType();
//	cout << "mobility model is:" << ptr_mm << endl;


    NS_LOG_INFO("Set phy and channel ...");
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	Ptr<YansWifiChannel> ptr_channel = channel.Create();
	Ptr<ConstantSpeedPropagationDelayModel> ptr_propagation_delay_model =
			CreateObject<ConstantSpeedPropagationDelayModel>();
	cout << "propagation model's delay is: "
			<< ptr_propagation_delay_model->GetDelay(ptr_cpmm, ptr_cpmm).As(
					Time::NS) << endl;
	ptr_channel->SetPropagationDelayModel(ptr_propagation_delay_model);
	phy.SetChannel(ptr_channel);

    Ssid ssid = Ssid("ns3-ssid");

    WifiHelper wifi;
	wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
			StringValue("HtMcs0"));

	WifiMacHelper mac;
	mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));


    NS_LOG_INFO("Install netdevices ...");
    NetDeviceContainer netdev;
	netdev.Add(wifi.Install(phy, mac, nodes_container.Get(0)));

	//Ptr<WifiNetDevice> ptrWifiNetDev = DynamicCast<WifiNetDevice>(netdev.Get(0));
	//ptrWifiNetDev->GetMac()->SetAckTimeout(Time(NanoSeconds(1000000)));

	WifiMacHelper mac_ap;
	mac_ap.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(ssid));
	netdev.Add(wifi.Install(phy, mac_ap, nodes_container.Get(1)));
	netdev.Add(wifi.Install(phy, mac, nodes_container.Get(2)));

    NS_LOG_INFO("Install internet stack ...");
    InternetStackHelper stack;
    stack.Install(nodes_container);

    NS_LOG_INFO("Set nodes' address ...");
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer intfc;
    intfc.Add(address.Assign(netdev.Get(0)));
    intfc.Add(address.Assign(netdev.Get(1)));
    intfc.Add(address.Assign(netdev.Get(2)));


    NS_LOG_INFO("Set nodes' position ...");
    AnimationInterface anim("xml/simpleNetwork.xml");
    anim.SetConstantPosition(nodes_container.Get(0), 10,10);
    anim.SetConstantPosition(nodes_container.Get(1), 30,20);
    anim.SetConstantPosition(nodes_container.Get(2), 500,300);
    anim.UpdateNodeColor(nodes_container.Get(0), 0, 255, 0);
    anim.UpdateNodeColor(nodes_container.Get(1), 0, 0, 255);

    NS_LOG_INFO("Creating onoff application ...");
    uint16_t port = 1024;
    OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(intfc.GetAddress(2),port)));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
	onoff.SetAttribute("OffTime",
			StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
	onoff.SetConstantRate(DataRate("5Kbps"), 512);

    ApplicationContainer app_onoff = onoff.Install(nodes_container.Get(0));
    app_onoff.Start(Seconds(1.0));
    app_onoff.Stop(Seconds(10.0));

    NS_LOG_INFO("Creating sink application ...");
    PacketSinkHelper sink("ns3::TcpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    ApplicationContainer app_sink = sink.Install(nodes_container.Get(2));
    app_sink.Start(Seconds(0.0));
    app_sink.Stop(Seconds(10.0));

    NS_LOG_INFO("Enable tracing ...");
    if(tracing){
		phy.EnablePcapAll("pcap/simpleNetwork");
    }

	for (int i = 0; i < nWifi; i++) {
		Ptr<Ipv4> ptr_ipv4 = nodes_container.Get(i)->GetObject<Ipv4>();
		Ptr<NetDevice> ptr_netdev = nodes_container.Get(i)->GetDevice(0);
		Ptr<WifiNetDevice> ptr_wifi_netdev = DynamicCast<WifiNetDevice>(
				ptr_netdev);
		Ptr<WifiMac> ptr_wifimac = ptr_wifi_netdev->GetMac();
//		ptr_wifimac->SetAckTimeout(Time(NanoSeconds(100000)));
		Ptr<WifiPhy> ptr_wifiphy = ptr_wifi_netdev->GetPhy();
//		cout << "Antenna's receive number is: "
//				<< ptr_wifiphy->GetNumberOfReceiveAntennas() << endl;
//		cout << "Antenna's transmit number is: "
//				<< ptr_wifiphy->GetNumberOfTransmitAntennas() << endl;
//		cout << "channel number is:" << ptr_wifiphy->GetChannelNumber() << endl;
		cout << "wifi mac's ack timeout is:"
				<< ptr_wifimac->GetAckTimeout().As(Time::NS) << endl;
//		cout << "node " << i << "'s mac address is "
//				<< ptr_wifimac->GetAddress() << endl;
//		cout << "node " << i << "'s max propagation delay is: "
//				<< ptr_wifimac->GetMaxPropagationDelay().As(Time::NS)
//				<< endl;
//		cout << "node " << i << "'s slot is: "
//				<< ptr_wifimac->GetSlot().As(Time::NS) << endl;
//		cout << "node " << i << "'s ssid is: "
//				<< ptr_wifimac->GetSsid().PeekString()
//				<< endl << endl;

//        std::cout<<"node "<<i<<"'s address is "<<ptr_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
//        std::cout<<"node "<<i<<"'s netdevice's MTU is "<<ptr_netdev->GetMtu()<<std::endl;
//
//
	}
//
//
//    std::cout<<"Whether the program enables checksum(1:yes,0:no): "<<ns3::Node::ChecksumEnabled()<<std::endl;

//	Config::SetDefault("ns3::ConfigStore::Filename",
//			StringValue("output-attributes.xml"));
//	Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("Xml"));
//	Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
//	ConfigStore outputConfig;
//	outputConfig.ConfigureDefaults();
//	outputConfig.ConfigureAttributes();

	// Output config store to txt format
//	Config::SetDefault("ns3::ConfigStore::Filename",
//			StringValue("output-attributes.txt"));
//	Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
//	Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
//	ConfigStore outputConfig2;
//	outputConfig2.ConfigureDefaults();
//	outputConfig2.ConfigureAttributes();

    Config::Connect("/NodeList/0/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/MacTx", MakeCallback(&Sta0DevTxTrace));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Simulator::Stop(Seconds(5.0));

    NS_LOG_INFO("Simulator is running ...");
    Simulator::Run();
    Simulator::Destroy();
    return 0;

}
