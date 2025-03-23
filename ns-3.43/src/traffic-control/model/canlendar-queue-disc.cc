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
 
     NS_ASSERT_MSG(band < GetNQueueDiscClasses(), "Selected band out of range");
     band = m_rotationOffset; //current round
     uint32_t queueSizeBefore = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets(); // 记录入队前的队列长度
     uint32_t packetSize = item->GetPacket()->GetSize();
     uint32_t csize = GetQueueDiscClass(band)->GetQueueDisc()->GetNBytes();
     NS_LOG_INFO("current size: "<< csize<<" size " << GetMaxSize().GetValue());
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
    FlowTypeTag flowtype;
    DeadlineTag ddl;
    bool hastypetag = item->GetPacket()->PeekPacketTag(flowtype);
    bool hasddltag = item->GetPacket()->PeekPacketTag(ddl);
    Time current_enqueue_time = Simulator::Now();
 
    
    if (hastypetag && hasddltag)
    {
        NS_LOG_INFO("flowtype: "<<flowtype.GetType());
        NS_LOG_INFO("ddl: "<<ddl.GetDeadline());
        if(flowtype.GetType()==FlowTypeTag::PREFILL)
        {
            for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
            {
                band = (i + m_rotationOffset) % GetNQueueDiscClasses();
                Time remain_time =  Seconds(1) * (1 + (band-m_rotationOffset+GetNQueueDiscClasses()%GetNQueueDiscClasses()))-(current_enqueue_time - rotation_time);
                double remain_bytes = remain_time.ToDouble(Time::S) * (2*10e6)-csize;
                if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue()&&packetSize<=remain_bytes)
                {
                    bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
                    NS_LOG_INFO("need time:"<<packetSize*8/(2*10e6));

                    uint32_t queueSizeAfter = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets();
                    m_Bytesbudget[band] +=  item->GetSize();

                    NS_LOG_LOGIC("ttt " << ttt);
                    NS_LOG_LOGIC("Packet enqueued to band " << band
                        << ". Queue size: " << queueSizeBefore << " -> " << queueSizeAfter
                        << " packet size: " << packetSize
                        <<" time now:"<< rotation_time
                        <<" bytebudget:" << m_Bytesbudget[band]
                        <<" flowtype"<<(flowtype.GetType() == FlowTypeTag::PREFILL ? "PREFILL" : "DECODE")
                        <<" remainbytes:"<<remain_bytes
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
                Time remain_time =  Seconds(1) * (1 + (band-m_rotationOffset+GetNQueueDiscClasses()%GetNQueueDiscClasses()))-(current_enqueue_time - rotation_time);
                double remain_bytes = remain_time.ToDouble(Time::S) * (2*10e6)-csize;
                if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue()&&packetSize<=remain_bytes)
                {
                NS_LOG_LOGIC("move to band"<<band);
                bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
                uint32_t queueSizeAfter = GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets();
                m_Bytesbudget[band] +=  item->GetSize();

                NS_LOG_INFO("rotation times " << ttt);
                NS_LOG_INFO("Packet enqueued to band " << band
                    << ". Queue size: " << queueSizeBefore << " -> " << queueSizeAfter
                    << " packet size: " << packetSize
                    <<" time now:"<< rotation_time
                    <<" bytebudget:" << m_Bytesbudget[band]
                    <<" type: decode"
                    <<" ddl: "<<ddl.GetDeadline()<<"s"
                    <<" remainbytes:"<<remain_bytes
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
     for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
     {
         
         band = (i + m_rotationOffset) % GetNQueueDiscClasses();
         if ((item = GetQueueDiscClass(band)->GetQueueDisc()->Dequeue()))
         {   
             uint32_t csize = GetQueueDiscClass(band)->GetQueueDisc()->GetNBytes();
             uint32_t packetSize = item->GetPacket()->GetSize();

             NS_LOG_INFO("Popped from band " << band << ": " << item);
             NS_LOG_INFO("Number packets band "
                          << band << ": " << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets()
                          <<" current size: "<< csize
                          <<" packetsize: "<< packetSize
                          <<" time:"<<Simulator::Now());
             return item;
         }
   
     }
     NS_LOG_LOGIC("Queue empty!!!! band:"<<band);
     return item;
    //  if ((item = GetQueueDiscClass(band)->GetQueueDisc()->Dequeue()))
    //      {   
    //          uint32_t csize = GetQueueDiscClass(band)->GetQueueDisc()->GetNBytes();
    //          NS_LOG_LOGIC("Popped from band " << band << ": " << item);
    //          NS_LOG_LOGIC("Number packets band "
    //                       << band << ": " << GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets()
    //                       <<" bytebudget: "<< m_Bytesbudget[band]
    //          );
    //          return item;
    //      }
    //  NS_LOG_LOGIC("Queue empty!!!!"<<m_rotationOffset);
    //  return item;
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
         for (uint8_t i = 0; i < 20; i++)
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
    uint32_t lbytes = GetQueueDiscClass(m_rotationOffset)->GetQueueDisc()->GetNBytes();
    NS_LOG_INFO("queue:"<<m_rotationOffset<<" current queue bytes:"<<lbytes);
    m_Bytesbudget[m_rotationOffset] = 0;

    NS_LOG_INFO("Cleared byte count for band " << m_rotationOffset);
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

     m_Bytesbudget.resize(GetNQueueDiscClasses(), 0);

    // 每隔10秒轮转一次
     m_rotationEvent = Simulator::Schedule(Seconds(1.0), &CanlendarQueueDisc::RotatePriority, this);
  }
 
 } // namespace ns3
 