#ifndef CALENDAR_QUEUE_DISC_H
#define CALENDAR_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include "ns3/event-id.h"
 
 #include <array>
 
 namespace ns3
 {
 
 /// Priority map
 typedef std::array<uint16_t, 16> Priomap;
 
 /**
  * \ingroup traffic-control
  *
  * The Prio qdisc is a simple classful queueing discipline that contains an
  * arbitrary number of classes of differing priority. The classes are dequeued
  * in numerical descending order of priority. By default, three Fifo queue
  * discs are created, unless the user provides (at least two) child queue
  * discs.
  *
  * If no packet filter is installed or able to classify a packet, then the
  * packet is assigned a priority band based on its priority (modulo 16), which
  * is used as an index into an array called priomap. If a packet is classified
  * by a packet filter and the returned value is non-negative and less than the
  * number of priority bands, then the packet is assigned the priority band
  * corresponding to the value returned by the packet filter. Otherwise, the
  * packet is assigned the priority band specified by the first element of the
  * priomap array.
  */
 class CanlendarQueueDisc : public QueueDisc
 {
   public:
     /**
      * \brief Get the type ID.
      * \return the object TypeId
      */
     static TypeId GetTypeId();
     /**
      * \brief CanlendarQueueDisc constructor
      */
     CanlendarQueueDisc();
 
     ~CanlendarQueueDisc() override;
 
     /**
      * Set the band (class) assigned to packets with specified priority.
      *
      * \param prio the priority of packets (a value between 0 and 15).
      * \param band the band assigned to packets.
      */
     void SetBandForPriority(uint8_t prio, uint16_t band);
 
     /**
      * Get the band (class) assigned to packets with specified priority.
      *
      * \param prio the priority of packets (a value between 0 and 15).
      * \returns the band assigned to packets.
      */
     uint16_t GetBandForPriority(uint8_t prio) const;
 
   private:
     bool DoEnqueue(Ptr<QueueDiscItem> item) override;
     Ptr<QueueDiscItem> DoDequeue() override;
     Ptr<const QueueDiscItem> DoPeek() override;
     bool CheckConfig() override;
     void InitializeParams() override;
     void RotatePriority();
     uint16_t ttt;
     Priomap m_prio2band; //!< Priority to band mapping
     uint32_t m_rotationOffset;   // 轮转偏移量
     uint32_t m_rotationOffset2; 
     EventId m_rotationEvent;     // 轮转定时器
     Time rotation_time;
     Time m_rotationInterval;
     uint32_t maxQueueSize;
     std::vector<uint32_t> m_Bytesbudget; 
     std::vector<uint32_t> remain_processing_time;
 };
 
 /**
  * Serialize the priomap to the given ostream
  *
  * \param os
  * \param priomap
  *
  * \return std::ostream
  */
//  std::ostream& operator<<(std::ostream& os, const Priomap& priomap);
 
//  /**
//   * Serialize from the given istream to this priomap.
//   *
//   * \param is
//   * \param priomap
//   *
//   * \return std::istream
//   */
//  std::istream& operator>>(std::istream& is, Priomap& priomap);
 
//  ATTRIBUTE_HELPER_HEADER(Priomap);
 
 } // namespace ns3
 
 #endif /* PRIO_QUEUE_DISC_H */
 