/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Daniel Rothfusz (drothfusz@ucsd.edu)
 */

#include "ns3/buildings-module.h"
#include "ns3/core-module.h"
#include "ns3/lorawan-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/point-to-point-helper.h"

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("wesSim");

int seed = 3;

// Network settings
int nDevicesA = 25;                 //!< Number of end device nodes to create
int nDevicesB = 25;                 //!< Number of end device nodes to create
int nGateways = 1;                  //!< Number of gateway nodes to create
double widthMeters = 2000;         //!< Width (m) of the BDR
double simulationTimeSeconds = 3600; //!< Scenario duration (s) in simulated time - 1 hour default

// Channel model
bool realisticChannelModel = false; //!< Whether to use a more realistic channel model with
                                    //!< Buildings and correlated shadowing

int appPeriodSecondsA = 900; //!< Duration (s) of the inter-transmission time of end devices - 15 min default
int appPeriodSecondsB = 900; //!< Duration (s) of the inter-transmission time of end devices - 15 min default
int packetSizeA = 25; //!< Base packet size (bytes) of group A end devices
int packetSizeB = 500; //!< Base packet size (bytes) of group B end devices

int dataMode = -1;

// Output control
bool printBuildingInfo = false; //!< Whether to print building information

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("nDevicesA", "Number of end devices in Group A to include in the simulation", nDevicesA);
    cmd.AddValue("nDevicesB", "Number of end devices to Group B to include in the simulation", nDevicesB);
    cmd.AddValue("width", "The radius (m) of the area to simulate", widthMeters);
    cmd.AddValue("simulationTime", "The time (s) for which to simulate", simulationTimeSeconds);
    cmd.AddValue("appPeriodA",
                 "The period in seconds to be used by periodically transmitting applications on device group A",
                 appPeriodSecondsA);
    cmd.AddValue("appPeriodB",
                 "The period in seconds to be used by periodically transmitting applications on device group B",
                 appPeriodSecondsB);
    cmd.AddValue("packetSizeA", "Base size in bytes in device group A packets", packetSizeA);
    cmd.AddValue("packetSizeB", "Base size in bytes in device group B packets", packetSizeB);
    cmd.AddValue("seed", "Random generator seed", seed);
    cmd.AddValue("dataMode", "Data Mode to send uplink traffic, -1 for auto selection by range", dataMode);
    cmd.Parse(argc, argv);

    // Set up logging
    // LogComponentEnable("wesSim", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
    // LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("LorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable("EndDeviceLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable("ContinuousEndDeviceLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable("GatewayLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("LorawanMacHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("PeriodicSenderHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("PeriodicBurstSenderHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("PeriodicSender", LOG_LEVEL_ALL);
    // LogComponentEnable("PeriodicBurstSender", LOG_LEVEL_ALL);
    // LogComponentEnable("LorawanMacHeader", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);
    // LogComponentEnable("NetworkScheduler", LOG_LEVEL_ALL);
    // LogComponentEnable("NetworkServer", LOG_LEVEL_ALL);
    // LogComponentEnable("NetworkStatus", LOG_LEVEL_ALL);
    // LogComponentEnable("GatewayStatus", LOG_LEVEL_ALL);
    // LogComponentEnable("NetworkController", LOG_LEVEL_ALL);
    // LogComponentEnable("LoraPacketTracker", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Group A nodes: " << nDevicesA);
    NS_LOG_DEBUG("Group A packet size: " << packetSizeA);
    NS_LOG_DEBUG("Group A app period: " << appPeriodSecondsA);
    NS_LOG_DEBUG("Group B nodes: " << nDevicesB);
    NS_LOG_DEBUG("Group B packet size: " << packetSizeB);
    NS_LOG_DEBUG("Group B app period: " << appPeriodSecondsB);
    NS_LOG_DEBUG("Simulation time: " << simulationTimeSeconds);
    NS_LOG_DEBUG("Data Mode: " << dataMode);
    NS_LOG_DEBUG("Seed: " << seed);

    NS_ASSERT(dataMode >= -1 && dataMode <= 4);

    ns3::RngSeedManager::SetSeed(seed);

    // Create the time value from the period
    Time appPeriodA = Seconds(appPeriodSecondsA);
    Time appPeriodB = Seconds(appPeriodSecondsB);

    // Mobility
    std::string xRange = "ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(widthMeters) + "]";
    std::string yRange = "ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(widthMeters) + "]";

    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                               "X", StringValue (xRange),
                               "Y", StringValue (yRange));

    /************************
     *  Create the channel  *
     ************************/

    // Create the lora channel object
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(3.76);
    loss->SetReference(1, 7.7);

    if (realisticChannelModel)
    {
        // Create the correlated shadowing component
        Ptr<CorrelatedShadowingPropagationLossModel> shadowing =
            CreateObject<CorrelatedShadowingPropagationLossModel>();

        // Aggregate shadowing to the logdistance loss
        loss->SetNext(shadowing);

        // Add the effect to the channel propagation loss
        Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss>();

        shadowing->SetNext(buildingLoss);
    }

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();

    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    /************************
     *  Create the helpers  *
     ************************/

    // Create the LoraPhyHelper
    LoraPhyHelper phyHelper = LoraPhyHelper();
    phyHelper.SetChannel(channel);

    // Create the LorawanMacHelper
    LorawanMacHelper macHelper = LorawanMacHelper();
    macHelper.SetRegion(LorawanMacHelper::Regions::US);

    // Create the LoraHelper
    LoraHelper helper = LoraHelper();
    helper.EnablePacketTracking(); // Output filename
    // helper.EnableSimulationTimePrinting ();

    // Create the NetworkServerHelper
    NetworkServerHelper nsHelper = NetworkServerHelper();

    // Create the ForwarderHelper
    ForwarderHelper forHelper = ForwarderHelper();

    /************************
     *  Create End Devices  *
     ************************/

    // Create a set of nodes
    NodeContainer endDevicesA, endDevicesB, endDevices;
    endDevicesA.Create(nDevicesA);
    endDevicesB.Create(nDevicesB);

    endDevices.Add(endDevicesA);
    endDevices.Add(endDevicesB);

    // Assign a mobility model to each node
    mobility.Install(endDevices);

    // Make it so that nodes are at a certain height > 0
    for (auto j = endDevices.Begin(); j != endDevices.End(); ++j)
    {
        Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel>();
        Vector position = mobility->GetPosition();
        position.z = 1.2;
        mobility->SetPosition(position);
        // NS_LOG_INFO("Node " << (*j)->GetId() << " placed at " << position.x << "," << position.y);
    }

    // Create the LoraNetDevices of the end devices
    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator>(nwkId, nwkAddr);

    // Create the LoraNetDevices of the end devices
    macHelper.SetAddressGenerator(addrGen);
    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    helper.Install(phyHelper, macHelper, endDevicesA);
    macHelper.SetDeviceType(LorawanMacHelper::ED_CONT);
    helper.Install(phyHelper, macHelper, endDevicesB);

    // Now end devices are connected to the channel

    // Connect trace sources
    for (auto j = endDevices.Begin(); j != endDevices.End(); ++j)
    {
        Ptr<Node> node = *j;
        Ptr<LoraNetDevice> loraNetDevice = DynamicCast<LoraNetDevice>(node->GetDevice(0));
        Ptr<LoraPhy> phy = loraNetDevice->GetPhy();
    }

    /*********************
     *  Create Gateways  *
     *********************/

    // Create the gateway nodes (allocate them uniformly on the disc)
    NodeContainer gateways;
    gateways.Create(nGateways);

    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator>();
    // Make it so that nodes are at a certain height > 0
    // Places Gateway at the center of the region
    allocator->Add(Vector(widthMeters / 2, widthMeters / 2, 15.0));
    mobility.SetPositionAllocator(allocator);
    mobility.Install(gateways);

    // NS_LOG_INFO("Gateway placed at " << widthMeters / 2 << "," << widthMeters / 2);

    // Create a netdevice for each gateway
    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    helper.Install(phyHelper, macHelper, gateways);

    // TODO: channel model currently unused but could be added later
#if 0
    /**********************
     *  Handle buildings  *
     **********************/
    double xLength = 130;
    double deltaX = 32;
    double yLength = 64;
    double deltaY = 17;
    int gridWidth = widthMeters / (xLength + deltaX);
    int gridHeight = widthMeters / (yLength + deltaY);
    if (!realisticChannelModel)
    {
        gridWidth = 0;
        gridHeight = 0;
    }
    Ptr<GridBuildingAllocator> gridBuildingAllocator;
    gridBuildingAllocator = CreateObject<GridBuildingAllocator>();
    gridBuildingAllocator->SetAttribute("GridWidth", UintegerValue(gridWidth));
    gridBuildingAllocator->SetAttribute("LengthX", DoubleValue(xLength));
    gridBuildingAllocator->SetAttribute("LengthY", DoubleValue(yLength));
    gridBuildingAllocator->SetAttribute("DeltaX", DoubleValue(deltaX));
    gridBuildingAllocator->SetAttribute("DeltaY", DoubleValue(deltaY));
    gridBuildingAllocator->SetAttribute("Height", DoubleValue(6));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsX", UintegerValue(2));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsY", UintegerValue(4));
    gridBuildingAllocator->SetBuildingAttribute("NFloors", UintegerValue(2));
    gridBuildingAllocator->SetAttribute(
        "MinX",
        DoubleValue(-gridWidth * (xLength + deltaX) / 2 + deltaX / 2));
    gridBuildingAllocator->SetAttribute(
        "MinY",
        DoubleValue(-gridHeight * (yLength + deltaY) / 2 + deltaY / 2));
    BuildingContainer bContainer = gridBuildingAllocator->Create(gridWidth * gridHeight);

    BuildingsHelper::Install(endDevices);
    BuildingsHelper::Install(gateways);

    // Print the buildings
    if (printBuildingInfo)
    {
        std::ofstream myfile;
        myfile.open("buildings.txt");
        std::vector<Ptr<Building>>::const_iterator it;
        int j = 1;
        for (it = bContainer.Begin(); it != bContainer.End(); ++it, ++j)
        {
            Box boundaries = (*it)->GetBoundaries();
            myfile << "set object " << j << " rect from " << boundaries.xMin << ","
                   << boundaries.yMin << " to " << boundaries.xMax << "," << boundaries.yMax
                   << std::endl;
        }
        myfile.close();
    }

#endif
    /**********************************************
     *  Set up the end device's spreading factor  *
     **********************************************/

    // NOTE: can set a specific device's spreading factor with:
    // DynamicCast<EndDeviceLorawanMac>(node->GetMac())->SetDataRate(value);
    auto spreadFactorDistribution = LorawanMacHelper::SetSpreadingFactorsUp(endDevices, gateways, channel, LorawanMacHelper::US);
    if (dataMode != -1)
    {
        for (auto node=endDevices.Begin (); node != endDevices.End (); ++node)
        {
            Ptr<Node> object = *node;
            Ptr<MobilityModel> position = object->GetObject<MobilityModel>();
            NS_ASSERT(position);
            Ptr<NetDevice> netDevice = object->GetDevice(0);
            Ptr<LoraNetDevice> loraNetDevice = DynamicCast<LoraNetDevice>(netDevice);
            NS_ASSERT(loraNetDevice);
            Ptr<EndDeviceLorawanMac> mac =
                DynamicCast<EndDeviceLorawanMac>(loraNetDevice->GetMac());
            NS_ASSERT(mac);
            mac->SetDataRate(dataMode);
        }
    }

    // NS_LOG_DEBUG("Completed configuration");

    /*********************************************
     *  Install applications on the end devices  *
     *********************************************/

    Time appStopTime = Seconds(simulationTimeSeconds);
    PeriodicSenderHelper appHelperA = PeriodicSenderHelper();
    appHelperA.SetPeriod(appPeriodA);
    appHelperA.SetPacketSize(packetSizeA);
    Ptr<RandomVariableStream> rv_a =
        CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                          DoubleValue(0),
                                                          "Max",
                                                          DoubleValue(10));
    // appHelperA.SetPacketSizeRandomVariable(rv_a);
    ApplicationContainer appContainerA = appHelperA.Install(endDevicesA);

    // Set up Group B to send traffic in bursts
    PeriodicBurstSenderHelper appHelperB = PeriodicBurstSenderHelper();
    appHelperB.SetPeriod(appPeriodB);
    appHelperB.SetPacketSize(packetSizeB);
    appHelperB.SetDwellTime(MilliSeconds(400)); // TODO: this is restricted to 2s min by using a class A device

    ApplicationContainer appContainerB = appHelperB.Install(endDevicesB);


    appContainerA.Start(Time(0));
    appContainerA.Stop(appStopTime);

    appContainerB.Start(Time(0));
    appContainerB.Stop(appStopTime);


    /**************************
     *  Create network server  *
     ***************************/

    // Create the network server node
    Ptr<Node> networkServer = CreateObject<Node>();

    // PointToPoint links between gateways and server
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    // Store network server app registration details for later
    P2PGwRegistration_t gwRegistration;
    for (auto gw = gateways.Begin(); gw != gateways.End(); ++gw)
    {
        auto container = p2p.Install(networkServer, *gw);
        auto serverP2PNetDev = DynamicCast<PointToPointNetDevice>(container.Get(0));
        gwRegistration.emplace_back(serverP2PNetDev, *gw);
    }

    // Create a network server for the network
    nsHelper.SetGatewaysP2P(gwRegistration);
    nsHelper.SetEndDevices(endDevices);
    nsHelper.Install(networkServer);

    // Create a forwarder for each gateway
    forHelper.Install(gateways);

    ////////////////
    // Simulation //
    ////////////////

    Simulator::Stop(appStopTime + Hours(1));

    // NS_LOG_INFO("Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    ///////////////////////////
    // Print results to file //
    ///////////////////////////
    // NS_LOG_INFO("Computing performance metrics...");

    LoraPacketTracker& tracker = helper.GetPacketTracker();
    // std::cout << "Packets Sent, Received: " << tracker.CountMacPacketsGlobally(Time(0), appStopTime + Hours(1)) << std::endl;
    auto tracker_map = tracker.CountMacPacketsByEndDevice (Time(0), appStopTime + Hours(1));
    uint32_t groupA_sent = 0;
    uint32_t groupA_recv = 0;
    uint32_t groupB_sent = 0;
    uint32_t groupB_recv = 0;
    for (auto j = endDevicesA.Begin(); j != endDevicesA.End(); ++j)
    {
        Ptr<Node> node = *j;
        if(tracker_map.contains(node->GetId()))
        {
            groupA_sent += tracker_map.at(node->GetId()).first;
            groupA_recv += tracker_map.at(node->GetId()).second;
        }
    }
    for (auto j = endDevicesB.Begin(); j != endDevicesB.End(); ++j)
    {
        Ptr<Node> node = *j;
        if(tracker_map.contains(node->GetId()))
        {
            groupB_sent += tracker_map.at(node->GetId()).first;
            groupB_recv += tracker_map.at(node->GetId()).second;
        }
    }
    // std::cout << "Results - " << nDevicesA << ", " << seed << ", " << groupA_sent << ", " << groupA_recv << ", " << 100 * (groupA_sent - groupA_recv) / (double)groupA_sent << "%\n";
    std::cout << appPeriodSecondsA << ", " << nDevicesA << ", " << groupA_sent << ", " << groupA_recv << ", " << 100*(groupA_sent - groupA_recv) / (double)groupA_sent;
    std::cout << ", " << appPeriodSecondsB << ", " << nDevicesB << ", " << groupB_sent << ", " << groupB_recv  << ", " << 100*(groupB_sent - groupB_recv) / (double)groupB_sent;
    std::cout << ", " << dataMode << ", " << seed << ", " << packetSizeB << std::endl;
    // Individual Node logging:
    // std::cout << "Packet details:" << std::endl;
    // for (const auto& entry: tracker_map)
    // {
    //     std::cout << "\tID: " << entry.first << ", sent: " << entry.second.first << ", received at gw: " << entry.second.second << std::endl;
    // }

    return 0;
}

