#ifndef TAGS_H
#define TAGS_H
#include "ns3/tag.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
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
        enum DeadlineType
        {
            DDL_1S = 1,
            DDL_5S = 5,
            DDL_10S = 10
        };

        static TypeId GetTypeId();
        virtual TypeId GetInstanceTypeId() const;
        virtual void Serialize(TagBuffer i) const;
        virtual void Deserialize(TagBuffer i);
        virtual uint32_t GetSerializedSize() const;
        virtual void Print(std::ostream &os) const;

        void SetDeadline(DeadlineType deadline);
        DeadlineType GetDeadline() const;
        uint32_t GetDdlValue(DeadlineType deadline);


        private:
        DeadlineType m_deadline;
        uint32_t m_ddlvalue;

    };
/* ... */

}

#endif /* TAGS_H */
