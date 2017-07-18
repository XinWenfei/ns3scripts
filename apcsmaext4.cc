#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/olsr-helper.h"

// add another two aps and a csma line
// the global routes doesn't work
// try to use static routes for AP
// add static route to host
// 
// Default Network Topology
//
//
//         Wifi 192.168.1.0
//        AP(ssid1)      AP(ssid1)
//        *    *    *    *
//        |    |    |    | 
//       n1   n2   n3   n4 
//       ||              ||   
//       ||              ||   
//       ||              ||   
//       ||1             ||1    
//       ||9             ||9                  
//       ||2             ||2 
//       ||.             ||.                   
//       ||1 c           ||1 c 
//       ||6 s           ||6 s
//       ||8 m           ||8 m
//       ||. a           ||. a
//       ||2             ||2 
//       ||.             ||. 
//       ||0             ||0  
//       ||              ||              
//       ||              ||              
//       n5   n6   n7   n8
//        |    |    |    |
//        *    *    *    *
//        AP(ssid2)      AP(ssid2)
//        Wifi 192.168.3.0
//
//
//

using namespace ns3;
// using namespace std;

NS_LOG_COMPONENT_DEFINE ("APCSMAEXT4");

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
      LogComponentEnable ("APCSMAEXT4", LOG_LEVEL_INFO);
    }

    GlobalValue::Bind ("ChecksumEnabled", BooleanValue(true));

    NodeContainer nc;
    nc.Create(nodeCount);

    NS_LOG_INFO("Divide 8 nodes into 3 network");
    NodeContainer wn1nc, csmanc, wn2nc;
    uint16_t i=0;
    csmanc.Add(nc.Get(i));
    for(i=1; i<3; i++)
        wn1nc.Add(nc.Get(i));
    csmanc.Add(nc.Get(i));
    csmanc.Add(nc.Get(++i));
    for(++i; i<7; i++)
        wn2nc.Add(nc.Get(i));
    csmanc.Add(nc.Get(i));

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
    Ssid ssid1 = Ssid("ssid1");
    Ssid ssid2 = Ssid("ssid2");
    Ssid ssid3 = Ssid("ssid3");
    Ssid ssid4 = Ssid("ssid4");

    NS_LOG_INFO("Set wireless network1's ap1 mac info");
    NetDeviceContainer wn1ndc;
    mac.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(ssid1));
    wn1ndc.Add( wifi.Install(phy, mac, csmanc.Get(0)));

    NS_LOG_INFO("Set wireless network1's station mac info");
    mac.SetType("ns3::StaWifiMac",
            "Ssid", SsidValue(ssid1));
    
    wn1ndc.Add( wifi.Install(phy, mac, wn1nc.Get(0)));
    wn1ndc.Add( wifi.Install(phy, mac, wn1nc.Get(1)));

    NS_LOG_INFO("Set wireless network1's ap2 mac info");
    mac.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(ssid1));
    wn1ndc.Add( wifi.Install(phy, mac, csmanc.Get(1)));

    
    NS_LOG_INFO("Set wireless network2's ap1 mac info");
    YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy2 = YansWifiPhyHelper::Default();
    phy2.SetChannel( channel2.Create() );
    phy2.Set("ChannelNumber", UintegerValue(6));
    NetDeviceContainer wn2ndc;
    mac.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(ssid2));
    wn2ndc.Add( wifi.Install(phy2, mac, csmanc.Get(2)));

    NS_LOG_INFO("Set wireless network2's station mac info");
    mac.SetType("ns3::StaWifiMac",
            "Ssid", SsidValue(ssid2));
    for(i=0; i<wn2nc.GetN(); i++)
        wn2ndc.Add( wifi.Install(phy2, mac, wn2nc.Get(i)) );

    NS_LOG_INFO("Set wireless network2's ap2 mac info");
    mac.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(ssid2));
    wn2ndc.Add( wifi.Install(phy2, mac, csmanc.Get(3)));
    
    NS_LOG_INFO("Set csma network's channel");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmandc;
    csmandc = csma.Install(csmanc);
    
    InternetStackHelper stack;
    Ipv4StaticRoutingHelper staticRoutingHelper;
    // OlsrHelper olsr;
    stack.SetRoutingHelper(staticRoutingHelper);
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

    // add static route to host
    Ptr<Ipv4StaticRouting> ptr_staticRouting = staticRoutingHelper.GetStaticRouting(wn1nc.Get(0)->GetObject<Ipv4>());
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.2.0"), Ipv4Address("192.168.1.1"), 1, 0);
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.2.0"), Ipv4Address("192.168.1.4"), 1, 1);
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.3.0"), Ipv4Address("192.168.1.1"), 1, 0);

    NS_LOG_INFO("Set static routing for network1's ap1 and ap2");
    ptr_staticRouting = staticRoutingHelper.GetStaticRouting(csmanc.Get(0)->GetObject<Ipv4>());
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.3.0"), Ipv4Address("192.168.2.3"), 2, 0);
    ptr_staticRouting = staticRoutingHelper.GetStaticRouting(csmanc.Get(1)->GetObject<Ipv4>());
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.3.0"), Ipv4Address("192.168.2.4"), 2, 0);

    NS_LOG_INFO("Set static routing for network2's ap1 and ap2");
    ptr_staticRouting = staticRoutingHelper.GetStaticRouting(csmanc.Get(2)->GetObject<Ipv4>());
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.1.0"), Ipv4Address("192.168.2.1"), 2, 0);
    ptr_staticRouting = staticRoutingHelper.GetStaticRouting(csmanc.Get(3)->GetObject<Ipv4>());
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.1.0"), Ipv4Address("192.168.2.2"), 2, 0);

    ptr_staticRouting = staticRoutingHelper.GetStaticRouting(wn2nc.Get(1)->GetObject<Ipv4>());
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.2.0"), Ipv4Address("192.168.3.1"), 1, 0);
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.2.0"), Ipv4Address("192.168.3.4"), 1, 1);
    ptr_staticRouting->AddHostRouteTo(Ipv4Address("192.168.1.0"), Ipv4Address("192.168.3.1"), 1, 0);

    for(i=0; i<nc.GetN(); i++){
        Ptr<Ipv4> ptr_ipv4 = nc.Get(i)->GetObject<Ipv4>();
        std::cout<<ptr_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
    }
    std::cout<<nc.Get(0)->GetObject<Ipv4>()->GetAddress(2,0).GetLocal()<<std::endl;
    std::cout<<nc.Get(3)->GetObject<Ipv4>()->GetAddress(2,0).GetLocal()<<std::endl;
    std::cout<<nc.Get(4)->GetObject<Ipv4>()->GetAddress(2,0).GetLocal()<<std::endl;
    std::cout<<nc.Get(7)->GetObject<Ipv4>()->GetAddress(2,0).GetLocal()<<std::endl;

    NS_LOG_INFO("Set mobility model and position allocator");
    MobilityHelper mobility;

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    for(i=0; i<4; i++)
        positionAlloc->Add (Vector (i*50, 0.0, 0.0));
    for(i=4; i<nodeCount; i++)
        positionAlloc->Add (Vector ((i%4)*50, 100.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(nc);

    

    NS_LOG_INFO("Create traffic producing application ...");
    UdpEchoServerHelper echoServer (9);

    uint16_t serverid = 5;
    uint16_t clientid = 1;
    ApplicationContainer serverApps = echoServer.Install (nc.Get (serverid));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    UdpEchoClientHelper echoClient (wn2intf.GetAddress (1), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (nc.Get (clientid));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));

    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("route/apcsmaext4.routes", std::ios::out);
    Ipv4StaticRoutingHelper::PrintRoutingTableAllAt (Seconds (1), routingStream);

    //NS_LOG_INFO("Establish global routes");
    //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Simulator::Stop (Seconds(10.0));
    if(tracing == true){
        csma.EnablePcapAll("pcap/apcsmaext4");
        phy.EnablePcapAll("pcap/apcsmaext4");
    }

    AnimationInterface anim("xml/apcsmaext4.xml");

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
