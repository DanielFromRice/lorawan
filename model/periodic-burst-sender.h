/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Daniel Rothfusz
 */

#ifndef PERIODIC_BURST_SENDER_H
#define PERIODIC_BURST_SENDER_H

#include "ns3/application.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{
namespace lorawan
{

class EndDeviceLorawanMac;

/**
 * @ingroup lorawan
 *
 * Implements a sender application generating a burst of packets following a periodic point process.
 * NOTE: currently requires US parameters and assumes no retransmits.
 * Will need to be modified to adjust for either of these constraints.
 */
class PeriodicBurstSender : public Application
{
  public:
    PeriodicBurstSender();           //!< Default constructor
    ~PeriodicBurstSender() override; //!< Destructor

    /**
     *  Register this type.
     *  @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set the sending interval.
     *
     * @param interval The interval between two packet send instances.
     */
    void SetInterval(Time interval);

    /**
     * Get the sending interval.
     *
     * @return The interval between two packet sends.
     */
    Time GetInterval() const;

    /**
     * Set the initial delay of this application.
     *
     * @param delay The initial delay value.
     */
    void SetInitialDelay(Time delay);

    /**
     * Set packet size.
     *
     * @param size The base packet size value in bytes.
     */
    void SetPacketSize(uint16_t size);

    /**
     * Set the duration of each packet within the burst.
     *
     * @param duration The dwell time value.
    */
    void SetDwellTime(Time duration);

    /**
     * Set to add randomness to the base packet size.
     *
     * On each call to SendPacket(), an integer number is picked from a random variable. That
     * integer number is then added to the base packet size to create the new packet.
     *
     * @param rv The random variable used to extract the additional number of packet bytes.
     * Extracted values can be negative, but if they are lower than the base packet size they
     * produce a runtime error. This check is left to the caller during definition of the random
     * variable.
     */
    void SetPacketSizeRandomVariable(Ptr<RandomVariableStream> rv);

    /**
     * Send a packet using the LoraNetDevice's Send method.
     */
    void SendPacket();

    /**
     * Start the application by scheduling the first SendPacket event.
     */
    void StartApplication() override;

    /**
     * Stop the application.
     */
    void StopApplication() override;

  private:
    Time m_interval;       //!< The interval between to consecutive send events.
    Time m_initialDelay;   //!< The initial delay of this application.
    Time m_dwellTime;      //!< The maximum duration of a single send event (without retransmit).
    EventId m_sendEvent;   //!< The sending event scheduled as next.
    Ptr<EndDeviceLorawanMac> m_mac; //!< The MAC layer of this node.
    uint16_t m_basePktSize; //!< The combined total burst packet size.
    uint16_t m_remainingPktSize;   //!< The remaining packet size to be sent.
    uint8_t m_fragmentCount; //!< The number of fragments sent by the current burst
    Ptr<RandomVariableStream>
        m_pktSizeRV; //!< The random variable that adds bytes to the packet size.
};

} // namespace lorawan
} // namespace ns3

#endif /* SENDER_APPLICATION */
