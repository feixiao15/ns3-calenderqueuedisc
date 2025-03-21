#include "tags.h"

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
       i.WriteU8(m_deadline);
    }
    
    void DeadlineTag::Deserialize(TagBuffer i)
    {
       m_deadline = static_cast<DeadlineType>(i.ReadU8());
    }
    
    uint32_t DeadlineTag::GetSerializedSize() const
    {
       return 1;
    }
    
    void DeadlineTag::Print(std::ostream &os) const
    {
       os << "Deadline=" << static_cast<uint32_t>(m_deadline) << "s";
    }
    
    void DeadlineTag::SetDeadline(DeadlineType deadline)
    {
       m_deadline = deadline;
    }
    
    DeadlineTag::DeadlineType DeadlineTag::GetDeadline() const
    {
       return m_deadline;
    }
    uint32_t DeadlineTag::GetDdlValue(DeadlineType ddl)
    {
       if (ddl == DeadlineTag::DDL_1S){
             m_ddlvalue = 1;
       }
       if (ddl == DeadlineTag::DDL_5S){
             m_ddlvalue = 5;
       }
       if (ddl == DeadlineTag::DDL_10S){
             m_ddlvalue = 10;
       }
       return m_ddlvalue;
    }
/* ... */

}
