/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//
// This program configures a grid (default 5x5) of nodes on an 
// 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000 
// (application) bytes to node 1.
//
// The default layout is like this, on a 2-D grid.
//
//Network topology
//   |  200m  |---------800m----------|  200m  |
//
//   *        *                       *        *-
//                                              2
//                                              0
//                                              0
//                                              m
//   *        *                       *        *-
//
//
//
//                       (gw)
//                        *(600,600)
//
//
//
//
//   *        *                       *        *
//
//
//
//
//   *        *                       *        *
//
// the layout is affected by the parameters given to GridPositionAllocator;
// by default, GridWidth is 5 and numStaNodes is 25..
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc-grid --help"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the ns-3 documentation.
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when distance increases beyond
// the default of 500m.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --distance=500"
// ./waf --run "wifi-simple-adhoc --distance=1000"
// ./waf --run "wifi-simple-adhoc --distance=1500"
// 
// The source node and sink node can be changed like this:
// 
// ./waf --run "wifi-simple-adhoc --sourceNode=20 --sinkNode=10"
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
// 
// ./waf --run "wifi-simple-adhoc-grid --verbose=1"
//
// By default, trace file writing is off-- to enable it, try:
// ./waf --run "wifi-simple-adhoc-grid --tracing=1"
//
// When you are done tracing, you will notice many pcap trace files 
// in your directory.  If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-grid-0-0.pcap -nn -tt
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/global-value.h"
#include "ns3/simple-ref-count.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

void SocketReceivePacket (Ptr<Socket> socket)
{
 // while (socket->Recv ())
 //   {
      NS_LOG_UNCOND ("Received one packet!");
 //   }
}

void Tx(std::string context, Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
{
    // std::cout<<context;
    std::cout<<Simulator::Now().As(Time::S)<<" ";
    std::cout<<ipv4->GetAddress(1,0).GetLocal()<<" send a packet!"<<std::endl;
}

void Rx(std::string context, Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
{
    // std::cout<<context;
    std::cout<<Simulator::Now().As(Time::S)<<" ";
    std::cout<<ipv4->GetAddress(1,0).GetLocal()<<" recv a packet!"<<std::endl;
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
        int sendbytes = socket->Send (Create<Packet> (pktSize));
        //sendCb(socket, socket->GetTxAvailable());
      //std::cout<<"Send "<<sendbytes<<" bytes!"<<std::endl;
      Simulator::Schedule (pktInterval, &GenerateTraffic, 
                           socket, pktSize,pktCount-1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}


int main (int argc, char *argv[])
{
    LogComponentEnable("WifiSimpleAdhocGrid", LOG_LEVEL_INFO);
  std::string phyMode ("DsssRate1Mbps");
  double distance = 200;  // m
  double distance2= 600;
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 10;
  uint32_t numStaNodes = 16;  
  uint32_t numGwNodes = 1;
  uint32_t sinkNode = 0;
  uint32_t sourceNode = 0;
  uint32_t simTime = 45;
  uint32_t routeTime = 30;
  double interval = 1.0; // seconds
  bool verbose = false;
  bool tracing = true;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("numStaNodes", "number of nodes", numStaNodes);
  cmd.AddValue ("sinkNode", "Receiver node number", sinkNode);
  cmd.AddValue ("sourceNode", "Sender node number", sourceNode);

  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // Enable checksum
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));


  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", 
                      StringValue (phyMode));

  NodeContainer sta_nc, gw_nc;
  sta_nc.Create (numStaNodes);
  gw_nc.Create  (numGwNodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (-10) ); 
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add an upper mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer sta_devices = wifi.Install (wifiPhy, wifiMac, sta_nc);
  NetDeviceContainer  gw_devices = wifi.Install (wifiPhy, wifiMac,  gw_nc);

  MobilityHelper mobility;
  // mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  //                                "MinX", DoubleValue (0.0),
  //                                "MinY", DoubleValue (0.0),
  //                                "DeltaX", DoubleValue (distance),
  //                                "DeltaY", DoubleValue (distance),
  //                                "GridWidth", UintegerValue (5),
  //                                "LayoutType", StringValue ("RowFirst"));
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for(uint32_t i=0; i<4; i++){
    for(uint32_t j=0; j<4; j++){
      positionAlloc->Add (Vector (j*distance+(uint32_t)(j/2)*distance2, i*distance+(uint32_t)(i/2)*distance2, 0));
    }
  }
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (sta_nc);

  // Unref(positionAlloc);
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (distance2, distance2, 0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install (gw_nc);

  // Enable OLSR
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  internet.Install (sta_nc);
  internet.Install ( gw_nc);

  Ipv4AddressHelper ipv4;
  //NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (sta_devices);
  i.Add( ipv4.Assign(gw_devices));

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (gw_nc.Get (sinkNode), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  // recvSink->SetRecvCallback (MakeCallback (&SocketReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (sta_nc.Get (sourceNode), tid);
  InetSocketAddress remote = InetSocketAddress (i.GetAddress (numStaNodes, 0), 80);
  source->Connect (remote);

  if (tracing == true)
    {
        std::cout<<"if node enable checksum:"<<Node::ChecksumEnabled()<<std::endl;
      // AsciiTraceHelper ascii;
      // wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("tr/gw-adhoc.tr"));
      //wifiPhy.EnablePcap ("./pcap/wifi-simple-adhoc2", devices);
      wifiPhy.EnablePcapAll("./pcap/gw-adhoc");
      // Trace routing tables
      // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("route/wifi-simple-adhoc-grid.routes", std::ios::out);
      // olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
      // Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("route/wifi-simple-adhoc-grid.neighbors", std::ios::out);
      // olsr.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

      // To do-- enable an IP-level trace that shows forwarding events only
    }

  // Give OLSR time to converge-- 30 seconds perhaps
  Simulator::Schedule (Seconds (routeTime), &GenerateTraffic, 
                       source, packetSize, numPackets, interPacketInterval);

  // Output the xml file
  AnimationInterface anim("./xml/gw-adhoc.xml");

  std::ostringstream oss;
  oss << "/NodeList/0/$ns3::Ipv4L3Protocol/Tx";
  // Config::ConnectWithoutContext(oss.str(), MakeCallback(&Tx));
  Config::Connect(oss.str(), MakeCallback(&Tx));

  // oss.clear();
  oss.str("");
  oss << "/NodeList/16/$ns3::Ipv4L3Protocol/Rx";
  Config::Connect(oss.str(), MakeCallback(&Rx));

  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << numStaNodes << " with grid distance " << distance);

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

