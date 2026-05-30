/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Daniel Rothfusz
 */

#include "continuous-end-device-lorawan-mac.h"

#include "end-device-lora-phy.h"
#include "lora-tag.h"

#include "ns3/simulator.h"

namespace ns3
{
namespace lorawan
{

NS_LOG_COMPONENT_DEFINE("ContinuousEndDeviceLorawanMac");

NS_OBJECT_ENSURE_REGISTERED(ContinuousEndDeviceLorawanMac);

TypeId
ContinuousEndDeviceLorawanMac::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ContinuousEndDeviceLorawanMac")
                            .SetParent<EndDeviceLorawanMac>()
                            .SetGroupName("lorawan")
                            .AddConstructor<ContinuousEndDeviceLorawanMac>();
    return tid;
}

ContinuousEndDeviceLorawanMac::ContinuousEndDeviceLorawanMac()
    : m_rx1DrOffset(0)
{
    NS_LOG_FUNCTION(this);
}

ContinuousEndDeviceLorawanMac::~ContinuousEndDeviceLorawanMac()
{
    NS_LOG_FUNCTION_NOARGS();
}

/////////////////////
// Sending methods //
/////////////////////

void
ContinuousEndDeviceLorawanMac::SendToPhy(Ptr<Packet> packetToSend)
{
    // Switch to sleep if receiving
    Ptr<EndDeviceLoraPhy> phy = DynamicCast<EndDeviceLoraPhy>(m_phy);
    if (phy->GetState() == EndDeviceLoraPhy::State::RX)
    {
        DynamicCast<EndDeviceLoraPhy>(m_phy)->SwitchToStandby();
    }

    /////////////////////////////////////////////////////////
    // Add headers, prepare TX parameters and send the packet
    /////////////////////////////////////////////////////////

    NS_LOG_DEBUG("PacketToSend: " << packetToSend);

    // Craft LoraTxParameters object
    LoraTxParameters params;
    params.sf = GetSfFromDataRate(m_dataRate);
    params.headerDisabled = m_headerDisabled;
    params.codingRate = m_codingRate;
    params.bandwidthHz = GetBandwidthFromDataRate(m_dataRate);
    params.nPreamble = m_nPreambleSymbols;
    params.crcEnabled = true;
    params.lowDataRateOptimizationEnabled = LoraPhy::GetTSym(params) > MilliSeconds(16);

    // Wake up PHY layer and directly send the packet

    Ptr<LogicalLoraChannel> txChannel = GetRandomChannelForTx();

    NS_LOG_DEBUG("PacketToSend: " << packetToSend);
    m_phy->Send(packetToSend, params, txChannel->GetFrequency(), m_txPowerDbm);

    //////////////////////////////////////////////
    // Register packet transmission for duty cycle
    //////////////////////////////////////////////

    // Compute packet duration
    Time duration = LoraPhy::GetOnAirTime(packetToSend, params);

    // Register the sent packet into the DutyCycleHelper
    m_channelHelper->AddEvent(duration, txChannel);

    //////////////////////////////
    // Prepare for the downlink //
    //////////////////////////////

    // Switch the PHY to the channel so that it will listen here for downlink
    DynamicCast<EndDeviceLoraPhy>(m_phy)->SetFrequency(txChannel->GetFrequency());

    // Instruct the PHY on the right Spreading Factor to listen for during the window
    // create a SetReplyDataRate function?
    uint8_t replyDataRate = GetReceiveWindowDataRate(1);
    NS_LOG_DEBUG("m_dataRate: " << unsigned(m_dataRate)
                                << ", m_rx1DrOffset: " << unsigned(m_rx1DrOffset)
                                << ", replyDataRate: " << unsigned(replyDataRate) << ".");

    DynamicCast<EndDeviceLoraPhy>(m_phy)->SetSpreadingFactor(GetSfFromDataRate(replyDataRate));
}

//////////////////////////
//  Receiving methods   //
//////////////////////////
void
ContinuousEndDeviceLorawanMac::Receive(Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    // Work on a copy of the packet
    Ptr<Packet> packetCopy = packet->Copy();

    // Remove the Mac Header to get some information
    LorawanMacHeader mHdr;
    packetCopy->RemoveHeader(mHdr);

    NS_LOG_DEBUG("Mac Header: " << mHdr);

    // Only keep analyzing the packet if it's downlink
    if (!mHdr.IsUplink())
    {
        NS_LOG_INFO("Found a downlink packet.");

        // Remove the Frame Header
        LoraFrameHeader fHdr;
        fHdr.SetAsDownlink();
        packetCopy->RemoveHeader(fHdr);

        NS_LOG_DEBUG("Frame Header: " << fHdr);

        // Determine whether this packet is for us
        bool messageForUs = (m_address == fHdr.GetAddress());

        if (messageForUs)
        {
            NS_LOG_INFO("The message is for us!");

            // Reset ADR backoff counter
            m_adrAckCnt = 0;

            LoraTag tag;
            packet->PeekPacketTag(tag);
            /// @see ns3::lorawan::AdrComponent::RxPowerToSNR
            m_lastRxSnr = tag.GetReceivePower() + 174 - 10 * log10(125000) - 6;

            // Parse the MAC commands
            ParseCommands(fHdr);

            // TODO Pass the packet up to the NetDevice

            // Call the trace source
            m_receivedPacket(packet);
        }
        else
        {
            NS_LOG_DEBUG("The message is intended for another recipient.");

            // If we no longer have any retransmissions left, we declare failure.
            if (m_retxParams.waitingAck)
            {
                /// TODO: UNCONFIRMED packets CAN be retransmitted, but behave slightly differently.
                /// The current implementation only considers re-txs for CONFIRMED, change this
                if (m_retxParams.retxLeft == 0)
                {
                    uint8_t txs = m_nbTrans - (m_retxParams.retxLeft);
                    m_requiredTxCallback(txs,
                                         false,
                                         m_retxParams.firstAttempt,
                                         m_retxParams.packet);
                    NS_LOG_DEBUG("Failure: no more retransmissions left. Used "
                                 << unsigned(txs) << " transmissions.");

                    // Reset retransmission parameters
                    ResetRetransmissionParameters();
                }
                else // Reschedule
                {
                    this->Send(m_retxParams.packet);
                    NS_LOG_INFO("We have " << unsigned(m_retxParams.retxLeft)
                                           << " retransmissions left: rescheduling transmission.");
                }
            }
        }
    }
    else if (m_retxParams.waitingAck)
    {
        NS_LOG_INFO("The packet we are receiving is in uplink.");
        if (m_retxParams.retxLeft > 0)
        {
            this->Send(m_retxParams.packet);
            NS_LOG_INFO("We have " << unsigned(m_retxParams.retxLeft)
                                   << " retransmissions left: rescheduling transmission.");
        }
        else
        {
            uint8_t txs = m_nbTrans - (m_retxParams.retxLeft);
            m_requiredTxCallback(txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
            NS_LOG_DEBUG("Failure: no more retransmissions left. Used " << unsigned(txs)
                                                                        << " transmissions.");

            // Reset retransmission parameters
            ResetRetransmissionParameters();
        }
    }

    DynamicCast<EndDeviceLoraPhy>(m_phy)->SwitchToSleep();
}

void
ContinuousEndDeviceLorawanMac::FailedReception(Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    if (m_retxParams.waitingAck)
    {
        if (m_retxParams.retxLeft > 0)
        {
            this->Send(m_retxParams.packet);
            NS_LOG_INFO("We have " << unsigned(m_retxParams.retxLeft)
                                   << " retransmissions left: rescheduling transmission.");
        }
        else
        {
            uint8_t txs = m_nbTrans - (m_retxParams.retxLeft);
            m_requiredTxCallback(txs, false, m_retxParams.firstAttempt, m_retxParams.packet);
            NS_LOG_DEBUG("Failure: no more retransmissions left. Used " << unsigned(txs)
                                                                        << " transmissions.");

            // Reset retransmission parameters
            ResetRetransmissionParameters();
        }
    }
}

void
ContinuousEndDeviceLorawanMac::TxFinished(Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION_NOARGS();

    // Set Phy in Standby mode
    DynamicCast<EndDeviceLoraPhy>(m_phy)->SwitchToStandby();
}

/////////////////////////
// Getters and Setters //
/////////////////////////

Time
ContinuousEndDeviceLorawanMac::GetNextClassTransmissionDelay(Time waitTime)
{
    NS_LOG_FUNCTION_NOARGS();

    return waitTime;
}

uint8_t
ContinuousEndDeviceLorawanMac::GetReceiveWindowDataRate(uint8_t window)
{
    // TODO: properly set up receive windows
    if (window == 1 || window == 2)
    {
        return m_replyDataRateMatrix.at(m_dataRate).at(m_rx1DrOffset);
    }
    else
    {
        NS_ABORT_MSG("Invalid window number");
    }
}

/////////////////////////
// MAC command methods //
/////////////////////////

void
ContinuousEndDeviceLorawanMac::OnRxParamSetupReq(uint8_t rx1DrOffset, uint8_t rx2DrOffset, double frequencyHz)
{
    NS_LOG_FUNCTION(this << unsigned(rx1DrOffset));

    // Adapted from: github.com/Lora-net/SWL2001.git v4.3.1
    // For the time being, this implementation is valid for the EU868 region

    bool rx1DrOffsetAck = true;

    if (rx1DrOffset >= m_replyDataRateMatrix.at(m_dataRate).size())
    {
        NS_LOG_WARN("Invalid rx1DrOffset");
        rx1DrOffsetAck = false;
    }

    if (rx1DrOffsetAck)
    {
        m_rx1DrOffset = rx1DrOffset;
    }

    NS_LOG_INFO("Adding RxParamSetupAns reply");
    m_macCommandList.emplace_back(
        Create<RxParamSetupAns>(rx1DrOffsetAck, true, true));
}

} /* namespace lorawan */
} /* namespace ns3 */
