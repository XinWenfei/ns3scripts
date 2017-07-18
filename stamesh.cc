#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <sstream>
#include <fstream>

/*
 * The topology is displayed as following
 * Nodes in rectangle are mesh nodes
 * sta0 and sta1 are general station nodes
 * The program is to send data from sta0 
 * to sta1 nodes transmiting by mesh nodes
 *
 *    sta0
 *        *  
 *         -----------
 *   ap0-->|*   *   *|
 *         |         |
 *         |*   *   *|
 *         |         |
 *         |*   *   *|<--ap1
 *         ----------- 
 *                    *
 *                  sta1
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MeshScript");

int main(int argc, char* argv[])
{
    uint16_t rowNodes = 3;
    uint16_t colNodes = 3;
    double   distance = 50;// meter
    double   totalTime = 100;//seconds
    double   pktInterval = 0.1; //second
    uint16_t pktSize  = 1024; 
    uint32_t nIntf    = 1;
    bool     pcap     = true;
    bool     log      = true;
    double   randomStart = 0.1;
    uint16_t staNum   = 2;// numbers of station nodes
    double   nodeWidth = 5.0;
    double   nodeHeight = 5.0;
    std::string phyMode = "HtMcs0";

    CommandLine cmd;
    cmd.AddValue("rowN", "Number of nodes in a row", rowNodes);
    cmd.AddValue("colN", "Number of nodes in a column", colNodes);
    cmd.AddValue("dis",  "Distance between two nodes", distance);
    cmd.AddValue("time", "Sum simulation time", totalTime);
    cmd.AddValue("interval", "Interval between sending two pakcets", pktInterval);
    cmd.AddValue("intfN","Number of radio interfaces used by each mesh point.[0.001s]", nIntf);
    cmd.AddValue("pcap", "Enable pcap trace on interfaces.[true]", pcap);
    cmd.AddValue("log",  "Enable log info when running", log);

    if(log){
        LogComponentEnable("MeshScript", LOG_LEVEL_INFO);
	    //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	    //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
    NS_LOG_INFO("Grid: "<<rowNodes<<"*"<<colNodes);
    NS_LOG_INFO("Simulation time: "<<totalTime<<" s.");

    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel (channel.Create());

    MeshHelper mesh = MeshHelper::Default();
    std::string stack = "ns3::Dot11sStack";
    std::string root  = "ff:ff:ff:ff:ff:ff";
    mesh.SetStackInstaller(stack);
    mesh.SetSpreadInterfaceChannels(MeshHelper::SPREAD_CHANNELS);
    mesh.SetMacType("RandomStart", TimeValue(Seconds(randomStart)));
    mesh.SetNumberOfInterfaces(nIntf);

    NodeContainer meshNC;
    meshNC.Create(rowNodes*colNodes);

    NetDeviceContainer meshDevices = mesh.Install(phy, meshNC);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
            "MinX", DoubleValue(50.0),
            "MinY", DoubleValue(50.0),
            "DeltaX", DoubleValue(distance),
            "DeltaY", DoubleValue(distance),
            "GridWidth", UintegerValue(rowNodes),
            "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(meshNC);
 
    NodeContainer staNC;
    staNC.Create(staNum);

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
            "DataMode",StringValue(phyMode),
            "ControlMode",StringValue(phyMode));
    wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
    //wifi.SetStandard(WIFI_PHY_STANDARD_80211g);

    Ssid ssid = Ssid("ap");
    NqosWifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac","Ssid", SsidValue(ssid));
    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, staNC);

    NodeContainer apNC;
    apNC.Add(meshNC.Get(0));
    apNC.Add(meshNC.Get(rowNodes*colNodes-1));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, apNC);

    //std::cout<<"Ap 1's mac address: "<<Mac48Address::ConvertFrom( apDevices.Get(0)->GetAddress() )<<std::endl;
    //std::cout<<"Ap 2's mac address: "<<Mac48Address::ConvertFrom( apDevices.Get(1)->GetAddress() )<<std::endl;

    //Mesh nodes have two netdevice in default.
    //for(int i = 0; i < meshNC.GetN(); ++i){
    //    std::cout<<meshNC.Get(i)->GetNDevices()<<std::endl;
    //}
    
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));
    positionAlloc->Add (Vector ((rowNodes+1)*50.0, (colNodes+1)*50.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(staNC);

   if(pcap)
        phy.EnablePcapAll(std::string("pcap/stamesh"));

    InternetStackHelper istack;
    istack.Install(meshNC);
    istack.Install(staNC);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer intf = address.Assign(staDevices.Get(0));
    intf.Add( address.Assign(apDevices.Get(0)) );
    intf.Add( address.Assign(meshDevices) );
    intf.Add( address.Assign(apDevices.Get(1)) );
    intf.Add( address.Assign(staDevices.Get(1)) );

    for(uint16_t i = 0; i < staNC.GetN(); i++){
        Ptr<NetDevice> ptr_netdev = staNC.Get(i)->GetDevice(0);
        Ptr<WifiNetDevice> ptr_wifi_netdev = DynamicCast<WifiNetDevice>(ptr_netdev);
        Ptr<WifiMac> ptr_wifimac = ptr_wifi_netdev->GetMac();
        std::cout<<"staNode "<<i<<"'s(id:"<<staNC.Get(i)->GetId()<<") mac address: "<<ptr_wifimac->GetAddress()<<std::endl;
    }

    for(uint16_t i = 0; i < apNC.GetN(); i++){
        uint16_t ndevs = apNC.Get(i)->GetNDevices();
        //This is strange that ndevs is four as strange as
        //the reresult: apNC.Get(0)->GetN() = 3 after I installed the
        //wifi netdevice.
        //The last index(j) cannot return a valid pointer that
        //point to netdevice, at the same time, return values of
        //index(j) one and two are the same, I think the function of
        //MeshHelper::Install() has installed two netdevices, the
        //first one is MeshPointDevice, and the second one is 
        //WifiNetDevice. but the value of ndevs of apnode is four, this
        //still makes me puzzled.
        //
        //std::cout<<ndevs<<std::endl;
        for(uint16_t j = 1; j < ndevs - 1; j++){
            Ptr<NetDevice> ptr_netdev = apNC.Get(i)->GetDevice(j);
            //only the first is the mesh point device
            //I don't want to see the repeated value, therefore, I
            //make the j begin from one.
            if(j == 0){
                Ptr<MeshPointDevice> ptr_mesh_netdev = DynamicCast<MeshPointDevice>(ptr_netdev);
                //Using mesh wifi interface mac to output the mac address
                //std::vector< Ptr<NetDevice> > vec = ptr_mesh_netdev->GetInterfaces();
                //for(std::vector<Ptr<NetDevice> >::const_iterator k = vec.begin(); k != vec.end(); ++k){
                //    Ptr<WifiNetDevice> device = (*k)->GetObject<WifiNetDevice>();
                //    Ptr<MeshWifiInterfaceMac> ptr_meshmac = device->GetMac()->GetObject<MeshWifiInterfaceMac>();
                //    std::cout<<"apNode "<<i<<"'s "<<j<<"'s device mac address: "<<ptr_meshmac->GetMeshPointAddress()<<std::endl;
                //}
                std::cout<<"apNode "<<i<<"'s "<<j<<"'s device mac address: "<<Mac48Address::ConvertFrom( ptr_mesh_netdev->GetAddress() )<<std::endl;
            }else{
                Ptr<WifiNetDevice> ptr_wifi_netdev = DynamicCast<WifiNetDevice>(ptr_netdev);
                Ptr<WifiMac> ptr_wifimac = ptr_wifi_netdev->GetMac();
                std::cout<<"apNode "<<i<<"'s "<<j<<"'s device mac address: "<<ptr_wifimac->GetAddress()<<std::endl;
            }
        }
    }

    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApp = echoServer.Install(staNC.Get(1));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(totalTime));
    UdpEchoClientHelper echoClient(intf.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(pktInterval)));
    echoClient.SetAttribute("PacketSize", UintegerValue(pktSize));
    ApplicationContainer clientApp = echoClient.Install(staNC.Get(0));
    clientApp.Start(Seconds(0.0));
    clientApp.Stop(Seconds(totalTime));

    AnimationInterface anim("xml/stamesh.xml");
    anim.UpdateNodeColor(staNC.Get(0), 0, 255, 0);
    anim.UpdateNodeSize (staNC.Get(0)->GetId(), nodeWidth, nodeHeight);
    anim.UpdateNodeColor(staNC.Get(1), 255, 0, 0);
    anim.UpdateNodeSize (staNC.Get(1)->GetId(), nodeWidth, nodeHeight);
    for(int i = 0; i < rowNodes*colNodes; i++){
        anim.UpdateNodeColor(meshNC.Get(i), 0, 0, 255);
        anim.UpdateNodeSize(meshNC.Get(i)->GetId(), nodeWidth, nodeHeight);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Simulator::Stop(Seconds(totalTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;

}
