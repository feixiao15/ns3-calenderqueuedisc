#include "tags.h"
#include "ns3/core-module.h"
namespace ns3
{
    TypeId FlowTypeTag::GetTypeId()
    {
       static TypeId tid = TypeId("ns3::FlowTypeTag")
           .SetParent<Tag>()
           .AddConstructor<FlowTypeTag>();
       return tid;
    }
    
    TypeId FlowTypeTag::GetInstanceTypeId() const
    {
       return GetTypeId();
    }
    
    void FlowTypeTag::Serialize(TagBuffer i) const
    {
       i.WriteU8(m_type);
    }
    
    void FlowTypeTag::Deserialize(TagBuffer i)
    {
       m_type = static_cast<FlowType>(i.ReadU8());
    }
    
    uint32_t FlowTypeTag::GetSerializedSize() const
    {
       return 1;
    }
    
    void FlowTypeTag::Print(std::ostream &os) const
    {
       os << "FlowType=" << (m_type == PREFILL ? "PREFILL" : "DECODE");
    }
    
    void FlowTypeTag::SetType(FlowType type)
    {
       m_type = type;
    }
    
    FlowTypeTag::FlowType FlowTypeTag::GetType() const
    {
       return m_type;
    }
    
    /* ---------- DeadlineTag 实现 ---------- */
    TypeId DeadlineTag::GetTypeId()
{
  static TypeId tid = TypeId("ns3::DeadlineTag")
    .SetParent<Tag>()
    .AddConstructor<DeadlineTag>();
  return tid;
}

TypeId DeadlineTag::GetInstanceTypeId() const
{
  return GetTypeId();
}

void DeadlineTag::Serialize(TagBuffer i) const
{
  i.WriteDouble(m_deadline);
}

void DeadlineTag::Deserialize(TagBuffer i)
{
  m_deadline = i.ReadDouble();
}

uint32_t DeadlineTag::GetSerializedSize() const
{
  return sizeof(double);
}

void DeadlineTag::Print(std::ostream &os) const
{
  os << "Deadline=" << m_deadline << "s";
}

void DeadlineTag::SetDeadline(double deadline)
{
  m_deadline = deadline;
}

double DeadlineTag::GetDeadline() const
{
  return m_deadline;
}


/* ... */
TypeId
DelayTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DelayTag")
    .SetParent<Tag> ()
    .AddConstructor<DelayTag> ();
  return tid;
}

TypeId
DelayTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
DelayTag::GetSerializedSize (void) const
{
  // 使用 double 存储时间戳，共 8 字节
  return 8;
}

void
DelayTag::Serialize (TagBuffer i) const
{
  double t = m_timestamp.GetSeconds ();
  i.WriteDouble (t);
}

void
DelayTag::Deserialize (TagBuffer i)
{
  double t = i.ReadDouble ();
  m_timestamp = Seconds (t);
}

void
DelayTag::Print (std::ostream &os) const
{
  os << "timestamp=" << m_timestamp.GetSeconds ();
}

void
DelayTag::SetTimestamp (Time t)
{
  m_timestamp = t;
}

Time
DelayTag::GetTimestamp (void) const
{
  return m_timestamp;
}
}