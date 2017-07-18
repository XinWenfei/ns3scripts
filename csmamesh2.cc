#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/csma-module.h"
#include "ns3/olsr-helper.h"

#include <iostream>
#include <sstream>
#include <fstream>

/*
 * n1: EchoClient
 * n2: Middle device,like a router
 * n3: EchoServer
 * 
 *Network topology:
 n1       n2       n3
 |         |        |
 ===========        *
 .1      .2 .2      .1
 
   10.0.1.0  10.0.2.0
    csma       mesh
 */
using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("CsmaMeshTest");

int main(int argc, char* argv[])
{

    ns3::PacketMetadata::Enable();
    LogComponentEnable("CsmaMeshTest", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    NodeContainer csmaNodes;
    csmaNodes.Create(2);
    Ptr<Node> meshNode = CreateObject<Node>();

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
  csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
 
  // MobilityHelper mobility;
  // Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  //   positionAlloc->Add (Vector (0.0, 25.0, 0.0));
  //   positionAlloc->Add (Vector (50.0, 50.0, 0.0));
  //   positionAlloc->Add (Vector (100.0, 50.0, 0.0));
  //   mobility.SetPositionAllocator(positionAlloc);
  //   mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  // mobility.Install(csmaNodes);
  // mobility.Install(meshNode);

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install(csmaNodes);


  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
  phy.SetChannel(channel.Create());

  MeshHelper mesh = MeshHelper::Default();
  mesh.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
  mesh.SetRemoteStationManager("ns3::ConstantRateWifiManager",
    "DataMode", StringValue("HtMcs0"),
    "ControlMode", StringValue("HtMcs0"));
  mesh.SetStackInstaller("ns3::Dot11sStack");
  mesh.SetSpreadInterfaceChannels(MeshHelper::SPREAD_CHANNELS);
  mesh.SetMacType("RandomStart", TimeValue(Seconds(0.1)));
  mesh.SetNumberOfInterfaces(1);

  NetDeviceContainer meshDevices;
  meshDevices.Add( mesh.Install(phy, meshNode));
  meshDevices.Add(mesh.Install(phy, csmaNodes.Get(1)));

  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRoutingHelper;
  Ipv4ListRoutingHelper listRouting;
  listRouting.Add(staticRoutingHelper, 0);
  // listRouting.Add(olsr, 10);

  InternetStackHelper stack;
  stack.SetRoutingHelper(listRouting);
  stack.Install(csmaNodes);
  stack.Install(meshNode);
  

 
  Ipv4AddressHelper addr;
  addr.SetBase("10.0.1.0","255.255.255.0");
  Ipv4InterfaceContainer csmaIntf;
  csmaIntf = addr.Assign(csmaDevices);
  addr.SetBase("10.0.2.0","255.255.255.0");
  Ipv4InterfaceContainer meshIntf;
  meshIntf = addr.Assign(meshDevices);

  Ptr<Ipv4> ptr_ipv4N1 = csmaNodes.Get(0)->GetObject<Ipv4>();
  Ptr<Ipv4> ptr_ipv4N2 = csmaNodes.Get(1)->GetObject<Ipv4>();
  Ptr<Ipv4> ptr_ipv4N3 = meshNode->GetObject<Ipv4>();
  Ptr<Ipv4StaticRouting> ptr_staticRouting1 = CreateObject<Ipv4StaticRouting>();
  Ptr<Ipv4StaticRouting> ptr_staticRouting2 = staticRoutingHelper.GetStaticRouting(ptr_ipv4N2);
  Ptr<Ipv4StaticRouting> ptr_staticRouting3 = CreateObject<Ipv4StaticRouting>();
  //Remove the default route
  // for(uint16_t i = 0; i<= ptr_staticRouting1->GetNRoutes(); i++){
  //     ptr_staticRouting1->RemoveRoute(i);
  // }
  for(uint16_t i = 0; i<= ptr_staticRouting2->GetNRoutes(); i++){
      ptr_staticRouting2->RemoveRoute(i);
  }
  // for(uint16_t i = 0; i<= ptr_staticRouting3->GetNRoutes(); i++){
  //     ptr_staticRouting3->RemoveRoute(i);
  // }
  ptr_staticRouting1->AddHostRouteTo(Ipv4Address("10.0.2.1"), Ipv4Address("10.0.1.2"), 1);
  ptr_staticRouting1->SetIpv4(ptr_ipv4N1);
  ptr_staticRouting2->AddHostRouteTo(Ipv4Address("10.0.2.1"), 2);
  ptr_staticRouting2->AddHostRouteTo(Ipv4Address("10.0.1.1"), 1);
  ptr_staticRouting3->AddHostRouteTo(Ipv4Address("10.0.1.1"), Ipv4Address("10.0.2.2"), 1);
  ptr_staticRouting3->SetIpv4(ptr_ipv4N3);

  //Add route to node2, interface zero is the lookback
  // ptr_staticRouting->AddHostRouteTo(Ipv4Address("10.0.2.1"), 2);
  for(uint16_t i=0; i < ptr_staticRouting1->GetNRoutes(); i++){
        Ipv4RoutingTableEntry routeTable = ptr_staticRouting1->GetRoute(i);
        std::cout<<"N1 dest: "<<routeTable.GetDest()<<", Gateway: "<<routeTable.GetGateway()<<std::endl;
        std::cout<<"N1 dest network: "<<routeTable.GetDestNetwork()<<", Interface: "<<routeTable.GetInterface()<<std::endl<<std::endl;
    }
  
  for(uint16_t i=0; i < ptr_staticRouting2->GetNRoutes(); i++){
      Ipv4RoutingTableEntry routeTable = ptr_staticRouting2->GetRoute(i);
      std::cout<<"N2 dest: "<<routeTable.GetDest()<<", Gateway: "<<routeTable.GetGateway()<<std::endl;
      std::cout<<"N2 dest network: "<<routeTable.GetDestNetwork()<<", Interface: "<<routeTable.GetInterface()<<std::endl<<std::endl;
  }
  for(uint16_t i=0; i < ptr_staticRouting3->GetNRoutes(); i++){
        Ipv4RoutingTableEntry routeTable = ptr_staticRouting3->GetRoute(i);
        std::cout<<"N3 dest: "<<routeTable.GetDest()<<", Gateway: "<<routeTable.GetGateway()<<std::endl;
        std::cout<<"N3 dest network: "<<routeTable.GetDestNetwork()<<", Interface: "<<routeTable.GetInterface()<<std::endl<<std::endl;
    }
  UdpEchoServerHelper echoServer(9);

  ApplicationContainer serverApps = echoServer.Install(meshNode);
  serverApps.Start(Seconds(0.0));
  serverApps.Stop(Seconds(10.0));

  UdpEchoClientHelper echoClient(meshIntf.GetAddress(0), 9);
  echoClient.SetAttribute("MaxPackets", UintegerValue(2));
  echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApps = echoClient.Install(csmaNodes.Get(0));
  clientApps.Start(Seconds(2.0));
  clientApps.Stop(Seconds(10.0));

  csma.EnablePcapAll("pcap/csma-mesh2");
  phy.EnablePcapAll("pcap/csma-mesh2");

  AsciiTraceHelper ascii;
  csma.EnableAsciiAll(ascii.CreateFileStream("tr/csma-mesh-0.tr"));
  phy.EnableAscii(ascii.CreateFileStream("tr/csma-mesh-1.tr"), 1, 1);
  phy.EnableAscii(ascii.CreateFileStream("tr/csma-mesh-2.tr"), 2, 0);

  AnimationInterface anim("xml/csma-mesh.xml");
  anim.SetConstantPosition(csmaNodes.Get(0), 0, 50);
  anim.SetConstantPosition(csmaNodes.Get(1), 50, 50);
  anim.SetConstantPosition(meshNode, 100, 50);
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop(Seconds(10));

  Simulator::Run();
  Simulator::Destroy();
  
  return 0;


}