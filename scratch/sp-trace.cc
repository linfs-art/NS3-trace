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

#include <string>
#include <vector>
#include <sys/sysinfo.h>
#include <sys/time.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SPExample");

uint32_t checkTimes;
double avgQueueDiscSize;

// The times
double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double client_start_time;
double client_stop_time;

uint32_t uniquePacketId = 1;



std::stringstream filePlotQueueDisc;
std::stringstream filePlotQueueDiscAvg;

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

  // UniquePacketIdTag tag;
  // tag.SetUniquePacketId (uniquePacketId);
  // packet->AddPacketTag (tag); 
  // uniquePacketId++;
  // std::cout << "packet size is-->: " << packet->GetSize () << std::endl;

  // Ptr<Packet> p = const_cast <Packet> (packet);


  // const IngressTimeHeader timeHeader;
  // timeHeader.SetData (10);
  // packet->AddTrailer (timeHeader);


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
  
  *incomePacketTraceStream->GetStream () << iph.GetIdentification () << ","
                                   << iph.GetPayloadSize () + 20 << ","
                                   << (uint16_t)iph.GetProtocol () << ","
                                   << iph.GetSource () << ","
                                   << iph.GetDestination () << ","
                                   << incomePort << ","
                                   << Simulator::Now ().GetMicroSeconds () << std::endl; 

}


static void
outcomePacketTracer (std::string context, Ptr<const Packet> packet)
{

  cxt_v.clear ();
  split(context, '/' , cxt_v);
  int outcomePort = atoi (cxt_v [4].c_str ());
// std::cout << "packet size is+++++++: " << packet->GetSize () << std::endl;

  Ptr<Packet> copy = packet->Copy ();

  // UniquePacketIdTag tag;


  PppHeader ppph;
  Ipv4Header iph;
  copy->RemoveHeader (ppph);
  copy->PeekHeader (iph);



//   bool isTag = packet->PeekPacketTag (tag);
//   if (isTag)
//   {
//   std::cout << tag.GetUniquePacketId () << std::endl;
// }
// else 
//   std::cout << "not tag" << std::endl;



  if (iph.GetPayloadSize () == 0)
  {
    return;
  }
  // *outcomePacketTraceStream->GetStream () << "wtf: " << packet->ToString () << std::endl;
  *outcomePacketTraceStream->GetStream () << iph.GetIdentification () << ","
                                   << iph.GetPayloadSize () + 20  << ","
                                   << (uint16_t)iph.GetProtocol () << ","
                                   << iph.GetSource () << ","
                                   << iph.GetDestination () << ","
                                   << outcomePort << ","
                                   << Simulator::Now ().GetMicroSeconds () << std::endl; 

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
BuildAppsTest (NodeContainer terminals)
{
  // SINK is in the right side
  uint16_t tcpPort = 6666;
  uint16_t udpPort = 8888;
  Address sinkLocalAddressTcp (InetSocketAddress (Ipv4Address::GetAny (), tcpPort));
  PacketSinkHelper sinkHelperTcp ("ns3::TcpSocketFactory", sinkLocalAddressTcp);

  Address sinkLocalAddressUdp (InetSocketAddress (Ipv4Address::GetAny (), udpPort));
  PacketSinkHelper sinkHelperUdp ("ns3::UdpSocketFactory", sinkLocalAddressUdp);


  ApplicationContainer sinkAppTcp = sinkHelperTcp.Install (terminals.Get (10));
  ApplicationContainer sinkAppUdp = sinkHelperUdp.Install (terminals.Get (10));

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
  clientHelperTcp.SetAttribute ("PacketSize", UintegerValue (1000));
  clientHelperTcp.SetAttribute ("DataRate", DataRateValue (DataRate ("2Mb/s")));

  // Connection two
  OnOffHelper clientHelperUdp ("ns3::UdpSocketFactory", Address ());
  clientHelperUdp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelperUdp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelperUdp.SetAttribute ("PacketSize", UintegerValue (1000));
  clientHelperUdp.SetAttribute ("DataRate", DataRateValue (DataRate ("2Mb/s")));

  ApplicationContainer clientAppsTcp;
  AddressValue remoteAddressTcp (InetSocketAddress (Ipv4Address ("10.1.10.1"), tcpPort));
  clientHelperTcp.SetAttribute ("Remote", remoteAddressTcp);

  ApplicationContainer clientAppsUdp;
  AddressValue remoteAddressUdp (InetSocketAddress (Ipv4Address ("10.1.10.1"), udpPort));
  clientHelperUdp.SetAttribute ("Remote", remoteAddressUdp);

  for (int i = 0; i < 5; i++)
  {
    clientAppsTcp.Add (clientHelperTcp.Install (terminals.Get (i)));
  }

  for (int i = 5; i < 10; i++)
  {
    clientAppsUdp.Add (clientHelperUdp.Install (terminals.Get (i)));
  }


  clientAppsTcp.Start (Seconds (client_start_time));
  clientAppsUdp.Start (Seconds (client_start_time));

  clientAppsTcp.Stop (Seconds (client_stop_time));
  clientAppsUdp.Stop (Seconds (client_stop_time));

}

int
main (int argc, char *argv[])
{

  set_start_time ();


  NodeContainer terminals;
  terminals.Create (11);

  NodeContainer router;
  router.Create (1);

  NetDeviceContainer terminalDevices;
  NetDeviceContainer routerDevices;


  LogComponentEnable ("PrioQueueDisc", LOG_LEVEL_INFO);





  // bool printDRRStats = true;

  global_start_time = 0.0;
  sink_start_time = global_start_time;
  client_start_time = global_start_time;
  global_stop_time = 1000.0;
  sink_stop_time = global_stop_time;
  client_stop_time = global_stop_time;

  // Configuration and command line parameter parsing
  // Will only save in the directory if enable opts below
  // pathOut = "."; // Current directory
  // CommandLine cmd;
  // cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
  // cmd.AddValue ("writeForPlot", "<0/1> to write results for plot (gnuplot)", writeForPlot);
  // cmd.AddValue ("writePcap", "<0/1> to write results in pcapfile", writePcap);
  // cmd.AddValue ("writeFlowMonitor", "<0/1> to enable Flow Monitor and write their results", flowMonitor);

  // cmd.Parse (argc, argv);



  NS_LOG_INFO ("Create terminals");


  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  // 42 = headers size
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000 - 42));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  //uint32_t meanPktSize = 1000;



  TrafficControlHelper tchPfifo;
  uint16_t handle = tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");
  tchPfifo.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));


  TrafficControlHelper spTch;
  spTch.SetRootQueueDisc ("ns3::PrioQueueDisc", "Priomap",
                         StringValue ("0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7"));

  NS_LOG_INFO ("Create channels");

  PointToPointHelper p2p;
  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));


  Ipv4AddressHelper ipv4;
  // ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  // ipv4.Assign (terminalDevices);

  InternetStackHelper internet;
  internet.Install (terminals);
  internet.Install (router);


  NetDeviceContainer link[11];
  for (int i = 0; i < 11; i++)
  {
      
      // std::string ipBase = "10.1." + std::to_string (i) + ".0" ;
      link[i] = p2p.Install (NodeContainer (terminals.Get (i), router.Get (0)));
      
      // ipv4.SetBase (&ipBase[0], "255.255.255.0");
      // ipv4.Assign (link);

      terminalDevices.Add (link[i].Get (0));
      routerDevices.Add (link[i].Get (1));
  }




  QueueDiscContainer queueDiscs;
  // std::cout << "====================1" << std::endl;
  tchPfifo.Install (terminalDevices.Get (0) );
  // std::cout << "====================3" << std::endl;
  queueDiscs = spTch.Install (routerDevices);
// std::cout << "====================4" << std::endl;

  for (int i = 0; i< 11; i++)
  {
    std::string ipBase = "10.1." + std::to_string (i) + ".0" ;
    ipv4.SetBase (&ipBase[0], "255.255.255.0");
    ipv4.Assign (link[i]);    
  }


// std::cout << "====================3" << std::endl;
  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  BuildAppsTest (terminals);


  AsciiTraceHelper ascii;
  incomePacketTraceStream = ascii.CreateFileStream ("/home/guolab/LFS/NS3/sp-router-income.csv");
  outcomePacketTraceStream = ascii.CreateFileStream ("/home/guolab/LFS/NS3/sp-router-outcome.csv");

  *incomePacketTraceStream->GetStream () << "PacketId" << ","
                        << "PacketSize" << ","
                        << "Protocol" << ","
                        << "Source" << ","
                        << "Destination" << ","
                        << "IngressPort" << ","
                        << "PhyRxEndTime" << std::endl;

  *outcomePacketTraceStream->GetStream () << "PacketId" << ","
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
//       stmp << pathOut << "/DRR";
//       p2p.EnablePcapAll (stmp.str ().c_str (), false);
//     }

  TraceIncomePacket ("11");
  TraceOutcomePacket ("11");

  // p2p.EnableAsciiAll (ascii.CreateFileStream ("/home/guolab/LFS/NS3/drr-router.tr"));

  Simulator::Stop (Seconds (sink_stop_time));
  Simulator::Run ();

  QueueDisc::Stats st = queueDiscs.Get (0)->GetStats ();


  // if (printDRRStats)
  //   {
  //     std::cout << "*** DRR stats from Node 2 queue ***" << std::endl;
  //     std::cout << "\t " << st.GetNDroppedPackets (DRRQueueDisc::UNCLASSIFIED_DROP)
  //               << " drops because packet could not be classified by any filter" << std::endl;
  //   }

  Simulator::Destroy ();

  set_end_time ();

  print_consumed_time ();

  return 0;
}