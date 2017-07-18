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


/*
 *  Usage:
 *  NS_GLOBAL_VALUE="RngRun=3" ./waf --run "scratch/myManet --numNodes=27"
 *  
 */

//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the ns-3 documentation.
//
// 
// The source node and sink node can be changed like this:
// 
// ./waf --run "scratch/myManet --sourceNode=20 --sinkNode=10"
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
// 
// ./waf --run "scratch/myManet --verbose=1"
//
// By default, trace file writing is off-- to enable it, try:
// ./waf --run "scratch/myManet --tracing=1"
//
// When you are done tracing, you will notice many pcap trace files 
// in your directory.  If you have tcpdump installed, you can try this:
//
// tcpdump -r scratch/myManet-0-0.pcap -nn -tt
//

#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/gnuplot.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/udp-client.h"
#include "ns3/seq-ts-header.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

int counter = 0;
int m_sent = 0;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

static bool g_verbose = true;

void MacRxDrop(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_UNCOND(context << "at "<<(Simulator::Now()).As(Time::S)
            <<"has droped a packet as receving");
}

void MacTxDrop(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_UNCOND(context << "at "<<(Simulator::Now()).As(Time::S)
            <<"has droped a packet as trasimiting");
}

void MacTxTrace(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_UNCOND(context << "at "<<(Simulator::Now()).As(Time::S)
            <<"has transmited a packet.");
    std::cout<<"Transmit a packet!"<<std::endl;
}


void
DevTxTrace (std::string context, Ptr<const Packet> p)
{
  if (g_verbose)
    {   
      std::cout << " TX p: " << (*p).ToString() << std::endl;
    }   
}
void
DevRxTrace (std::string context, Ptr<const Packet> p)
{
  if (g_verbose)
    {   
		//std::cout<<(Simulator::Now()).As(Time::S)<<" ";
        //std::cout << " RX p: " << *p << std::endl;
    }   
}

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
{
	double localThrou=0;
	std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
	Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
	{
		//Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
		//std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
		//std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
		//std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
		//std::cout<<"Duration		: "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
		//std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
		//std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
		localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
		// updata gnuplot data
		DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
		//std::cout<<"---------------------------------------------------------------------------"<<std::endl;
	}
	
	Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon,DataSet);
	
	//if(flowToXml)
	//{
	flowMon->SerializeToXmlFile ("./scratch/ThroughputMonitor.xml", true, true);
	//}

}

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      //NS_LOG_UNCOND ("Received one packet!");
      std::cout << Simulator::Now().GetSeconds() << " Received packet: " << ++counter << std::endl;
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval, uint8_t const *buffer )
{
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  Ptr<Packet> p = Create<Packet> (buffer, pktSize);
  p->AddHeader (seqTs);
  
  if (pktCount > 0)
    {
      //socket->Send (Create<Packet> (pktSize));
      socket->Send (p);
      ++m_sent;
      Simulator::Schedule (pktInterval, &GenerateTraffic, 
                           socket, pktSize,pktCount-1, pktInterval, buffer );
    }
  else
    {
      socket->Close ();
    }
}


int main (int argc, char *argv[])
{
  //std::string phyMode ("DsssRate1Mbps");
  std::string phyMode ("ErpOfdmRate6Mbps");
  //double distance = 500;  // m
  uint32_t packetSize = 1024; // application bytes
  uint32_t numPackets = 20;
  uint32_t numNodes = 27;  // n routers + sink + source
  double interval = 0.1; // seconds
  bool verbose = false;
  bool tracing = true;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("numNodes", "number of nodes", numNodes);

  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));
  
  uint32_t sinkNode = numNodes - 1;
  uint32_t sourceNode = 0;

  // disable fragmentation for frames below 2200 bytes
  //Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  //// turn off RTS/CTS for frames below 2200 bytes
  //Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  //// Fix non-unicast data rate to be the same as that of unicast
  //Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

  NodeContainer c;
  c.Create (numNodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // set it to zero; otherwise, gain will be added
  //wifiPhy.Set ("RxGain", DoubleValue (-10) );
  wifiPhy.Set("TxPowerStart", DoubleValue(5));  //Minimum available transmission level (dbm)
  wifiPhy.Set("TxPowerEnd", DoubleValue(5));  //Maximum available transmission level (dbm)
  wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-83.0) ); //Receive Sensivity: -85/-83 dbm
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  //wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode),
                                "RtsCtsThreshold",UintegerValue(2200),
                                "FragmentationThreshold",UintegerValue(2200),
                                "NonUnicastMode", StringValue(phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
            "MinX", DoubleValue(0),
            "MinY", DoubleValue(0),
            "DeltaX", DoubleValue(30),
            "DeltaY", DoubleValue(30),
            "GridWidth", UintegerValue(5),
            "LayoutType", StringValue("RowFirst")
            );
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  //Ptr<UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable>();
  //randomizer->SetAttribute("Min", DoubleValue(0.0));
  //randomizer->SetAttribute("Max", DoubleValue(500.0));
  //
  //mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator",
  //                              "X", PointerValue(randomizer),
  //                              "Y", PointerValue(randomizer));
  
  //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                           "Mode", StringValue ("Time"),
  //                           "Time", StringValue ("300s"),
  //                           "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
  //                           "Bounds", StringValue ("0|500|0|500"));
  
  mobility.Install (c);

  // Enable OLSR
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (sinkNode), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (c.Get (sourceNode), tid);
  InetSocketAddress remote = InetSocketAddress (i.GetAddress (sinkNode, 0), 80);
  source->Connect (remote);

  if (tracing == true)
    {
      AsciiTraceHelper ascii;
      wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("./scratch/myManet.tr"));
      //wifiPhy.EnablePcap ("./scratch/myManet", devices);
      wifiPhy.EnablePcap ("./pcap/adhoc3", devices.Get (sourceNode));
      wifiPhy.EnablePcap ("./pcap/adhoc3", devices.Get (sinkNode));
      // Trace routing tables
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("./scratch/myManet.routes", std::ios::out);
      olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
      Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("./scratch/myManet.neighbors", std::ios::out);
      olsr.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

      // To do-- enable an IP-level trace that shows forwarding events only
    }

  AnimationInterface anim("xml/adhoc3.xml");
  anim.UpdateNodeColor(c.Get(sourceNode), 0, 255, 0);
  anim.UpdateNodeColor(c.Get(sinkNode), 255, 0, 0);
  
  //Ptr<Ipv4> ptrIpv4 = c.Get(sourceNode)->GetObject<Ipv4>();
  //std::cout<<"source node's address is: "<< ptrIpv4->GetAddress(1,0).GetLocal()<<std::endl;
  //ptrIpv4 = c.Get(sinkNode)->GetObject<Ipv4>();
  //std::cout<<"sink node's address is: "<< ptrIpv4->GetAddress(1,0).GetLocal()<<std::endl;
  //for(int i = 1; i < sinkNode; i++){
  //    anim.UpdateNodeColor(c.Get(i), 135, 206, 235);
  //    ptrIpv4 = c.Get(i)->GetObject<Ipv4>();
  //    std::cout<<"node "<<i<<" address is: "<< ptrIpv4->GetAddress(1,0).GetLocal()<<std::endl;
  //}

  //The max num can't great than 127, uint8_t type's variable means a 8 bits value, 
  //which means that the variable of this type only can indicate 127 ascii characters
  uint8_t *ptr = new uint8_t[packetSize];
  // The array is the string "Hello world! " in ascii character.
  int array[] = {72, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 33, 32};
  uint8_t const *ptrdata = NULL;
  for(int i = 0; i < packetSize; i++){
      ptr[i] = array[i%13];
  }
  ptrdata = ptr;

  //for(int i = 0; i < packetSize; i++){
  //    std::cout<<ptr[i]<<" ";
  //}
  //std::cout<<std::endl;
  //Config::Connect ("/NodeList/26/DeviceList/0/Mac/MacTx", MakeCallback (&DevTxTrace));
  //Config::Connect ("/NodeList/26/DeviceList/0/Mac/MacRx", MakeCallback (&DevRxTrace));

  // Trace the source Mac TxDrop 
  std::ostringstream oss;
  oss << "/NodeList/" << c.Get(sourceNode)->GetId()<<
         "/DeviceList/" << devices.Get(sourceNode)->GetIfIndex()<<
         "/$ns3::WifiNetDevice/Mac/MacTxDrop";
  Config::Connect(oss.str(), MakeCallback(&MacTxDrop));

  //Trace the sink Mac RxDrop
  oss.clear();//clear the error state 
  oss.str("");//clear the contents
  oss << "/NodeList/" << c.Get(sinkNode)->GetId()<<
         "/DeviceList/" << devices.Get(sinkNode)->GetIfIndex()<<
         "/$ns3::WifiNetDevice/Mac/MacRxDrop";
  Config::Connect(oss.str(), MakeCallback(&MacRxDrop));

  //Trace the tx event
  //oss.clear();
  //oss.str("");
  //oss << "/NodeList/" << c.Get(sourceNode)->GetId()<<
  //       "/DeviceList/" << devices.Get(sourceNode)->GetIfIndex()<<
  //       "/$ns3::WifiNetDevice/Mac/MacTx";
  //Config::Connect(oss.str(), MakeCallback(&MacTxTrace));

  // Give OLSR time to converge-- 30 seconds perhaps
  Simulator::Schedule (Seconds (31.0), &GenerateTraffic, 
                       source, packetSize, numPackets, interPacketInterval, ptrdata);

  // Output what we are doing
  NS_LOG_UNCOND ("Source node is: " << sourceNode << " to sink node: " << sinkNode);
  
  
  // Gnuplot parameters      
  std::string fileNameWithNoExtension = "./scratch/FlowVSThroughput_";
  std::string graphicsFileName        = fileNameWithNoExtension + ".png";
  std::string plotFileName            = fileNameWithNoExtension + ".plt";
  std::string plotTitle               = "Flow vs Throughput";
  std::string dataTitle               = "Throughput";

  // Instantiate the plot and set its title.
  Gnuplot gnuplot (graphicsFileName);
  gnuplot.SetTitle (plotTitle);

  // Make the graphics file, which the plot file will be when it
  // is used with Gnuplot, be a PNG file.
  gnuplot.SetTerminal ("png");

  // Set the labels for each axis.
  gnuplot.SetLegend ("Flow", "Throughput");
     
  Gnuplot2dDataset dataset;
  dataset.SetTitle (dataTitle);
  dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);
  
  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();
  
  // call the flow monitor function
  ThroughputMonitor(&flowHelper, flowMonitor, dataset);

  //Simulator::Stop (Seconds(4000.0));
  Simulator::Stop (Seconds(50.0)); // for testing/debugging only
  Simulator::Run ();
  
  //Gnuplot ...continued
  gnuplot.AddDataset (dataset);
  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str());
  // Write the plot file.
  gnuplot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();
  
  // Print per flow statistics
  flowMonitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();
  /*
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
  {
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);
	  NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
	  NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
	  NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
	  NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
  }       
  */
  flowMonitor->SerializeToXmlFile("./scratch/myManet.xml", true, true);

  Simulator::Destroy ();
  delete [] ptrdata;

  return 0;
}

