/*
 * Copyright (c) 2012 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv6-address-generator.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/log.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/simple-net-device.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * \ingroup internet-test
 *
 * \brief IPv6 AddressHelper Test
 */
class IpAddressHelperTestCasev6 : public TestCase
{
  public:
    IpAddressHelperTestCasev6();
    ~IpAddressHelperTestCasev6() override;

  private:
    void DoRun() override;
    void DoTeardown() override;
};

IpAddressHelperTestCasev6::IpAddressHelperTestCasev6()
    : TestCase("IpAddressHelper Ipv6 test case")
{
}

IpAddressHelperTestCasev6::~IpAddressHelperTestCasev6()
{
}

void
IpAddressHelperTestCasev6::DoRun()
{
    Ipv6AddressHelper ip1;
    Ipv6Address ipAddr1;
    ipAddr1 = ip1.NewAddress();
    // Ipv6AddressHelper that is unconfigured
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:db8::1"), "Ipv6AddressHelper failure");
    ipAddr1 = ip1.NewAddress();
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:db8::2"), "Ipv6AddressHelper failure");
    ip1.NewNetwork();
    ipAddr1 = ip1.NewAddress();
    NS_TEST_ASSERT_MSG_EQ(ipAddr1,
                          Ipv6Address("2001:db8:0:1:0:0:0:1"),
                          "Ipv6AddressHelper failure");

    // Reset
    ip1.SetBase(Ipv6Address("2001:db81::"), Ipv6Prefix(32), Ipv6Address("::1"));
    ipAddr1 = ip1.NewAddress();
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:db81::1"), "Ipv6AddressHelper failure");
    // Skip a few addresses
    ip1.SetBase(Ipv6Address("2001:db81::"), Ipv6Prefix(32), Ipv6Address("::15"));
    ipAddr1 = ip1.NewAddress();
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:db81::15"), "Ipv6AddressHelper failure");
    ipAddr1 = ip1.NewAddress();
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:db81::16"), "Ipv6AddressHelper failure");
    // Skip a some more addresses
    ip1.SetBase(Ipv6Address("2001:db81::"), Ipv6Prefix(32), Ipv6Address("::ff"));
    ipAddr1 = ip1.NewAddress();
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:db81::ff"), "Ipv6AddressHelper failure");
    ipAddr1 = ip1.NewAddress();
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:db81::100"), "Ipv6AddressHelper failure");
    // Increment network
    ip1.NewNetwork();
    ipAddr1 = ip1.NewAddress();
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:db82::ff"), "Ipv6AddressHelper failure");

    // Reset
    ip1.SetBase(Ipv6Address("2001:dddd::"), Ipv6Prefix(64), Ipv6Address("::1"));
    ipAddr1 = ip1.NewAddress(); // ::1
    ipAddr1 = ip1.NewAddress(); // ::2
    ipAddr1 = ip1.NewAddress(); // ::3
    ip1.NewNetwork();           // 0:1
    ip1.NewNetwork();           // 0:2
    ipAddr1 = ip1.NewAddress(); // ::1 again
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:dddd:0:2::1"), "Ipv6AddressHelper failure");

    // Set again
    ip1.SetBase(Ipv6Address("2001:db82::"), Ipv6Prefix(32));
    ipAddr1 = ip1.NewAddress();
    NS_TEST_ASSERT_MSG_EQ(ipAddr1, Ipv6Address("2001:db82::1"), "Ipv6AddressHelper failure");

    // Reset again
    ip1.SetBase(Ipv6Address("2001:f00d:cafe:00ff::"), Ipv6Prefix(64), Ipv6Address("::1"));
    ip1.NewNetwork();           // "2001:f00d:cafe:0100::"
    ipAddr1 = ip1.NewAddress(); // ::1 again
    NS_TEST_ASSERT_MSG_EQ(ipAddr1,
                          Ipv6Address("2001:f00d:cafe:0100::1"),
                          "Ipv6AddressHelper failure");

    // Test container assignment
    NodeContainer n;
    n.Create(2);

    /* Install IPv4/IPv6 stack */
    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.Install(n);

    NetDeviceContainer d;
    Ptr<SimpleNetDevice> s1 = CreateObject<SimpleNetDevice>();
    s1->SetAddress(Mac48Address::Allocate());
    n.Get(0)->AddDevice(s1);

    Ptr<SimpleNetDevice> s2 = CreateObject<SimpleNetDevice>();
    s2->SetAddress(Mac48Address::Allocate());
    n.Get(1)->AddDevice(s2);
    d.Add(s1);
    d.Add(s2);

    ip1.SetBase(Ipv6Address("2001:00aa::"), Ipv6Prefix(56));
    Ipv6InterfaceContainer ic;
    ic = ip1.Assign(d);

    // Check that d got intended addresses
    Ipv6InterfaceAddress d1addr;
    Ipv6InterfaceAddress d2addr;
    // Interface 0 is loopback, interface 1 is s1, and the 0th addr is linklocal
    // so we look at address (1,1)
    d1addr = n.Get(0)->GetObject<Ipv6>()->GetAddress(1, 1);
    NS_TEST_ASSERT_MSG_EQ(d1addr.GetAddress(),
                          Ipv6Address("2001:00aa:0000:0000:0200:00ff:fe00:0001"),
                          "Ipv6AddressHelper failure");
    // Look also at the interface container (device 0, address 1)
    NS_TEST_ASSERT_MSG_EQ(ic.GetAddress(0, 1),
                          Ipv6Address("2001:00aa:0000:0000:0200:00ff:fe00:0001"),
                          "Ipv6AddressHelper failure");
    d2addr = n.Get(1)->GetObject<Ipv6>()->GetAddress(1, 1);
    // Look at second container
    NS_TEST_ASSERT_MSG_EQ(d2addr.GetAddress(),
                          Ipv6Address("2001:00aa:0000:0000:0200:00ff:fe00:0002"),
                          "Ipv6AddressHelper failure");
    // Look also at the interface container (device 0, address 1)
    NS_TEST_ASSERT_MSG_EQ(ic.GetAddress(1, 1),
                          Ipv6Address("2001:00aa:0000:0000:0200:00ff:fe00:0002"),
                          "Ipv6AddressHelper failure");
}

void
IpAddressHelperTestCasev6::DoTeardown()
{
    Ipv6AddressGenerator::Reset();
    Simulator::Destroy();
}

/**
 * \ingroup internet-test
 *
 * \brief IPv6 AddressHelper TestSuite
 */
class Ipv6AddressHelperTestSuite : public TestSuite
{
  public:
    Ipv6AddressHelperTestSuite();
};

Ipv6AddressHelperTestSuite::Ipv6AddressHelperTestSuite()
    : TestSuite("ipv6-address-helper", Type::UNIT)
{
    AddTestCase(new IpAddressHelperTestCasev6, TestCase::Duration::QUICK);
}

static Ipv6AddressHelperTestSuite
    ipv6AddressHelperTestSuite; //!< Static variable for test initialization
