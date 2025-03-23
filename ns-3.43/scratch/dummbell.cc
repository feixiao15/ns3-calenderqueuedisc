
 #include "ns3/applications-module.h"
 #include "ns3/core-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/network-module.h"
 #include "ns3/point-to-point-layout-module.h"
 #include "ns3/point-to-point-module.h"
 #include "ns3/traffic-control-module.h"
 #include "ns3/log.h"
 #include <iomanip>
 #include <iostream>
 #include <map>
 #include "ns3/tags.h"
 #include "ns3/random-variable-stream.h"
 #include "ns3/packet.h"
 NS_LOG_COMPONENT_DEFINE("TestTagging");
 using namespace ns3;
//  void AddTagBeforeSending(Ptr<const Packet> packet)
//  {
//      // 复制数据包，避免修改原包
//      Ptr<Packet> p = packet->Copy();
 
//      // 创建一个均匀随机变量对象（范围在 [0,1)）
//      Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
 
//      // 随机选择流类型：假设 0-0.5 选择 PREFILL，0.5-1 选择 DECODE
//      FlowTypeTag flowTag;
//      double r = uv->GetValue();
//      if (r < 0.5)
//      {
//          flowTag.SetType(FlowTypeTag::PREFILL);
//      }
//      else
//      {
//          flowTag.SetType(FlowTypeTag::DECODE);
//      }
//      p->AddPacketTag(flowTag);
 
//      // 随机选择 ddl 值：随机生成整数 0~2，对应 1s、5s、10s
//      DeadlineTag ddlTag;
//      uint32_t choice = uv->GetInteger(0, 2); // 返回 0、1 或 2
//      switch (choice)
//      {
//      case 0:
//          ddlTag.SetDeadline(DeadlineTag::DDL_1S);
//          break;
//      case 1:
//          ddlTag.SetDeadline(DeadlineTag::DDL_5S);
//          break;
//      case 2:
//          ddlTag.SetDeadline(DeadlineTag::DDL_10S);
//          break;
//      default:
//          ddlTag.SetDeadline(DeadlineTag::DDL_5S);
//          break;
//      }
//      p->AddPacketTag(ddlTag);
 
//      NS_LOG_INFO("Tag added before sending: FlowType=" << flowTag.GetType() 
//                    << ", DDL=" << ddlTag.GetDeadline());
//  }

 int
 main(int argc, char* argv[])
 {
     LogComponentEnable("TestTagging", LOG_LEVEL_INFO);
     LogComponentEnable("CanlendarQueueDisc", LOG_LEVEL_INFO);    
    //  LogComponentEnable("OnOffApplication", LOG_LEVEL_DEBUG);
     LogComponentEnable("Simulator", LOG_LEVEL_INFO);
     uint32_t nLeaf = 2;
     uint32_t maxPackets = 100;
     uint32_t modeBytes = 0;
     uint32_t queueDiscLimitPackets = 1000;
     double minTh = 50;
     double maxTh = 80;
     uint32_t pktSize = 512;
     std::string appDataRate = "10Mbps";
     std::string queueDiscType = "PfifoFast";
     uint16_t port = 5001;
     std::string bottleNeckLinkBw = "2Mbps";
     std::string bottleNeckLinkDelay = "50ms";
     uint16_t rt = 1;
     
     Config::SetDefault(
         "ns3::CanlendarQueueDisc::MaxSize",
         QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, 0.5*10e6/8)));
     
     

     // Create the point-to-point link helpers
     PointToPointHelper bottleNeckLink;
     bottleNeckLink.SetDeviceAttribute("DataRate", StringValue(bottleNeckLinkBw));
     bottleNeckLink.SetChannelAttribute("Delay", StringValue(bottleNeckLinkDelay));

     PointToPointHelper pointToPointLeaf;
     pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
     pointToPointLeaf.SetChannelAttribute("Delay", StringValue("1ms"));

     PointToPointDumbbellHelper d(nLeaf, pointToPointLeaf, nLeaf, pointToPointLeaf, bottleNeckLink);

     // Install Stack
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
    //  tchBottleneck.SetRootQueueDisc("ns3::CanlendarQueueDisc");
    //  tchBottleneck.Install(d.GetLeft()->GetDevice(0));
    //  tchBottleneck.Install(d.GetRight()->GetDevice(0));
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
     // Install on/off app on all right side nodes
     OnOffHelper clientHelper("ns3::UdpSocketFactory", Address());
    //  clientHelper.SetAttribute("OnTime", StringValue("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
    //  clientHelper.SetAttribute("OffTime", StringValue("ns3::UniformRandomVariable[Min=0.|Max=1.]"));
     clientHelper.SetConstantRate(DataRate("4Mb/s"), 512);
     clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
     clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    //  clientHelper.SetAttribute("EnableSeqTsSizeHeader",BooleanValue(true));
     Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
     PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkLocalAddress);
     ApplicationContainer sinkApps;
     for (uint32_t i = 0; i < d.LeftCount(); ++i)
     {
         sinkApps.Add(packetSinkHelper.Install(d.GetLeft(i)));
     }
     sinkApps.Start(Seconds(0.0));
     sinkApps.Stop(Seconds(20.0));

     ApplicationContainer clientApps;
     for (uint32_t i = 0; i < d.RightCount(); ++i)
     {
         // Create an on/off app sending packets to the left side
         AddressValue remoteAddress(InetSocketAddress(d.GetLeftIpv4Address(i), port));
         clientHelper.SetAttribute("Remote", remoteAddress);
         clientApps.Add(clientHelper.Install(d.GetRight(i)));
     }
     clientApps.Start(Seconds(0.0)); // Start 1 second after sink
     clientApps.Stop(Seconds(150.0)); // Stop before the sink

     Ipv4GlobalRoutingHelper::PopulateRoutingTables();
     
    //  // 绑定 Trace 回调，在数据包发送前添加 Tag
    //  Ptr<Application> app = clientApps.Get(0);
    //  Ptr<OnOffApplication> onoff = DynamicCast<OnOffApplication>(app);
    //  if (onoff)
    //  {
    //      onoff->TraceConnectWithoutContext("Tx", MakeCallback(&AddTagBeforeSending));
    //      std::cout << "assssssssssssssssssssssssssssssssssss" << std::endl;
    //  }

     Simulator::Stop(Seconds(1.01));
     std::cout << "Running the simulation" << std::endl;
     Simulator::Run();

     uint64_t totalRxBytesCounter = 0;
     for (uint32_t i = 0; i < sinkApps.GetN(); i++)
     {
         Ptr<Application> app = sinkApps.Get(i);
         Ptr<PacketSink> pktSink = DynamicCast<PacketSink>(app);
         totalRxBytesCounter += pktSink->GetTotalRx();
     }
     NS_LOG_UNCOND("----------------------------\nQueueDisc Type:"
                   << queueDiscType
                   << "\nGoodput Bytes/sec:" << totalRxBytesCounter / Simulator::Now().GetSeconds());
     NS_LOG_UNCOND("----------------------------");


     std::cout << "Destroying the simulation" << std::endl;
     Simulator::Destroy();
     return 0;
 }
