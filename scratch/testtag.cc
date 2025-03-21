#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/tags.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"
NS_LOG_COMPONENT_DEFINE("TestTagging");

using namespace ns3;

// 接收端回调函数：在收到数据包时打印其中的 tag 信息
void PrintTagOnReception(Ptr<const Packet> packet, const Address &from)
{
    MyHeader header;
    if(packet->PeekHeader(header))
    {
        NS_LOG_INFO("接收到数据包 FlowType : " << header.GetFlowType());     
        NS_LOG_INFO("接收到数据包 Deadline : " << header.GetDeadline());
    }
}

int main()
{   
    LogComponentEnable("TestTagging", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_INFO);
    LogComponentEnable("MyHeader", LOG_LEVEL_INFO);

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
    InternetStackHelper stack;
    stack.Install(nodes);

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 创建 OnOff 应用
    uint16_t port = 5001;
    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApps;
    sinkApps.Add(packetSinkHelper.Install(nodes.Get(0)));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));
    // 为 PacketSink 应用添加接收 trace，输出 Tag 信息
    Ptr<Application> sinkApp = sinkApps.Get(0);
    sinkApp->TraceConnectWithoutContext("Rx", MakeCallback(&PrintTagOnReception));

    OnOffHelper clientHelper("ns3::TcpSocketFactory", Address());
    clientHelper.SetConstantRate(DataRate("1Mb/s"), 512);
    clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    AddressValue remoteAddress(InetSocketAddress(interfaces.GetAddress(0), port));
    clientHelper.SetAttribute("Remote", remoteAddress);
    ApplicationContainer clientApps = clientHelper.Install(nodes.Get(1));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(10.0));

    // //绑定 Trace 回调，在数据包发送前添加 Tag
    // Ptr<Application> app = clientApps.Get(0);
    // Ptr<OnOffApplication> onoff = DynamicCast<OnOffApplication>(app);
    // if (onoff)
    // {
    //     onoff->TraceConnectWithoutContext("Tx", MakeCallback(&AddTagBeforeSending));
    // }

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
