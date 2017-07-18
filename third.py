import ns.core
import ns.internet
import ns.point_to_point
import ns.applications
import ns.network
import ns.csma
import ns.wifi
import ns.mobility

ns.core.LogComponentEnable("UdpEchoClientApplication", ns.core.LOG_LEVEL_INFO)
ns.core.LogComponentEnable("UdpEchoServerApplication", ns.core.LOG_LEVEL_INFO)

p2pNodes = ns.network.NodeContainer()
p2pNodes.Create(2)

pointToPoint = ns.point_to_point.PointToPointHelper()
pointToPoint.SetDeviceAttribute("DataRate", ns.core.StringValue("5Mbps"))
pointToPoint.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))
p2pDevices = pointToPoint.Install(p2pNodes)

nCsma = 3
csmaNodes = ns.network.NodeContainer()
csmaNodes.Add(p2pNodes.Get(1))
csmaNodes.Create(nCsma)

csma = ns.csma.CsmaHelper()
csma.SetChannelAttribute("DataRate", ns.core.StringValue("100Mbps"))
csma.SetChannelAttribute("Delay", ns.core.TimeValue(ns.core.NanoSeconds(65600)))
csmaDevices = csma.Install(csmaNodes)

nWifi = 3
wifiStaNodes = ns.network.NodeContainer()
wifiStaNodes.Create(nWifi)
wifiApNode = p2pNodes.Get(0)

wifiChannel = ns.wifi.YansWifiChannelHelper.Default()
wifiPhy = ns.wifi.YansWifiPhyHelper.Default()
wifiPhy.SetChannel(wifiChannel.Create())

wifi = ns.wifi.WifiHelper.Default()
wifi.SetRemoteStationManager("ns3::AarfWifiManager")

mac = ns.wifi.NqosWifiMacHelper.Default()
ssid = ns.wifi.Ssid("ns-3-ssid")

mac.SetType("ns3::StaWifiMac","Ssid",ns.wifi.SsidValue(ssid), "ActiveProbing", ns.core.BooleanValue(False))
staDevices = wifi.Install(wifiPhy,mac,wifiStaNodes)

mac.SetType("ns3::ApWifiMac","Ssid",ns.wifi.SsidValue(ssid))
apDevices = wifi.Install(wifiPhy, mac, wifiApNode)

mobility = ns.mobility.MobilityHelper()
mobility.SetPositionAllocator("ns3::GridPositionAllocator", 
    "MinX", ns.core.DoubleValue(0.0),
    "MinY", ns.core.DoubleValue(0.0),
    "DeltaX", ns.core.DoubleValue(5.0),
    "DeltaY", ns.core.DoubleValue(10.0),
    "GridWidth", ns.core.UintegerValue(3),
    "LayoutType", ns.core.StringValue("RowFirst"))
mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
    "Bounds", ns.mobility.RectangleValue(ns.mobility.Rectangle(-50,50,-50,50)))
mobility.Install(wifiStaNodes)

mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel")
mobility.Install(wifiApNode)

stack = ns.internet.InternetStackHelper()
stack.Install(csmaNodes)
stack.Install(wifiApNode)
stack.Install(wifiStaNodes)

address = ns.internet.Ipv4AddressHelper()
address.SetBase(ns.network.Ipv4Address("10.1.1.0"),ns.network.Ipv4Mask("255.255.255.0"))
p2pInterfaces = address.Assign(p2pDevices)

address.SetBase(ns.network.Ipv4Address("10.1.2.0"),ns.network.Ipv4Mask("255.255.255.0"))
csmaInterfaces = address.Assign(csmaDevices)

address.SetBase(ns.network.Ipv4Address("10.1.3.0"),ns.network.Ipv4Mask("255.255.255.0"))
address.Assign(staDevices)
address.Assign(apDevices)

echoServer = ns.applications.UdpEchoServerHelper(9)

serverApps = echoServer.Install(csmaNodes.Get(nCsma))
serverApps.Start(ns.core.Seconds(1.0))
serverApps.Stop(ns.core.Seconds(20.0))

echoClient = ns.applications.UdpEchoClientHelper(csmaInterfaces.GetAddress(nCsma),9)
echoClient.SetAttribute("MaxPackets", ns.core.UintegerValue(5))
echoClient.SetAttribute("Interval", ns.core.TimeValue(ns.core.Seconds(1.0)))
echoClient.SetAttribute("PacketSize", ns.core.UintegerValue(1024))

clientApps = echoClient.Install(wifiStaNodes.Get(nWifi-1))
clientApps.Start(ns.core.Seconds(2.0))
clientApps.Stop(ns.core.Seconds(20.0))

ns.internet.Ipv4GlobalRoutingHelper.PopulateRoutingTables()
ns.core.Simulator.Stop(ns.core.Seconds(10.0))
pointToPoint.EnablePcapAll("third")
csma.EnablePcap("third", csmaDevices.Get(1), True)
wifiPhy.EnablePcap("third", apDevices.Get(0))

ns.core.Simulator.Run()
ns.core.Simulator.Destroy()
