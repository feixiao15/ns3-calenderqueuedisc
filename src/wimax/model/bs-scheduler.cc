/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#include "bs-scheduler.h"

#include "bs-net-device.h"
#include "burst-profile-manager.h"
#include "cid.h"
#include "connection-manager.h"
#include "service-flow-manager.h"
#include "service-flow-record.h"
#include "service-flow.h"
#include "ss-manager.h"
#include "ss-record.h"
#include "wimax-connection.h"
#include "wimax-mac-header.h"
#include "wimax-mac-queue.h"

#include "ns3/log.h"
#include "ns3/packet-burst.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BSScheduler");

NS_OBJECT_ENSURE_REGISTERED(BSScheduler);

TypeId
BSScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BSScheduler").SetParent<Object>().SetGroupName("Wimax")
        // No AddConstructor because this is an abstract class.
        ;
    return tid;
}

BSScheduler::BSScheduler()
    : m_downlinkBursts(new std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst>>>())
{
    // m_downlinkBursts is filled by AddDownlinkBurst and emptied by
    // wimax-bs-net-device::sendBurst and wimax-ss-net-device::sendBurst
}

BSScheduler::BSScheduler(Ptr<BaseStationNetDevice> bs)
    : m_downlinkBursts(new std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst>>>())
{
}

BSScheduler::~BSScheduler()
{
    std::list<std::pair<OfdmDlMapIe*, Ptr<PacketBurst>>>* downlinkBursts = m_downlinkBursts;
    std::pair<OfdmDlMapIe*, Ptr<PacketBurst>> pair;
    while (!downlinkBursts->empty())
    {
        pair = downlinkBursts->front();
        pair.second = nullptr;
        delete pair.first;
    }
    SetBs(nullptr);
    delete m_downlinkBursts;
    m_downlinkBursts = nullptr;
}

void
BSScheduler::SetBs(Ptr<BaseStationNetDevice> bs)
{
    m_bs = bs;
}

Ptr<BaseStationNetDevice>
BSScheduler::GetBs()
{
    return m_bs;
}

bool
BSScheduler::CheckForFragmentation(Ptr<WimaxConnection> connection,
                                   int availableSymbols,
                                   WimaxPhy::ModulationType modulationType)
{
    NS_LOG_INFO("BS Scheduler, CheckForFragmentation");
    if (connection->GetType() != Cid::TRANSPORT)
    {
        NS_LOG_INFO("\t No Transport connection, Fragmentation IS NOT possible");
        return false;
    }
    uint32_t availableByte = GetBs()->GetPhy()->GetNrBytes(availableSymbols, modulationType);

    uint32_t headerSize =
        connection->GetQueue()->GetFirstPacketHdrSize(MacHeaderType::HEADER_TYPE_GENERIC);
    NS_LOG_INFO("\t availableByte = " << availableByte << " headerSize = " << headerSize);

    if (availableByte > headerSize)
    {
        NS_LOG_INFO("\t Fragmentation IS possible");
        return true;
    }
    else
    {
        NS_LOG_INFO("\t Fragmentation IS NOT possible");
        return false;
    }
}
} // namespace ns3
