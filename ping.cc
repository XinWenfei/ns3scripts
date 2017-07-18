#include "ns3/core-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/v4ping.h"

using namespace ns3;

int main(int argc, char *argv[])
{
	Time::SetResolution (Time::NS);
	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  CommandLine cmd;
  cmd.Parse (argc, argv);

  /* Configuration. */

  /* Build nodes. */
  NodeContainer term_0;
  term_0.Create (1);
  NodeContainer term_1;
  term_1.Create (1);

  /* Build link. */
  CsmaHelper csma_hub_0;
  csma_hub_0.SetChannelAttribute ("DataRate", DataRateValue (100000000));
  csma_hub_0.SetChannelAttribute ("Delay",  TimeValue (MilliSeconds (10000)));

  /* Build link net device container. */
  NodeContainer all_hub_0;
  all_hub_0.Add (term_0);
  all_hub_0.Add (term_1);
  NetDeviceContainer ndc_hub_0 = csma_hub_0.Install (all_hub_0);

  /* Install the IP stack. */
  InternetStackHelper internetStackH;
  internetStackH.Install (term_0);
  internetStackH.Install (term_1);

  /* IP assign. */
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_hub_0 = ipv4.Assign (ndc_hub_0);

  /* Generate Route. */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Generate Application. */
  InetSocketAddress dst_ping_0 = InetSocketAddress (iface_ndc_hub_0.GetAddress(1));
  OnOffHelper onoff_ping_0 = OnOffHelper ("ns3::Ipv4RawSocketFactory", dst_ping_0);
  onoff_ping_0.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onoff_ping_0.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer apps_ping_0 = onoff_ping_0.Install(term_0.Get(0));
  apps_ping_0.Start (Seconds (1.1));
  apps_ping_0.Stop (Seconds (10.1));
  PacketSinkHelper sink_ping_0 = PacketSinkHelper ("ns3::Ipv4RawSocketFactory", dst_ping_0);
  apps_ping_0 = sink_ping_0.Install (term_1.Get(0));
  apps_ping_0.Start (Seconds (1.0));
  apps_ping_0.Stop (Seconds (10.2));
  V4PingHelper ping_ping_0 = V4PingHelper(iface_ndc_hub_0.GetAddress(1));
  apps_ping_0 = ping_ping_0.Install(term_0.Get(0));
  apps_ping_0.Start (Seconds (1.2));
  apps_ping_0.Stop (Seconds (10.0));

  /* Simulation. */
  /* Pcap output. */
  /* Stop the simulation after x seconds. */
  uint32_t stopTime = 11;
  Simulator::Stop (Seconds (stopTime));
  /* Start and clean simulation. */
  Simulator::Run ();
  Simulator::Destroy ();
}
