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
// Description:
// * and # indicate two routers using 802.11n 2.4GHz
// wifi standard and connected with csma protocol but
// different channel number, the routers of asterisk use 
// channel 1 while another using channel number 6. 
// Every pair of routes are at a distance of 50 meters
// in horizon and vertical direction.
//
// We use coordinate (1,1),(1,2) ... (5,5) to delegate 
// them, choosing the asterisk routers as access point
// and choosing the # routers as four gateways by defaut
// indicated with sign G in (2,2), (2,4), (4,2), (4,4).
// Asterisk and sharp are a pair of routers, if a 
// asterisk router has been choosed as the access point, 
// then the sharp router connected with it will be the 
// gateway. we choose the sharp routers of (3,3) as a 
// sink node(SN) station associated with gateway, which 
// is used to receieve traffic from gateways.
//
//
//
//
//    *-#       *-#       *-#       *-#       *-#
//
//
//
//    *-#       A-G       *-#       A-G       *-#    
//
//
//                         SN
//    *-#       *-#       *-#       *-#       *-#    
//
//
//
//    *-#       A-G       *-#       A-G       *-#    
//
//
//
//    *-#       *-#       *-#       *-#       *-#    
//
//
//    
using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("EXP1");

int main(int argc, char *argv[])
{
    uint16_t acNodeCount = 25;
    uint16_t gwNodeCount = 25;
    uint16_t acChannelNum = 1;
    uint16_t gwChannelNum = 6;
    uint16_t rowNum     =   5;
    uint16_t colNum     =   5;
    bool tracing = true;
    bool verbose = true;
    std::string phymode = "HtMcs0";
    
    if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("EXP1", LOG_LEVEL_INFO);
    }

    //ac_nc: all potential nodes that can be the ap(include ap)
    //gw_nc: all potential nodes that can be the gateway(include gw)
    //apNC : all ap nodes
    //gwNC : all gw nodes
    //staNC: all station nodes(except ap and middle node)
    //snNC : the node which is used to receieve the traffic
    NodeContainer ac_nc, gw_nc, apNC, gwNC, staNC, snNC;
    ac_nc.Create(acNodeCount);
    gw_nc.Create(gwNodeCount);
    uint16_t i = 0;
    //assign all nodes to different node container objects.
    NS_LOG_INFO("assign all nodes to different node container objects.");
    for(i=0; i<acNodeCount; i++){
        if(i!=6 && i!=12){
            staNC.Add(ac_nc.Get(i));
        }else if(i!=12){
            apNC.Add(ac_nc.Get(i));
            gwNC.Add(gw_nc.Get(i));
        }else{
            snNC.Add(gw_nc.Get(i));
        }
    }

    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
            "DataMode", StringValue(phymode),
            "ControlMode", StringValue(phymode));
    
    WifiMacHelper mac;
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();


    //*************  Solve all the station and ap nodes  *****************//
    //
    NS_LOG_INFO("Set wifi channel, mac and ssid of station and ap nodes");
    YansWifiChannelHelper acChannel = YansWifiChannelHelper::Default();
    phy.SetChannel( acChannel.Create() );
    phy.Set("ChannelNumber", UintegerValue(acChannelNum));

    Ssid acSsid = Ssid("ac-ssid");

    mac.SetType("ns3::StaWifiMac",
            "Ssid", SsidValue(acSsid));
    NetDeviceContainer staNdc;
    staNdc = wifi.Install(phy, mac, staNC);

    mac.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(acSsid));
    NetDeviceContainer apNdc;
    apNdc = wifi.Install(phy, mac, apNC);



    //*************  Solve all the gateway nodes  *****************//
    //
    NS_LOG_INFO("Set wifi channel, mac and ssid of gateways");
    YansWifiChannelHelper gwChannel = YansWifiChannelHelper::Default();
    phy.SetChannel( gwChannel.Create() );
    phy.Set("ChannelNumber", UintegerValue(gwChannelNum));

    NetDeviceContainer gwNdc;
    Ssid gwSsid = Ssid("gw-ssid");
    mac.SetType("ns3::ApWifiMac",
            "Ssid", SsidValue(gwSsid));
    gwNdc = wifi.Install(phy, mac, gwNC);



    //*************  Solve the sink node  *****************//
    //

    NS_LOG_INFO("Set the sink node ");
    mac.SetType("ns3::StaWifiMac",
            "Ssid", SsidValue(gwSsid));
    NetDeviceContainer snNdc;
    snNdc = wifi.Install(phy, mac, snNC);

    //*** Establish connection between access node and gateway node ***//
    //
    
    NS_LOG_INFO("Install csma device to all nodes");
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

    NetDeviceContainer apCsmaNdc, gwCsmaNdc;
    apCsmaNdc = csma.Install(apNC);
    gwCsmaNdc = csma.Install(gwNC);
    
    InternetStackHelper stack;
    stack.Install (ac_nc);
    stack.Install (gw_nc);



    //*******  Set ip address for all nodes which will be used *******//
    //


    NS_LOG_INFO("Set ip address");
    
    Ipv4AddressHelper addr;

    addr.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer apIntf, staIntf;
    apIntf = addr.Assign(apNdc);
    staIntf = addr.Assign(staNdc);

    addr.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer gwIntf, snIntf;
    gwIntf = addr.Assign(gwNdc);
    snIntf = addr.Assign(snNdc);

    addr.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer apCsmaIntf, gwCsmaIntf;
    apCsmaIntf = addr.Assign(apCsmaNdc);
    gwCsmaIntf = addr.Assign(gwCsmaNdc);


    //*******  Output all nodes' ip address  *******//
    //
    NS_LOG_INFO("Output all nodes' ip address");

    NS_LOG_INFO("Gw nodes ip address:");
    for(i=0; i<gwNC.GetN(); i++){
        Ptr<Ipv4> ptr_ipv4 = gwNC.Get(i)->GetObject<Ipv4>();
        std::cout<<"Gw node's id:"<<gwNC.Get(i)->GetId()<<", NIC1's ip: "<<ptr_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
        std::cout<<"Gw node's id:"<<gwNC.Get(i)->GetId()<<", NIC2's ip: "<<ptr_ipv4->GetAddress(2,0).GetLocal()<<std::endl;
    }

    NS_LOG_INFO("Ap nodes ip address:");
    for(i=0; i<apNC.GetN(); i++){
        Ptr<Ipv4> ptr_ipv4 = apNC.Get(i)->GetObject<Ipv4>();
        std::cout<<"Ap node's id:"<<apNC.Get(i)->GetId()<<", NIC1's ip: "<<ptr_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
        std::cout<<"Ap node's id:"<<apNC.Get(i)->GetId()<<", NIC2's ip: "<<ptr_ipv4->GetAddress(2,0).GetLocal()<<std::endl;
    }

    NS_LOG_INFO("Sta nodes ip address:");
    for(i=0; i<staNC.GetN(); i++){
        Ptr<Ipv4> ptr_ipv4 = staNC.Get(i)->GetObject<Ipv4>();
        std::cout<<"Sta node's id:"<<staNC.Get(i)->GetId()<<", NIC's ip: "<<ptr_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
    }

    std::cout<<"Sink node's id:"<<snNC.Get(0)->GetId()<<", NIC's ip: "<<snNC.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()<<std::endl;
    
    NS_LOG_INFO("Set mobility model and position allocator");
    MobilityHelper mobility;

    NS_LOG_INFO("Set mobility model for all access nodes");
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    for(i=0; i<rowNum; i++)
        for(uint16_t j=0; j<colNum; j++)
            positionAlloc->Add (Vector (j*50.0, i*50.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(ac_nc);


    NS_LOG_INFO("Set mobility model for all gateways nodes");
    for(i=0; i<rowNum; i++)
        for(uint16_t j=0; j<colNum; j++)
            positionAlloc->Add (Vector (j*50.0+1, i*50.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(gw_nc);
    

    NS_LOG_INFO("Create traffic producing application ...");
    UdpEchoServerHelper echoServer (9);

    uint16_t serverid = 0;
    uint16_t clientid = 0;
    ApplicationContainer serverApps = echoServer.Install (snNC.Get (serverid));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    UdpEchoClientHelper echoClient (snIntf.GetAddress (serverid), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (3));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (staNC.Get (clientid));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Simulator::Stop (Seconds(10.0));
    if(tracing == true){
        csma.EnablePcapAll("pcap/exp1simp");
        phy.EnablePcapAll("pcap/exp1simp");
    }

    AnimationInterface anim("xml/exp1simp");

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
