/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
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
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/log.h"
#include "my-fifo-queue-disc.h"
#include "ns3/object-factory.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/net-device-queue-interface.h"

#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/trace-helper.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/ethernet-header.h"

#include "ns3/unique-packet-id.h"

#include <fstream>
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MyFifoQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (MyFifoQueueDisc);

static std::ofstream fifoStream;

TypeId MyFifoQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyFifoQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<MyFifoQueueDisc> ()
    .AddAttribute ("MaxSize",
                   "The max queue size",
                   QueueSizeValue (QueueSize ("10000p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
  ;
  return tid;
}

MyFifoQueueDisc::MyFifoQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
  NS_LOG_FUNCTION (this);
  // freopen("/home/guolab/LFS/NS3/traffic-trace-fifo.csv", "w", stdout);
  // std::cout << "Type," << "Queue," << "Quantum,"
  //           << "PacketUid," << "PacketId" << std::endl;
}

MyFifoQueueDisc::~MyFifoQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

bool
MyFifoQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  if (GetCurrentSize () + item > GetMaxSize ())
    {
      NS_LOG_LOGIC ("Queue full -- dropping pkt");
      DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP);
      return false;
    }

  bool retval = GetInternalQueue (0)->Enqueue (item);

  // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
  // internal queue because QueueDisc::AddInternalQueue sets the trace callback

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());
  // std::cout << "enqueue" << std::endl;

  return retval;
}

Ptr<QueueDiscItem>
MyFifoQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item = GetInternalQueue (0)->Dequeue ();

  if (!item)
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }
// std::cout << "dequeue" << std::endl;

  Ptr<Ipv4QueueDiscItem> ipv4Item = DynamicCast<Ipv4QueueDiscItem> (item);
  Ipv4Header hdr = ipv4Item->GetHeader ();
  Ptr<Packet> packet = ipv4Item->GetPacket ();
  // fifoStream.open("/home/guolab/LFS/NS3/traffic-trace-fifo.csv");

            UniquePacketIdTag tag;       
            if (packet->FindFirstMatchingByteTag (tag))
            {
              std::cout << "FIFO," << "0," << "-1,"
                        << (uint64_t) tag.GetUniquePacketId () << std::endl;
              std::cout.flush();
                        
            }


  return item;
}

Ptr<const QueueDiscItem>
MyFifoQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<const QueueDiscItem> item = GetInternalQueue (0)->Peek ();

  if (!item)
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  return item;
}

bool
MyFifoQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("MyFifoQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("MyFifoQueueDisc needs no packet filter");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // add a DropTail queue
      AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> >
                          ("MaxSize", QueueSizeValue (GetMaxSize ())));
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("MyFifoQueueDisc needs 1 internal queue");
      return false;
    }

  return true;
}

void
MyFifoQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3
