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

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <sys/sysinfo.h>
#include <sys/time.h>

#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>

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


std::map<uint32_t, NodeContainer> terminalsMap;
std::map<uint32_t, NodeContainer> serversMap;
std::map<uint32_t, std::string> serversIpMap;


int terminalsNumPerSchedule = 50;
static Ptr<OutputStreamWrapper> incomePacketTraceStream;
static Ptr<OutputStreamWrapper> outcomePacketTraceStream;


double start_time;
double end_time;


static int random_scratch(int min, int max) //range : [min, max)
{
   static bool first_scratch = true;
   if (first_scratch) 
   {  
      srand( time(NULL) ); //seeding for the first time only!
      first_scratch = false;
   }
   return min + rand() % (( max + 1 ) - min);
}


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
                                   << Simulator::Now ().GetMicroSeconds () << std::endl; 
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
                                   << Simulator::Now ().GetMicroSeconds () << std::endl; 
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
BuildAppsServer ( NodeContainer servers, std::string serverIp)
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
}


void
BuildAppsClient (NodeContainer terminals, std::string serverIp, uint32_t miniFlowDurationTime)
{

  // Connection one
  // Clients are in left side
  /*
   * Create the OnOff applications to send TCP to the server
   * onoffhelper is a client that send data to TCP destination
  */
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uint32_t uniform_value = uv->GetValue (1, 15);
  std::string data_rate = std::to_string (uniform_value) + "Mb/s";

  uint16_t tcpPort = 6666;
  uint16_t udpPort = 8888;
  OnOffHelper clientHelperTcp ("ns3::TcpSocketFactory", Address ());
  clientHelperTcp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelperTcp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelperTcp.SetAttribute ("PacketSize", UintegerValue (1400));
  clientHelperTcp.SetAttribute ("DataRate", DataRateValue (DataRate (data_rate)));
  // Connection two
  OnOffHelper clientHelperUdp ("ns3::UdpSocketFactory", Address ());
  clientHelperUdp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelperUdp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelperUdp.SetAttribute ("PacketSize", UintegerValue (1400));
  clientHelperUdp.SetAttribute ("DataRate", DataRateValue (DataRate (data_rate)));

  ApplicationContainer clientAppsTcp[terminalsNumPerSchedule];
  AddressValue remoteAddressTcp (InetSocketAddress (Ipv4Address (serverIp.c_str ()), tcpPort));
  clientHelperTcp.SetAttribute ("Remote", remoteAddressTcp);

  ApplicationContainer clientAppsUdp[terminalsNumPerSchedule];
  AddressValue remoteAddressUdp (InetSocketAddress (Ipv4Address (serverIp.c_str ()), udpPort));
  clientHelperUdp.SetAttribute ("Remote", remoteAddressUdp);
  // uint32_t start_time = 0;
  // uint32_t stop_time = 0;
  for (uint32_t i = 0; i < terminals.GetN (); i++)
   {
     // uint32_t durationTimeNum = i % 10 + 1;
     // start_time = first_client_start_time + i * miniFlowDurationTime;
     // stop_time = start_time + durationTimeNum * miniFlowDurationTime;
     // if (start_time > global_stop_time)
     // {
     //  start_time = global_stop_time - 10;
     //  stop_time = global_stop_time;
     // }

     if (i % 2 == TCPAPP)  
     {
      // std::cout << "Sum: " << terminals.GetN () << "  i:  " << i << std::endl;
      // // std::cout << "tcp app start time: " << first_client_start_time + i * miniFlowDurationTime << std::endl;

       clientAppsTcp[i].Add (clientHelperTcp.Install (terminals.Get (i)));
       // clientAppsTcp[i].Start (Seconds (first_client_start_time + i * miniFlowDurationTime));
       // clientAppsTcp[i].Stop (Seconds (first_client_start_time + durationTimeNum * miniFlowDurationTime));

          clientAppsTcp[i].Start (Seconds (global_start_time));

          clientAppsTcp[i].Stop (Seconds (global_stop_time));
     }
     else if ( i % 2 == UDPAPP)
     {
      // std::cout << "Sum: " << terminals.GetN () << "  i:  " << i << std::endl;
      // // std::cout << "udp app start time: " << first_client_start_time + i * miniFlowDurationTime << std::endl;
       clientAppsUdp[i].Add (clientHelperUdp.Install (terminals.Get (i)));
       // clientAppsUdp[i].Start (Seconds (first_client_start_time + i * miniFlowDurationTime));
       // clientAppsUdp[i].Stop (Seconds (first_client_start_time + durationTimeNum * miniFlowDurationTime)); 

          clientAppsUdp[i].Start (Seconds (global_start_time));
          clientAppsUdp[i].Stop (Seconds (global_stop_time));

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

  freopen("/home/guolab/LFS/NS3/traffic-schedule-data-20200623/forward-matrix-10/traffic-trace-queues.csv", "w", stdout);
  std::cout << "Type," << "Queue," << "Quantum,"
            << "PacketUid," << "w1," << "w2," << "w3,"
            << "w4," << "w5," << "w6," << "w7," << "w8,"
            << "LinkRate," << "Delay," << "Tag" << std::endl;

  std::cout << setiosflags(std::ios::fixed)<< std::setprecision(2);

  global_start_time = 0.0;
  sink_start_time = global_start_time;
  first_client_start_time = 0;
  global_stop_time = 16.0;
  sink_stop_time = global_stop_time;
  last_client_stop_time = global_stop_time;


  NodeContainer terminalsFifoLeft;
  NodeContainer terminalsSpLeft;
  NodeContainer terminalsDrrLeft;
  NodeContainer terminalsWfqLeft;

  NodeContainer terminalsFifoRight;
  NodeContainer terminalsSpRight;
  NodeContainer terminalsDrrRight;
  NodeContainer terminalsWfqRight;


  terminalsFifoLeft.Create (terminalsNumPerSchedule);
  terminalsSpLeft.Create (terminalsNumPerSchedule);
  terminalsDrrLeft.Create (terminalsNumPerSchedule);
  terminalsWfqLeft.Create (terminalsNumPerSchedule);

  terminalsFifoRight.Create (terminalsNumPerSchedule);
  terminalsSpRight.Create (terminalsNumPerSchedule);
  terminalsDrrRight.Create (terminalsNumPerSchedule);
  terminalsWfqRight.Create (terminalsNumPerSchedule);

  terminalsMap[0] = terminalsFifoLeft;
  terminalsMap[1] = terminalsSpLeft;
  terminalsMap[2] = terminalsDrrLeft;
  terminalsMap[3] = terminalsWfqLeft;

  terminalsMap[4] = terminalsFifoRight;
  terminalsMap[5] = terminalsSpRight;
  terminalsMap[6] = terminalsDrrRight;
  terminalsMap[7] = terminalsWfqRight;

  NodeContainer serversFifoRight;
  NodeContainer serversSpRight;
  NodeContainer serversDrrRight;
  NodeContainer serversWfqRight;

  serversFifoRight.Create (1);
  serversSpRight.Create (1);
  serversDrrRight.Create (1);
  serversWfqRight.Create (1);

  NodeContainer routerForTerminalsFifoLeft;
  NodeContainer routerForTerminalsSpLeft;
  NodeContainer routerForTerminalsDrrLeft;
  NodeContainer routerForTerminalsWfqLeft;

  routerForTerminalsFifoLeft.Create (1);
  routerForTerminalsSpLeft.Create (1);
  routerForTerminalsDrrLeft.Create (1);
  routerForTerminalsWfqLeft.Create (1);

  serversMap[0] = serversFifoRight;
  serversMap[1] = serversSpRight;
  serversMap[2] = serversDrrRight;
  serversMap[3] = serversWfqRight;

  serversMap[4] = routerForTerminalsFifoLeft;
  serversMap[5] = routerForTerminalsSpLeft;
  serversMap[6] = routerForTerminalsDrrLeft;
  serversMap[7] = routerForTerminalsWfqLeft;

  NodeContainer router;
  router.Create (1);

  NetDeviceContainer terminalsFifoLeftDevices;
  NetDeviceContainer terminalsSpLeftDevices;
  NetDeviceContainer terminalsDrrLeftDevices;
  NetDeviceContainer terminalsWfqLeftDevices;

  NetDeviceContainer terminalsFifoRightDevices;
  NetDeviceContainer terminalsSpRightDevices;
  NetDeviceContainer terminalsDrrRightDevices;
  NetDeviceContainer terminalsWfqRightDevices;

  NetDeviceContainer serversFifoRightDevices;
  NetDeviceContainer serversSpRightDevices;
  NetDeviceContainer serversDrrRightDevices;
  NetDeviceContainer serversWfqRightDevices;

  NetDeviceContainer routerForTerminalsFifoLeftDevices;
  NetDeviceContainer routerForTerminalsSpLeftDevices;
  NetDeviceContainer routerForTerminalsDrrLeftDevices;
  NetDeviceContainer routerForTerminalsWfqLeftDevices;
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
  p2p.SetDeviceAttribute ("DataRate", StringValue ("20Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("20Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("2ms"));


  Ipv4AddressHelper ipv4;

  InternetStackHelper internet;
  internet.Install (terminalsFifoLeft);
  internet.Install (terminalsSpLeft);
  internet.Install (terminalsDrrLeft);
  internet.Install (terminalsWfqLeft);

  internet.Install (terminalsFifoRight);
  internet.Install (terminalsSpRight);
  internet.Install (terminalsDrrRight);
  internet.Install (terminalsWfqRight);

  internet.Install (serversFifoRight);
  internet.Install (serversSpRight);
  internet.Install (serversDrrRight);
  internet.Install (serversWfqRight);

  internet.Install (routerForTerminalsFifoLeft);
  internet.Install (routerForTerminalsSpLeft);
  internet.Install (routerForTerminalsDrrLeft);
  internet.Install (routerForTerminalsWfqLeft);

  internet.Install (router);

  NetDeviceContainer p2pLinkFifoLeft[terminalsNumPerSchedule];
  NetDeviceContainer p2pLinkSpLeft[terminalsNumPerSchedule];
  NetDeviceContainer p2pLinkDrrLeft[terminalsNumPerSchedule];
  NetDeviceContainer p2pLinkWfqLeft[terminalsNumPerSchedule];
// std::cout << "0.1" << std::endl;
  for (int i = 0; i < terminalsNumPerSchedule; i++)
  {

      p2pLinkFifoLeft[i] = p2p.Install (NodeContainer (terminalsFifoLeft.Get (i), routerForTerminalsFifoLeft.Get (0)));
      p2pLinkSpLeft[i] = p2p.Install (NodeContainer (terminalsSpLeft.Get (i), routerForTerminalsSpLeft.Get (0)));
      p2pLinkDrrLeft[i] = p2p.Install (NodeContainer (terminalsDrrLeft.Get (i), routerForTerminalsDrrLeft.Get (0)));
      p2pLinkWfqLeft[i] = p2p.Install (NodeContainer (terminalsWfqLeft.Get (i), routerForTerminalsWfqLeft.Get (0)));
      

      terminalsFifoLeftDevices.Add (p2pLinkFifoLeft[i].Get (0));
      routerForTerminalsFifoLeftDevices.Add (p2pLinkFifoLeft[i].Get (1));

      terminalsSpLeftDevices.Add (p2pLinkSpLeft[i].Get (0));
      routerForTerminalsSpLeftDevices.Add (p2pLinkSpLeft[i].Get (1));

      terminalsDrrLeftDevices.Add (p2pLinkDrrLeft[i].Get (0));
      routerForTerminalsDrrLeftDevices.Add (p2pLinkDrrLeft[i].Get (1));

      terminalsWfqLeftDevices.Add (p2pLinkWfqLeft[i].Get (0));
      routerForTerminalsWfqLeftDevices.Add (p2pLinkWfqLeft[i].Get (1));
  }


  NetDeviceContainer p2pLinkFifoRight[terminalsNumPerSchedule];
  NetDeviceContainer p2pLinkSpRight[terminalsNumPerSchedule];
  NetDeviceContainer p2pLinkDrrRight[terminalsNumPerSchedule];
  NetDeviceContainer p2pLinkWfqRight[terminalsNumPerSchedule];
// std::cout << "0.1" << std::endl;
  for (int i = 0; i < terminalsNumPerSchedule; i++)
  {

      p2pLinkFifoRight[i] = p2p.Install (NodeContainer (terminalsFifoRight.Get (i), serversFifoRight.Get (0)));
      p2pLinkSpRight[i] = p2p.Install (NodeContainer (terminalsSpRight.Get (i), serversSpRight.Get (0)));
      p2pLinkDrrRight[i] = p2p.Install (NodeContainer (terminalsDrrRight.Get (i), serversDrrRight.Get (0)));
      p2pLinkWfqRight[i] = p2p.Install (NodeContainer (terminalsWfqRight.Get (i), serversWfqRight.Get (0)));
      

      terminalsFifoRightDevices.Add (p2pLinkFifoRight[i].Get (0));
      serversFifoRightDevices.Add (p2pLinkFifoRight[i].Get (1));

      terminalsSpRightDevices.Add (p2pLinkSpRight[i].Get (0));
      serversSpRightDevices.Add (p2pLinkSpRight[i].Get (1));

      terminalsDrrRightDevices.Add (p2pLinkDrrRight[i].Get (0));
      serversDrrRightDevices.Add (p2pLinkDrrRight[i].Get (1));

      terminalsWfqRightDevices.Add (p2pLinkWfqRight[i].Get (0));
      serversWfqRightDevices.Add (p2pLinkWfqRight[i].Get (1));
  }



// std::cout << "1" << std::endl;
  NetDeviceContainer p2pLinkLeftFifoSwToRouter;
  NetDeviceContainer p2pLinkLeftSpSwToRouter;
  NetDeviceContainer p2pLinkLeftDrrSwToRouter;
  NetDeviceContainer p2pLinkLeftWfqSwToRouter;

  p2pLinkLeftFifoSwToRouter = p2p.Install (NodeContainer (routerForTerminalsFifoLeft.Get (0), router.Get (0)));
  routerForTerminalsFifoLeftDevices.Add (p2pLinkLeftFifoSwToRouter.Get (0));
  routerDevices.Add (p2pLinkLeftFifoSwToRouter.Get (1));

  p2pLinkLeftSpSwToRouter = p2p.Install (NodeContainer (routerForTerminalsSpLeft.Get (0), router.Get (0)));
  routerForTerminalsSpLeftDevices.Add (p2pLinkLeftSpSwToRouter.Get (0));
  routerDevices.Add (p2pLinkLeftSpSwToRouter.Get (1));

  p2pLinkLeftDrrSwToRouter = p2p.Install (NodeContainer (routerForTerminalsDrrLeft.Get (0), router.Get (0)));
  routerForTerminalsDrrLeftDevices.Add (p2pLinkLeftDrrSwToRouter.Get (0));
  routerDevices.Add (p2pLinkLeftDrrSwToRouter.Get (1));

  p2pLinkLeftWfqSwToRouter = p2p.Install (NodeContainer (routerForTerminalsWfqLeft.Get (0), router.Get (0)));
  routerForTerminalsWfqLeftDevices.Add (p2pLinkLeftWfqSwToRouter.Get (0));
  routerDevices.Add (p2pLinkLeftWfqSwToRouter.Get (1));


  NetDeviceContainer p2pLinkRouterToRightFifoServer;
  NetDeviceContainer p2pLinkRouterToRightSpServer;
  NetDeviceContainer p2pLinkRouterToRightDrrServer;
  NetDeviceContainer p2pLinkRouterToRightWfqServer;

  p2pLinkRouterToRightFifoServer = p2p.Install (NodeContainer (router.Get (0), serversFifoRight.Get (0)));
  routerDevices.Add (p2pLinkRouterToRightFifoServer.Get (0));
  serversFifoRightDevices.Add (p2pLinkRouterToRightFifoServer.Get (1));

  p2pLinkRouterToRightSpServer = p2p.Install (NodeContainer (router.Get (0), serversSpRight.Get (0)));
  routerDevices.Add (p2pLinkRouterToRightSpServer.Get (0));
  serversSpRightDevices.Add (p2pLinkRouterToRightSpServer.Get (1));

  p2pLinkRouterToRightDrrServer = p2p.Install (NodeContainer (router.Get (0), serversDrrRight.Get (0)));
  routerDevices.Add (p2pLinkRouterToRightDrrServer.Get (0));
  serversDrrRightDevices.Add (p2pLinkRouterToRightDrrServer.Get (1));

  p2pLinkRouterToRightWfqServer = p2p.Install (NodeContainer (router.Get (0), serversWfqRight.Get (0)));
  routerDevices.Add (p2pLinkRouterToRightWfqServer.Get (0));
  serversWfqRightDevices.Add (p2pLinkRouterToRightWfqServer.Get (1));

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
    ipv4.Assign (p2pLinkFifoLeft[i]);

    ipv4.SetBase (&ipBaseSp[0], "255.255.255.0");
    ipv4.Assign (p2pLinkSpLeft[i]);

    ipv4.SetBase (&ipBaseDrr[0], "255.255.255.0");
    ipv4.Assign (p2pLinkSpLeft[i]);

    ipv4.SetBase (&ipBaseWfq[0], "255.255.255.0");
    ipv4.Assign (p2pLinkWfqLeft[i]);
  }


  for (int i = 0; i < terminalsNumPerSchedule; i++)
  {
    std::string ipBaseFifo ="111.";
    std::string ipBaseSp ="112.";
    std::string ipBaseDrr ="113.";
    std::string ipBaseWfq ="114.";

    int secondByte = (int) (i / 253);
    int thirdByte = i % 253;

    ipBaseFifo = ipBaseFifo + std::to_string (secondByte) + "." + std::to_string (thirdByte) + ".0";
    ipBaseSp = ipBaseSp + std::to_string (secondByte) + "." + std::to_string (thirdByte) + ".0";
    ipBaseDrr = ipBaseDrr + std::to_string (secondByte) + "." + std::to_string (thirdByte) + ".0";
    ipBaseWfq = ipBaseWfq + std::to_string (secondByte) + "." + std::to_string (thirdByte) + ".0";
// std::cout << ipBaseFifo << std::endl;
    ipv4.SetBase (&ipBaseFifo[0], "255.255.255.0");
    ipv4.Assign (p2pLinkFifoRight[i]);

    ipv4.SetBase (&ipBaseSp[0], "255.255.255.0");
    ipv4.Assign (p2pLinkSpRight[i]);

    ipv4.SetBase (&ipBaseDrr[0], "255.255.255.0");
    ipv4.Assign (p2pLinkSpRight[i]);

    ipv4.SetBase (&ipBaseWfq[0], "255.255.255.0");
    ipv4.Assign (p2pLinkWfqRight[i]);
  }



  ipv4.SetBase ("201.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkLeftFifoSwToRouter);

  ipv4.SetBase ("202.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkLeftSpSwToRouter);

  ipv4.SetBase ("203.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkLeftDrrSwToRouter);

  ipv4.SetBase ("204.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkLeftWfqSwToRouter);

  serversIpMap[0] = "201.1.1.1";
  serversIpMap[1] = "202.1.1.1";
  serversIpMap[2] = "203.1.1.1";
  serversIpMap[3] = "204.1.1.1";

// std::cout << "3" << std::endl;

// std::cout << "3.1" << std::endl;
  // router ip (server)


  ipv4.SetBase ("11.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkRouterToRightFifoServer);

  ipv4.SetBase ("12.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkRouterToRightSpServer);

  ipv4.SetBase ("13.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkRouterToRightDrrServer);

  ipv4.SetBase ("14.1.1.0", "255.255.255.0");
  ipv4.Assign (p2pLinkRouterToRightWfqServer);

  serversIpMap[4] = "11.1.1.2";
  serversIpMap[5] = "12.1.1.2";
  serversIpMap[6] = "13.1.1.2";
  serversIpMap[7] = "14.1.1.2";


// std::cout << "3.3" << std::endl;



// // std::cout << "====================3" << std::endl;
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  for (int i = 0; i < 8; i++)
  {
    BuildAppsServer (serversMap[i], serversIpMap[i]);
  }
  std::vector<std::string> assignedServersIpVec;
  for (int i = 0; i < 8; i++)
  {
    uint32_t terminalsMapNum = random_scratch (1, 9999) % 8;
    uint32_t serversMapNum;
    while (1)
    {
      serversMapNum = random_scratch (1, 9999) % 8;
      if (!(std::find(assignedServersIpVec.begin(), assignedServersIpVec.end(),
                    serversIpMap[serversMapNum]) != assignedServersIpVec.end()))
      {
        BuildAppsClient (terminalsMap[terminalsMapNum], serversIpMap[serversMapNum], 1);
        assignedServersIpVec.push_back (serversIpMap[serversMapNum]);
        break;
      }
    }
  }

  // BuildApps (terminalsFifoLeft, serversFifoRight, "11.1.1.2", 1);
  // BuildApps (terminalsSpLeft, serversSpRight, "12.1.1.2", 1);
  // BuildApps (terminalsDrrLeft, serversDrrRight, "13.1.1.2", 1);
  // BuildApps (terminalsWfqLeft, serversWfqRight, "14.1.1.2", 1);
// std::cout << "4" << std::endl;

  AsciiTraceHelper ascii;
  incomePacketTraceStream = ascii.CreateFileStream ("/home/guolab/LFS/NS3/traffic-schedule-data-20200623/forward-matrix-10/traffic-schedule-income.csv");
  outcomePacketTraceStream = ascii.CreateFileStream ("/home/guolab/LFS/NS3/traffic-schedule-data-20200623/forward-matrix-10/traffic-schedule-outcome.csv");

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