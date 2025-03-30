
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/tags.h"
#include "ns3/traffic-control-module.h"
#include <iostream>
#include "ns3/socket.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/random-variable-stream.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestTag");

/************************************************************
 *              StopAndWaitReceiver 类
 ************************************************************/
class StopAndWaitReceiver : public Application
{
public:
  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::StopAndWaitReceiver")
      .SetParent<Application> ()
      .AddConstructor<StopAndWaitReceiver> ();
    return tid;
  }

  StopAndWaitReceiver () : m_packetsReceived (0), m_firstReceiveTime (Seconds (0)), m_lastReceiveTime (Seconds (0)) {}
  virtual ~StopAndWaitReceiver () {}

  // 设置本地绑定地址和端口
  void Setup (Address localAddress, uint16_t port)
  {
    m_localAddress = localAddress;
    m_localPort = port;
  }

  // 在仿真结束后调用，输出统计信息
  void ReportStatistics () const
  {
    std::cout << "Receiver on port " << m_localPort << " statistics:" << std::endl;
    std::cout << "  Packets received: " << m_packetsReceived << std::endl;
    if (m_packetsReceived > 0)
      {
        std::cout << "  First packet received at: " << m_firstReceiveTime.GetSeconds () << " s" << std::endl;
        std::cout << "  Last packet received at: " << m_lastReceiveTime.GetSeconds () << " s" << std::endl;
      }
  }

private:
  virtual void StartApplication (void)
  {
    if (!m_socket)
      {
        m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        InetSocketAddress local = InetSocketAddress (Ipv4Address::ConvertFrom (m_localAddress), m_localPort);
        m_socket->Bind (local);
      }
    m_socket->SetRecvCallback (MakeCallback (&StopAndWaitReceiver::HandleRead, this));
  }

  virtual void StopApplication (void)
  {
    if (m_socket)
      {
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      }
  }

  // 收到数据包时，回发一个 ACK 包，并更新统计信息
  void HandleRead (Ptr<Socket> socket)
  {
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom (from)))
      {
        NS_LOG_INFO ("[Receiver] Received " << packet->GetSize () << " bytes"<<" time now:"<<Simulator::Now());
        // 更新统计数据
        if (m_packetsReceived == 0)
          {
            m_firstReceiveTime = Simulator::Now ();
          }
        m_packetsReceived++;
        m_lastReceiveTime = Simulator::Now ();

        // 简单演示：用一个 4 字节的包作为 ACK
        Ptr<Packet> ackPacket = Create<Packet> (4);
        socket->SendTo (ackPacket, 0, from);
        NS_LOG_INFO ("[Receiver] Sent ACK back.");
      }
  }

  Ptr<Socket> m_socket;
  Address     m_localAddress;
  uint16_t    m_localPort;

  // 统计变量
  uint32_t m_packetsReceived;
  Time     m_firstReceiveTime;
  Time     m_lastReceiveTime;
};


/************************************************************
 *              StopAndWaitSender 类
 ************************************************************/
class StopAndWaitSender : public Application
{
public:
  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::StopAndWaitSender")
      .SetParent<Application> ()
      .AddConstructor<StopAndWaitSender> ();
    return tid;
  }

  StopAndWaitSender ()
    : m_socket (0),
      m_remotePort (0),
      m_prefillSize (0),
      m_decodeSize (0),
      m_decodeCount (0),
      m_decodeSent (0),
      m_sentPrefill (false),
      m_packetsSent (0),
      m_packetsAckReceived (0),
      m_firstSendTime (Seconds (0)),
      m_lastAckTime (Seconds (0))
  {}

  virtual ~StopAndWaitSender () {}

  // 设置目标地址、目标端口以及包大小等参数
  void Setup (Address remoteAddress, uint16_t remotePort,
              uint32_t prefillSize, uint32_t decodeSize,
              uint32_t decodeCount)
  {
    m_remoteAddress = remoteAddress;
    m_remotePort    = remotePort;
    m_prefillSize   = prefillSize;
    m_decodeSize    = decodeSize;
    m_decodeCount   = decodeCount;
  }

  void ReportStatistics () const
  {
    std::cout << "Sender to port " << m_remotePort << " statistics:" << std::endl;
    std::cout << "  Packets sent: " << m_packetsSent << std::endl;
    std::cout << "  ACKs received: " << m_packetsAckReceived << std::endl;
    if (m_packetsSent > 0 && !m_firstSendTime.IsZero() && !m_lastAckTime.IsZero())
      {
        std::cout << "  Elapsed time (first send to last ACK): " << (m_lastAckTime - m_firstSendTime).GetSeconds () << " s" << std::endl;
      }
  }
  Time GetElapsedTime() const {
    // 如果没有发送任何包，返回 0
    if (m_packetsSent == 0) {
      return Seconds(0);
    }
    return m_lastAckTime - m_firstSendTime;
  }
  Time GetFirstTime() const {
    return m_firstSendTime;

  }
private:
//   virtual void StartApplication (void)
//   {
//     if (!m_socket)
//       {
//         m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
//         m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_remoteAddress), m_remotePort));
//         m_socket->SetRecvCallback (MakeCallback (&StopAndWaitSender::HandleRead, this));
//       }
//     // 应用启动后先发送第一个大包
//     Simulator::ScheduleNow (&StopAndWaitSender::SendPacket, this);
//   }
  virtual void StartApplication (void)
  {
  NS_LOG_FUNCTION(this);
  
  if (!m_socket)
    {
      // 创建 UDP Socket
      m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
      NS_ABORT_MSG_IF(m_remoteAddress.IsInvalid(), "'RemoteAddress' attribute not properly set");
      
      // 根据地址类型进行判断
      if (Ipv4Address::IsMatchingType(m_remoteAddress))
        {
          // 对于 IPv4，先绑定默认地址
          if (m_socket->Bind() == -1)
            {
              NS_FATAL_ERROR("Failed to bind socket");
            }

          // m_socket->SetIpTos(m_tos);
          m_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_remoteAddress), m_remotePort));
        }
      else if (Ipv6Address::IsMatchingType(m_remoteAddress))
        {
          if (m_socket->Bind6() == -1)
            {
              NS_FATAL_ERROR("Failed to bind socket");
            }
          m_socket->Connect(Inet6SocketAddress(Ipv6Address::ConvertFrom(m_remoteAddress), m_remotePort));
        }
      else if (InetSocketAddress::IsMatchingType(m_remoteAddress))
        {
          if (m_socket->Bind() == -1)
            {
              NS_FATAL_ERROR("Failed to bind socket");
            }
          // m_socket->SetIpTos(m_tos);
          m_socket->Connect(m_remoteAddress);
        }
      else if (Inet6SocketAddress::IsMatchingType(m_remoteAddress))
        {
          if (m_socket->Bind6() == -1)
            {
              NS_FATAL_ERROR("Failed to bind socket");
            }
          m_socket->Connect(m_remoteAddress);
        }
      else
        {
          NS_ASSERT_MSG(false, "Incompatible address type: " << m_remoteAddress);
        }
      
      m_socket->SetRecvCallback(MakeCallback(&StopAndWaitSender::HandleRead, this));
      m_socket->SetAllowBroadcast(true);
    }
  // 应用启动后立即发送第一个数据包
  m_sendEvent = Simulator::ScheduleNow(&StopAndWaitSender::SendPacket, this);
  } 


//   virtual void StopApplication (void)
//   {
//     if (m_socket)
//       {
//         m_socket->Close ();
//       }
//   }
virtual void StopApplication(void)
{
    NS_LOG_FUNCTION(this);

    if (m_socket)
    {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_socket = nullptr;
    }

    Simulator::Cancel(m_sendEvent);
}


  // 发送数据包，按照状态选择发送大包或小包，并更新统计变量
//   void SendPacket (void)
//   {
//     Ptr<Packet> packet;
//     if (!m_sentPrefill)
//       {
//         packet = Create<Packet> (m_prefillSize);
//         FlowTypeTag flowtype;
//         flowtype.SetType(FlowTypeTag::PREFILL);
//         packet->AddPacketTag(flowtype);
//         m_sentPrefill = true;
//       }
//     else
//       {
//         packet = Create<Packet> (m_decodeSize);
//         FlowTypeTag flowtype;
//         flowtype.SetType(FlowTypeTag::DECODE);
//         packet->AddPacketTag(flowtype);
//         }
//     // 记录第一次发送时间
//     if (m_packetsSent == 0)
//       {
//         m_firstSendTime = Simulator::Now ();
//       }
//     m_packetsSent++;
//     m_socket->Send (packet);
//     FlowTypeTag flowtype;
//     bool hastypetag = packet->PeekPacketTag(flowtype);
//     NS_LOG_INFO ("state:"<<hastypetag<<" [Sender] Sending packet, size = " << m_prefillSize<<" type:"<<(flowtype.GetType()== FlowTypeTag::PREFILL ? "PREFILL" : "DECODE"));
//     // 发送后等待对端的 ACK，再决定是否发送下一个包
//   }
void SendPacket (void)
{
  NS_LOG_FUNCTION(this);
  
  Ptr<Packet> packet;
  // 构造 packet：根据当前状态选择 prefill 或 decode
  if (!m_sentPrefill)
    {
      packet = Create<Packet>(m_prefillSize);
       // 获取本地地址，用于 trace 调用
      Address localAddress;
      m_socket->GetSockName(localAddress);

      m_txTrace(packet);
      if (Ipv4Address::IsMatchingType(m_remoteAddress))
      {
      m_txTraceWithAddresses(packet,
                             localAddress,
                             InetSocketAddress(Ipv4Address::ConvertFrom(m_remoteAddress),
                                                m_remotePort));
      }
      else if (Ipv6Address::IsMatchingType(m_remoteAddress))
      {
      m_txTraceWithAddresses(packet,
                             localAddress,
                             Inet6SocketAddress(Ipv6Address::ConvertFrom(m_remoteAddress),
                                                 m_remotePort));
      }
      FlowTypeTag flowtype;
      DeadlineTag ddlTag;
      flowtype.SetType(FlowTypeTag::PREFILL);
      Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
      uint32_t choice = uv->GetInteger (0, 2); // 随机生成 0,1,2
      switch (choice)
      {
      case 0:
          ddlTag.SetDeadline(DeadlineTag::DDL_1S);
          break;
      case 1:
          ddlTag.SetDeadline(DeadlineTag::DDL_5S);
          break;
      case 2:
          ddlTag.SetDeadline(DeadlineTag::DDL_10S);
          break;
      default:
          ddlTag.SetDeadline(DeadlineTag::DDL_5S);
          break;
      }
      packet->AddPacketTag(ddlTag);

      packet->AddPacketTag(flowtype);
      m_sentPrefill = true;
    }
  else
    {
      packet = Create<Packet>(m_decodeSize);
      Address localAddress;
      m_socket->GetSockName(localAddress);

      m_txTrace(packet);
      if (Ipv4Address::IsMatchingType(m_remoteAddress))
      {
      m_txTraceWithAddresses(packet,
                             localAddress,
                             InetSocketAddress(Ipv4Address::ConvertFrom(m_remoteAddress),
                                                m_remotePort));
      }
      else if (Ipv6Address::IsMatchingType(m_remoteAddress))
      {
      m_txTraceWithAddresses(packet,
                             localAddress,
                             Inet6SocketAddress(Ipv6Address::ConvertFrom(m_remoteAddress),
                                                 m_remotePort));
      }
      FlowTypeTag flowtype;
      DeadlineTag ddlTag;
      Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
      uint32_t choice = uv->GetInteger (0, 2); // 随机生成 0,1,2
      switch (choice)
      {
      case 0:
          ddlTag.SetDeadline(DeadlineTag::DDL_1S);
          break;
      case 1:
          ddlTag.SetDeadline(DeadlineTag::DDL_5S);
          break;
      case 2:
          ddlTag.SetDeadline(DeadlineTag::DDL_10S);
          break;
      default:
          ddlTag.SetDeadline(DeadlineTag::DDL_5S);
          break;
      }
      packet->AddPacketTag(ddlTag);
      flowtype.SetType(FlowTypeTag::DECODE);
      packet->AddPacketTag(flowtype);
    }
  
  // 记录首次发送时间
  if (m_packetsSent == 0)
    {
      m_firstSendTime = Simulator::Now();
    }
  m_packetsSent++;

 
  
  // 发送 packet 到 UDP/IP 层
  m_socket->Send(packet);
  

  FlowTypeTag flowTag;
  DeadlineTag ddlTag;
  bool hastypetag = packet->PeekPacketTag(flowTag);
  NS_LOG_INFO("state:" << hastypetag 
              << " [Sender] Sending packet, size = " << packet->GetSize() 
              << " type:" << (flowTag.GetType() == FlowTypeTag::PREFILL ? "PREFILL" : "DECODE")<<" time now:"<<Simulator::Now()
              <<" ddl:"<<ddlTag.GetDeadline());

}

  // 收到 ACK 后，更新统计数据并决定是否继续发送 decode 包
  void HandleRead (Ptr<Socket> socket)
  {
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom (from)))
      {
        NS_LOG_INFO ("[Sender] Received ACK, size = " << packet->GetSize());
        m_packetsAckReceived++;
        m_lastAckTime = Simulator::Now ();
        // 如果已经发送了 prefill 包，则继续发送 decode 包
        if (m_sentPrefill)
          {
            m_decodeSent++;
            if (m_decodeSent < m_decodeCount)
              {
                m_sendEvent = Simulator::ScheduleNow (&StopAndWaitSender::SendPacket, this);
              }
            else
              {
                NS_LOG_INFO ("All decode packets have been sent!");
              }
          }
        else
          {
            NS_LOG_INFO ("[Sender] Prefill ACK received, now start decode packets.");
            m_sentPrefill = true;
            m_sendEvent = Simulator::ScheduleNow (&StopAndWaitSender::SendPacket, this);
          }
      }
  }

  Ptr<Socket> m_socket;
  Address     m_remoteAddress;
  uint16_t    m_remotePort;
  uint32_t    m_prefillSize;
  uint32_t    m_decodeSize;
  uint32_t    m_decodeCount;
  uint32_t    m_decodeSent;
  bool        m_sentPrefill;
  EventId     m_sendEvent; 
  TracedCallback<Ptr<const Packet>> m_txTrace;
  TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_txTraceWithAddresses;

  // 统计变量
  uint32_t m_packetsSent;
  uint32_t m_packetsAckReceived;
  Time     m_firstSendTime;
  Time     m_lastAckTime;
};


int main (int argc, char *argv[])
{
  // 启用日志组件
  // LogComponentEnable ("TestTag", LOG_LEVEL_INFO);
  // LogComponentEnable("CanlendarQueueDisc", LOG_LEVEL_INFO);  

  CommandLine cmd;
  cmd.Parse (argc, argv);

  /*DUMMBELL*/
  uint32_t nLeaf = 20;
  uint32_t maxPackets = 415;
  std::string leafBw = "100Gbps";

  std::string bottleNeckLinkBw = "25Gbps";
  std::string bottleNeckLinkDelay = "1ms";
  float rt = 1;
  
  Config::SetDefault(
      "ns3::CanlendarQueueDisc::MaxSize",
      QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, 25*0.02*10e9/8)));


PointToPointHelper bottleNeckLink;
bottleNeckLink.SetDeviceAttribute("DataRate", StringValue(bottleNeckLinkBw));
bottleNeckLink.SetChannelAttribute("Delay", StringValue(bottleNeckLinkDelay));
bottleNeckLink.SetDeviceAttribute("Mtu", UintegerValue(9000));
PointToPointHelper pointToPointLeaf;
pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue(leafBw));
pointToPointLeaf.SetChannelAttribute("Delay", StringValue("1ms"));
pointToPointLeaf.SetDeviceAttribute("Mtu", UintegerValue(9000));

PointToPointDumbbellHelper d(nLeaf, pointToPointLeaf, nLeaf, pointToPointLeaf, bottleNeckLink);


InternetStackHelper stack;
for (uint32_t i = 0; i < d.LeftCount(); ++i)
{
    stack.Install(d.GetLeft(i));
}
for (uint32_t i = 0; i < d.RightCount(); ++i)
{
    stack.Install(d.GetRight(i));
}
stack.Install(d.GetLeft());
stack.Install(d.GetRight());


  TrafficControlHelper tchBottleneck;
  tchBottleneck.SetRootQueueDisc("ns3::FifoQueueDisc");
  // tchBottleneck.SetRootQueueDisc("ns3::RedQueueDisc");
  //  tchBottleneck.SetRootQueueDisc("ns3::CanlendarQueueDisc");
  //tchBottleneck.SetRootQueueDisc("ns3::PfifoFastQueueDisc");

  // // // // 在设备上安装调度器
   QueueDiscContainer qdiscs;
   qdiscs.Add(tchBottleneck.Install(d.GetLeft()->GetDevice(0)));
   qdiscs.Add(tchBottleneck.Install(d.GetRight()->GetDevice(0)));

  // // 获取已安装的 CanlendarQueueDisc 并修改 RotationInterval
  //  for (uint32_t i = 0; i < qdiscs.GetN(); ++i)
  //  {
  //      Ptr<QueueDisc> qdisc = qdiscs.Get(i);
  //      Ptr<CanlendarQueueDisc> canlendarQ = DynamicCast<CanlendarQueueDisc>(qdisc);
  //      QueueSize maxSize = canlendarQ->GetMaxSize();
  //      std::cout << "size" << maxSize <<std::endl;
  //      if (canlendarQ)
  //      {
  //          canlendarQ->SetAttribute("RotationInterval", TimeValue(Seconds(rt))); 
  //          std::cout << "time" << rt <<std::endl;
  //      }
  //  }
   // Assign IP Addresses
   d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
   Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
   Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));
   Ipv4GlobalRoutingHelper::PopulateRoutingTables();
   uint16_t basePort = 5001;
   std::vector< Ptr<StopAndWaitSender> > senderApps;
   std::vector< Ptr<StopAndWaitReceiver> > receiverApps;
   for (uint32_t i = 0; i < d.LeftCount(); ++i)
   {
    for (uint32_t j = 0; j < 100; ++j)
    {
        uint16_t port = basePort + i * 100 + j;
  


        Ptr<Node> senderNode = d.GetLeft(i);
        Ptr<Node> receiverNode = d.GetRight(j % d.RightCount());
        Ptr<StopAndWaitReceiver> receiverApp = CreateObject<StopAndWaitReceiver>();
        receiverApp->Setup(d.GetRightIpv4Address(j % d.RightCount()), port);
        receiverNode->AddApplication(receiverApp);
        receiverApps.push_back(receiverApp);
        receiverApp->SetStartTime(Seconds(0));
        receiverApp->SetStopTime(Seconds(500.0));

        Ptr<StopAndWaitSender> senderApp = CreateObject<StopAndWaitSender>();
        senderApp->Setup(d.GetRightIpv4Address(j % d.RightCount()), port, 1700*5, 5*512, maxPackets);
        senderNode->AddApplication(senderApp);
        senderApps.push_back(senderApp);

        Ptr<ExponentialRandomVariable> interval = CreateObject<ExponentialRandomVariable>();
        interval->SetAttribute("Mean", DoubleValue(1.0 / 10e6));
        double nextInterval = interval->GetValue();
        senderApp->SetStartTime(Seconds(0));
        senderApp->SetStopTime(Seconds(500.0));
    }
  }

//   // 创建两个节点
//   NodeContainer nodes;
//   nodes.Create (2);

//   // 搭建点对点链路
//   PointToPointHelper p2p;
//   p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
//   p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
//   NetDeviceContainer devices = p2p.Install (nodes);

//   // 安装协议栈
//   InternetStackHelper stack;
//   stack.Install (nodes);

//   // 分配IP地址
//   Ipv4AddressHelper address;
//   address.SetBase ("10.1.1.0", "255.255.255.0");
//   Ipv4InterfaceContainer ifaces = address.Assign (devices);

//   Ipv4Address senderIp   = ifaces.GetAddress (0);
//   Ipv4Address receiverIp = ifaces.GetAddress (1);

//   // 模拟 100 条流，每条流使用不同的端口进行区分
//   uint16_t basePort = 8000;
//   // 保存所有应用指针，方便后面输出统计信息
//   std::vector< Ptr<StopAndWaitSender> > senderApps;
//   std::vector< Ptr<StopAndWaitReceiver> > receiverApps;

//   for (uint32_t i = 0; i < 100; ++i)
//     {
//       uint16_t port = basePort + i;

//       // 在节点1上安装接收端应用
//       Ptr<StopAndWaitReceiver> receiverApp = CreateObject<StopAndWaitReceiver> ();
//       receiverApp->Setup (receiverIp, port);
//       nodes.Get (1)->AddApplication (receiverApp);
//       receiverApp->SetStartTime (Seconds (0.0));
//       receiverApp->SetStopTime (Seconds (10.0));
//       receiverApps.push_back (receiverApp);

//       // 在节点0上安装发送端应用
//       Ptr<StopAndWaitSender> senderApp = CreateObject<StopAndWaitSender> ();
//       // 示例参数：prefill 包 5000 字节，decode 包 100 字节，共发送 10 个 decode 包
//       senderApp->Setup (receiverIp, port, 5000, 100, 10);
//       nodes.Get (0)->AddApplication (senderApp);
//       senderApp->SetStartTime (Seconds (0.1 + i * 0.001));
//       senderApp->SetStopTime (Seconds (10.0));
//       senderApps.push_back (senderApp);
//     }

  Simulator::Stop (Seconds (500.0));
  Simulator::Run ();
  Simulator::Destroy ();

  // 仿真结束后，输出所有应用的统计数据
  std::cout << "========== Statistics ==========" << std::endl;
  for (uint32_t i = 0; i < senderApps.size (); i++)
    {
      senderApps[i]->ReportStatistics ();
    }
  for (uint32_t i = 0; i < receiverApps.size (); i++)
    {
      receiverApps[i]->ReportStatistics ();
    }
  std::cout << "================================" << std::endl;
// 统计所有流的耗时，计算平均耗时和最大耗时
  Time totalElapsed = Seconds(0);
  Time maxElapsed = Seconds(0);
  Time minElapsed = Seconds(500);
  uint32_t validFlows = 0;
  for (uint32_t i = 0; i < senderApps.size(); i++)
  {
    // std::cout <<"first send time: "<<senderApps[i]->GetFirstTime();
      Time elapsed = senderApps[i]->GetElapsedTime();
      // 仅统计有发送记录的流
      if (!elapsed.IsZero())
      {
          validFlows++;
          totalElapsed += elapsed;
          if (elapsed > maxElapsed)
          {
              maxElapsed = elapsed;
          }
          if (elapsed < minElapsed)
          {
              minElapsed = elapsed;
          }
      }
      
  }

  if (validFlows > 0)
  {
      double avgElapsed = totalElapsed.GetSeconds() / validFlows;
      std::cout << "Average flow elapsed time: " << avgElapsed << " s" << std::endl;
      std::cout << "Max flow elapsed time: " << maxElapsed.GetSeconds() << " s" << std::endl;
      std::cout << "Min flow elapsed time: " << minElapsed.GetSeconds() << " s" << std::endl;
  }
  else
  {
      std::cout << "No valid flows were recorded." << std::endl;
  }

  return 0;
}
