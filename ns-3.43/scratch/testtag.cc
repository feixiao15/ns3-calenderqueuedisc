
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/tags.h"
#include "ns3/traffic-control-module.h"
#include <iostream>

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
        NS_LOG_INFO ("[Receiver] Received " << packet->GetSize () << " bytes");
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

  // 在仿真结束后调用，输出统计信息
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

private:
  virtual void StartApplication (void)
  {
    if (!m_socket)
      {
        m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_remoteAddress), m_remotePort));
        m_socket->SetRecvCallback (MakeCallback (&StopAndWaitSender::HandleRead, this));
      }
    // 应用启动后先发送第一个大包
    Simulator::ScheduleNow (&StopAndWaitSender::SendPacket, this);
  }

  virtual void StopApplication (void)
  {
    if (m_socket)
      {
        m_socket->Close ();
      }
  }

  // 发送数据包，按照状态选择发送大包或小包，并更新统计变量
  void SendPacket (void)
  {
    Ptr<Packet> packet;
    if (!m_sentPrefill)
      {
        packet = Create<Packet> (m_prefillSize);
        FlowTypeTag flowtype;
        flowtype.SetType(FlowTypeTag::PREFILL);
        packet->AddPacketTag(flowtype);
        m_sentPrefill = true;
      }
    else
      {
        packet = Create<Packet> (m_decodeSize);
        FlowTypeTag flowtype;
        flowtype.SetType(FlowTypeTag::DECODE);
        packet->AddPacketTag(flowtype);
        }
    // 记录第一次发送时间
    if (m_packetsSent == 0)
      {
        m_firstSendTime = Simulator::Now ();
      }
    m_packetsSent++;
    m_socket->Send (packet);
    FlowTypeTag flowtype;
    NS_LOG_INFO ("state:"<<(packet->PeekPacketTag(flowtype))<<" [Sender] Sending prefill packet, size = " << m_prefillSize<<" type:"<<(flowtype.GetType()== FlowTypeTag::PREFILL ? "PREFILL" : "DECODE"));
    // 发送后等待对端的 ACK，再决定是否发送下一个包
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
                Simulator::ScheduleNow (&StopAndWaitSender::SendPacket, this);
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
            Simulator::ScheduleNow (&StopAndWaitSender::SendPacket, this);
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

  // 统计变量
  uint32_t m_packetsSent;
  uint32_t m_packetsAckReceived;
  Time     m_firstSendTime;
  Time     m_lastAckTime;
};


int main (int argc, char *argv[])
{
  // 启用日志组件
  LogComponentEnable ("TestTag", LOG_LEVEL_INFO);
  LogComponentEnable("CanlendarQueueDisc", LOG_LEVEL_INFO);  

  CommandLine cmd;
  cmd.Parse (argc, argv);

  /*DUMMBELL*/
  uint32_t nLeaf = 2;
  uint32_t maxPackets = 415;
  std::string appDataRate = "10Mbps";

  std::string bottleNeckLinkBw = "2Mbps";
  std::string bottleNeckLinkDelay = "50ms";
  float rt = 1;
  
  Config::SetDefault(
      "ns3::CanlendarQueueDisc::MaxSize",
      QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, 2*0.02*10e6/8)));


PointToPointHelper bottleNeckLink;
bottleNeckLink.SetDeviceAttribute("DataRate", StringValue(bottleNeckLinkBw));
bottleNeckLink.SetChannelAttribute("Delay", StringValue(bottleNeckLinkDelay));

PointToPointHelper pointToPointLeaf;
pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
pointToPointLeaf.SetChannelAttribute("Delay", StringValue("1ms"));


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

  // 配置调度器类型
   tchBottleneck.SetRootQueueDisc("ns3::CanlendarQueueDisc");

  // 在设备上安装调度器
   QueueDiscContainer qdiscs;
   qdiscs.Add(tchBottleneck.Install(d.GetLeft()->GetDevice(0)));
   qdiscs.Add(tchBottleneck.Install(d.GetRight()->GetDevice(0)));

  // 获取已安装的 CanlendarQueueDisc 并修改 RotationInterval
   for (uint32_t i = 0; i < qdiscs.GetN(); ++i)
   {
       Ptr<QueueDisc> qdisc = qdiscs.Get(i);
       Ptr<CanlendarQueueDisc> canlendarQ = DynamicCast<CanlendarQueueDisc>(qdisc);
       QueueSize maxSize = canlendarQ->GetMaxSize();
       std::cout << "size" << maxSize <<std::endl;
       if (canlendarQ)
       {
           canlendarQ->SetAttribute("RotationInterval", TimeValue(Seconds(rt))); 
           std::cout << "time" << rt <<std::endl;
       }
   }
   // Assign IP Addresses
   d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
   Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
   Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));
   Ipv4GlobalRoutingHelper::PopulateRoutingTables();
   uint16_t basePort = 5001;
   std::vector< Ptr<StopAndWaitSender> > senderApps;
   std::vector< Ptr<StopAndWaitReceiver> > receiverApps;
    for (uint32_t i = 0; i < 1; ++i)
    {
        uint16_t port = basePort + i;
        
        // 根据左侧节点数量取模，选择一个左侧叶节点作为发送端
        Ptr<Node> senderNode = d.GetLeft(0);
        // 根据右侧节点数量取模，选择一个右侧叶节点作为接收端
        Ptr<Node> receiverNode = d.GetRight(0);

        // 创建并安装自定义接收端应用
        Ptr<StopAndWaitReceiver> receiverApp = CreateObject<StopAndWaitReceiver>();
        // 使用该右侧节点的IP地址作为接收端地址
        receiverApp->Setup(d.GetRightIpv4Address(0), port);
        receiverNode->AddApplication(receiverApp);
        receiverApps.push_back (receiverApp);
        receiverApp->SetStartTime(Seconds(1.0));
        receiverApp->SetStopTime(Seconds(100.0));

        // 创建并安装自定义发送端应用
        Ptr<StopAndWaitSender> senderApp = CreateObject<StopAndWaitSender>();
        // 这里使用接收端IP地址和端口来配置发送端，
        // 参数依次为：目标地址、端口、prefill包大小（例如5000字节）、decode包大小（例如100字节）、decode包个数（例如maxPackets）
        senderApp->Setup(d.GetRightIpv4Address(0), port, 512, 6, maxPackets);
        senderNode->AddApplication(senderApp);
        senderApps.push_back (senderApp);
        // 为了使每个流的启动时间错开，可以在启动时间上加上一个微小偏移
        senderApp->SetStartTime(Seconds(1.0 + i * 0.001));
        senderApp->SetStopTime(Seconds(10.0));
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

  Simulator::Stop (Seconds (20.0));
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

  return 0;
}
