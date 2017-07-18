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
#include "ns3/aodv-routing-protocol.h"
#include "ns3/aodv-helper.h"
#include <string.h>

// Default Network Topology

// There are only 3 wifi nodes in this simple network.

// The program is to realize the function that sending 
// data from n1 to n2, transmiting by ap, three nodes are
// in the same network, their ip address are assigned 
// as following.

//    n1             ap            ap           n2
//     *              *             *            *
//    1.2            1.1          2.1          2.2
//                   1.3          2.3             
//
//              192.168.1.0
using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("ApRelay");

void Sta0DevTxTrace(std::string context, Ptr<const Packet> p)
{
    std::cout<<Simulator::Now().As(Time::S)<<std::endl;
    std::cout<<context<<", TX p:"<<*p<<std::endl;
}


int main(int argc, char* argv[])
{
	Time::SetResolution(Time::NS);

	bool verbose = true;
    uint32_t nWifi = 4;
    bool tracing = true;

    CommandLine cmd;
    cmd.AddValue("verbose","Boolean value, enable log component when it is true", verbose);
    cmd.AddValue("nWifi", "Number of wifi nodes", nWifi);
    cmd.AddValue("tracing", "Boolean value, enable tracing function when it is true",tracing);

    if(verbose){
        LogComponentEnable("ApRelay", LOG_LEVEL_INFO);
        LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
        LogComponentEnable("ArfWifiManager", LOG_LEVEL_INFO);
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
    NodeContainer nc;
    nc.Create(nWifi);
    NodeContainer apnc = NodeContainer(nc.Get(1), nc.Get(2));
    NodeContainer stanc = NodeContainer(nc.Get(0), nc.Get(3));

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
	mobility.Install(nc);
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

    NqosWifiMacHelper adhoc_mac = NqosWifiMacHelper::Default();
    adhoc_mac.SetType("ns3::AdhocWifiMac", "Slot", StringValue("16us"));


    NS_LOG_INFO("Install netdevices ...");
    NetDeviceContainer apNetdev;
    NetDeviceContainer staNetdev;
    NetDeviceContainer adhocNetdev;

	//Ptr<WifiNetDevice> ptrWifiNetDev = DynamicCast<WifiNetDevice>(netdev.Get(0));
	//ptrWifiNetDev->GetMac()->SetAckTimeout(Time(NanoSeconds(1000000)));

	WifiMacHelper mac_ap;
	mac_ap.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(ssid));
	apNetdev.Add(wifi.Install(phy, mac_ap, apnc));
	staNetdev.Add(wifi.Install(phy, mac, stanc));
	//adhocNetdev.Add(wifi.Install(phy, adhoc_mac, apnc));

    NS_LOG_INFO("Install internet stack ...");
    InternetStackHelper staStack;
    InternetStackHelper apStack;
    staStack.Install(stanc);
    AodvHelper aodv;
    //apStack.SetRoutingHelper(aodv);
    apStack.Install(apnc);

    NS_LOG_INFO("Set nodes' address ...");
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer intfc;
    Ipv4InterfaceContainer apIntfc;
    Ipv4InterfaceContainer adhocIntfc;
    apIntfc.Add(address.Assign(apNetdev.Get(0)));
    intfc.Add(address.Assign(staNetdev.Get(0)));
    //address.SetBase("192.168.2.0", "255.255.255.0");
    apIntfc.Add(address.Assign(apNetdev.Get(1)));
    intfc.Add(address.Assign(staNetdev.Get(1)));
    //address.SetBase("192.168.3.0", "255.255.255.0");
    //adhocIntfc.Add(address.Assign(adhocNetdev.Get(0)));
    //adhocIntfc.Add(address.Assign(adhocNetdev.Get(1)));
    //intfc.Add(address.Assign(staNetdev.Get(1)));
    //adhocIntfc.Add(address.Assign(adhocNetdev.Get(0)));
    //adhocIntfc.Add(address.Assign(adhocNetdev.Get(1)));
    for(uint16_t i = 0; i < nc.GetN(); i++){
        cout<<"Node "<<i<<" ip addr: "<< nc.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal()<<endl;
    }
    //cout<<"Node 1 adhoc ip addr: "<< nc.Get(1)->GetObject<Ipv4>()->GetAddress(2, 0).GetLocal()<<endl;
    //cout<<"Node 2 adhoc ip addr: "<< nc.Get(2)->GetObject<Ipv4>()->GetAddress(2, 0).GetLocal()<<endl;


    NS_LOG_INFO("Set nodes' position ...");
    AnimationInterface anim("xml/simpleNetwork.xml");
    anim.SetConstantPosition(nc.Get(0), 10,10);
    anim.SetConstantPosition(nc.Get(1), 30,20);
    anim.SetConstantPosition(nc.Get(2), 50,20);
    anim.SetConstantPosition(nc.Get(3), 70,10);
    anim.UpdateNodeColor(nc.Get(0), 0, 255, 0);
    anim.UpdateNodeColor(nc.Get(3), 0, 0, 255);

    NS_LOG_INFO("Creating onoff application ...");
    uint16_t port = 1024;
    OnOffHelper onoff("ns3::TcpSocketFactory", Address(InetSocketAddress(intfc.GetAddress(1),port)));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
	onoff.SetAttribute("OffTime",
			StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
	onoff.SetConstantRate(DataRate("5Kbps"), 512);

    ApplicationContainer app_onoff = onoff.Install(nc.Get(0));
    app_onoff.Start(Seconds(1.0));
    app_onoff.Stop(Seconds(10.0));

    NS_LOG_INFO("Creating sink application ...");
    PacketSinkHelper sink("ns3::TcpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    ApplicationContainer app_sink = sink.Install(nc.Get(nWifi-1));
    app_sink.Start(Seconds(0.0));
    app_sink.Stop(Seconds(10.0));

    NS_LOG_INFO("Enable tracing ...");
    if(tracing){
		phy.EnablePcapAll("pcap/aprelay");
    }

	for (int i = 0; i < nWifi; i++) {
		Ptr<Ipv4> ptr_ipv4 = nc.Get(i)->GetObject<Ipv4>();
		Ptr<NetDevice> ptr_netdev = nc.Get(i)->GetDevice(0);
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
