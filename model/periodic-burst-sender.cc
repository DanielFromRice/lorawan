/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Daniel Rothfusz
 */

#include "periodic-burst-sender.h"

#include "lora-net-device.h"
#include "end-device-lorawan-mac.h"

#include "ns3/simulator.h"

namespace ns3
{
namespace lorawan
{

NS_LOG_COMPONENT_DEFINE("PeriodicBurstSender");

NS_OBJECT_ENSURE_REGISTERED(PeriodicBurstSender);

TypeId
PeriodicBurstSender::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PeriodicBurstSender")
                            .SetParent<Application>()
                            .AddConstructor<PeriodicBurstSender>()
                            .SetGroupName("lorawan")
                            .AddAttribute("Interval",
                                          "The interval between packet sends of this app",
                                          TimeValue(Time(0)),
                                          MakeTimeAccessor(&PeriodicBurstSender::GetInterval,
                                                           &PeriodicBurstSender::SetInterval),
                                          MakeTimeChecker());
    // .AddAttribute ("PacketSizeRandomVariable", "The random variable that determines the shape of
    // the packet size, in bytes",
    //                StringValue ("ns3::UniformRandomVariable[Min=0,Max=10]"),
    //                MakePointerAccessor (&PeriodicSender::m_pktSizeRV),
    //                MakePointerChecker <RandomVariableStream>());
    return tid;
}

PeriodicBurstSender::PeriodicBurstSender()
    : m_interval(Minutes(1)),
      m_initialDelay(Seconds(1)),
      m_dwellTime(MilliSeconds(400)),
      m_basePktSize(100),
      m_remainingPktSize(0),
      m_fragmentCount(0),
      m_pktSizeRV(nullptr)

{
    NS_LOG_FUNCTION_NOARGS();
}

PeriodicBurstSender::~PeriodicBurstSender()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
PeriodicBurstSender::SetInterval(Time interval)
{
    NS_LOG_FUNCTION(this << interval);
    m_interval = interval;
}

Time
PeriodicBurstSender::GetInterval() const
{
    NS_LOG_FUNCTION(this);
    return m_interval;
}

void
PeriodicBurstSender::SetInitialDelay(Time delay)
{
    NS_LOG_FUNCTION(this << delay);
    m_initialDelay = delay;
}

void
PeriodicBurstSender::SetPacketSizeRandomVariable(Ptr<RandomVariableStream> rv)
{
    m_pktSizeRV = rv;
}

void
PeriodicBurstSender::SetPacketSize(uint16_t size)
{
    m_basePktSize = size;
}

void
PeriodicBurstSender::SetDwellTime(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    m_dwellTime = duration;
}

void
PeriodicBurstSender::SendPacket()
{
    NS_LOG_FUNCTION(this);

    if (m_remainingPktSize == 0)
    {
        m_remainingPktSize = m_basePktSize;
        NS_LOG_DEBUG("Set new packet to size " << m_basePktSize);
        m_fragmentCount = 0;
    }
    uint8_t datarate = m_mac->GetDataRate();
    uint8_t payloadSize = std::min(static_cast<uint16_t>(m_mac->GetMaxAppPayloadSize(datarate)),m_remainingPktSize);
    // Create and send a new packet
    // TODO: add feedback from MAC to determine if ready for a new packet
    Ptr<Packet> packet;
    packet = Create<Packet>(payloadSize);
    m_mac->Send(packet);
    m_fragmentCount++;

    NS_LOG_DEBUG("Sent a packet of size " << packet->GetSize());

    m_remainingPktSize -= payloadSize;
    if (m_remainingPktSize > 0)
    {
        // Schedule the next SendPacket event
        m_sendEvent = Simulator::Schedule(m_dwellTime, &PeriodicBurstSender::SendPacket, this);

        NS_LOG_DEBUG("Scheduled next fragment for " << m_dwellTime << " later");
    }
    else
    {
        // TODO: Reduce interval by total packet duration so bursts start at
        // the interval, rather than separated by the interval. Requires
        // deciding what to do when a burst takes longer than the interval.
        // Could either throw away the rest of the burst or skip the next
        // interval start.

        // Time total_duration = m_fragmentCount * m_dwellTime;
        // NS_ASSERT(total_duration < m_interval);
        NS_LOG_DEBUG("Event complete, " << (int)m_fragmentCount << " fragments sent");
        NS_LOG_DEBUG("Scheduled next event for " << (m_interval) << " later");

        m_sendEvent = Simulator::Schedule(m_interval, &PeriodicBurstSender::SendPacket, this);
    }
}

void
PeriodicBurstSender::StartApplication()
{
    NS_LOG_FUNCTION(this);

    // Make sure we have a MAC layer
    if (!m_mac)
    {
        // Assumes there's only one device
        Ptr<LoraNetDevice> loraNetDevice = DynamicCast<LoraNetDevice>(m_node->GetDevice(0));

        m_mac = DynamicCast<EndDeviceLorawanMac>(loraNetDevice->GetMac());
        NS_ASSERT(m_mac);
    }

    // Schedule the next SendPacket event
    Simulator::Cancel(m_sendEvent);
    NS_LOG_DEBUG("Starting up application with a first event with a " << m_initialDelay.As(Time::S)
                                                                      << " delay");
    m_sendEvent = Simulator::Schedule(m_initialDelay, &PeriodicBurstSender::SendPacket, this);
    NS_LOG_DEBUG("Event Id: " << m_sendEvent.GetUid());
}

void
PeriodicBurstSender::StopApplication()
{
    NS_LOG_FUNCTION_NOARGS();
    Simulator::Cancel(m_sendEvent);
}

} // namespace lorawan
} // namespace ns3
