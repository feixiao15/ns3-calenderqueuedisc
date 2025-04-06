#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/queue-disc.h"
#include "ns3/queue-disc-container.h"
#include "ns3/random-variable-stream.h"
#include <iostream>
#include <vector>
#include "ns3/tags.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LeafSpineSenderReceiver");

//************************************************************
//           StopAndWaitReceiver 类定义
//************************************************************
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
enum SenderPhase { PREFILL, DECODE };

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
      m_prefillCount (0),
      m_prefillSent (0),
      m_decodeSent (0),
      m_phase (PREFILL),
      m_packetsSent (0),
      m_packetsAckReceived (0),
      m_firstSendTime (Seconds (0)),
      m_lastAckTime (Seconds (0)),
      m_firstPrefillSendTime (Seconds (0)),
      m_lastPrefillAckTime (Seconds (0)),
      m_prefillAckCount (0)
  {}

  virtual ~StopAndWaitSender () {}

  // Setup() 增加 prefillCount 参数：连续发送的 prefill 包数量
  void Setup (Address remoteAddress, uint16_t remotePort,
              uint32_t prefillSize, uint32_t decodeSize,
              uint32_t decodeCount, uint32_t prefillCount)
  {
    m_remoteAddress = remoteAddress;
    m_remotePort    = remotePort;
    m_prefillSize   = prefillSize;
    m_decodeSize    = decodeSize;
    m_decodeCount   = decodeCount;
    m_prefillCount  = prefillCount;
  }

  void ReportStatistics () const
  {
    std::cout << "Sender to port " << m_remotePort << " statistics:" << std::endl;
    std::cout << "  Packets sent: " << m_packetsSent << std::endl;
    std::cout << "  ACKs received: " << m_packetsAckReceived << std::endl;
    if (m_packetsSent > 0 && !m_firstSendTime.IsZero() && !m_lastAckTime.IsZero())
    {
      std::cout << "  Total elapsed time (first send to last ACK): " 
                << (m_lastAckTime - m_firstSendTime).GetSeconds () << " s" << std::endl;
    }
    if (m_prefillCount > 0 && !m_firstPrefillSendTime.IsZero() && !m_lastPrefillAckTime.IsZero())
    {
      Time prefillElapsed = m_lastPrefillAckTime - m_firstPrefillSendTime;
      std::cout << "  Prefill flow elapsed time (first prefill send to last prefill ACK): "
                << prefillElapsed.GetSeconds() << " s" << std::endl;
    }
  }
  Time GetPrefillTime() const{
    if (m_prefillCount > 0 && !m_firstPrefillSendTime.IsZero() && !m_lastPrefillAckTime.IsZero()){
      Time prefillElapsed = m_lastPrefillAckTime - m_firstPrefillSendTime;
      return prefillElapsed;
    }
     return Seconds(0);

  }
  
  Time GetElapsedTime() const {
    if (m_packetsSent == 0) {
      return Seconds(0);
    }
    return m_lastAckTime - m_firstSendTime;
  }

private:
  virtual void StartApplication (void)
  {
    NS_LOG_FUNCTION(this);
  
    if (!m_socket)
      {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        NS_ABORT_MSG_IF(m_remoteAddress.IsInvalid(), "'RemoteAddress' attribute not properly set");
      
        if (Ipv4Address::IsMatchingType(m_remoteAddress))
          {
            if (m_socket->Bind() == -1)
              {
                NS_FATAL_ERROR("Failed to bind socket");
              }
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
    // 应用启动后立即连续发送所有 prefill 包
    Simulator::ScheduleNow(&StopAndWaitSender::SendPrefillPackets, this);
  } 

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

  // 连续发送所有 prefill 包
  void SendPrefillPackets(void)
  {
    NS_LOG_FUNCTION(this);
    // 记录首次发送时间
    if (m_packetsSent == 0)
      {
        m_firstSendTime = Simulator::Now();
        m_firstPrefillSendTime = Simulator::Now();
      }
    for ( ; m_prefillSent < m_prefillCount; m_prefillSent++)
      {
        Ptr<Packet> packet = Create<Packet>(m_prefillSize);
        FlowTypeTag flowtype;
        flowtype.SetType(FlowTypeTag::PREFILL);
        DeadlineTag ddlTag;
        // 这里可以采用随机设置的方式
        Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
        // uint32_t choice = uv->GetInteger (0, 2);
        // switch (choice)
        //   {
        //   case 0:
        //     ddlTag.SetDeadline(DeadlineTag::DDL_1S);
        //     break;
        //   case 1:
        //     ddlTag.SetDeadline(DeadlineTag::DDL_5S);
        //     break;
        //   case 2:
        //     ddlTag.SetDeadline(DeadlineTag::DDL_10S);
        //     break;
        //   default:
        //     ddlTag.SetDeadline(DeadlineTag::DDL_5S);
        //     break;
        //   }
        ddlTag.SetDeadline(0.1);
        packet->AddPacketTag(ddlTag);
        packet->AddPacketTag(flowtype);
        m_socket->Send(packet);
        m_packetsSent++;
        NS_LOG_INFO("[Sender] Sent prefill packet, size = " << packet->GetSize()
                    << " time:" << Simulator::Now());
      }
    NS_LOG_INFO("[Sender] All prefill packets sent. Waiting for ACK to start decode packets.");
  }

  // 连续发送所有 decode 包
  void SendDecodePackets(void)
  {
    NS_LOG_FUNCTION(this);
    for ( ; m_decodeSent < m_decodeCount; m_decodeSent++)
      {
        Ptr<Packet> packet = Create<Packet>(m_decodeSize);
        FlowTypeTag flowtype;
        flowtype.SetType(FlowTypeTag::DECODE);
        DeadlineTag ddlTag;
        Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
        // uint32_t choice = uv->GetInteger (0, 2);
        // switch (choice)
        //   {
        //   case 0:
        //     ddlTag.SetDeadline(DeadlineTag::DDL_1S);
        //     break;
        //   case 1:
        //     ddlTag.SetDeadline(DeadlineTag::DDL_5S);
        //     break;
        //   case 2:
        //     ddlTag.SetDeadline(DeadlineTag::DDL_10S);
        //     break;
        //   default:
        //     ddlTag.SetDeadline(DeadlineTag::DDL_5S);
        //     break;
        //   }
        ddlTag.SetDeadline(0.1);
        packet->AddPacketTag(ddlTag);
        packet->AddPacketTag(flowtype);
        m_socket->Send(packet);
        m_packetsSent++;
        NS_LOG_INFO("[Sender] Sent decode packet, size = " << packet->GetSize()
                    << " time:" << Simulator::Now()<<" ddl:"<<ddlTag.GetDeadline());
      }
    NS_LOG_INFO("[Sender] All decode packets have been sent.");
  }

 
  // 其他阶段只记录统计数据
  void HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom(from)))
  {
    NS_LOG_INFO ("[Sender] Received ACK, size = " << packet->GetSize() 
                 << " at " << Simulator::Now());
    m_packetsAckReceived++;

    if (m_phase == PREFILL)
    {
      m_prefillAckCount++;
      if (m_prefillAckCount == m_prefillCount)
      {
          m_lastPrefillAckTime = Simulator::Now();
          NS_LOG_INFO("[Sender] Received ACK for last prefill packet at " << m_lastPrefillAckTime);
          m_phase = DECODE;
          Simulator::ScheduleNow(&StopAndWaitSender::SendDecodePackets, this);
      }
    }
    m_lastAckTime = Simulator::Now();
  }
}


  Ptr<Socket> m_socket;
  Address     m_remoteAddress;
  uint16_t    m_remotePort;
  uint32_t    m_prefillSize;
  uint32_t    m_decodeSize;
  uint32_t    m_decodeCount;
  uint32_t    m_prefillCount;   // 总的 prefill 包数量
  uint32_t    m_prefillSent;    // 已发送的 prefill 包数量
  uint32_t    m_decodeSent;     // 已发送的 decode 包数量
  SenderPhase m_phase = PREFILL;
  EventId     m_sendEvent; 
  TracedCallback<Ptr<const Packet>> m_txTrace;
  TracedCallback<Ptr<const Packet>, const Address&, const Address&> m_txTraceWithAddresses;

  // 统计变量
  uint32_t m_packetsSent;
  uint32_t m_packetsAckReceived;
  Time     m_firstSendTime;
  Time     m_lastAckTime;

  Time m_firstPrefillSendTime;  // 第一个 prefill 包发送时间
  Time m_lastPrefillAckTime;    // 最后一个 prefill 包的 ACK 收到时间
  uint32_t m_prefillAckCount;   // 已收到 ACK 的 prefill 包数量
};


//************************************************************
//                       main()
//************************************************************
int main (int argc, char *argv[])
{
  //LogComponentEnable ("LeafSpineSenderReceiver", LOG_LEVEL_INFO);
  //LogComponentEnable ("CanlendarQueueDisc", LOG_LEVEL_ALL);
  //LogComponentEnable ("FifoQueueDisc", LOG_LEVEL_ALL);

  CommandLine cmd;
  cmd.Parse (argc, argv);

  bool statement = true; 
  float rt = 0.01;      
  uint32_t qz = 100;   
  uint32_t numd = 40000;
  uint32_t nump = 100;
  uint32_t flowsPerHost = 100;  
  uint32_t hostsPerLeaf = 8; 
  uint32_t numLeaves = 4;


  NodeContainer spineNodes;
  spineNodes.Create (2);
  NodeContainer leafNodes;
  leafNodes.Create (numLeaves);

  NodeContainer hostNodes;
  hostNodes.Create (numLeaves*hostsPerLeaf);

  // 安装 Internet 协议栈
  InternetStackHelper internet;
  internet.Install (spineNodes);
  internet.Install (leafNodes);
  internet.Install (hostNodes);

  // (a) leaf 与 spine 之间链路
  PointToPointHelper p2pLeafSpine;
  p2pLeafSpine.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
  p2pLeafSpine.SetChannelAttribute ("Delay", StringValue ("1ms"));
  // (b) leaf 与 host 之间链路
  PointToPointHelper p2pLeafHost;
  p2pLeafHost.SetDeviceAttribute ("DataRate", StringValue ("25Gbps"));
  p2pLeafHost.SetChannelAttribute ("Delay", StringValue ("1ms"));

  // 用于分配 IP 地址
  Ipv4AddressHelper address;
  uint32_t networkId = 1;


  TrafficControlHelper tch;
  if (statement)
    {
      tch.SetRootQueueDisc ("ns3::FifoQueueDisc");
    }
  else
    {
      tch.SetRootQueueDisc ("ns3::CanlendarQueueDisc");
    }


  // 构建 leaf-spine 链路
  // 为每个 leaf 与每个 spine 之间建立 p2p 链路，并在链路设备上安装队列调度器
  QueueDiscContainer qdiscsInstalled;
  for (uint32_t i = 0; i < leafNodes.GetN (); ++i)
    {
      for (uint32_t j = 0; j < spineNodes.GetN (); ++j)
        {
          NodeContainer pair (leafNodes.Get (i), spineNodes.Get (j));
          NetDeviceContainer devices = p2pLeafSpine.Install (pair);

          QueueDiscContainer qdiscs;
          qdiscs.Add (tch.Install (devices.Get (0)));
          qdiscs.Add (tch.Install (devices.Get (1)));
          qdiscsInstalled.Add (qdiscs);
          for (uint32_t k = 0; k < qdiscs.GetN (); ++k)
            {
              Ptr<QueueDisc> qdisc = qdiscs.Get (k);
              if (statement)
                {
                  Ptr<FifoQueueDisc> fifoQ = DynamicCast<FifoQueueDisc> (qdisc);
                  if (fifoQ)
                    {
                      fifoQ->SetAttribute ("MaxSize", QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, 10e20)));
                    }
                }
              else
                {
                  Ptr<CanlendarQueueDisc> calQ = DynamicCast<CanlendarQueueDisc> (qdisc);
                  if (calQ)
                    {
                      calQ->SetAttribute ("RotationInterval", TimeValue (Seconds (rt)));
                      calQ->SetQSize (qz);
                    }
                }
            }

          std::ostringstream subnet;
          subnet << "10." << networkId << ".0.0";
          address.SetBase (Ipv4Address (subnet.str ().c_str ()), "255.255.255.0");
          address.Assign (devices);
          networkId++;
        }
    }

  // 构建 leaf-host 链路
  std::vector<Ipv4Address> hostAddresses;
 
  for (uint32_t i = 0; i < leafNodes.GetN (); ++i)
  {
      for (uint32_t j = 0; j < 4; ++j)
      {
          uint32_t hostIndex = i * 4 + j;
          NodeContainer pair (leafNodes.Get (i), hostNodes.Get (hostIndex));
          NetDeviceContainer devices = p2pLeafHost.Install (pair);
          std::ostringstream subnet;
          subnet << "10." << networkId << ".0.0";
          address.SetBase (Ipv4Address (subnet.str ().c_str ()), "255.255.255.0");
          Ipv4InterfaceContainer ifCont = address.Assign (devices);
          // 设备0在 leaf，设备1在 host，记录 host 的 IP 地址
          hostAddresses.push_back (ifCont.GetAddress (1));
          networkId++;
      }
  }


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

uint16_t basePort = 5001;

std::vector< Ptr<StopAndWaitSender> > senderApps;
std::vector< Ptr<StopAndWaitReceiver> > receiverApps;
uint32_t numHosts = hostNodes.GetN();

//receiver apps
for (uint32_t i = 0; i < numHosts; ++i)
{
    for (uint32_t j = 0; j < flowsPerHost; ++j)
    {
        uint16_t port = basePort + i * flowsPerHost + j;
        Ptr<StopAndWaitReceiver> receiverApp = CreateObject<StopAndWaitReceiver> ();
        receiverApp->Setup (hostAddresses[i], port);
        hostNodes.Get(i)->AddApplication(receiverApp);
        receiverApp->SetStartTime (Seconds(0.0));
        receiverApp->SetStopTime (Seconds(500.0));
        receiverApps.push_back(receiverApp);
    }
}
//sender apps
for (uint32_t i = 0; i < numHosts; ++i)
{
    uint32_t myLeaf = i / hostsPerLeaf;
    uint32_t offset = i % hostsPerLeaf;
    uint32_t remoteLeaf = (myLeaf + 1) % numLeaves; 
    uint32_t remoteIndex = remoteLeaf * hostsPerLeaf + offset; 

    for (uint32_t j = 0; j < flowsPerHost; ++j)
    {
        uint16_t port = basePort + remoteIndex * flowsPerHost + j;
        Ptr<StopAndWaitSender> senderApp = CreateObject<StopAndWaitSender> ();
        senderApp->Setup (hostAddresses[remoteIndex], port, 1700 * 5, 5 * 512, numd, nump);
        hostNodes.Get(i)->AddApplication(senderApp);
        Ptr<ExponentialRandomVariable> interval = CreateObject<ExponentialRandomVariable> ();
        interval->SetAttribute ("Mean", DoubleValue (1.0 / 100));
        double nextInterval = interval->GetValue ();
        senderApp->SetStartTime (Seconds (nextInterval));
        senderApp->SetStopTime (Seconds (500.0));
        senderApps.push_back(senderApp);
    }
}


  Simulator::Stop (Seconds (500.0));
  Simulator::Run ();
  Simulator::Destroy ();

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
  uint32_t validFlows2 = 0;
  Time totalpt =Seconds(0);
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
      Time pt = senderApps[i]->GetPrefillTime();
      if (!pt.IsZero()){
        validFlows2++;
        totalpt += pt;}
      
  }

  if (validFlows > 0)
  {
      double avgElapsed = totalElapsed.GetSeconds() / validFlows;
      double avgpt = totalpt.GetSeconds()/validFlows2;
      std::cout << "Average flow elapsed time: " << avgElapsed << " s" << std::endl;
      // std::cout << "Average flow preill time: " << avgpt << " s" << std::endl; //maybe it's wrong
      std::cout << "Max flow elapsed time: " << maxElapsed.GetSeconds() << " s" << std::endl;
      std::cout << "Min flow elapsed time: " << minElapsed.GetSeconds() << " s" << std::endl;
  }
  else
  {
      std::cout << "No valid flows were recorded." << std::endl;
  }

  for (uint32_t i = 0; i < qdiscsInstalled.GetN(); i++)
  {
      Ptr<QueueDisc> qdisc = qdiscsInstalled.Get(i);
      if(statement==true){
      Ptr<FifoQueueDisc> fifoQueue = DynamicCast<FifoQueueDisc>(qdisc);
      if (fifoQueue)
      {
          fifoQueue->ReportTimeoutStatistics();
      }}
      else{
      Ptr<CanlendarQueueDisc> cQueue = DynamicCast<CanlendarQueueDisc>(qdisc);
      if (cQueue)
      {
          cQueue->ReportTimeoutStatistics();
      }}
  }


  return 0;
}
