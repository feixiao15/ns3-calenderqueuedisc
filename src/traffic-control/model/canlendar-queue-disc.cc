 #include "canlendar-queue-disc.h"
 #include "ns3/log.h"
 #include "ns3/packet.h"
 #include "ns3/simulator.h"
 #include "ns3/log.h"
 #include "ns3/object-factory.h"
 #include "ns3/pointer.h"
 #include "ns3/socket.h"
 #include <algorithm>
 #include <iterator>
 #include "ns3/prio-queue-disc.h"
 #include "ns3/tags.h"


 namespace ns3
 {
 
 NS_LOG_COMPONENT_DEFINE("CanlendarQueueDisc");
 
 NS_OBJECT_ENSURE_REGISTERED(CanlendarQueueDisc);
 
//  ATTRIBUTE_HELPER_CPP(Priomap);
 
//  std::ostream&
//  operator<<(std::ostream& os, const Priomap& priomap)
//  {
//      std::copy(priomap.begin(), priomap.end() - 1, std::ostream_iterator<uint16_t>(os, " "));
//      os << priomap.back();
//      return os;
//  }
 
//  std::istream&
//  operator>>(std::istream& is, Priomap& priomap)
//  {
//      for (int i = 0; i < 16; i++)
//      {
//          if (!(is >> priomap[i]))
//          {
//              NS_FATAL_ERROR("Incomplete priomap specification ("
//                             << i << " values provided, 16 required)");
//          }
//      }
//      return is;
//  }
 
 TypeId
 CanlendarQueueDisc::GetTypeId()
 {
     static TypeId tid =
         TypeId("ns3::CanlendarQueueDisc")
             .SetParent<QueueDisc>()
             .SetGroupName("TrafficControl")
             .AddConstructor<CanlendarQueueDisc>()
             .AddAttribute("Priomap",
                           "The priority to band mapping.",
                           PriomapValue(Priomap{{1, 2, 2, 2, 1, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}}),
                           MakePriomapAccessor(&CanlendarQueueDisc::m_prio2band),
                           MakePriomapChecker())
             .AddAttribute("RotationInterval",
                            "Time interval for priority rotation",
                            TimeValue(Seconds(1.0)),  // 默认 1.0s
                            MakeTimeAccessor(&CanlendarQueueDisc::m_rotationInterval),
                            MakeTimeChecker())
             .AddAttribute ("MaxSize",
                            "The maximum number of packets or bytes the queue can hold.",
                            QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, 512*10e6)),
                            MakeQueueSizeAccessor (&CanlendarQueueDisc::SetMaxSize,
                                                    &CanlendarQueueDisc::GetMaxSize),
                            MakeQueueSizeChecker ());               
     return tid;
 }

 CanlendarQueueDisc::CanlendarQueueDisc()
     : QueueDisc(QueueDiscSizePolicy::MULTIPLE_QUEUES)
 {
     NS_LOG_FUNCTION(this);
 }
 
 CanlendarQueueDisc::~CanlendarQueueDisc()
 {
     NS_LOG_FUNCTION(this);
 }
 
 void
CanlendarQueueDisc::SetBandForPriority(uint8_t prio, uint16_t band)
 {
     NS_LOG_FUNCTION(this << prio << band);
 
     NS_ASSERT_MSG(prio < 16, "Priority must be a value between 0 and 15");
 
     m_prio2band[prio] = band;
 }
 
 uint16_t
CanlendarQueueDisc::GetBandForPriority(uint8_t prio) const
 {
     NS_LOG_FUNCTION(this << prio);
 
     NS_ASSERT_MSG(prio < 16, "Priority must be a value between 0 and 15");
 
     return m_prio2band[prio];
 }
 /*
 to:
 check the type of the flow, give different band (=processing time?)
 give flows specific tags or ??
 check the queue full or not
 */
 bool
 CanlendarQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
 {
     NS_LOG_FUNCTION(this << item);
 
     uint32_t band;
     uint32_t bdelay;
 
    //  int32_t ret = Classify(item);
 
    //  if (ret == PacketFilter::PF_NO_MATCH)
    //  {
    //      NS_LOG_DEBUG("No filter has been able to classify this packet, using priomap.");
 
    //      SocketPriorityTag priorityTag;
    //      if (item->GetPacket()->PeekPacketTag(priorityTag))
    //      {   
    //          NS_LOG_LOGIC("prio:"<<priorityTag.GetPriority());
    //          band = m_prio2band[priorityTag.GetPriority() & 0x0f];
    //      }
    //  }
    //  else
    //  {
    //      NS_LOG_DEBUG("Packet filters returned " << ret);
 
    //      if (ret >= 0 && static_cast<uint32_t>(ret) < GetNQueueDiscClasses())
    //      {
    //          band = ret;
    //      }
    //}
    // 获取目标队列

 
     NS_ASSERT_MSG(band < GetNQueueDiscClasses(), "Selected band out of range");
     band = m_rotationOffset; //current round

     uint32_t packetSize = item->GetPacket()->GetSize();
     uint32_t csize = GetQueueDiscClass(band)->GetQueueDisc()->GetNBytes();
     NS_LOG_LOGIC("current size: "<< csize<<" size " << GetMaxSize().GetValue());
    //  for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
    //  {
    //     band = (i + m_rotationOffset) % GetNQueueDiscClasses();
    //     if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue())
    //     {

    //         bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
    //         uint32_t queueSizeAfter = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets();
    //         m_Bytesbudget[band] +=  item->GetSize();

    //         NS_LOG_LOGIC("ttt " << ttt);
    //         NS_LOG_LOGIC("Packet enqueued to band " << band
    //             << ". Queue size: " << queueSizeBefore << " -> " << queueSizeAfter
    //             << " packet size: " << packetSize
    //             <<" time now:"<< rotation_time
    //             <<" bytebudget:" << m_Bytesbudget[band]
    //             << "). Success: " << retval);
    //             return retval;
    //     }
    //     NS_LOG_WARN("Queue is full! Moving packet to next queue."<<(band+1));
    // }
    //  MyHeader header;
    //  bool hasHeader = item->GetPacket()->PeekHeader(header);
    //  item->GetPacket()->Print(std::cout);
    //  if(hasHeader)
    //  {
    //      FlowType flowtype = header.GetFlowType();
    //      DeadlineType ddltype = header.GetDeadline();

    //      NS_LOG_INFO("Packet header info: FlowType = " 
    //         << (flowtype == ns3::PREFILL ? "PREFILL" : "DECODE")
    //         << ", Deadline = " << ddltype << "s");
    //     //prefill
    //     if (flowtype ==FlowType::PREFILL)
    //     {
    //         for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++){
    //             band = (i + m_rotationOffset) % GetNQueueDiscClasses();
    //             if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue())
    //             {

    //                 bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
    //                 uint32_t queueSizeAfter = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets();
    //                 m_Bytesbudget[band] +=  item->GetSize();

    //                 NS_LOG_LOGIC("ttt " << ttt);
    //                 NS_LOG_LOGIC("Packet enqueued to band " << band
    //                     << ". Queue size: " << queueSizeBefore << " -> " << queueSizeAfter
    //                     << " packet size: " << packetSize
    //                     <<" time now:"<< rotation_time
    //                     <<" bytebudget:" << m_Bytesbudget[band]
    //                     <<"type: preill"
    //                     << "). Success: " << retval);
    //                     return retval;
    //             }
    //             NS_LOG_WARN("Queue is full! Moving packet to next queue."<<(band+1));
    //         }
    //     }
    //     else if(flowtype == FlowType::DECODE)
    //     {
    //         uint32_t ddl = header.GetDeadlineValue();
    //         band += (ddl - 1) % GetNQueueDiscClasses();
    //         for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++){
    //             band = (i + m_rotationOffset) % GetNQueueDiscClasses();
    //             NS_LOG_LOGIC("omve to band"<<band);
    //             if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue())
    //             {

    //                 bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
    //                 uint32_t queueSizeAfter = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets();
    //                 m_Bytesbudget[band] +=  item->GetSize();

    //                 NS_LOG_LOGIC("ttt " << ttt);
    //                 NS_LOG_LOGIC("Packet enqueued to band " << band
    //                     << ". Queue size: " << queueSizeBefore << " -> " << queueSizeAfter
    //                     << " packet size: " << packetSize
    //                     <<" time now:"<< rotation_time
    //                     <<" bytebudget:" << m_Bytesbudget[band]
    //                     <<"type: decode"
    //                     <<"ddl: "<<ddl
    //                     << "). Success: " << retval);
    //                     return retval;
    //             }
    //             // note the packet is overtime
    //             NS_LOG_WARN("Queue is full! Moving packet to next queue."<<(band+1));
    //         }
    //     }
    //     else
    //     {
    //         for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++){
    //             band = (i + m_rotationOffset) % GetNQueueDiscClasses();
    //             if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue())
    //             {

    //                 bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
    //                 uint32_t queueSizeAfter = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets();
    //                 m_Bytesbudget[band] +=  item->GetSize();

    //                 NS_LOG_LOGIC("ttt " << ttt);
    //                 NS_LOG_LOGIC("Packet enqueued to band " << band
    //                     << ". Queue size: " << queueSizeBefore << " -> " << queueSizeAfter
    //                     << " packet size: " << packetSize
    //                     <<" time now:"<< rotation_time
    //                     <<" bytebudget:" << m_Bytesbudget[band]
    //                     << "). Success: " << retval);
    //                     return retval;
    //             }
    //             NS_LOG_WARN("Queue is full! Moving packet to next queue."<<(band+1));
    //         }
    //         NS_LOG_INFO("flowtype raw value: " << flowtype);

    //     }
    // }
    // else
    // {
    //      NS_LOG_INFO("No header");
         
        
    // }
    FlowTypeTag flowtype;
    DeadlineTag ddl;
    bool hastypetag = item->GetPacket()->PeekPacketTag(flowtype);
    bool hasddltag = item->GetPacket()->PeekPacketTag(ddl);
    if (hastypetag && hasddltag)
    {
        NS_LOG_INFO("flowtype: "<<flowtype.GetType());
        NS_LOG_INFO("ddl: "<<ddl.GetDeadline());
        if(flowtype.GetType()==FlowTypeTag::PREFILL)
        {
            for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++){
                band = (i + m_rotationOffset) % GetNQueueDiscClasses();
                if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue())
                {
                    uint32_t queueSizeBefore = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets(); // 记录入队前的队列长度
                    bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
                    uint32_t queueSizeAfter = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets();
                    m_Bytesbudget[band] +=  item->GetSize();

                    NS_LOG_LOGIC("ttt " << ttt);
                    NS_LOG_LOGIC("Packet enqueued to band " << band
                        << ". Queue size: " << queueSizeBefore << " -> " << queueSizeAfter
                        << " packet size: " << packetSize
                        <<" time now:"<< rotation_time
                        <<" bytebudget:" << m_Bytesbudget[band]
                        <<"flowtype"<<(flowtype.GetType() == FlowTypeTag::PREFILL ? "PREFILL" : "DECODE")
                        << "). Success: " << retval);
                        return retval;
                }
                NS_LOG_WARN("Queue is full! Moving packet to next queue."<<(band+1));
            }
        }
        else if(flowtype.GetType()==FlowTypeTag::DECODE)
        {
            for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
            {
                band = (i + m_rotationOffset+ddl.GetDeadline() - 1) % GetNQueueDiscClasses();
                NS_LOG_LOGIC("move to band"<<band);
                if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue())
                {
                    uint32_t queueSizeBefore = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets(); // 记录入队前的队列长度
                    bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
                    uint32_t queueSizeAfter = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets();
                    m_Bytesbudget[band] +=  item->GetSize();

                    NS_LOG_LOGIC("rotation times " << ttt);
                    NS_LOG_LOGIC("Packet enqueued to band " << band
                        << ". Queue size: " << queueSizeBefore << " -> " << queueSizeAfter
                        << " packet size: " << packetSize
                        <<" time now:"<< rotation_time
                        <<" bytebudget:" << m_Bytesbudget[band]
                        <<"type: decode"
                        <<"ddl: "<<ddl.GetDeadline()<<"s"
                        << "). Success: " << retval);
                        return retval;
                }
                // note the packet is overtime
                NS_LOG_WARN("Queue is full! Moving packet to next queue."<<(band+1));
            }
        }
    }
    else
    {
    NS_LOG_INFO("NO TAGS");
    }
    
}
 
 Ptr<QueueDiscItem>
 CanlendarQueueDisc::DoDequeue()
 {
     NS_LOG_FUNCTION(this);
 
     Ptr<QueueDiscItem> item;
     uint32_t band = m_rotationOffset;
    //  for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
    //  {
         
    //      band = (i + m_rotationOffset) % GetNQueueDiscClasses();
    //      if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Dequeue()))
    //      {   
    //          NS_LOG_LOGIC(i);
    //          uint32_t csize = GetQueueDiscClass(band)->GetQueueDisc()->GetNBytes();
    //          NS_LOG_LOGIC("Popped from band " << i << ": " << item);
    //          NS_LOG_LOGIC("Number packets band "
    //                       << i << ": " << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets()
    //                       <<" current size: "<< csize
    //                       <<" band: "<< band);
    //          return item;
    //      }
    //  }
     if ((item = GetQueueDiscClass(band)->GetQueueDisc()->Dequeue()))
         {   
             uint32_t csize = GetQueueDiscClass(band)->GetQueueDisc()->GetNBytes();
             uint32_t queueSizeAfter = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets();
             NS_LOG_LOGIC("Popped from band " << band << ": " << item);
             NS_LOG_LOGIC("Number packets band "
                          << band << ": " << GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets()
                          <<" bytebudget: "<< m_Bytesbudget[band]
                          <<"remain packets: "<<queueSizeAfter
             );
             return item;
         }
     NS_LOG_LOGIC("Queue empty!!!!band:"<<m_rotationOffset);
     return item;
 }
 
 Ptr<const QueueDiscItem>
 CanlendarQueueDisc::DoPeek()
 {
     NS_LOG_FUNCTION(this);
 
     Ptr<const QueueDiscItem> item;
 
     for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
     {
         if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Peek()))
         {
             NS_LOG_LOGIC("Peeked from band " << i << ": " << item);
             NS_LOG_LOGIC("Number packets band "
                          << i << ": " << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
             return item;
         }
     }
 
     NS_LOG_LOGIC("Queue empty");
     return item;
 }
 
 bool
 CanlendarQueueDisc::CheckConfig()
 {
     NS_LOG_FUNCTION(this);
     if (GetNInternalQueues() > 0)
     {
         NS_LOG_ERROR("CanlendarQueueDisc cannot have internal queues");
         return false;
     }
 
     if (GetNQueueDiscClasses() == 0)
     {
         // create 3 fifo queue discs
         ObjectFactory factory;
         factory.SetTypeId("ns3::FifoQueueDisc");
         for (uint8_t i = 0; i < 40; i++)
         {
             Ptr<QueueDisc> qd = factory.Create<QueueDisc>();
             qd->Initialize();
             Ptr<QueueDiscClass> c = CreateObject<QueueDiscClass>();
             c->SetQueueDisc(qd);
             AddQueueDiscClass(c);
         }
     }
 
     if (GetNQueueDiscClasses() < 2)
     {
         NS_LOG_ERROR("CanlendarQueueDisc needs at least 2 classes");
         return false;
     }
 
     return true;
 }
 //111
 void
 CanlendarQueueDisc::RotatePriority()
{
     NS_LOG_FUNCTION(this);
    // 轮转偏移量加1，模队列数量
    m_Bytesbudget[m_rotationOffset] = 0;
    NS_LOG_LOGIC("Cleared byte count for band " << m_rotationOffset);
    ttt = ttt + 1;
    rotation_time = Simulator::Now();
     m_rotationOffset = (m_rotationOffset + 1) % GetNQueueDiscClasses();
    // 重新安排下一次轮转
     m_rotationEvent = Simulator::Schedule(m_rotationInterval, &CanlendarQueueDisc::RotatePriority, this);
     NS_LOG_LOGIC("rotate "<< m_rotationOffset<<"class"<<GetNQueueDiscClasses()<<" ttt "<<ttt<<"time now:"<<rotation_time);
    }    
 void
 CanlendarQueueDisc::InitializeParams()
 {
     NS_LOG_FUNCTION(this);
     uint16_t ttt = 0;
     m_rotationOffset = 0;
     m_rotationOffset2 = 0;
     m_Bytesbudget.resize(GetNQueueDiscClasses(), 0);
    // 每隔10秒轮转一次
     m_rotationEvent = Simulator::Schedule(Seconds(1.0), &CanlendarQueueDisc::RotatePriority, this);
  }
 
 } // namespace ns3
 