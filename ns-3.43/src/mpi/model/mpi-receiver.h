/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: George Riley <riley@ece.gatech.edu>
 */

/**
 * \file
 * \ingroup mpi
 * ns3::MpiReceiver declaration,
 * provides an interface to aggregate to MPI-compatible NetDevices.
 */

#ifndef NS3_MPI_RECEIVER_H
#define NS3_MPI_RECEIVER_H

#include "ns3/object.h"
#include "ns3/packet.h"

namespace ns3
{

/**
 * \ingroup mpi
 *
 * \brief Class to aggregate to a NetDevice if it supports MPI capability
 *
 * MpiInterface::ReceiveMessages () needs to send packets to a NetDevice
 * Receive() method.  Since each NetDevice's Receive() method is specific
 * to the derived class, and since we do not know whether such a NetDevice
 * is MPI-capable, we aggregate one of these objects to each MPI-capable
 * device.  In addition, we must hook up a NetDevice::Receive() method
 * to the callback.  So the two steps to enable MPI capability are to
 * aggregate this object to a NetDevice, and to set the callback.
 */
class MpiReceiver : public Object
{
  public:
    /**
     * Register this type.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();
    ~MpiReceiver() override;

    /**
     * \brief Direct an incoming packet to the device Receive() method
     * \param p Packet to receive
     */
    void Receive(Ptr<Packet> p);
    /**
     * \brief Set the receive callback to get packets to net devices
     * \param callback the callback itself
     */
    void SetReceiveCallback(Callback<void, Ptr<Packet>> callback);

  private:
    void DoDispose() override;

    /** Callback to send received packets to. */
    Callback<void, Ptr<Packet>> m_rxCallback;
};

} // namespace ns3

#endif /* NS3_MPI_RECEIVER_H */
