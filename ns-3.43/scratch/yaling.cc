#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("test");
int main() {    
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    NodeContainer leftNodes, rightNodes, switchNodes;
    leftNodes.Create(2);  
    rightNodes.Create(2); 
    switchNodes.Create(2); 

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer switchDevices;
    switchDevices = p2p.Install(switchNodes);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer leftCsmaDevices;
    NodeContainer leftSwitch = NodeContainer(leftNodes, switchNodes.Get(0)); 
    leftCsmaDevices = csma.Install(leftSwitch);

    NetDeviceContainer rightCsmaDevices;
    NodeContainer rightSwitch = NodeContainer(rightNodes, switchNodes.Get(1)); 
    rightCsmaDevices = csma.Install(rightSwitch);

    InternetStackHelper stack;
    stack.Install(leftNodes);
    stack.Install(rightNodes);
    stack.Install(switchNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer leftInterfaces = address.Assign(leftCsmaDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer rightInterfaces = address.Assign(rightCsmaDevices);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interSwitchInterfaces = address.Assign(switchDevices);
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(rightNodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(rightInterfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(leftNodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(200.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    p2p.EnablePcapAll("test");
    csma.EnablePcap("test",leftCsmaDevices.Get(0),true);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
