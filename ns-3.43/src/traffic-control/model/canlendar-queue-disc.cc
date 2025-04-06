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
#include "ns3/udp-header.h"
#include "ns3/timestamp-tag.h"
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
     NS_LOG_INFO("current size: "<< csize<<" size " << GetMaxSize().GetValue()<<" packetsize: "<<packetSize);
 
    FlowTypeTag flowtype;
    DeadlineTag ddl;
    bool hastypetag = item->GetPacket()->PeekPacketTag(flowtype);
    bool hasddltag = item->GetPacket()->PeekPacketTag(ddl);
    double ddlvalue = ddl.GetDeadline();
    if (hastypetag && hasddltag)
    {
        NS_LOG_INFO("flowtype: "<<flowtype.GetType());
        NS_LOG_INFO("ddl: "<<ddlvalue);
        if(flowtype.GetType()==FlowTypeTag::PREFILL)
        {
            for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
            {
                Time current_enqueue_time = Simulator::Now();
 
                band = (i + m_rotationOffset) % GetNQueueDiscClasses();
                Time remain_time =  Seconds(1) -(current_enqueue_time - rotation_time);
                remain_bytes[band] = remain_time.ToDouble(Time::S) * (25*10e6)-csize;
                if(band==m_rotationOffset)
                {
                    if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue()&&packetSize<=remain_bytes[band])
                    {
                        bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
                        TimestampTag tsTag;
                        if(!item->GetPacket()->PeekPacketTag(tsTag)){
                        tsTag.SetTimestamp(Simulator::Now());
                        item->GetPacket()->AddPacketTag(tsTag);}
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
                            <<" remainbytes:"<<remain_bytes[band]
                            << "). Success: " << retval);
                            return retval;
                    }
                }
                else
                {
                    if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue())
                    {
                        bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
                        TimestampTag tsTag;
                        if(!item->GetPacket()->PeekPacketTag(tsTag)){
                        tsTag.SetTimestamp(Simulator::Now());
                        item->GetPacket()->AddPacketTag(tsTag);}
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
                            <<" remainbytes:"<<remain_bytes[band]
                            << "). Success: " << retval);
                            return retval;
                    }
                }
                NS_LOG_WARN("Queue "<<band<<" is full! Moving packet to next queue."<<(band+1)
                <<" remainbytes:"<<remain_bytes[band]
                <<". Queue size: "<<(GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets())
                <<" bytebudget:" << m_Bytesbudget[band]
                <<" time now:"<< current_enqueue_time);
            }
 
        }
        else if(flowtype.GetType()==FlowTypeTag::DECODE)
        {
           DelayTag delaytag;
            for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
            {
                Time current_enqueue_time = Simulator::Now();
                uint16_t backward = static_cast<int>(floor((ddl.GetDeadline()- (item->GetPacket()->PeekPacketTag(delaytag)?delaytag.GetTimestamp().GetSeconds():0))/m_rotationInterval.GetSeconds()));
                NS_LOG_INFO((ddl.GetDeadline()- (item->GetPacket()->PeekPacketTag(delaytag)?delaytag.GetTimestamp().GetSeconds():0))/m_rotationInterval.GetSeconds()<<" back"<<backward);
                band = (i + m_rotationOffset+ backward - 1) % GetNQueueDiscClasses();
                Time remain_time =  Seconds(1) -(current_enqueue_time - rotation_time);
                remain_bytes[band] = remain_time.ToDouble(Time::S) * (2*10e6)-csize;
                if(band==m_rotationOffset)
                {
                    if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue()&&packetSize<=remain_bytes[band])
                    {
                    NS_LOG_LOGIC("move to band"<<band);
                    bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
                    TimestampTag tsTag;
                    if(!item->GetPacket()->PeekPacketTag(tsTag)){
                    tsTag.SetTimestamp(Simulator::Now());
                    item->GetPacket()->AddPacketTag(tsTag);}
                    
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
                        <<" remainbytes:"<<remain_bytes[band]
                        << "). Success: " << retval);
                        return retval;
                    }
                }
                else
                {
                    if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue())
                    {
                    NS_LOG_LOGIC("move to band"<<band);
                    bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);
                    TimestampTag tsTag;
                    if(!item->GetPacket()->PeekPacketTag(tsTag)){
                    tsTag.SetTimestamp(Simulator::Now());
                    item->GetPacket()->AddPacketTag(tsTag);}
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
                        <<" remainbytes:"<<remain_bytes[band]
                        << "). Success: " << retval);
                        return retval;
                    }
                }
                NS_LOG_WARN("Queue "<<band<<" is full! Moving packet to next queue."<<(band+1)
                <<" remainbytes:"<<remain_bytes[band]
                <<". Queue size: "<<(GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets())
                <<" bytebudget:" << m_Bytesbudget[band]
                <<" time now:"<< current_enqueue_time);
            }
        }
    }
    else
    {
    NS_LOG_INFO("NO TAGS");
    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
            {
                Time current_enqueue_time = Simulator::Now();
 
                band = (i + m_rotationOffset) % GetNQueueDiscClasses();
                Time remain_time =  Seconds(1) -(current_enqueue_time - rotation_time);
                remain_bytes[band] = remain_time.ToDouble(Time::S) * (2*10e6)-csize;
                if(band==m_rotationOffset)
                {
                    if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue()&&packetSize<=remain_bytes[band])
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
                            <<" remainbytes:"<<remain_bytes[band]
                            << "). Success: " << retval);
                            return retval;
                    }
                }
                else
                {
                    if (m_Bytesbudget[band] + item->GetSize() <= GetMaxSize().GetValue())
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
                            <<" remainbytes:"<<remain_bytes[band]
                            << "). Success: " << retval);
                            return retval;
                    }
                }
                NS_LOG_WARN("Queue "<<band<<" is full! Moving packet to next queue."<<(band+1)
                <<" remainbytes:"<<remain_bytes[band]
                <<". Queue size: "<<(GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets())
                <<" bytebudget:" << m_Bytesbudget[band]
                <<" time now:"<< current_enqueue_time);
            }
    }
 
 }
 
 Ptr<QueueDiscItem>
 CanlendarQueueDisc::DoDequeue()
 {
     NS_LOG_FUNCTION(this);
     
     Time m_delay=Seconds(0);
     Ptr<QueueDiscItem> item;
     uint32_t band = m_rotationOffset;
     for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
     {
         
         band = (i + m_rotationOffset) % GetNQueueDiscClasses();
         if ((item = GetQueueDiscClass(band)->GetQueueDisc()->Dequeue()))
         {   
             DelayTag delaytag;
              //no delaytag->first dequeue->add delaytag=0
             if(!item->GetPacket()->PeekPacketTag(delaytag)){
                 delaytag.SetTimestamp(Seconds(0));
                 item->GetPacket()->AddPacketTag(delaytag);
             }
             
             uint32_t csize = GetQueueDiscClass(band)->GetQueueDisc()->GetNBytes();
             uint32_t packetSize = item->GetPacket()->GetSize();
             FlowTypeTag flowType;
             if (item->GetPacket()->PeekPacketTag(flowType) && flowType.GetType() == FlowTypeTag::DECODE)
             {
                TimestampTag tsTag;
                DelayTag dtag;
                if (item->GetPacket()->PeekPacketTag(tsTag)&&item->GetPacket()->PeekPacketTag(dtag))
                {
                    Time delay = Simulator::Now() - tsTag.GetTimestamp();
                    m_totalQueueDelay += delay;
                    m_dequeuedPackets++;
                    m_delay = dtag.GetTimestamp() + delay - m_timeoutThreshold;
                    dtag.SetTimestamp(m_delay>=Seconds(0)?m_delay:Seconds(0));
                    NS_LOG_INFO("Decode packet delay: " << delay.GetSeconds() << " s");
                    if (delay > m_timeoutThreshold)
                    {
                        m_timeoutCount++;
                    NS_LOG_INFO("Packet timeout: delay = " << delay.GetSeconds() << " s");
                    }
                }
                
            }
            if(item->GetPacket()->PeekPacketTag(flowType)&&flowType.GetType()==FlowTypeTag::PREFILL)
            {
                TimestampTag tsTag;
                if (item->GetPacket()->PeekPacketTag(tsTag))
                {
                Time delay = Simulator::Now() - tsTag.GetTimestamp();
                m_pt += delay;
                m_prefillpacket++;
                }
            }
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
         for (uint8_t i = 0; i <200; i++)
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
     remain_bytes.resize(GetNQueueDiscClasses(), qsize);
    // 重新安排下一次轮转
     m_rotationEvent = Simulator::Schedule(m_rotationInterval, &CanlendarQueueDisc::RotatePriority, this);
     NS_LOG_INFO("current band: "<< m_rotationOffset<<" rotation times: "<<ttt<<" time now:"<<rotation_time);
    }    
 void
 CanlendarQueueDisc::InitializeParams()
 {
     NS_LOG_FUNCTION(this);
     uint16_t ttt = 0;
     m_rotationOffset = 0;
     m_timeoutCount = 0;
     m_timeoutThreshold = Seconds(0.01);
     m_dequeuedPackets = 0;
     m_Bytesbudget.resize(GetNQueueDiscClasses(), 0);
     remain_bytes.resize(GetNQueueDiscClasses(), qsize);
    // 每隔10秒轮转一次
     m_rotationEvent = Simulator::Schedule(m_rotationInterval, &CanlendarQueueDisc::RotatePriority, this);
  }
  void
CanlendarQueueDisc::ReportTimeoutStatistics () const
{
    double avgDelay = (m_dequeuedPackets > 0) ? m_totalQueueDelay.GetSeconds() / m_dequeuedPackets : 0.0;
    double timeoutRate = (m_dequeuedPackets > 0) ? static_cast<double>(m_timeoutCount) / m_dequeuedPackets : 0.0;
    double avgp = (m_prefillpacket>0) ? m_pt.GetSeconds() / m_prefillpacket : 0.0;
    std::cout << "===== Calendar Queue Timeout Statistics =====" << std::endl;
    std::cout << "Total dequeued decode packets: " << m_dequeuedPackets << std::endl;
    std::cout << "Total delay: " << m_totalQueueDelay.GetSeconds()<<"S" << std::endl;
    std::cout << "Total timeout packets: " << m_timeoutCount << std::endl;
    std::cout << "Average queue delay: " << avgDelay << " s" << std::endl;
    std::cout << "Average pt: " << avgp << " s" << std::endl;
    std::cout << "Timeout rate: " << timeoutRate * 100 << " %" << std::endl;
    std::cout << "=========================================" << std::endl;
  }
  void 
  CanlendarQueueDisc::SetQSize(uint32_t size) 
  {
    qsize = size;
  }
 } // namespace ns3
 