/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Daniel Rothfusz
 */

#ifndef CONTINUOUS_END_DEVICE_LORAWAN_MAC_H
#define CONTINUOUS_END_DEVICE_LORAWAN_MAC_H

#include "end-device-lorawan-mac.h"

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * Class representing the MAC layer of a custom end device that can continuously
 * send uplink traffic across different channels
 */
class ContinuousEndDeviceLorawanMac : public EndDeviceLorawanMac
{
  public:
    /**
     *  Register this type.
     *  @return The object TypeId.
     */
    static TypeId GetTypeId();

    ContinuousEndDeviceLorawanMac();           //!< Default constructor
    ~ContinuousEndDeviceLorawanMac() override; //!< Destructor

    /////////////////////
    // Sending methods //
    /////////////////////

    /**
     * Add headers and send a packet with the sending function of the physical layer.
     * Also switches the phy out of receive mode in preparation for the packet.
     *
     * @param packet The packet to send.
     */
    void SendToPhy(Ptr<Packet> packet) override;

    //////////////////////////
    //  Receiving methods   //
    //////////////////////////

    /**
     * Receive a packet.
     *
     * This method is typically registered as a callback in the underlying PHY
     * layer so that it's called when a packet is going up the stack.
     *
     * @param packet The received packet.
     */
    void Receive(Ptr<const Packet> packet) override;

    /**
     * Function called by lower layers to inform this layer that reception of a
     * packet we were locked on failed.
     *
     * @param packet The packet we failed to receive.
     */
    void FailedReception(Ptr<const Packet> packet) override;

    /**
     * Perform the actions that are required after a packet send.
     *
     * This function handles the transition to receive mode.
     *
     * @param packet The packet that has just been transmitted.
     */
    void TxFinished(Ptr<const Packet> packet) override;

    /////////////////////////
    // Getters and Setters //
    /////////////////////////

    /**
     * Find the minimum wait time before the next possible transmission based
     * on end device's Class Type.
     *
     * @param waitTime The minimum wait time that has to be respected,
     * irrespective of the class (e.g., because of duty cycle limitations).
     * @return The Time value.
     */
    Time GetNextClassTransmissionDelay(Time waitTime) override;

    /**
     * Get the data rate that will be used in the receive.
     *
     * @param window The window number to get (1)
     * @return The data rate.
     */
    uint8_t GetReceiveWindowDataRate(uint8_t window) override;

    /////////////////////////
    // MAC command methods //
    /////////////////////////

    void OnRxParamSetupReq(uint8_t rx1DrOffset, uint8_t rx2DataRate, double frequencyHz) override;

  private:
    /**
     * The RX1DROffset parameter value.
     */
    uint8_t m_rx1DrOffset;

}; /* ContinuousEndDeviceLorawanMac */
} /* namespace lorawan */
} /* namespace ns3 */
#endif /* CONTINUOUS_END_DEVICE_LORAWAN_MAC_H */
