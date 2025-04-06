/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#include "fifo-queue-disc.h"

#include "ns3/drop-tail-queue.h"
#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/timestamp-tag.h"
#include "ns3/simulator.h"
#include "ns3/tags.h"
namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FifoQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(FifoQueueDisc);

TypeId
FifoQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::FifoQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<FifoQueueDisc>()
            .AddAttribute("MaxSize",
                          "The max queue size",
                          QueueSizeValue(QueueSize("1000p")),
                          MakeQueueSizeAccessor(&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                          MakeQueueSizeChecker());
    return tid;
}

FifoQueueDisc::FifoQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
    NS_LOG_FUNCTION(this);
}

FifoQueueDisc::~FifoQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

bool
FifoQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    if (GetCurrentSize() + item > GetMaxSize())
    {
        NS_LOG_LOGIC("Queue full -- dropping pkt");
        DropBeforeEnqueue(item, LIMIT_EXCEEDED_DROP);
        return false;
    }
    Ptr<Packet> pkt = item->GetPacket();
    TimestampTag tag;
    if(!pkt->PeekPacketTag(tag)){
    tag.SetTimestamp(Simulator::Now());
    pkt->AddPacketTag(tag);}

    bool retval = GetInternalQueue(0)->Enqueue(item);

    // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
    // internal queue because QueueDisc::AddInternalQueue sets the trace callback

    NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
    NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

    return retval;
}

Ptr<QueueDiscItem>
FifoQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    Ptr<QueueDiscItem> item = GetInternalQueue(0)->Dequeue();
    if (!item)
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }
    FlowTypeTag flowtype;
    Ptr<Packet> pkt = item->GetPacket();
    if(pkt->PeekPacketTag(flowtype)&&flowtype.GetType()==FlowTypeTag::DECODE)
    {
        TimestampTag tsTag;
        if (pkt->PeekPacketTag(tsTag))
        {
        Time delay = Simulator::Now() - tsTag.GetTimestamp();
        m_totalQueueDelay += delay;
        m_dequeuedPackets++;
        if (delay > m_timeoutThreshold)
            {
                m_timeoutCount++;
            NS_LOG_INFO("Packet timeout: delay = " << delay.GetSeconds() << " s");
            }
        }
    }
    if(pkt->PeekPacketTag(flowtype)&&flowtype.GetType()==FlowTypeTag::PREFILL)
    {
        TimestampTag tsTag;
        if (pkt->PeekPacketTag(tsTag))
        {
        Time delay = Simulator::Now() - tsTag.GetTimestamp();
        m_pt += delay;
        m_prefillpacket++;
        }
    }
 

    return item;
}

Ptr<const QueueDiscItem>
FifoQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item = GetInternalQueue(0)->Peek();

    if (!item)
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }

    return item;
}

bool
FifoQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("FifoQueueDisc cannot have classes");
        return false;
    }

    if (GetNPacketFilters() > 0)
    {
        NS_LOG_ERROR("FifoQueueDisc needs no packet filter");
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // add a DropTail queue
        AddInternalQueue(
            CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>("MaxSize",
                                                                     QueueSizeValue(GetMaxSize())));
    }

    if (GetNInternalQueues() != 1)
    {
        NS_LOG_ERROR("FifoQueueDisc needs 1 internal queue");
        return false;
    }

    return true;
}

void
FifoQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
    m_timeoutCount = 0;
    m_timeoutThreshold = Seconds(0.1);
    m_dequeuedPackets = 0;
}
void
FifoQueueDisc::ReportTimeoutStatistics () const
{
  double avgDelay = (m_dequeuedPackets > 0) ? m_totalQueueDelay.GetSeconds() / m_dequeuedPackets : 0.0;
  double timeoutRate = (m_dequeuedPackets > 0) ? static_cast<double>(m_timeoutCount) / m_dequeuedPackets : 0.0;
  double avgp = (m_prefillpacket>0) ? m_pt.GetSeconds() / m_prefillpacket : 0.0;
  std::cout << "===== FIFO Queue Timeout Statistics =====" << std::endl;
  std::cout << "Total dequeued decode packets: " << m_dequeuedPackets << std::endl;
  std::cout << "Total delay: " << m_totalQueueDelay.GetSeconds()<<"S" << std::endl;
  std::cout << "Total timeout packets: " << m_timeoutCount << std::endl;
  std::cout << "Average queue delay: " << avgDelay << " s" << std::endl;
  std::cout << "Average pt: " << avgp << " s" << std::endl;
  std::cout << "Timeout rate: " << timeoutRate * 100 << " %" << std::endl;
  std::cout << "=========================================" << std::endl;
}
} // namespace ns3
