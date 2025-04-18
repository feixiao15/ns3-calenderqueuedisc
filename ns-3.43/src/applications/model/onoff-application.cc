//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#include "onoff-application.h"

#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/tags.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OnOffApplication");

NS_OBJECT_ENSURE_REGISTERED(OnOffApplication);

TypeId
OnOffApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::OnOffApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<OnOffApplication>()
            .AddAttribute("DataRate",
                          "The data rate in on state.",
                          DataRateValue(DataRate("500kb/s")),
                          MakeDataRateAccessor(&OnOffApplication::m_cbrRate),
                          MakeDataRateChecker())
            .AddAttribute("PacketSize",
                          "The size of packets sent in on state",
                          UintegerValue(512),
                          MakeUintegerAccessor(&OnOffApplication::m_pktSize),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("Remote",
                          "The address of the destination",
                          AddressValue(),
                          MakeAddressAccessor(&OnOffApplication::m_peer),
                          MakeAddressChecker())
            .AddAttribute("Local",
                          "The Address on which to bind the socket. If not set, it is generated "
                          "automatically.",
                          AddressValue(),
                          MakeAddressAccessor(&OnOffApplication::m_local),
                          MakeAddressChecker())
            .AddAttribute("Tos",
                          "The Type of Service used to send IPv4 packets. "
                          "All 8 bits of the TOS byte are set (including ECN bits).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&OnOffApplication::m_tos),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("OnTime",
                          "A RandomVariableStream used to pick the duration of the 'On' state.",
                          StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                          MakePointerAccessor(&OnOffApplication::m_onTime),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("OffTime",
                          "A RandomVariableStream used to pick the duration of the 'Off' state.",
                          StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                          MakePointerAccessor(&OnOffApplication::m_offTime),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("MaxBytes",
                          "The total number of bytes to send. Once these bytes are sent, "
                          "no packet is sent again, even in on state. The value zero means "
                          "that there is no limit.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&OnOffApplication::m_maxBytes),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("Protocol",
                          "The type of protocol to use. This should be "
                          "a subclass of ns3::SocketFactory",
                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&OnOffApplication::m_tid),
                          // This should check for SocketFactory as a parent
                          MakeTypeIdChecker())
            .AddAttribute("EnableSeqTsSizeHeader",
                          "Enable use of SeqTsSizeHeader for sequence number and timestamp",
                          BooleanValue(false),
                          MakeBooleanAccessor(&OnOffApplication::m_enableSeqTsSizeHeader),
                          MakeBooleanChecker())
            .AddTraceSource("Tx",
                            "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&OnOffApplication::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxWithAddresses",
                            "A new packet is created and is sent",
                            MakeTraceSourceAccessor(&OnOffApplication::m_txTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback")
            .AddTraceSource("TxWithSeqTsSize",
                            "A new packet is created with SeqTsSizeHeader",
                            MakeTraceSourceAccessor(&OnOffApplication::m_txTraceWithSeqTsSize),
                            "ns3::PacketSink::SeqTsSizeCallback");
    return tid;
}

OnOffApplication::OnOffApplication()
    : m_socket(nullptr),
      m_connected(false),
      m_residualBits(0),
      m_lastStartTime(Seconds(0)),
      m_totBytes(0),
      m_unsentPacket(nullptr)
{
    NS_LOG_FUNCTION(this);
}

OnOffApplication::~OnOffApplication()
{
    NS_LOG_FUNCTION(this);
}

void
OnOffApplication::SetMaxBytes(uint64_t maxBytes)
{
    NS_LOG_FUNCTION(this << maxBytes);
    m_maxBytes = maxBytes;
}

Ptr<Socket>
OnOffApplication::GetSocket() const
{
    NS_LOG_FUNCTION(this);
    return m_socket;
}

int64_t
OnOffApplication::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_onTime->SetStream(stream);
    m_offTime->SetStream(stream + 1);
    return 2;
}

void
OnOffApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);

    CancelEvents();
    m_socket = nullptr;
    m_unsentPacket = nullptr;
    // chain up
    Application::DoDispose();
}

// Application Methods
void
OnOffApplication::StartApplication() // Called at time specified by Start
{
    NS_LOG_FUNCTION(this);

    // Create the socket if not already
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        int ret = -1;

        NS_ABORT_MSG_IF(m_peer.IsInvalid(), "'Remote' attribute not properly set");

        if (!m_local.IsInvalid())
        {
            NS_ABORT_MSG_IF((Inet6SocketAddress::IsMatchingType(m_peer) &&
                             InetSocketAddress::IsMatchingType(m_local)) ||
                                (InetSocketAddress::IsMatchingType(m_peer) &&
                                 Inet6SocketAddress::IsMatchingType(m_local)),
                            "Incompatible peer and local address IP version");
            ret = m_socket->Bind(m_local);
        }
        else
        {
            if (Inet6SocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind6();
            }
            else if (InetSocketAddress::IsMatchingType(m_peer) ||
                     PacketSocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind();
            }
        }

        if (ret == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }

        m_socket->SetConnectCallback(MakeCallback(&OnOffApplication::ConnectionSucceeded, this),
                                     MakeCallback(&OnOffApplication::ConnectionFailed, this));

        if (InetSocketAddress::IsMatchingType(m_peer))
        {
            m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
        }
        m_socket->Connect(m_peer);
        m_socket->SetAllowBroadcast(true);
        m_socket->ShutdownRecv();
    }
    m_cbrRateFailSafe = m_cbrRate;

    // Ensure no pending event
    CancelEvents();

    // If we are not yet connected, there is nothing to do here,
    // the ConnectionComplete upcall will start timers at that time.
    // If we are already connected, CancelEvents did remove the events,
    // so we have to start them again.
    if (m_connected)
    {
        ScheduleStartEvent();
    }
}

void
OnOffApplication::StopApplication() // Called at time specified by Stop
{
    NS_LOG_FUNCTION(this);

    CancelEvents();
    if (m_socket)
    {
        m_socket->Close();
    }
    else
    {
        NS_LOG_WARN("OnOffApplication found null socket to close in StopApplication");
    }
}

void
OnOffApplication::CancelEvents()
{
    NS_LOG_FUNCTION(this);

    if (m_sendEvent.IsPending() && m_cbrRateFailSafe == m_cbrRate)
    { // Cancel the pending send packet event
        // Calculate residual bits since last packet sent
        Time delta(Simulator::Now() - m_lastStartTime);
        int64x64_t bits = delta.To(Time::S) * m_cbrRate.GetBitRate();
        m_residualBits += bits.GetHigh();
    }
    m_cbrRateFailSafe = m_cbrRate;
    Simulator::Cancel(m_sendEvent);
    Simulator::Cancel(m_startStopEvent);
    // Canceling events may cause discontinuity in sequence number if the
    // SeqTsSizeHeader is header, and m_unsentPacket is true
    if (m_unsentPacket)
    {
        NS_LOG_DEBUG("Discarding cached packet upon CancelEvents ()");
    }
    m_unsentPacket = nullptr;
}

// Event handlers
void
OnOffApplication::StartSending()
{
    NS_LOG_FUNCTION(this);
    m_lastStartTime = Simulator::Now();
    ScheduleNextTx(); // Schedule the send packet event
    ScheduleStopEvent();
}

void
OnOffApplication::StopSending()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();

    ScheduleStartEvent();
}

// Private helpers
void
OnOffApplication::ScheduleNextTx()
{
    NS_LOG_FUNCTION(this);

    if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
        NS_ABORT_MSG_IF(m_residualBits > m_pktSize * 8,
                        "Calculation to compute next send time will overflow");
        uint32_t bits = m_pktSize * 8 - m_residualBits;
        NS_LOG_LOGIC("bits = " << bits);
        Time nextTime(
            Seconds(bits / static_cast<double>(m_cbrRate.GetBitRate()))); // Time till next packet
        NS_LOG_LOGIC("nextTime = " << nextTime.As(Time::S));
        m_sendEvent = Simulator::Schedule(nextTime, &OnOffApplication::SendPacket, this);
    }
    else
    { // All done, cancel any pending events
        StopApplication();
    }
}

void
OnOffApplication::ScheduleStartEvent()
{ // Schedules the event to start sending data (switch to the "On" state)
    NS_LOG_FUNCTION(this);

    Time offInterval = Seconds(m_offTime->GetValue());
    NS_LOG_LOGIC("start at " << offInterval.As(Time::S));
    m_startStopEvent = Simulator::Schedule(offInterval, &OnOffApplication::StartSending, this);
}

void
OnOffApplication::ScheduleStopEvent()
{ // Schedules the event to stop sending data (switch to "Off" state)
    NS_LOG_FUNCTION(this);

    Time onInterval = Seconds(m_onTime->GetValue());
    NS_LOG_LOGIC("stop at " << onInterval.As(Time::S));
    m_startStopEvent = Simulator::Schedule(onInterval, &OnOffApplication::StopSending, this);
}

void
OnOffApplication::SendPacket()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_sendEvent.IsExpired());

    Ptr<Packet> packet;
    if (m_unsentPacket)
    {
        packet = m_unsentPacket;
        NS_LOG_INFO("??????");
    }
    else if (m_enableSeqTsSizeHeader)
    {
        Address from;
        Address to;
        m_socket->GetSockName(from);
        m_socket->GetPeerName(to);
        SeqTsSizeHeader header;
        header.SetSeq(m_seq++);
        header.SetSize(m_pktSize);
        NS_ABORT_IF(m_pktSize < header.GetSerializedSize());
        packet = Create<Packet>(m_pktSize - header.GetSerializedSize());
        // Trace before adding header, for consistency with PacketSink
        m_txTraceWithSeqTsSize(packet, from, to, header);
        packet->AddHeader(header);
    }
    else
    {
        packet = Create<Packet>(m_pktSize);
        FlowTypeTag flowTag;
  
        Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
        double r = uv->GetValue();
        NS_LOG_DEBUG("r: "<<r);
        if (r < 0.5)
        {
            flowTag.SetType(FlowTypeTag::PREFILL);
        }
        else
        {
            flowTag.SetType(FlowTypeTag::DECODE);
        }
        packet->AddPacketTag(flowTag);
        NS_LOG_INFO("Added FlowTypeTag to packet: " << flowTag.GetType());
 

    // 检查是否已有 DeadlineTag
        DeadlineTag ddlTag;
  
        uint32_t choice = uv->GetInteger (0, 2); // 随机生成 0,1,2
        NS_LOG_DEBUG("choice: "<<choice);
        switch (choice)
        {
        case 0:
            ddlTag.SetDeadline(1);
            break;
        case 1:
            ddlTag.SetDeadline(5);
            break;
        case 2:
            ddlTag.SetDeadline(10);
            break;
        default:
            ddlTag.SetDeadline(5);
            break;
        }
        packet->AddPacketTag(ddlTag);
        NS_LOG_INFO("Added DeadlineTag to packet: " << ddlTag.GetDeadline());
    
        
    }
    FlowTypeTag flowTag;
    DeadlineTag ddlTag;
    packet->PeekPacketTag(flowTag);
    packet->PeekPacketTag(ddlTag);
    NS_LOG_INFO("Added tag to packet: FlowType=" 
                << (flowTag.GetType() == FlowTypeTag::PREFILL ? "PREFILL" : "DECODE")
                << ", Deadline=" << ddlTag.GetDeadline() << "s");
    

    int actual = m_socket->Send(packet);
    NS_LOG_INFO("SEND packet");
    // if ((unsigned)actual == m_pktSize + 2)
    // {
    //     m_txTrace(packet);
    //     m_totBytes += m_pktSize;
    //     m_unsentPacket = nullptr;
    //     Address localAddress;
    //     m_socket->GetSockName(localAddress);
  
    //     if (InetSocketAddress::IsMatchingType(m_peer))
    //     {
    //         NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " on-off application sent "
    //                                << packet->GetSize() << " bytes to "
    //                                << InetSocketAddress::ConvertFrom(m_peer).GetIpv4() << " port "
    //                                << InetSocketAddress::ConvertFrom(m_peer).GetPort()
    //                                << " total Tx " << m_totBytes << " bytes");
    //         m_txTraceWithAddresses(packet, localAddress, InetSocketAddress::ConvertFrom(m_peer));
    //     }
    //     else if (Inet6SocketAddress::IsMatchingType(m_peer))
    //     {
    //         NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " on-off application sent "
    //                                << packet->GetSize() << " bytes to "
    //                                << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6() << " port "
    //                                << Inet6SocketAddress::ConvertFrom(m_peer).GetPort()
    //                                << " total Tx " << m_totBytes << " bytes");
    //         m_txTraceWithAddresses(packet, localAddress, Inet6SocketAddress::ConvertFrom(m_peer));
    //     }
    //     else
    //     {
    //         NS_LOG_INFO("WHYWHYWHYWHY");
    //     }
    // }
    // else
    // {
    //     NS_LOG_DEBUG("Unable to send packet; actual " << actual << " size " << m_pktSize
    //                                                   << "; caching for later attempt");
    //     m_unsentPacket = packet;
    // }
//
    m_totBytes += m_pktSize;
    m_unsentPacket = nullptr;
    Address localAddress;
    m_socket->GetSockName(localAddress);

    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " on-off application sent "
                            << packet->GetSize() << " bytes to "
                            << InetSocketAddress::ConvertFrom(m_peer).GetIpv4() << " port "
                            << InetSocketAddress::ConvertFrom(m_peer).GetPort()
                            << " total Tx " << m_totBytes << " bytes");
        m_txTraceWithAddresses(packet, localAddress, InetSocketAddress::ConvertFrom(m_peer));
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " on-off application sent "
                            << packet->GetSize() << " bytes to "
                            << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6() << " port "
                            << Inet6SocketAddress::ConvertFrom(m_peer).GetPort()
                            << " total Tx " << m_totBytes << " bytes");
        m_txTraceWithAddresses(packet, localAddress, Inet6SocketAddress::ConvertFrom(m_peer));
    }
    else
    {
        NS_LOG_INFO("WHYWHYWHYWHY");
    }
//
    m_residualBits = 0;
    m_lastStartTime = Simulator::Now();
    ScheduleNextTx();
}

void
OnOffApplication::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    ScheduleStartEvent();
    m_connected = true;
}

void
OnOffApplication::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_FATAL_ERROR("Can't connect");
}

} // Namespace ns3
