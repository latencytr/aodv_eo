/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This is an example script for AODV manet routing protocol.
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include "ns3/netanim-module.h"
//#include "ns3/aodvee-module.h"
#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/v4ping-helper.h"

#include "ns3/basic-energy-source.h"
#include "ns3/wifi-radio-energy-model.h"
#include "ns3/energy-module.h"

#include <iostream>
#include <cmath>


using namespace ns3;

/**
 * \brief Test script.
 *
 * This script creates 1-dimensional grid topology and then ping last node from the first one:
 *
 * [10.0.0.1] <-- step --> [10.0.0.2] <-- step --> [10.0.0.3] <-- step --> [10.0.0.4]
 *
 * ping 10.0.0.4
 */
class AodveeExample
{
public:
  AodveeExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /// Report results
  void Report (std::ostream & os);

private:

  // parameters
  /// Number of nodes
  uint32_t size;
  /// Distance between nodes, meters
  double step;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;

  // network
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
  void InstallEnergyModule ();
};

/// Trace function for remaining energy at node.
void
RemainingEnergy (std::string context, double oldValue, double remainingEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "s Node " << context
                 << " Current remaining energy = " << remainingEnergy << "J");
}

/// Trace function for total energy consumption at node.
void
TotalEnergy (std::string context, double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "s Node " << context
                 << " Total energy consumed by radio = " << totalEnergy << "J");
}

int main (int argc, char **argv)
{
  AodveeExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
AodveeExample::AodveeExample () :
  size (100),
  step (100),
  totalTime (240),
  pcap (true),
  printRoutes (true)
{
}

bool
AodveeExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("step", "Grid step, m", step);

  cmd.Parse (argc, argv);
  return true;
}

void
AodveeExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();
  InstallEnergyModule();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  //AnimationInterface anim ("AodveeExample.xml");


  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
AodveeExample::Report (std::ostream &)
{
}

void
AodveeExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes " << step << " m apart.\n";
  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }

  // Create static grid
  MobilityHelper mobility;
//  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
//                                 "MinX", DoubleValue (0.0),
//                                 "MinY", DoubleValue (0.0),
//                                 "DeltaX", DoubleValue (step),
//                                 "DeltaY", DoubleValue (0),
//                                 "GridWidth", UintegerValue (size),
//                                 "LayoutType", StringValue ("RowFirst"));
//  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

   int64_t nodeSpeed = 20;
   int64_t nodePause = 0;
   int64_t streamIndex = 0; // used to get consistent mobility across scenarios

   ObjectFactory pos;
   pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
   pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
   pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

   Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
   streamIndex += taPositionAlloc->AssignStreams (streamIndex);

   std::stringstream ssSpeed;
   ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
   std::stringstream ssPause;
   ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
   mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                   "Speed", StringValue (ssSpeed.str ()),
                                   "Pause", StringValue (ssPause.str ()),
                                   "PositionAllocator", PointerValue (taPositionAlloc));
   mobility.SetPositionAllocator (taPositionAlloc);

   mobility.Install (nodes);
}

void
AodveeExample::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes);

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodvee"));
    }
}

void
AodveeExample::InstallInternetStack ()
{
  //AodveeHelper aodvee;
  AodvHelper aodvee;
  // you can configure AODVEE attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodvee); // has effect on the next Install ()
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodvee.routes", std::ios::out);
      //aodvee.PrintRoutingTableAllAt (Seconds (8), routingStream);
      aodvee.PrintRoutingTableAllEvery(Seconds (5), routingStream);
    }
}

void
AodveeExample::InstallApplications ()
{
  //V4PingHelper ping (interfaces.GetAddress (size - 1));
  V4PingHelper ping (interfaces.GetAddress (91));
  ping.SetAttribute ("Verbose", BooleanValue (true));

  ApplicationContainer p = ping.Install (nodes.Get (0));
  p.Start (Seconds (0));
  p.Stop (Seconds (totalTime) - Seconds (0.001));

  // move node away
  //Ptr<Node> node = nodes.Get (size/2);
  //Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
  //Simulator::Schedule (Seconds (totalTime/3), &MobilityModel::SetPosition, mob, Vector (1e5, 1e5, 1e5));
}

void
AodveeExample::InstallEnergyModule()
{
	 RngSeedManager::SetSeed(5);
	 RngSeedManager::SetRun(100);
	 Ptr<UniformRandomVariable> x=CreateObject<UniformRandomVariable>();
	 BasicEnergySourceHelper basicSourceHelper;
	 basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue(300.0f));
	 EnergySourceContainer sources = basicSourceHelper.Install (nodes);
	 WifiRadioEnergyModelHelper radioEnergyHelper;
	 radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
	 DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (devices, sources);
	 //iterate on the node container an asign Energy model and Device to each Node
	 int index = 0;
	 for(NodeContainer::Iterator j= nodes.Begin(); j!= nodes.End(); ++j)
	 {
		 Ptr<BasicEnergySource> energySource = CreateObject<BasicEnergySource>();
		 Ptr<WifiRadioEnergyModel> energyModel = CreateObject<WifiRadioEnergyModel>();

		 energySource->SetInitialEnergy ((double)x->GetInteger(50,300));
		 energyModel->SetEnergySource (energySource);
		 energySource->AppendDeviceEnergyModel (energyModel);
		 energyModel->SetTxCurrentA(0.0174);
		 energyModel->SetRxCurrentA(0.0174);

		 // aggregate energy source to node
		 Ptr<Node> object = *j;
		 object->AggregateObject (energySource);

		 std::string context = static_cast<std::ostringstream*>( &(std::ostringstream() << index) )->str();
		 std::cout << "Connecting node " << context << std::endl;
		 Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (sources.Get (index));
		 //basicSourcePtr->TraceConnect ("RemainingEnergy", context, MakeCallback (&RemainingEnergy));
		 //basicSourcePtr->TraceConnect ("TotalEnergyConsumption", context, MakeCallback (&TotalEnergy));
		 index++;
	 }

}

