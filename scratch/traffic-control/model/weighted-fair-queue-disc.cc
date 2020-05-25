/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
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
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
*/

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/queue.h"
#include "weighted-fair-queue-disc.h"
#include "codel-queue-disc.h"
#include "ns3/net-device-queue-interface.h"

#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/trace-helper.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/ethernet-header.h"

#include "ns3/unique-packet-id.h"

#include <fstream>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WfqDisc2");

NS_OBJECT_ENSURE_REGISTERED (FqCoDelFlow2);

// static Ptr<OutputStreamWrapper> wfqTraceStream = asciiWfq.CreateFileStream ("/home/guolab/LFS/NS3/traffic-trace-wfq.csv");
std::ofstream wfqStream;
static int x_wfq;

static int random_wfq(int min, int max) //range : [min, max)
{
   static bool first_wfq = true;
   if (first_wfq) 
   {  
      srand( time(NULL) ); //seeding for the first time only!
      first_wfq = false;
   }
   return min + rand() % (( max + 1 ) - min);
}


TypeId FqCoDelFlow2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FqCoDelFlow2")
    .SetParent<QueueDiscClass> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<FqCoDelFlow2> ()
  ;
  return tid;
}

FqCoDelFlow2::FqCoDelFlow2 ()
  : m_deficit (0),
    m_status (INACTIVE)
{
  NS_LOG_FUNCTION (this);
}

FqCoDelFlow2::~FqCoDelFlow2 ()
{
  NS_LOG_FUNCTION (this);
}

void
FqCoDelFlow2::SetDeficit (uint32_t deficit)
{
  NS_LOG_FUNCTION (this << deficit);
  m_deficit = deficit;
}

int32_t
FqCoDelFlow2::GetDeficit (void) const
{
  NS_LOG_FUNCTION (this);
  return m_deficit;
}

void
FqCoDelFlow2::SetQueueId (uint32_t queueId)
{
  NS_LOG_FUNCTION (this << queueId);
  m_queueId = queueId;
}

int32_t
FqCoDelFlow2::GetQueueId (void) const
{
  NS_LOG_FUNCTION (this);
  return m_queueId;
}

void
FqCoDelFlow2::IncreaseDeficit (int32_t deficit)
{
  NS_LOG_FUNCTION (this << deficit);
  m_deficit += deficit;
}

void
FqCoDelFlow2::SetStatus (FlowStatus status)
{
  NS_LOG_FUNCTION (this);
  m_status = status;
}

FqCoDelFlow2::FlowStatus
FqCoDelFlow2::GetStatus (void) const
{
  NS_LOG_FUNCTION (this);
  return m_status;
}


NS_OBJECT_ENSURE_REGISTERED (WfqDisc);

TypeId WfqDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WfqDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<WfqDisc> ()
    .AddAttribute ("Interval",
                   "The CoDel algorithm interval for each FQCoDel queue",
                   StringValue ("100ms"),
                   MakeStringAccessor (&WfqDisc::m_interval),
                   MakeStringChecker ())
    .AddAttribute ("Target",
                   "The CoDel algorithm target queue delay for each FQCoDel queue",
                   StringValue ("5ms"),
                   MakeStringAccessor (&WfqDisc::m_target),
                   MakeStringChecker ())
    .AddAttribute ("MaxSize",
                   "The maximum number of packets accepted by this queue disc",
                   QueueSizeValue (QueueSize ("10240p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
    .AddAttribute ("Flows",
                   "The number of queues into which the incoming packets are classified",
                   UintegerValue (8),
                   MakeUintegerAccessor (&WfqDisc::m_flows),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("DropBatchSize",
                   "The maximum number of packets dropped from the fat flow",
                   UintegerValue (64),
                   MakeUintegerAccessor (&WfqDisc::m_dropBatchSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Perturbation",
                   "The salt used as an additional input to the hash function used to classify packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WfqDisc::m_perturbation),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

WfqDisc::WfqDisc ()
  : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS),
    m_quantum (0)
{
  NS_LOG_FUNCTION (this);

  // Ptr<NetDevice> device = GetNetDevice ();
  // NS_ASSERT_MSG (device, "Device not set for the queue disc");
  // uint32_t mtu = 800; //device->GetMtu ();

  // for (int i = 0; i < 8; i++)
  // {
  //   m_weightMap[i] = i + 1;
  //   m_quantumMap[i] = m_weightMap[i] * mtu;
  // }

  for (int i = 0; i < 100; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      m_quantumMapArray[i][j] = random_wfq (128, 4096);
    }
  }



  // wfqStream.open("/home/guolab/LFS/NS3/traffic-trace-wfq.csv");

  // wfqStream << "Type," << "Queue," << "Weight," << "Wfq-quantum,"
  //           << "PacketUid," << "PacketId" << std::endl;

      // std::cout << "Type," << "Queue," << "Quantum,"
      //       << "PacketUid," << "PacketId" << std::endl;
}

WfqDisc::~WfqDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
WfqDisc::SetQuantum (uint32_t quantum)
{
  NS_LOG_FUNCTION (this << quantum);
  m_quantum = quantum;
}

uint32_t
WfqDisc::GetQuantum (void) const
{
  return m_quantum;
}

bool
WfqDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  x_wfq = Simulator::Now ().GetMilliSeconds () / 1500;
  m_quantumMapSum = 0;
  for (int i = 0; i < 8; i++)
  {
    m_quantumMap[i] = m_quantumMapArray[x_wfq][i];
    m_quantumMapSum = m_quantumMapSum + m_quantumMap[i];
  }

  uint32_t h = 0;
  if (GetNPacketFilters () == 0)
    {
      h = item->Hash (m_perturbation) % m_flows;
      // std::cout << "h = item->Hash (m_perturbation) % m_flows: " << h << std::endl;
    }
  else
    {
      int32_t ret = Classify (item);
      // std::cout << "int32_t ret = Classify (item): " << ret << std::endl;

      if (ret != PacketFilter::PF_NO_MATCH)
        {
          h = ret % m_flows;
          // std::cout << "h = ret % m_flows: " << h << std::endl;
        }
      else
        {
          NS_LOG_ERROR ("No filter has been able to classify this packet, drop it.");
          DropBeforeEnqueue (item, UNCLASSIFIED_DROP);
          return false;
        }
    }

  Ptr<FqCoDelFlow2> flow;
  if (m_flowsIndices.find (h) == m_flowsIndices.end ())
    {
      NS_LOG_DEBUG ("Creating a new flow queue with index " << h);
      // std::cout << "Creating a new flow queue with index " << h << std::endl;
      flow = m_flowFactory.Create<FqCoDelFlow2> ();
      Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc> ();
      qd->Initialize ();
      flow->SetQueueDisc (qd);
      flow->SetQueueId (h);
      AddQueueDiscClass (flow);

      m_flowsIndices[h] = GetNQueueDiscClasses () - 1;
    }
  else
    {
      flow = StaticCast<FqCoDelFlow2> (GetQueueDiscClass (m_flowsIndices[h]));
    }

  if (flow->GetStatus () == FqCoDelFlow2::INACTIVE)
    {
      flow->SetStatus (FqCoDelFlow2::NEW_FLOW);
      int fid = flow->GetQueueId ();
      flow->SetDeficit (m_quantumMap[fid]);
      m_newFlows.push_back (flow);
    }

  flow->GetQueueDisc ()->Enqueue (item);

  NS_LOG_DEBUG ("Packet enqueued into flow " << h << "; flow index " << m_flowsIndices[h]);
// std::cout << "Packet enqueued into flow " << h << "; flow index " << m_flowsIndices[h] << std::endl;
  if (GetCurrentSize () > GetMaxSize ())
    {
      FqCoDelDrop ();
    }

  return true;
}

Ptr<QueueDiscItem>
WfqDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<FqCoDelFlow2> flow;
  Ptr<QueueDiscItem> item;
// std::cout << "DoDequeue" << std::endl;
  do
    {
      // std::cout << "do loop" << std::endl;
      bool found = false;
      while (!found && !m_newFlows.empty ())
        {
          flow = m_newFlows.front ();

          if (flow->GetDeficit () <= 0)
            {
              // std::cout << "new flow IncreaseDeficit: " << flow->GetDeficit () << std::endl;
              int fid = flow->GetQueueId (); 
              flow->IncreaseDeficit (m_quantumMap[fid]);
              flow->SetStatus (FqCoDelFlow2::OLD_FLOW);
              m_oldFlows.push_back (flow);
              m_newFlows.pop_front ();
            }
          else
            {
              NS_LOG_DEBUG ("Found a new flow with positive deficit");
              found = true;
            }
        }

      while (!found && !m_oldFlows.empty ())
        {
          flow = m_oldFlows.front ();

          if (flow->GetDeficit () <= 0)
            {
              // std::cout << "old flow IncreaseDeficit: " << flow->GetDeficit () << std::endl;
              int fid = flow->GetQueueId ();
              flow->IncreaseDeficit (m_quantumMap[fid]);
              m_oldFlows.push_back (flow);
              m_oldFlows.pop_front ();
              // std::cout << "old flow IncreaseDeficit" << std::endl;
            }
          else
            {
              NS_LOG_DEBUG ("Found an old flow with positive deficit");
              found = true;
            }
        }

      if (!found)
        {
          NS_LOG_DEBUG ("No flow found to dequeue a packet");
          return 0;
        }

      item = flow->GetQueueDisc ()->Dequeue ();

      // int fid = flow->GetQueueId ();

      // std::cout << "flow queue id : " << flow->GetQueueId () << " Deficit: " << flow->GetDeficit () << std::endl;
      // std::cout.flush();

      if (!item)
        {
          NS_LOG_DEBUG ("Could not get a packet from the selected flow queue");
          // std::cout << "Could not get a packet from the selected flow queue: " << flow->GetQueueId () <<std::endl;
          if (!m_newFlows.empty ())
            {
              flow->SetStatus (FqCoDelFlow2::OLD_FLOW);
              m_oldFlows.push_back (flow);
              m_newFlows.pop_front ();
            }
          else
            {
              flow->SetStatus (FqCoDelFlow2::INACTIVE);
              m_oldFlows.pop_front ();
            }
        }
      else
        {
              int fid = flow->GetQueueId ();
              Ptr<Ipv4QueueDiscItem> ipv4Item = DynamicCast<Ipv4QueueDiscItem> (item);
              Ipv4Header hdr = ipv4Item->GetHeader ();
              Ptr<Packet> packet = ipv4Item->GetPacket ();

            UniquePacketIdTag tag;       
            if (packet->FindFirstMatchingByteTag (tag))
            {
              std::cout << "WFQ," << fid << "," << m_quantumMap[fid] << ","
                        << (uint64_t) tag.GetUniquePacketId () << ","
                        << (float) m_quantumMap[0] / m_quantumMapSum << ","
                        << (float) m_quantumMap[1] / m_quantumMapSum << ","
                        << (float) m_quantumMap[2] / m_quantumMapSum << ","
                        << (float) m_quantumMap[3] / m_quantumMapSum << ","
                        << (float) m_quantumMap[4] / m_quantumMapSum << ","
                        << (float) m_quantumMap[5] / m_quantumMapSum << ","
                        << (float) m_quantumMap[6] / m_quantumMapSum << ","
                        << (float) m_quantumMap[7] / m_quantumMapSum << ","
                        << "20," << "2," << x_wfq << std::endl;
              std::cout.flush();
                        
            }


          NS_LOG_DEBUG ("Dequeued packet " << item->GetPacket ());
        }

    } while (item == 0);

  flow->IncreaseDeficit (item->GetSize () * -1);

  return item;
}

bool
WfqDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("WfqDisc cannot have classes");
      return false;
    }

  if (GetNInternalQueues () > 0)
    {
      NS_LOG_ERROR ("WfqDisc cannot have internal queues");
      return false;
    }

  return true;
}

void
WfqDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);

  // we are at initialization time. If the user has not set a quantum value,
  // set the quantum to the MTU of the device
  if (!m_quantum)
    {
      Ptr<NetDevice> device = GetNetDevice ();
      NS_ASSERT_MSG (device, "Device not set for the queue disc");
      m_quantum = device->GetMtu ();
      NS_LOG_DEBUG ("Setting the quantum to the MTU of the device: " << m_quantum);
    }

  m_flowFactory.SetTypeId ("ns3::FqCoDelFlow2");

  m_queueDiscFactory.SetTypeId ("ns3::CoDelQueueDisc");
  m_queueDiscFactory.Set ("MaxSize", QueueSizeValue (GetMaxSize ()));
  m_queueDiscFactory.Set ("Interval", StringValue (m_interval));
  m_queueDiscFactory.Set ("Target", StringValue (m_target));
}

uint32_t
WfqDisc::FqCoDelDrop (void)
{
  NS_LOG_FUNCTION (this);

  uint32_t maxBacklog = 0, index = 0;
  Ptr<QueueDisc> qd;

  /* Queue is full! Find the fat flow and drop packet(s) from it */
  for (uint32_t i = 0; i < GetNQueueDiscClasses (); i++)
    {
      qd = GetQueueDiscClass (i)->GetQueueDisc ();
      uint32_t bytes = qd->GetNBytes ();
      if (bytes > maxBacklog)
        {
          maxBacklog = bytes;
          index = i;
        }
    }

  /* Our goal is to drop half of this fat flow backlog */
  uint32_t len = 0, count = 0, threshold = maxBacklog >> 1;
  qd = GetQueueDiscClass (index)->GetQueueDisc ();
  Ptr<QueueDiscItem> item;

  do
    {
      item = qd->GetInternalQueue (0)->Dequeue ();
      DropAfterDequeue (item, OVERLIMIT_DROP);
      len += item->GetSize ();
    } while (++count < m_dropBatchSize && len < threshold);

  return index;
}

} // namespace ns3
