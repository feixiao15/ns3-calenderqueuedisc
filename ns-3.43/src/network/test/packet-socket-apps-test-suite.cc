/*
 * Copyright (c) 2014 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/traced-callback.h"
#include "ns3/uinteger.h"

using namespace ns3;

/**
 * \ingroup network-test
 * \ingroup tests
 *
 * \brief PacketSocket apps Unit Test
 */
class PacketSocketAppsTest : public TestCase
{
    uint32_t m_receivedPacketSize;   //!< Received packet size
    uint32_t m_receivedPacketNumber; //!< Number of received packets

  public:
    void DoRun() override;
    PacketSocketAppsTest();

    /**
     * Receive a packet
     * \param packet The packet
     * \param from Address of the sender
     */
    void ReceivePkt(Ptr<const Packet> packet, const Address& from);
};

PacketSocketAppsTest::PacketSocketAppsTest()
    : TestCase("Packet Socket Apps test")
{
    m_receivedPacketSize = 0;
    m_receivedPacketNumber = 0;
}

void
PacketSocketAppsTest::ReceivePkt(Ptr<const Packet> packet, const Address& from)
{
    if (packet)
    {
        m_receivedPacketSize = packet->GetSize();
        m_receivedPacketNumber++;
    }
}

void
PacketSocketAppsTest::DoRun()
{
    // Create topology

    NodeContainer nodes;
    nodes.Create(2);

    PacketSocketHelper packetSocket;

    // give packet socket powers to nodes.
    packetSocket.Install(nodes);

    Ptr<SimpleNetDevice> txDev;
    txDev = CreateObject<SimpleNetDevice>();
    nodes.Get(0)->AddDevice(txDev);

    Ptr<SimpleNetDevice> rxDev;
    rxDev = CreateObject<SimpleNetDevice>();
    nodes.Get(1)->AddDevice(rxDev);

    Ptr<SimpleChannel> channel = CreateObject<SimpleChannel>();
    txDev->SetChannel(channel);
    rxDev->SetChannel(channel);
    txDev->SetNode(nodes.Get(0));
    rxDev->SetNode(nodes.Get(1));

    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice(txDev->GetIfIndex());
    socketAddr.SetPhysicalAddress(rxDev->GetAddress());
    socketAddr.SetProtocol(1);

    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
    client->SetRemote(socketAddr);
    client->SetAttribute("PacketSize", UintegerValue(1000));
    client->SetAttribute("MaxPackets", UintegerValue(3));
    nodes.Get(0)->AddApplication(client);

    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
    server->TraceConnectWithoutContext("Rx", MakeCallback(&PacketSocketAppsTest::ReceivePkt, this));
    server->SetLocal(socketAddr);
    nodes.Get(1)->AddApplication(server);

    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_EXPECT_MSG_EQ(m_receivedPacketNumber, 3, "Number of packet received");
    NS_TEST_EXPECT_MSG_EQ(m_receivedPacketSize, 1000, "Size of packet received");
}

/**
 * \ingroup network-test
 * \ingroup tests
 *
 * \brief PacketSocket apps TestSuite
 */
class PacketSocketAppsTestSuite : public TestSuite
{
  public:
    PacketSocketAppsTestSuite()
        : TestSuite("packet-socket-apps", Type::UNIT)
    {
        AddTestCase(new PacketSocketAppsTest, TestCase::Duration::QUICK);
    }
};

static PacketSocketAppsTestSuite
    g_packetSocketAppsTestSuite; //!< Static variable for test initialization
