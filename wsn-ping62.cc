/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Universita' di Firenze
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

// Network topology
//
//       n0    n1
//       |     |
//       =================
//        WSN (802.15.4)
//
// - ICMPv6 echo request flows from n0 to n1 and back with ICMPv6 echo reply
// - DropTail queues 
// - Tracing of queues and packet receptions to file "wsn-ping6.tr"
//
// This example is based on the "ping6.cc" example.

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <math.h>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Ping6WsnExample");

static inline std::string
PrintRecvPacket (Address& from)
{
  InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (from);

  std::ostringstream oss;
  oss << "--\nReceived one packet! Socket: " << iaddr.GetIpv4 ()
      << " port: " << iaddr.GetPort ()
      << " at time = " << Simulator::Now ().GetSeconds ()
      << "\n--";

  return oss.str ();
}

void RecvPacket(Ptr<Socket> socket) {
    Ptr<Packet> pkt;
    Address from;
    while ((pkt=socket->RecvFrom(from))) {
        if (pkt->GetSize()>0) {
            NS_LOG_UNCOND(PrintRecvPacket(from));
        }
    }
}

static void GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize,
        Ptr<Node> node, uint32_t pktCount, Time pktInterval) {
    if (pktCount>0) {
        NS_LOG_DEBUG("Pkt No."<<pktCount);
        socket->Send(Create<Packet> (pktSize));
        Simulator::Schedule(pktInterval,&GenerateTraffic,socket,
                pktSize,node,--pktCount,pktInterval);

    }
    else
        NS_LOG_UNCOND("Socket Finished");
        socket->Close();
}
int main (int argc, char **argv)
{
  bool verbose = false;
  uint32_t PpacketSize = 2048;//bytes
  uint32_t numPacket = 10;
  double interval=2.0;

  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.Parse (argc, argv);

  Time interPacketInterval = Seconds (interval);

  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  nodes.Create (20);

  // Set seed for random numbers
  SeedManager::SetSeed (123);

  // Install mobility
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
          "X",StringValue("ns3::UniformRandomVariable[Min=0|Max=200]"),
          "Y",StringValue("ns3::UniformRandomVariable[Min=0|Max=200]"));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodes);
;

  NS_LOG_INFO ("Create channels.");
  LrWpanHelper lrWpanHelper;
  // Add and install the LrWpanNetDevice for each node
  // lrWpanHelper.EnableLogComponents();
  NetDeviceContainer devContainer = lrWpanHelper.Install(nodes);
  lrWpanHelper.AssociateToPan (devContainer, 10);

  std::cout << "Created " << devContainer.GetN() << " devices" << std::endl;
  std::cout << "There are " << nodes.GetN() << " nodes" << std::endl;

  /* Install IPv4/IPv6 stack */
  NS_LOG_INFO ("Install Internet stack.");
  InternetStackHelper internetv4;
  internetv4.SetIpv6StackInstall (false);
  internetv4.Install (nodes);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (150));
  NetDeviceContainer l = csma.Install (nodes);

  // Install 6LowPan layer
  NS_LOG_INFO ("Install 6LoWPAN.");
  SixLowPanHelper sixlowpan;
  NetDeviceContainer six1 = sixlowpan.Install (l);

  NS_LOG_INFO ("Assign addresses.");
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0","255.255.255.0");
  Ipv4InterfaceContainer i=address.Assign(six1);;

  //	Create Socket//最后一个节点给第一个节点发
      TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  //	Ptr<Socket> recvSink = Socket::CreateSocket(sinkNode.Get(0), tid);
  //	NS_LOG_UNCOND("recvSink is Node "<<sinkNode.Get(0)->GetId());
      Ptr<Socket> recvSink = Socket::CreateSocket(nodes.Get(0), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny(), 9);
      recvSink->Bind(local);
      recvSink->SetRecvCallback (MakeCallback (&RecvPacket));

      Ptr<Socket> source = Socket::CreateSocket(nodes.Get(19), tid);
      InetSocketAddress remote = InetSocketAddress (i.GetAddress(0), 9);
      source->Connect(remote);



      lrWpanHelper.EnablePcap ("masp",nodes.Get(0)->GetId(), 0, true);
      lrWpanHelper.EnablePcap ("masp",nodes.Get(19)->GetId(), 0, true);
      AnimationInterface anim("wsnping6.xml");

      Simulator::Schedule (Seconds (5.0), &GenerateTraffic, source,
              PpacketSize, nodes.Get(19), numPacket, interPacketInterval);
      Simulator::Stop(Seconds(30.0));

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}

