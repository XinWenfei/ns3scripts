#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/config-store.h"

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
void Sta0DevTxTrace(std::string context, Ptr<const Packet> p)
{
    std::cout<<Simulator::Now().As(Time::S)<<std::endl;
    std::cout<<context<<", TX p:"<<*p<<std::endl;
}
void Sta0DevTxTrace1(Ptr<const Packet> p)
{
    std::cout<<Simulator::Now().As(Time::S)<<std::endl;
    std::cout<<"TX p:"<<*p<<std::endl;
}


void Ap0DevRxTrace(std::string context, Ptr<const Packet> p)
{
    std::cout<< context <<", RX p: "<< *p <<std::endl;
}

void Mesh0DevRxTrace(std::string context, Ptr<const Packet> p)
{
    std::cout<<context<<", RX p: "<< *p <<std::endl;
}

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
    uint16_t bridgeNum = 2;
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

    GlobalValue::Bind ("ChecksumEnabled", BooleanValue(true));

    if(log){
        LogComponentEnable("MeshScript", LOG_LEVEL_INFO);
	    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
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
    wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
            "DataMode",StringValue(phyMode),
            "ControlMode",StringValue(phyMode));

    Ssid ssid1 = Ssid("ap1");
    Ssid ssid2 = Ssid("ap2");
    NetDeviceContainer staDevices;
    NqosWifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac","Ssid", SsidValue(ssid1));
    staDevices.Add( wifi.Install(phy, mac, staNC.Get(0) ));
    mac.SetType("ns3::StaWifiMac","Ssid", SsidValue(ssid2));
    staDevices.Add( wifi.Install(phy, mac, staNC.Get(1) ));

    NodeContainer apNC;
    apNC.Add(meshNC.Get(0));
    apNC.Add(meshNC.Get(rowNodes*colNodes-1));

    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid1));
    apDevices.Add( wifi.Install(phy, mac, apNC.Get(0)) );
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid2));
    apDevices.Add( wifi.Install(phy, mac, apNC.Get(1)) );

    BridgeHelper bridge;

    NetDeviceContainer bridgeNetDevs1;
    bridgeNetDevs1 = bridge.Install(apNC.Get(0), NetDeviceContainer(meshDevices.Get(0), apDevices.Get(0)));

    NetDeviceContainer bridgeNetDevs2;
    bridgeNetDevs2 = bridge.Install(apNC.Get(1), NetDeviceContainer(meshDevices.Get(rowNodes*colNodes - 1), apDevices.Get(1)));

    //std::cout<<"Ap 1's mac address: "<<Mac48Address::ConvertFrom( apDevices.Get(0)->GetAddress() )<<std::endl;
    //std::cout<<"Ap 2's mac address: "<<Mac48Address::ConvertFrom( apDevices.Get(1)->GetAddress() )<<std::endl;

    ////Mesh nodes have two netdevice in default.
    //for(int i = 0; i < meshNC.GetN(); ++i){
    //    std::cout<<meshNC.Get(i)->GetNDevices()<<std::endl;
    //}
    
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));
    positionAlloc->Add (Vector ((rowNodes+1)*distance, (colNodes+1)*distance, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(staNC);

    NodeContainer bridgeNC;
    bridgeNC.Create(bridgeNum);

    positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add(Vector (distance, 0.0, 0.0));
    positionAlloc->Add(Vector (rowNodes*distance, (colNodes+1)*distance, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(bridgeNC);

   if(pcap)
        phy.EnablePcapAll(std::string("pcap/stameshecho"));

    InternetStackHelper istack;
    istack.Install(meshNC);
    istack.Install(staNC);

    NetDeviceContainer net1devs = meshDevices;
    NetDeviceContainer net2devs;
    net2devs.Add(apDevices.Get(0));
    net2devs.Add(staDevices.Get(0));
    NetDeviceContainer net3devs;
    net3devs.Add(apDevices.Get(1));
    net3devs.Add(staDevices.Get(1));

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer net1intf = address.Assign(net1devs);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer net2intf = address.Assign(net2devs);
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer net3intf = address.Assign(net3devs);

    Ptr<Ipv4> ptr_ipv4Mesh0 = meshNC.Get(0)->GetObject<Ipv4>();
    Ptr<Ipv4> ptr_ipv4Mesh8 = meshNC.Get(rowNodes*colNodes - 1)->GetObject<Ipv4>();
    Ptr<Ipv4> ptr_ipv4Ap0 = apNC.Get(0)->GetObject<Ipv4>();
    Ptr<Ipv4> ptr_ipv4Ap1 = apNC.Get(1)->GetObject<Ipv4>();

    Ipv4Address ipMesh0  = ptr_ipv4Mesh0->GetAddress(1, 0).GetLocal();
    Ipv4Address ipMesh8  = ptr_ipv4Mesh8->GetAddress(1, 0).GetLocal();
    Ipv4Address ipAp0    = ptr_ipv4Ap0->GetAddress(2, 0).GetLocal();
    Ipv4Address ipAp1    = ptr_ipv4Ap1->GetAddress(2, 0).GetLocal();

    //std::cout<<"Ap0's   ip: "<<ipAp0<<std::endl;
    //std::cout<<"Mesh0's ip: "<<ipMesh0<<std::endl;
    //std::cout<<"Mesh8's ip: "<<ipMesh8<<std::endl;
    //std::cout<<"Ap1's   ip: "<<ipAp1<<std::endl;

    //Ipv4StaticRoutingHelper staticRoutingHelper;
    //Ptr<Ipv4StaticRouting> ptr_staticRotingAp0 = staticRoutingHelper.GetStaticRouting(ptr_ipv4Ap0);
    //for(uint16_t i=0; i <= ptr_staticRotingAp0->GetNRoutes(); i++){
    //    ptr_staticRotingAp0->RemoveRoute(i);
    //}
    ////std::cout<<"ipAp1: "<<ipAp1<<std::endl;
    //ptr_staticRotingAp0->AddHostRouteTo(ipMesh0, ipAp0, 2, 1);
    //ptr_staticRotingAp0->AddHostRouteTo(Ipv4Address("10.1.1.0"), ipAp0, 2, 0);
    //for(uint16_t i=0; i < ptr_staticRotingAp0->GetNRoutes(); i++){
    //    Ipv4RoutingTableEntry routeTable = ptr_staticRotingAp0->GetRoute(i);
    //    std::cout<<"Ap0 dest: "<<routeTable.GetDest()<<", Gateway: "<<routeTable.GetGateway()<<std::endl;
    //    std::cout<<"Ap0 dest network: "<<routeTable.GetDestNetwork()<<", Interface: "<<routeTable.GetInterface()<<std::endl<<std::endl;
    //}
    //
    //Ptr<Ipv4StaticRouting> ptr_staticRotingMesh8 = staticRoutingHelper.GetStaticRouting(ptr_ipv4Mesh8);
    //ptr_staticRotingMesh8->AddHostRouteTo(ipAp1, 1, 0);
    //ptr_staticRotingMesh8->SetDefaultRoute(ipMesh8, 1, 0);
    
    //print ap and station's ip address 
    //In ap0 node, the first parameter of GetAddress(index of interface) is one, because the ap netdevice is
    //installed before mesh device, the net devices order of ap1 node is as the same rule as ap0 node
    //std::cout<<"Sta0's   ip address: "<<staNC.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal()<<std::endl;
    //std::cout<<"Sta1's   ip address: "<<staNC.Get(1)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal()<<std::endl;
    //std::cout<<"Ap0's    ip address: "<< apNC.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal()<<std::endl;
    //std::cout<<"Ap1's    ip address: "<< apNC.Get(1)->GetObject<Ipv4>()->GetAddress(2, 0).GetLocal()<<std::endl;

    //print mesh nodes' ip address
    //std::cout<<"Mesh 0's ip address: "<<meshNC.Get(0)->GetObject<Ipv4>()->GetAddress(2, 0).GetLocal()<<std::endl;
    //for(uint16_t i = 1; i < meshNC.GetN(); i++){
    //    std::cout<<"Mesh "<<i<<"'s ip address: "<<meshNC.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal()<<std::endl;
    //}

    //print mac address
    for(uint16_t i = 0; i < staNC.GetN(); i++){
        Ptr<NetDevice> ptr_netdev = staNC.Get(i)->GetDevice(0);
        Ptr<WifiNetDevice> ptr_wifi_netdev = DynamicCast<WifiNetDevice>(ptr_netdev);
        Ptr<WifiMac> ptr_wifimac = ptr_wifi_netdev->GetMac();
        //std::cout<<"staNode "<<i<<"'s(id:"<<staNC.Get(i)->GetId()<<") mac address: "<<ptr_wifimac->GetAddress()<<std::endl;
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
        for(uint16_t j = 0; j < ndevs - 1; j++){
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
                //std::cout<<"apNode "<<i<<"'s "<<j<<"'s device mac address: "<<Mac48Address::ConvertFrom( ptr_mesh_netdev->GetAddress() )<<std::endl;
            }else{
                Ptr<WifiNetDevice> ptr_wifi_netdev = DynamicCast<WifiNetDevice>(ptr_netdev);
                //std::cout<<"Support sendFrom: "<<ptr_wifi_netdev->SupportsSendFrom()<<std::endl;
                Ptr<WifiMac> ptr_wifimac = ptr_wifi_netdev->GetMac();
                //std::cout<<"apNode "<<i<<"'s "<<j<<"'s device mac address: "<<ptr_wifimac->GetAddress()<<std::endl;
            }
        }
    }
    //std::cout<<staNC.Get(0)->GetId()<<std::endl;
    //Config::ConnectWithoutContext("/NodeList/9/DeviceList/*/Mac/MacTx", MakeCallback(&Sta0DevTxTrace1));

    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApp = echoServer.Install(staNC.Get(1));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(totalTime));

    UdpEchoClientHelper echoClient(net3intf.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(5));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(pktInterval)));
    echoClient.SetAttribute("PacketSize", UintegerValue(pktSize));
    ApplicationContainer clientApp = echoClient.Install(staNC.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(totalTime));

    AnimationInterface anim("xml/stameshecho.xml");
    anim.UpdateNodeColor(staNC.Get(0), 0, 255, 0);
    anim.UpdateNodeSize (staNC.Get(0)->GetId(), nodeWidth, nodeHeight);
    anim.UpdateNodeColor(staNC.Get(1), 255, 0, 0);
    anim.UpdateNodeSize (staNC.Get(1)->GetId(), nodeWidth, nodeHeight);
    anim.UpdateNodeColor(bridgeNC.Get(0), 255, 153, 0);
    anim.UpdateNodeSize (bridgeNC.Get(0)->GetId(), nodeWidth, nodeHeight);
    anim.UpdateNodeColor(bridgeNC.Get(1), 255, 153, 0);
    anim.UpdateNodeSize (bridgeNC.Get(1)->GetId(), nodeWidth, nodeHeight);
    for(int i = 0; i < rowNodes*colNodes; i++){
        anim.UpdateNodeColor(meshNC.Get(i), 0, 0, 255);
        anim.UpdateNodeSize(meshNC.Get(i)->GetId(), nodeWidth, nodeHeight);
    }

    //Config::Connect("/NodeList/9/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/MacTx", MakeCallback(&Sta0DevTxTrace));


    //Config::Connect("/NodeList/0/DeviceList/1/Mac/MacRx", MakeCallback(&Mesh0DevRxTrace));
    Config::Connect("/NodeList/0/DeviceList/1/Mac/MacRx", MakeCallback(&Ap0DevRxTrace));

    Simulator::Stop(Seconds(totalTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;

}
