#ifndef TAGS_H
#define TAGS_H
#include "ns3/tag.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/core-module.h"


// Add a doxygen group for this module.
// If you have more than one file, this should be in only one of them.
/**
 * \defgroup tags Description of the tags
 */

namespace ns3
{

// Each class should be documented using Doxygen,
// and have an \ingroup tags directive
    class FlowTypeTag : public Tag
    {
        public:
        enum FlowType
        {
            PREFILL = 0,
            DECODE = 1
        };

        static TypeId GetTypeId();
        virtual TypeId GetInstanceTypeId() const;
        virtual void Serialize(TagBuffer i) const;
        virtual void Deserialize(TagBuffer i);
        virtual uint32_t GetSerializedSize() const;
        virtual void Print(std::ostream &os) const;

        void SetType(FlowType type);
        FlowType GetType() const;

        private:
        FlowType m_type;
    };

    /* ---------- DeadlineTag (标记 DDL: 10ms, 50ms, 100ms) ---------- */
    class DeadlineTag : public Tag
    {
        public:
  

        static TypeId GetTypeId();
        virtual TypeId GetInstanceTypeId() const;
        virtual void Serialize(TagBuffer i) const;
        virtual void Deserialize(TagBuffer i);
        virtual uint32_t GetSerializedSize() const;
        virtual void Print(std::ostream &os) const;

        void SetDeadline(double deadline);
        double GetDeadline() const;



        private:
        double m_deadline;
        double m_ddlvalue;

    };
/* ... */
class DelayTag : public Tag
{
public:

  // 注册类型
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  // Tag 必须实现的接口：序列化/反序列化、打印、字节大小
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // 设置和获取时间戳
  void SetTimestamp (Time t);
  Time GetTimestamp (void) const;

private:
  Time m_timestamp;
};
}
#endif /* TAGS_H */
