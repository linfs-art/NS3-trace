/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 NITK Surathkal
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
 * Authors: Akhil Udathu <akhilu077@gmail.com>
 *					Kaushik S Kalmady <kaushikskalmady@gmail.com>
 *					Vilas M <vilasnitk19@gmail.com>
 *
 */

/** Network topology
 *
 *    2Mb/s, 2ms    _____________              
 * n0--------------|             |        
 * .               |             |
 * .               |   router    |------------n11
 * .  2Mb/s, 2ms   |             |
 * n10-------------|_____________|  
 *
 */

#include <string.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/queue.h"
#include "ns3/ppp-header.h"
#include "ns3/log.h"
#include "ns3/unique-packet-id.h"
#include "ns3/my-trace-header.h"
#include "ns3/ipv4-queue-disc-item.h"

#include <string>
#include <vector>
#include <sys/sysinfo.h>
#include <sys/time.h>

using namespace ns3;

#define UDPAPP 0
#define TCPAPP 1



// The times
double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double first_client_start_time;
double last_client_stop_time;




int terminalsNumPerSchedule = 500;
static Ptr<OutputStreamWrapper> incomePacketTraceStream;
static Ptr<OutputStreamWrapper> outcomePacketTraceStream;


double start_time;
double end_time;


/* us */
double now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void set_start_time ()
{
  start_time = now ();
}

void set_end_time ()
{
  end_time = now ();
}

void print_consumed_time ()   
{
  /* seconds */
  double total_time = (end_time - start_time) / 1000000.0;

  fprintf(stderr, "Total time consumed: %f seconds\n", total_time);  
}


void split(const std::string &s,char delimiter,std::vector<std::string> &v)
{
    std::string::size_type i = 0;
    std::string::size_type j = s.find(delimiter);
    while(j != std::string::npos)
    {
        v.push_back(s.substr(i,j-i));
        i = ++j;
        j = s.find(delimiter,i);
    }
    if(j == std::string::npos)
        v.push_back(s.substr(i,s.length()));
}

std::vector<std::string> cxt_v;

static void
incomePacketTracer (std::string context, Ptr<const Packet> packet)
{

  UniquePacketIdTag tag;
  // tag.SetUniquePacketId (uniquePacketId);
  // packet->AddPacketTag (tag); 
  // uniquePacketId++;
  // // std::cout << "packet size is-->: " << packet->GetSize () << std::endl;

  // Ptr<Packet> p = const_cast <Packet> (packet);


  // const IngressTimeHeader timeHeader;
  // timeHeader.SetData (10);
  // packet->AddTrailer (timeHeader);


  // if (packet->FindFirstMatchingByteTag (tag))
  //   // std::cout << "find it!" << std::endl;
  // else
  //   // std::cout << "not found!" << std::endl;


  cxt_v.clear ();
  split(context, '/' , cxt_v);
  int incomePort = atoi (cxt_v[4].c_str());

  Ptr<Packet> copy = packet->Copy ();
  PppHeader ppph;
  Ipv4Header iph;
  copy->RemoveHeader (ppph);
  copy->PeekHeader (iph);

  if (iph.GetPayloadSize () == 0)
  {
    return;
  }


  if (packet->FindFirstMatchingByteTag (tag))
  {
    *incomePacketTraceStream->GetStream () << (uint64_t) tag.GetUniquePacketId () << ","
                                   << iph.GetPayloadSize () + 20 << ","
                                   << (uint16_t)iph.GetProtocol () << ","
                                   << iph.GetSource () << ","
                                   << iph.GetDestination () << ","
                                   << incomePort << ","
                                   << Simulator::Now ().GetNanoSeconds () << std::endl; 
  }
}


static void
outcomePacketTracer (std::string context, Ptr<const Packet> packet)
{


  UniquePacketIdTag tag;
  // tag.SetUniquePacketId (uniquePacketId);
  // packet->AddPacketTag (tag); 
  // uniquePacketId++;
  // // std::cout << "packet size is-->: " << packet->GetSize () << std::endl;

  // Ptr<Packet> p = const_cast <Packet> (packet);


  // const IngressTimeHeader timeHeader;
  // timeHeader.SetData (10);
  // packet->AddTrailer (timeHeader);


  // if (packet->FindFirstMatchingByteTag (tag))
  // {
  //   // std::cout << "find it: " << tag.GetUniquePacketId () << std::endl;
  // }
  // else
    // std::cout << "not found!" << std::endl;

  

  cxt_v.clear ();
  split(context, '/' , cxt_v);
  int outcomePort = atoi (cxt_v [4].c_str ());
// // std::cout << "packet size is+++++++: " << packet->GetSize () << std::endl;

  Ptr<Packet> copy = packet->Copy ();

  // UniquePacketIdTag tag;


  PppHeader ppph;
  Ipv4Header iph;
  copy->RemoveHeader (ppph);
  copy->PeekHeader (iph);



//   bool isTag = packet->PeekPacketTag (tag);
//   if (isTag)
//   {
//   // std::cout << tag.GetUniquePacketId () << std::endl;
// }
// else 
//   // std::cout << "not tag" << std::endl;



  if (iph.GetPayloadSize () == 0)
  {
    return;
  }
  if (packet->FindFirstMatchingByteTag (tag))
  {
    // *outcomePacketTraceStream->GetStream () << "wtf: " << packet->ToString () << std::endl;
    *outcomePacketTraceStream->GetStream () << (uint64_t) tag.GetUniquePacketId () << ","
                                   << iph.GetPayloadSize () + 20  << ","
                                   << (uint16_t)iph.GetProtocol () << ","
                                   << iph.GetSource () << ","
                                   << iph.GetDestination () << ","
                                   << outcomePort << ","
                                   << Simulator::Now ().GetNanoSeconds () << std::endl; 
  }
}


static void
TraceIncomePacket (std::string node)
{

  std::string traceTarget = "/NodeList/" + node + "/DeviceList/*/$ns3::PointToPointNetDevice/MacRx";

  // Config::ConnectWithoutContext (traceTarget, MakeCallback (&PacketTracer));
 Config::Connect (traceTarget, MakeCallback (&incomePacketTracer));                                   
}

static void
TraceOutcomePacket (std::string node)
{

  std::string traceTarget = "/NodeList/" + node + "/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxEnd";

  // Config::ConnectWithoutContext (traceTarget, MakeCallback (&PacketTracer));
 Config::Connect (traceTarget, MakeCallback (&outcomePacketTracer));                                   
}


void
BuildApps (NodeContainer terminals, NodeContainer servers,
           std::string serverIp, uint32_t miniFlowDurationTime)
{
  // SINK is in the right side
  uint16_t tcpPort = 6666;
  uint16_t udpPort = 8888;
  Address sinkLocalAddressTcp (InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
  PacketSinkHelper sinkHelperTcp ("ns3::TcpSocketFactory", sinkLocalAddressTcp);

  Address sinkLocalAddressUdp (InetSocketAddress (Ipv4Address::GetAny (), udpPort));
  PacketSinkHelper sinkHelperUdp ("ns3::UdpSocketFactory", sinkLocalAddressUdp);


  ApplicationContainer sinkAppTcp = sinkHelperTcp.Install (servers.Get (0));
  ApplicationContainer sinkAppUdp = sinkHelperUdp.Install (servers.Get (0));

  sinkAppUdp.Start (Seconds (sink_start_time));
  sinkAppUdp.Stop (Seconds (sink_stop_time));

  sinkAppTcp.Start (Seconds (sink_start_time));
  sinkAppTcp.Stop (Seconds (sink_stop_time));

  // Connection one
  // Clients are in left side
  /*
   * Create the OnOff applications to send TCP to the server
   * onoffhelper is a client that send data to TCP destination
  */
  OnOffHelper clientHelperTcp ("ns3::TcpSocketFactory", Address ());
  clientHelperTcp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelperTcp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelperTcp.SetAttribute ("PacketSize", UintegerValue (800));
  clientHelperTcp.SetAttribute ("DataRate", DataRateValue (DataRate ("1Mb/s")));

  // Connection two
  OnOffHelper clientHelperUdp ("ns3::UdpSocketFactory", Address ());
  clientHelperUdp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelperUdp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelperUdp.SetAttribute ("PacketSize", UintegerValue (800));
  clientHelperUdp.SetAttribute ("DataRate", DataRateValue (DataRate ("1Mb/s")));

  ApplicationContainer clientAppsTcp[terminalsNumPerSchedule];
  AddressValue remoteAddressTcp (InetSocketAddress (Ipv4Address (serverIp.c_str ()), tcpPort));
  clientHelperTcp.SetAttribute ("Remote", remoteAddressTcp);

  ApplicationContainer clientAppsUdp[terminalsNumPerSchedule];
  AddressValue remoteAddressUdp (InetSocketAddress (Ipv4Address (serverIp.c_str ()), udpPort));
  clientHelperUdp.SetAttribute ("Remote", remoteAddressUdp);


  uint32_t start_time = 0;
  uint32_t stop_time = 0;
  for (uint32_t i = 0; i < terminals.GetN (); i++)
   {
     uint32_t durationTimeNum = i % 20 + 1;
     start_time = first_client_start_time + i * miniFlowDurationTime;
     stop_time = start_time + durationTimeNum * miniFlowDurationTime;
     if (start_time > global_stop_time)
     {
      start_time = global_stop_time - 10;
      stop_time = global_stop_time;
     }
     if (i % 2 == TCPAPP)  
     {
      // std::cout << "Sum: " << terminals.GetN () << "  i:  " << i << std::endl;
      // // std::cout << "tcp app start time: " << first_client_start_time + i * miniFlowDurationTime << std::endl;
       clientAppsTcp[i].Add (clientHelperTcp.Install (terminals.Get (i)));
       // clientAppsTcp[i].Start (Seconds (first_client_start_time + i * miniFlowDurationTime));
       // clientAppsTcp[i].Stop (Seconds (first_client_start_time + durationTimeNum * miniFlowDurationTime));

          clientAppsTcp[i].Start (Seconds (start_time));
          clientAppsTcp[i].Stop (Seconds (stop_time));
     }
     else if ( i % 2 == UDPAPP)
     {
      // std::cout << "Sum: " << terminals.GetN () << "  i:  " << i << std::endl;
      // // std::cout << "udp app start time: " << first_client_start_time + i * miniFlowDurationTime << std::endl;
       clientAppsUdp[i].Add (clientHelperUdp.Install (terminals.Get (i)));
       // clientAppsUdp[i].Start (Seconds (first_client_start_time + i * miniFlowDurationTime));
       // clientAppsUdp[i].Stop (Seconds (first_client_start_time + durationTimeNum * miniFlowDurationTime)); 

          clientAppsUdp[i].Start (Seconds (start_time));
          clientAppsUdp[i].Stop (Seconds (stop_time));

     }
   }
}

void assignIp (Ptr<NetDevice> netDevice, std::string addrStr)
{
  Ptr<Node> node = netDevice->GetNode();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

  int32_t interface = ipv4->GetInterfaceForDevice (netDevice);

 if (interface == -1) {
   interface = ipv4->AddInterface (netDevice);
 }

  Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (Ipv4Address(addrStr.c_str ()), Ipv4Mask ("/16"));
  ipv4->AddAddress (interface, ipv4Addr);
  ipv4->SetMetric (interface, 1);
  ipv4->SetUp (interface);
}


int
main (int argc, char *argv[])
{

  set_start_time ();

  freopen("/home/guolab/LFS/NS3/traffic-trace-queues.csv", "w", stdout);
        std::cout << "Type," << "Queue," << "Quantum,"
            << "PacketUid" << std::endl;

  global_start_time = 0.0;
  sink_start_time = global_start_time;
  first_client_start_time = 0;
  global_stop_time = 500.0;
  sink_stop_time = global_stop_time;
  last_client_stop_time = global_stop_time;


  NodeContainer terminalsFifo;
  NodeContainer terminalsSp;
  NodeContainer terminalsDrr;
  NodeContainer terminalsWfq;

  terminalsFifo.Create (terminalsNumPerSchedule);
  terminalsSp.Create (terminalsNumPerSchedule);
  terminalsDrr.Create (terminalsNumPerSchedule);
  terminalsWfq.Create (terminalsNumPerSchedule);

  NodeContainer serversFifo;
  NodeContainer serversSp;
  NodeContainer serversDrr;
  NodeContainer serversWfq;

  serversFifo.Create (1);
  serversSp.Create (1);
  serversDrr.Create (1);
  serversWfq.Create (1);

  NodeContainer routerForTerminalsFifo;
  NodeContainer routerForTerminalsSp;
  NodeContainer routerForTerminalsDrr;
  NodeContainer routerForTerminalsWfq;

  routerForTerminalsFifo.Create (1);
  routerForTerminalsSp.Create (1);
  routerForTerminalsDrr.Create (1);
  routerForTerminalsWfq.Create (1);

  NodeContainer router;
  router.Create (1);

  NetDeviceContainer terminalsFifoDevices;
  NetDeviceContainer terminalsSpDevices;
  NetDeviceContainer terminalsDrrDevices;
  NetDeviceContainer terminalsWfqDevices;

  NetDeviceContainer serversFifoDevices;
  NetDeviceContainer serversSpDevices;
  NetDeviceContainer serversDrrDevices;
  NetDeviceContainer serversWfqDevices;

  NetDeviceContainer routerForTerminalsFifoDevices;
  NetDeviceContainer routerForTerminalsSpDevices;
  NetDeviceContainer routerForTerminalsDrrDevices;
  NetDeviceContainer routerForTerminalsWfqDevices;
  NetDeviceContainer routerDevices;


  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  // 42 = headers size
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000 - 42));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  //uint32_t meanPktSize = 1000;


  // DRR
  TrafficControlHelper tchDrr;
  uint16_t handleDrr = tchDrr.SetRootQueueDisc ("ns3::FqCoDelQueueDisc");
  tchDrr.AddPacketFilter (handleDrr, "ns3::OrdinaryIpv4PacketFilter");


  // WFQ 
  TrafficControlHelper tchWfq;
  uint16_t handleWfq = tchWfq.SetRootQueueDisc ("ns3::WfqDisc");
  tchWfq.AddPacketFilter (handleWfq, "ns3::OrdinaryIpv4PacketFilter");


  // SP
  TrafficControlHelper tchSp;
  uint16_t handleSp = tchSp.SetRootQueueDisc ("ns3::PrioQueueDisc", "Priomap",
                         StringValue ("0 0 2 1 0 0 0 0 0 0 0 0 0 0 0 0"));
  tchSp.AddPacketFilter (handleSp, "ns3::OrdinaryIpv4PacketFilter");

  
  //FIFO
  TrafficControlHelper tchFifo;
  tchFifo.SetRootQueueDisc ("ns3::MyFifoQueueDisc", "MaxSize",
                         StringValue ("10000p"));

  PointToPointHelper p2p;
  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("1000Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("1ms"));


  Ipv4AddressHelper ipv4;

  InternetStackHelper internet;
  internet.Install (terminalsFifo);
  internet.Install (terminalsSp);
  internet.Install (terminalsDrr);
  internet.Install (terminalsWfq);

  internet.Install (serversFifo);
  internet.Install (serversSp);
  internet.Install (serversDrr);
  internet.Install (serversWfq);

  internet.Install (routerForTerminalsFifo);
  internet.Install (routerForTerminalsSp);
  internet.Install (routerForTerminalsDrr);
  internet.Install (routerForTerminalsWfq);

  internet.Install (router);

  NetDeviceContainer p2pLinkFifo[terminalsNumPerSchedule];
  NetDeviceContainer p2pLinkSp[terminalsNumPerSchedule];
  NetDeviceContainer p2pLinkDrr[terminalsNumPerSchedule];
  NetDeviceContainer p2pLinkWfq[terminalsNumPerSchedule];
// std::cout << "0.1" << std::endl;
  for (int i = 0; i < terminalsNumPerSchedule; i++)
  {

      p2pLinkFifo[i] = p2p.Install (NodeContainer (terminalsFifo.Get (i), routerForTerminalsFifo.Get (0)));
      p2pLinkSp[i] = p2p.Install (NodeContainer (terminalsSp.Get (i), routerForTerminalsSp.Get (0)));
      p2pLinkDrr[i] = p2p.Install (NodeContainer (terminalsDrr.Get (i), routerForTerminalsDrr.Get (0)));
      p2pLinkWfq[i] = p2p.Install (NodeContainer (terminalsWfq.Get (i), routerForTerminalsWfq.Get (0)));
      

      terminalsFifoDevices.Add (p2pLinkFifo[i].Get (0));
      routerForTerminalsFifoDevices.Add (p2pLinkFifo[i].Get (1));

      terminalsSpDevices.Add (p2pLinkSp[i].Get (0));
      routerForTerminalsSpDevices.Add (p2pLinkSp[i].Get (1));

      terminalsDrrDevices.Add (p2pLinkDrr[i].Get (0));
      routerForTerminalsDrrDevices.Add (p2pLinkDrr[i].Get (1));

      terminalsWfqDevices.Add (p2pLinkWfq[i].Get (0));
      routerForTerminalsWfqDevices.Add (p2pLinkWfq[i].Get (1));
  }
// std::cout << "1" << std::endl;
  NetDeviceContainer p2pLinkFifoSwToRouter;
  NetDeviceContainer p2pLinkSpSwToRouter;
  NetDeviceContainer p2pLinkDrrSwToRouter;
  NetDeviceContainer p2pLinkWfqSwToRouter;

  p2pLinkFifoSwToRouter = p2p.Install (NodeContainer (routerForTerminalsFifo.Get (0), router.Get (0)));
  routerForTerminalsFifoDevices.Add (p2pLinkFifoSwToRouter.Get (0));
  routerDevices.Add (p2pLinkFifoSwToRouter.Get (1));

  p2pLinkSpSwToRouter = p2p.Install (NodeContainer (routerForTerminalsSp.Get (0), router.Get (0)));
  routerForTerminalsSpDevices.Add (p2pLinkSpSwToRouter.Get (0));
  routerDevices.Add (p2pLinkSpSwToRouter.Get (1));

  p2pLinkDrrSwToRouter = p2p.Install (NodeContainer (routerForTerminalsDrr.Get (0), router.Get (0)));
  routerForTerminalsDrrDevices.Add (p2pLinkDrrSwToRouter.Get (0));
  routerDevices.Add (p2pLinkDrrSwToRouter.Get (1));

  p2pLinkWfqSwToRouter = p2p.Install (NodeContainer (routerForTerminalsWfq.Get (0), router.Get (0)));
  routerForTerminalsWfqDevices.Add (p2pLinkWfqSwToRouter.Get (0));
  routerDevices.Add (p2pLinkWfqSwToRouter.Get (1));


  NetDeviceContainer p2pLinkRouterToFifoServer;
  NetDeviceContainer p2pLinkRouterToSpServer;
  NetDeviceContainer p2pLinkRouterToDrrServer;
  NetDeviceContainer p2pLinkRouterToWfqServer;

  p2pLinkRouterToFifoServer = p2p.Install (NodeContainer (router.Get (0), serversFifo.Get (0)));
  routerDevices.Add (p2pLinkRouterToFifoServer.Get (0));
  serversFifoDevices.Add (p2pLinkRouterToFifoServer.Get (1));

  p2pLinkRouterToSpServer = p2p.Install (NodeContainer (router.Get (0), serversSp.Get (0)));
  routerDevices.Add (p2pLinkRouterToSpServer.Get (0));
  serversSpDevices.Add (p2pLinkRouterToSpServer.Get (1));

  p2pLinkRouterToDrrServer = p2p.Install (NodeContainer (router.Get (0), serversDrr.Get (0)));
  routerDevices.Add (p2pLinkRouterToDrrServer.Get (0));
  serversDrrDevices.Add (p2pLinkRouterToDrrServer.Get (1));

  p2pLinkRouterToWfqServer = p2p.Install (NodeContainer (router.Get (0), serversWfq.Get (0)));
  routerDevices.Add (p2pLinkRouterToWfqServer.Get (0));
  serversWfqDevices.Add (p2pLinkRouterToWfqServer.Get (1));

// std::cout << "2" << std::endl;
  tchFifo.Install (routerDevices.Get (0));
  tchFifo.Install (routerDevices.Get (4));
// std::cout << "2.1" << std::endl;
  tchSp.Install (routerDevices.Get (1));
  tchSp.Install (routerDevices.Get (5));
// std::cout << "2.2" << std::endl;
  tchDrr.Install (routerDevices.Get (2));
  tchDrr.Install (routerDevices.Get (6));
// std::cout << "2.3" << std::endl;
  tchWfq.Install (routerDevices.Get (3));
  tchWfq.Install (routerDevices.Get (7));
// std::cout << "2.4" << std::endl;


  // terminals ip
  for (int i = 0; i < terminalsNumPerSchedule; i++)
  {
    std::string ipBaseFifo ="101.";
    std::string ipBaseSp ="102.";
    std::string ipBaseDrr ="103.";
    std::string ipBaseWfq ="104.";

    int secondByte = (int) (i / 253);
    int thirdByte = i % 253;

    ipBaseFifo = ipBaseFifo + std::to_string (secondByte) + "." + std::to_string (thirdByte) + ".0";
    ipBaseSp = ipBaseSp + std::to_string (secondByte) + "." + std::to_string (thirdByte) + ".0";
    ipBaseDrr = ipBaseDrr + std::to_string (secondByte) + "." + std::to_string (thirdByte) + ".0";
    ipBaseWfq = ipBaseWfq + std::to_string (secondByte) + "." + std::to_string (thirdByte) + ".0";
// std::cout << ipBaseFifo << std::endl;
    ipv4.SetBase (&ipBaseFifo[0], "255.255.255.0");
    ipv4.Assign (p2pLinkFifo[i]);

    ipv4.SetBase (&ipBaseSp[0], "255.255.255.0");
    ipv4.Assign (p2pLinkSp[i]);

    ipv4.SetBase (&ipBaseDrr[0], "255.255.255.0");
    ipv4.Assign (p2pLinkDrr[i]);

    ipv4.SetBase (&ipBaseWfq[0], "255.255.255.0");
    ipv4.Assign (p2pLinkWfq[i]);
  }

  ipv4.SetBase ("201.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkFifoSwToRouter);

  ipv4.SetBase ("202.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkSpSwToRouter);

  ipv4.SetBase ("203.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkDrrSwToRouter);

  ipv4.SetBase ("204.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkWfqSwToRouter);

// std::cout << "3" << std::endl;

// std::cout << "3.1" << std::endl;
  // router ip (server)


  ipv4.SetBase ("11.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkRouterToFifoServer);

  ipv4.SetBase ("12.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkRouterToSpServer);

  ipv4.SetBase ("13.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkRouterToDrrServer);

  ipv4.SetBase ("14.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkRouterToWfqServer);


// std::cout << "3.3" << std::endl;



// // std::cout << "====================3" << std::endl;
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  BuildApps (terminalsFifo, serversFifo, "11.1.1.2", 1);
  BuildApps (terminalsSp, serversSp, "12.1.1.2", 1);
  BuildApps (terminalsDrr, serversDrr, "13.1.1.2", 1);
  BuildApps (terminalsWfq, serversWfq, "14.1.1.2", 1);
// std::cout << "4" << std::endl;

  AsciiTraceHelper ascii;
  incomePacketTraceStream = ascii.CreateFileStream ("/home/guolab/LFS/NS3/traffic-schedule-income.csv");
  outcomePacketTraceStream = ascii.CreateFileStream ("/home/guolab/LFS/NS3/traffic-schedule-outcome.csv");

  *incomePacketTraceStream->GetStream () << "PacketUid" << ","
                        << "PacketSize" << ","
                        << "Protocol" << ","
                        << "Source" << ","
                        << "Destination" << ","
                        << "IngressPort" << ","
                        << "PhyRxEndTime" << std::endl;

  *outcomePacketTraceStream->GetStream () << "PacketUid" << ","
                        << "PacketSize" << ","
                        << "Protocol" << ","
                        << "Source" << ","
                        << "Destination" << ","
                        << "EgressPort" << ","
                        << "PhyTxEndTime" << std::endl;


// std::string pathOut = "/home/guolab/LFS/NS3";
//   if (true)
//     {
//       // PointToPointHelper ptp;
//       std::stringstream stmp;
//       stmp << pathOut << "/p2p";
//       p2p.EnablePcapAll (stmp.str ().c_str (), false);
//     }

  TraceIncomePacket (std::to_string (router.Get (0)->GetId ()));
  TraceOutcomePacket (std::to_string (router.Get (0)->GetId ()));

  // p2p.EnableAsciiAll (ascii.CreateFileStream ("/home/guolab/LFS/NS3/traffic-schedule-trace.tr"));
  // p2p.EnablePcapAll ()
// std::cout << "5" << std::endl;
  Simulator::Stop (Seconds (sink_stop_time));
  Simulator::Run ();

  Simulator::Destroy ();

  set_end_time ();

  print_consumed_time ();

  return 0;
}