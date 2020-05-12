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

// static void
// incomePacketTracer (std::string context, Ptr<const Packet> packet)
// {

//   UniquePacketIdTag tag;
//   // tag.SetUniquePacketId (uniquePacketId);
//   // packet->AddPacketTag (tag); 
//   // uniquePacketId++;
//   // std::cout << "packet size is-->: " << packet->GetSize () << std::endl;

//   // Ptr<Packet> p = const_cast <Packet> (packet);


//   // const IngressTimeHeader timeHeader;
//   // timeHeader.SetData (10);
//   // packet->AddTrailer (timeHeader);


//   // if (packet->FindFirstMatchingByteTag (tag))
//   //   std::cout << "find it!" << std::endl;
//   // else
//   //   std::cout << "not found!" << std::endl;


//   cxt_v.clear ();
//   split(context, '/' , cxt_v);
//   int incomePort = atoi (cxt_v[4].c_str());

//   Ptr<Packet> copy = packet->Copy ();
//   PppHeader ppph;
//   Ipv4Header iph;
//   copy->RemoveHeader (ppph);
//   copy->PeekHeader (iph);

//   if (iph.GetPayloadSize () == 0)
//   {
//     return;
//   }
  
//   *incomePacketTraceStream->GetStream () << iph.GetIdentification () << ","
//                                    << iph.GetPayloadSize () + 20 << ","
//                                    << (uint16_t)iph.GetProtocol () << ","
//                                    << iph.GetSource () << ","
//                                    << iph.GetDestination () << ","
//                                    << incomePort << ","
//                                    << Simulator::Now ().GetMicroSeconds () << std::endl; 

// }


// static void
// outcomePacketTracer (std::string context, Ptr<const Packet> packet)
// {


//   UniquePacketIdTag tag;
//   // tag.SetUniquePacketId (uniquePacketId);
//   // packet->AddPacketTag (tag); 
//   // uniquePacketId++;
//   // std::cout << "packet size is-->: " << packet->GetSize () << std::endl;

//   // Ptr<Packet> p = const_cast <Packet> (packet);


//   // const IngressTimeHeader timeHeader;
//   // timeHeader.SetData (10);
//   // packet->AddTrailer (timeHeader);


//   if (packet->FindFirstMatchingByteTag (tag))
//   {
//     std::cout << "find it: " << tag.GetUniquePacketId () << std::endl;
//   }
//   else
//     std::cout << "not found!" << std::endl;

  

//   cxt_v.clear ();
//   split(context, '/' , cxt_v);
//   int outcomePort = atoi (cxt_v [4].c_str ());
// // std::cout << "packet size is+++++++: " << packet->GetSize () << std::endl;

//   Ptr<Packet> copy = packet->Copy ();

//   // UniquePacketIdTag tag;


//   PppHeader ppph;
//   Ipv4Header iph;
//   copy->RemoveHeader (ppph);
//   copy->PeekHeader (iph);



// //   bool isTag = packet->PeekPacketTag (tag);
// //   if (isTag)
// //   {
// //   std::cout << tag.GetUniquePacketId () << std::endl;
// // }
// // else 
// //   std::cout << "not tag" << std::endl;



//   if (iph.GetPayloadSize () == 0)
//   {
//     return;
//   }
//   // *outcomePacketTraceStream->GetStream () << "wtf: " << packet->ToString () << std::endl;
//   *outcomePacketTraceStream->GetStream () << iph.GetIdentification () << ","
//                                    << iph.GetPayloadSize () + 20  << ","
//                                    << (uint16_t)iph.GetProtocol () << ","
//                                    << iph.GetSource () << ","
//                                    << iph.GetDestination () << ","
//                                    << outcomePort << ","
//                                    << Simulator::Now ().GetMicroSeconds () << std::endl; 

// }


// static void
// TraceIncomePacket (std::string node)
// {

//   std::string traceTarget = "/NodeList/" + node + "/DeviceList/*/$ns3::PointToPointNetDevice/MacRx";

//   // Config::ConnectWithoutContext (traceTarget, MakeCallback (&PacketTracer));
//  Config::Connect (traceTarget, MakeCallback (&incomePacketTracer));                                   
// }

// static void
// TraceOutcomePacket (std::string node)
// {

//   std::string traceTarget = "/NodeList/" + node + "/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxEnd";

//   // Config::ConnectWithoutContext (traceTarget, MakeCallback (&PacketTracer));
//  Config::Connect (traceTarget, MakeCallback (&outcomePacketTracer));                                   
// }


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
  clientHelperTcp.SetAttribute ("PacketSize", UintegerValue (10));
  clientHelperTcp.SetAttribute ("DataRate", DataRateValue (DataRate ("1Mb/s")));

  // Connection two
  OnOffHelper clientHelperUdp ("ns3::UdpSocketFactory", Address ());
  clientHelperUdp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelperUdp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelperUdp.SetAttribute ("PacketSize", UintegerValue (10));
  clientHelperUdp.SetAttribute ("DataRate", DataRateValue (DataRate ("1Mb/s")));

  ApplicationContainer clientAppsTcp[10];
  AddressValue remoteAddressTcp (InetSocketAddress (Ipv4Address (serverIp.c_str ()), tcpPort));
  clientHelperTcp.SetAttribute ("Remote", remoteAddressTcp);

  ApplicationContainer clientAppsUdp[10];
  AddressValue remoteAddressUdp (InetSocketAddress (Ipv4Address (serverIp.c_str ()), udpPort));
  clientHelperUdp.SetAttribute ("Remote", remoteAddressUdp);


  for (uint32_t i = 0; i < terminals.GetN (); i++)
   {
     // uint32_t durationTimeNum = i % 10 + 1;
     if (i % 2 == TCPAPP)  
     {
      std::cout << "Sum: " << terminals.GetN () << "  i:  " << i << std::endl;
      // std::cout << "tcp app start time: " << first_client_start_time + i * miniFlowDurationTime << std::endl;
       clientAppsTcp[i].Add (clientHelperTcp.Install (terminals.Get (i)));
       clientAppsTcp[i].Start (Seconds (first_client_start_time));// + i * miniFlowDurationTime));
       // clientAppsTcp[i].Stop (Seconds (first_client_start_time + durationTimeNum * miniFlowDurationTime));
clientAppsTcp[i].Stop (Seconds (global_stop_time));
     }
     // else if ( i % 2 == UDPAPP)
     // {
     //  std::cout << "Sum: " << terminals.GetN () << "  i:  " << i << std::endl;
     //  // std::cout << "udp app start time: " << first_client_start_time + i * miniFlowDurationTime << std::endl;
     //   clientAppsUdp[i].Add (clientHelperUdp.Install (terminals.Get (i)));
     //   clientAppsUdp[i].Start (Seconds (first_client_start_time));// + i * miniFlowDurationTime));
     //   // clientAppsUdp[i].Stop (Seconds (first_client_start_time + durationTimeNum * miniFlowDurationTime)); 
     //  clientAppsUdp[i].Stop (Seconds (global_stop_time));
     // }
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


  global_start_time = 0.0;
  sink_start_time = global_start_time;
  first_client_start_time = 0;
  global_stop_time = 20.0;
  sink_stop_time = global_stop_time;
  last_client_stop_time = global_stop_time;


  NodeContainer terminalsFifo;
  NodeContainer terminalsSp;
  NodeContainer terminalsDrr;
  NodeContainer terminalsWfq;

  terminalsFifo.Create (10);
  terminalsSp.Create (10);
  terminalsDrr.Create (10);
  terminalsWfq.Create (10);

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
  tchFifo.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize",
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
  internet.Install (router);

  NetDeviceContainer csmaLinkFifo[10];
  NetDeviceContainer csmaLinkSp[10];
  NetDeviceContainer csmaLinkDrr[10];
  NetDeviceContainer csmaLinkWfq[10];

  for (int i = 0; i < 10; i++)
  {
    
      csmaLinkFifo[i] = csma.Install (NodeContainer (terminalsFifo.Get (i), routerForTerminalsFifo.Get (0)));
      csmaLinkSp[i] = csma.Install (NodeContainer (terminalsSp.Get (i), routerForTerminalsSp.Get (0)));
      csmaLinkDrr[i] = csma.Install (NodeContainer (terminalsDrr.Get (i), routerForTerminalsDrr.Get (0)));
      csmaLinkWfq[i] = csma.Install (NodeContainer (terminalsWfq.Get (i), routerForTerminalsWfq.Get (0)));
      

      terminalsFifoDevices.Add (csmaLinkFifo[i].Get (0));
      routerForTerminalsFifoDevices.Add (csmaLinkFifo[i].Get (1));

      terminalsSpDevices.Add (csmaLinkSp[i].Get (0));
      routerForTerminalsSpDevices.Add (csmaLinkSp[i].Get (1));

      terminalsDrrDevices.Add (csmaLinkDrr[i].Get (0));
      routerForTerminalsDrrDevices.Add (csmaLinkDrr[i].Get (1));

      terminalsWfqDevices.Add (csmaLinkWfq[i].Get (0));
      routerForTerminalsWfqDevices.Add (csmaLinkWfq[i].Get (1));
  }
std::cout << "1" << std::endl;
  NetDeviceContainer csmaLinkFifoSwToRouter;
  NetDeviceContainer csmaLinkSpSwToRouter;
  NetDeviceContainer csmaLinkDrrSwToRouter;
  NetDeviceContainer csmaLinkWfqSwToRouter;

  csmaLinkFifoSwToRouter = csma.Install (NodeContainer (routerForTerminalsFifo.Get (0), router.Get (0)));
  routerForTerminalsFifoDevices.Add (csmaLinkFifoSwToRouter.Get (0));
  routerDevices.Add (csmaLinkFifoSwToRouter.Get (1));

  csmaLinkSpSwToRouter = csma.Install (NodeContainer (routerForTerminalsSp.Get (0), router.Get (0)));
  routerForTerminalsSpDevices.Add (csmaLinkSpSwToRouter.Get (0));
  routerDevices.Add (csmaLinkSpSwToRouter.Get (1));

  csmaLinkDrrSwToRouter = csma.Install (NodeContainer (routerForTerminalsDrr.Get (0), router.Get (0)));
  routerForTerminalsDrrDevices.Add (csmaLinkDrrSwToRouter.Get (0));
  routerDevices.Add (csmaLinkDrrSwToRouter.Get (1));

  csmaLinkWfqSwToRouter = csma.Install (NodeContainer (routerForTerminalsWfq.Get (0), router.Get (0)));
  routerForTerminalsWfqDevices.Add (csmaLinkWfqSwToRouter.Get (0));
  routerDevices.Add (csmaLinkWfqSwToRouter.Get (1));


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

std::cout << "2" << std::endl;

  tchFifo.Install (routerDevices.Get (4));
std::cout << "2.1" << std::endl;
  tchSp.Install (routerDevices.Get (5));
std::cout << "2.2" << std::endl;
  tchDrr.Install (routerDevices.Get (6));
std::cout << "2.3" << std::endl;
  tchWfq.Install (routerDevices.Get (7));
std::cout << "2.4" << std::endl;


  // terminals ip
  std::string ipBaseFifo = "111.1.0.0";
  ipv4.SetBase (&ipBaseFifo[0], "255.255.0.0");
  ipv4.Assign (terminalsFifoDevices);

  std::string ipBaseSp = "112.1.0.0";
  ipv4.SetBase (&ipBaseSp[0], "255.255.0.0");
  ipv4.Assign (terminalsSpDevices);

  std::string ipBaseDrr = "113.1.0.0";
  ipv4.SetBase (&ipBaseDrr[0], "255.255.0.0");
  ipv4.Assign (terminalsDrrDevices);

  std::string ipBaseWfq = "114.1.0.0";
  ipv4.SetBase (&ipBaseWfq[0], "255.255.0.0");
  ipv4.Assign (terminalsWfqDevices);

std::cout << "3" << std::endl;
  // router ip (terminal)
  assignIp (routerDevices.Get (0), "111.1.254.254");
  assignIp (routerDevices.Get (1), "112.1.254.254");
  assignIp (routerDevices.Get (2), "113.1.254.254");
  assignIp (routerDevices.Get (3), "114.1.254.254");
std::cout << "3.1" << std::endl;
  // router ip (server)
  assignIp (routerDevices.Get (4), "11.1.254.254");
  assignIp (routerDevices.Get (5), "12.1.254.254");
  assignIp (routerDevices.Get (6), "13.1.254.254");
  assignIp (routerDevices.Get (7), "14.1.254.254");

  // server ip
  assignIp (serversFifoDevices.Get (0), "11.1.1.1");
std::cout << "3.2.1" << std::endl;
  assignIp (serversSpDevices.Get (0), "12.1.1.1");
std::cout << "3.2.2" << std::endl;
  assignIp (serversDrrDevices.Get (0), "13.1.1.1");
std::cout << "3.2.3" << std::endl;
  assignIp (serversWfqDevices.Get (0), "14.1.1.1");

std::cout << "3.3" << std::endl;

  BridgeHelper bridge;
  bridge.Install (routerForTerminalsFifo.Get (0) , routerForTerminalsFifoDevices);
  bridge.Install (routerForTerminalsSp.Get (0) , routerForTerminalsSpDevices);
  bridge.Install (routerForTerminalsDrr.Get (0) , routerForTerminalsDrrDevices);
  bridge.Install (routerForTerminalsWfq.Get (0) , routerForTerminalsWfqDevices);

// std::cout << "====================3" << std::endl;
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  BuildApps (terminalsFifo, serversFifo, "11.1.1.1", 1);
  BuildApps (terminalsSp, serversSp, "12.1.1.1", 1);
  BuildApps (terminalsDrr, serversDrr, "13.1.1.1", 1);
  BuildApps (terminalsWfq, serversWfq, "14.1.1.1", 1);
std::cout << "4" << std::endl;

  AsciiTraceHelper ascii;
//   incomePacketTraceStream = ascii.CreateFileStream ("/home/guolab/LFS/NS3/drr-router-income.csv");
//   outcomePacketTraceStream = ascii.CreateFileStream ("/home/guolab/LFS/NS3/drr-router-outcome.csv");

//   *incomePacketTraceStream->GetStream () << "PacketId" << ","
//                         << "PacketSize" << ","
//                         << "Protocol" << ","
//                         << "Source" << ","
//                         << "Destination" << ","
//                         << "IngressPort" << ","
//                         << "PhyRxEndTime" << std::endl;

//   *outcomePacketTraceStream->GetStream () << "PacketId" << ","
//                         << "PacketSize" << ","
//                         << "Protocol" << ","
//                         << "Source" << ","
//                         << "Destination" << ","
//                         << "EgressPort" << ","
//                         << "PhyTxEndTime" << std::endl;


std::string pathOut = "/home/guolab/LFS/NS3";
  if (true)
    {
      // PointToPointHelper ptp;
      std::stringstream stmp;
      stmp << pathOut << "/DRR-csma";
      csma.EnablePcapAll (stmp.str ().c_str (), false);
    }

//   TraceIncomePacket ("11");
//   TraceOutcomePacket ("11");

  p2p.EnableAsciiAll (ascii.CreateFileStream ("/home/guolab/LFS/NS3/traffic-schedule-trace.tr"));
  // p2p.EnablePcapAll ()

  Simulator::Stop (Seconds (sink_stop_time));
  Simulator::Run ();

  Simulator::Destroy ();

  set_end_time ();

  print_consumed_time ();

  return 0;
}