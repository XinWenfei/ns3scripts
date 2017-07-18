#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

// Default Network Topology
//
// Number of wifi or csma nodes can be increased up to 250
//                          |
//                 Rank 0   |   Rank 1
// -------------------------|----------------------------
//   Wifi 192.168.1.0
//                 AP
//  *    *    *    *
//  |    |    |    |    192.168.2.0
// n1   n2   n3   n4 ============== n5   n6   n7   n8
//                         csma      |    |    |    |
//                                   *    *    *    *
//                                   AP
//                                     Wifi 192.168.3.0

using namespace ns3;
// using namespace std;

NS_LOG_COMPONENT_DEFINE ("APCSMA");

int main(int argc, char *argv[])
{
    uint32_t nodeCount = 8;
    bool tracing = true;
    bool verbose = true;
    std::string phymode = "HtMcs0";
    
    if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("APCSMA", LOG_LEVEL_INFO);
    }

    NodeContainer nc;
    nc.Create(nodeCount);

    NS_LOG_INFO("Divide 8 nodes into 3 network");
    NodeContainer wn1nc, csmanc, wn2nc;
    uint16_t i=0;
    for(i=0; i<3; i++)
        wn1nc.Add(nc.Get(i));
    csmanc.Add(nc.Get(i));
    csmanc.Add(nc.Get(++i));
    for(++i; i<8; i++)
        wn2nc.Add(nc.Get(i));

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel( channel.Create() );
    phy.Set("ChannelNumber", UintegerValue(1));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
            "DataMode", StringValue(phymode),
            "ControlMode", StringValue(phymode));

    WifiMacHelper mac;
    Ssid ssid1 = Ssid("wn1");
    Ssid ssid2 = Ssid("wn2");

    NS_LOG_INFO("Set wireless network1's station mac info");
    mac.SetType("ns3::StaWifiMac",
            "Ssid", SsidValue(ssid1));
    NetDeviceContainer wn1ndc;
    wn1ndc = wifi.Install(phy, mac, wn1nc);

    NS_LOG_INFO("Set wireless network1's ap mac info");
    mac.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(ssid1));
    wn1ndc.Add( wifi.Install(phy, mac, csmanc.Get(0)));

    NS_LOG_INFO("Set wireless network2's ap mac info");
    YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy2 = YansWifiPhyHelper::Default();
    phy2.SetChannel( channel2.Create() );
    phy2.Set("ChannelNumber", UintegerValue(6));
    NetDeviceContainer wn2ndc;
    mac.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(ssid2));
    wn2ndc.Add( wifi.Install(phy2, mac, csmanc.Get(1)));

    NS_LOG_INFO("Set wireless network2's station mac info");
    mac.SetType("ns3::StaWifiMac",
            "Ssid", SsidValue(ssid2));
    for(i=0; i<wn2nc.GetN(); i++)
        wn2ndc.Add( wifi.Install(phy2, mac, wn2nc.Get(i)) );
    
    NS_LOG_INFO("Set csma network's channel");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmandc;
    csmandc = csma.Install(csmanc);
    
    InternetStackHelper stack;
    stack.Install (nc);

    NS_LOG_INFO("Set all network's ip address");
    
    Ipv4AddressHelper addr;

    addr.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wn1intf;
    wn1intf = addr.Assign(wn1ndc);

    addr.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer wn2intf;
    wn2intf = addr.Assign(wn2ndc);

    addr.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaintf;
    csmaintf = addr.Assign(csmandc);

    for(i=0; i<nc.GetN(); i++){
        Ptr<Ipv4> ptr_ipv4 = nc.Get(i)->GetObject<Ipv4>();
        std::cout<<ptr_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
    }
    std::cout<<nc.Get(3)->GetObject<Ipv4>()->GetAddress(2,0).GetLocal()<<std::endl;
    std::cout<<nc.Get(4)->GetObject<Ipv4>()->GetAddress(2,0).GetLocal()<<std::endl;

    NS_LOG_INFO("Set mobility model and position allocator");
    MobilityHelper mobility;

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    for(i=0; i<3; i++)
        positionAlloc->Add (Vector (i*10, 0.0, 0.0));
    positionAlloc->Add (Vector (10.0, 10.0, 0.0));
    positionAlloc->Add (Vector (60.0, 20.0, 0.0));
    for(i=5; i<nodeCount; i++)
        positionAlloc->Add (Vector (i*10, 30.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(nc);

    

    NS_LOG_INFO("Create traffic producing application ...");
    UdpEchoServerHelper echoServer (9);

    uint16_t serverid = 3;
    uint16_t clientid = 0;
    ApplicationContainer serverApps = echoServer.Install (wn2nc.Get (serverid-1));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    UdpEchoClientHelper echoClient (wn2intf.GetAddress (serverid), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (wn1nc.Get (clientid));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Simulator::Stop (Seconds(10.0));
    if(tracing == true){
        csma.EnablePcapAll("pcap/apcsma");
        phy.EnablePcapAll("pcap/apcsma");
    }

    AnimationInterface anim("xml/apcsma");

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}