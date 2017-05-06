/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Five base nodes, one mover node
 *
 *      m ->
 *     /
 *    /
 *   /
 *  b1         b2         b3         b4         b5
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/basic-energy-source.h"
#include "ns3/wifi-radio-energy-model.h"
#include "ns3/energy-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>


NS_LOG_COMPONENT_DEFINE ("wireless");

using namespace ns3;

//TODO: ___WayPointModel
Ptr<ConstantVelocityMobilityModel> cvmm;
double position_interval = 1.0;
std::string tracebase = "scratch/wireless";

// two callbacks
void printPosition() 
{
  Vector thePos = cvmm->GetPosition();
  Simulator::Schedule(Seconds(position_interval), &printPosition);
  std::cout << "position: " << thePos << std::endl;
}

void stopMover() 
{
  cvmm -> SetVelocity(Vector(0,0,0));
}

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





int main (int argc, char *argv[])
{
  // Dsss options: DsssRate1Mbps, DsssRate2Mbps, DsssRate5_5Mbps, DsssRate11Mbps
  // also ErpOfdmRate to 54 Mbps and OfdmRate to 150Mbps w 40MHz band-width
  std::string phyMode = "DsssRate1Mbps";                
  //phyMode = "DsssRate11Mbps";


  int hSpacing = 200;           // horizontal spacing between nodes
  int vSpacing = 150;			// Vertical spacing between nodes
 // int mheight = 150;            // height of mover below the top nodes



  int screenY = 1000;

  int  rows   = 5; //number of node rows
  int columns = 5; // number of node columns


  int nTopNodes= rows * columns;
 // int nBottomNodes = rows * columns;


  int X = (columns-1)*hSpacing+1;              // X and Y are the dimensions of the field
  //int Y = 300; 
  int packetsize = 500;
  double factor = 1.0;  // allows slowing down rate and extending runtime; same total # of packets
  int endtime = (int)100*factor;
  double speed = (X-1.0)/endtime;
  double bitrate = 80*1000.0/factor;  // bits/sec
  uint32_t interval = 1000*packetsize*8/bitrate*1000;    // in microsec, computed from packetsize and bitrate
  uint32_t packetcount = 1000000*endtime/ interval;
  std::cout << "interval = " << interval <<", rate=" << bitrate << ", packetcount=" << packetcount << std::endl;

  CommandLine cmd;              // no options, actually
  cmd.Parse (argc, argv);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Set non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

  // Create nodes
  NodeContainer nodeContainer;
  //NodeContainer fixedPosBottom;

  nodeContainer.Create(nTopNodes); //fixedpos.Create(bottomrow);
  // Node'lar olusturulur
  //fixedPosBottom.Create(nBottomNodes);

  //Ptr<Node> lowerleft = fixedPosTop.Get(0);

  Ptr<Node> mover = CreateObject<Node>();

  // The below set of helpers will help us to put together the desired Wi-Fi behavior
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager"); // Use AARF rate control
  // to view AARF rate changes, set in the shell NS_LOG=AarfWifiManager=level_debug

  // The PHY layer here is "yans"
  YansWifiPhyHelper wifiPhyHelper =  YansWifiPhyHelper::Default ();
  // for .pcap tracing
  // wifiPhyHelper.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannelHelper;              // *not* ::Default() !
  wifiChannelHelper.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel"); // pld: default?
  // the following has an absolute cutoff at distance > 250
  wifiChannelHelper.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(250)); 
  Ptr<YansWifiChannel> pchan = wifiChannelHelper.Create ();
  wifiPhyHelper.SetChannel (pchan);

  // Add a non-QoS upper-MAC layer "AdhocWifiMac", and set rate control
  NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default ();
  wifiMacHelper.SetType ("ns3::AdhocWifiMac"); 
  NetDeviceContainer fixedPosTopDev = wifi.Install (wifiPhyHelper, wifiMacHelper, nodeContainer);
  NetDeviceContainer devices = fixedPosTopDev;
 //devices.Add(wifi.Install (wifiPhyHelper, wifiMacHelper, fixedPosBottom));
 //Dugume wifi cihaz ozelligi verir
  devices.Add (wifi.Install (wifiPhyHelper, wifiMacHelper, mover));



  // set positions. 
  MobilityHelper mobilityFixedNodes;

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> (); // Top nodes
 // Ptr<ListPositionAllocator> positionAlloc2 = CreateObject<ListPositionAllocator> (); // Top nodes


  int Xpos = 0;
  bool band = true;
  int brheight = vSpacing;

  for(int j=0; j< columns; j++){
	  for( int i=0; i< rows;i++){

		  // Node ların yerini ayarlar sırayla dizmis osition alloc a atamıs
		  positionAlloc->Add(Vector(Xpos,screenY-brheight, 0.0));
		 // positionAlloc2->Add(Vector(Xpos,(rows*vSpacing+(vSpacing*2))+brheight, 0.0));//CAMBIAR AQUI
		 //TODO: Yapabilirsek random dağıtmamız lazım

		  Xpos += hSpacing;

	  }
	  brheight += vSpacing;
	  if(band) {
		  Xpos = 50;
		  band = false;
	  }
	  else{
		  Xpos=0;
		  band = true;

	  }
  }


   mobilityFixedNodes.SetPositionAllocator (positionAlloc);
   //
   //TODO: ___WayPointModel
   mobilityFixedNodes.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   mobilityFixedNodes.Install (nodeContainer);


   //mobilityFixedNodes.SetPositionAllocator (positionAlloc2);
  // mobilityFixedNodes.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   //mobilityFixedNodes.Install (fixedPosBottom);



  // ConstantVelocityMobilityModel is a subclass of MobilityModel
  Vector pos (0,(rows*vSpacing+(vSpacing*2)), 0);  //CAMBIAR AQUI
  Vector vel (speed, -speed, 0);
  MobilityHelper mobile; 
  //TODO: Velocity modelle birlikte yalnızca 1 node hareket ettirilmiş. Burada Random modelle hepsi hareket ettirilecektir.
  mobile.SetMobilityModel("ns3::ConstantVelocityMobilityModel");        // no Attributes
  mobile.Install(mover);
  cvmm = mover->GetObject<ConstantVelocityMobilityModel> ();
  cvmm->SetPosition(pos);
  cvmm->SetVelocity(vel);


  std::cout << "position: " << cvmm->GetPosition() << " velocity: " << cvmm->GetVelocity() << std::endl;
  std::cout << "mover mobility model: " << mobile.GetMobilityModelType() << std::endl; // just for confirmation


 /////////////-------- Energy Source and Device Energy Model  configuration -----------------------------------


  /***************************************************************************/
  /* energy source */
  //Enerji kaynagı olusturur.
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  //Baslangıc enerjisi her birinin 300 verilmis.

  RngSeedManager::SetSeed(5);
  RngSeedManager::SetRun(100);
  Ptr<UniformRandomVariable> x=CreateObject<UniformRandomVariable>();















//
//  for (int i = 0; i <nTopNodes ; i++) {
//	  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", IntegerValue(x->GetInteger(50,100)) );
//	  EnergySourceContainer sources = basicSourceHelper.Install (nodeContainer.Get(i));
//	  WifiRadioEnergyModelHelper radioEnergyHelper;
//	  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (10));
//	  //radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
//	  // install device model
//	  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (fixedPosTopDev.Get(i), sources);
//
//}





  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue(300));
  // install source
  //Enerji özelliği eklendi
  EnergySourceContainer sources = basicSourceHelper.Install (nodeContainer);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (10));
  //radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (fixedPosTopDev, sources);
  /***************************************************************************/

  //iterate on the node container an asign Energy model and Device to each Node
  int index = 0;
  for(NodeContainer::Iterator j= nodeContainer.Begin(); j!= nodeContainer.End(); ++j)
    {

      Ptr<BasicEnergySource> energySource = CreateObject<BasicEnergySource>();
      Ptr<WifiRadioEnergyModel> energyModel = CreateObject<WifiRadioEnergyModel>();

      energySource->SetInitialEnergy (x->GetInteger(50,300));
      energyModel->SetEnergySource (energySource);
      energySource->AppendDeviceEnergyModel (energyModel);
      energyModel->SetTxCurrentA(20);

      // aggregate energy source to node
      Ptr<Node> object = *j;
      object->AggregateObject (energySource);


      // adding tracing functions


      std::string context = static_cast<std::ostringstream*>( &(std::ostringstream() << index) )->str();
      std::cout << "Connecting node " << context << std::endl;
      Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (sources.Get (index));
      basicSourcePtr->TraceConnect ("RemainingEnergy", context, MakeCallback (&RemainingEnergy));

      // device energy model
      Ptr<DeviceEnergyModel> basicRadioModelPtr =
        basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);



      // device energy model
      // Ptr<DeviceEnergyModel> basicRadioModelPtr =
      //   basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
      NS_ASSERT (basicSourcePtr != NULL);
      basicSourcePtr->TraceConnect ("TotalEnergyConsumption", context, MakeCallback (&TotalEnergy));
      index++;
  }
  ////////////////////////////////////////////////////////////////////////////






  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;
  Ipv4ListRoutingHelper listrouting;
  //listrouting.Add(olsr, 10);                          // generates less traffic
  listrouting.Add(aodv, 10);                            // fastest to find new routes

  InternetStackHelper internet;
  internet.SetRoutingHelper(listrouting);
  internet.Install (nodeContainer);
 // internet.Install (fixedPosBottom);
  internet.Install (mover);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");           // there is only one subnet
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
  
  uint16_t port = 80;

  // create a receiving application (UdpServer) on node mover
  // Address sinkaddr(InetSocketAddress (Ipv4Address::GetAny (), port));
  Config::SetDefault("ns3::UdpServer::Port", UintegerValue(port));

  Ptr<UdpServer> UdpRecvApp = CreateObject<UdpServer>();
  UdpRecvApp->SetStartTime(Seconds(0.0));
  UdpRecvApp->SetStopTime(Seconds(endtime+60));
  mover->AddApplication(UdpRecvApp);

// create sender application on node lowerleft of type UdpClient
// packets are marked in the tracefile with ns3::SeqTsHeader, including sequence numbers

  Ptr<Ipv4> m4 = mover->GetObject<Ipv4>();
  Ipv4Address Maddr = m4->GetAddress(1,0).GetLocal();
  std::cout << "IPv4 address of mover: " << Maddr << std::endl;
  // Address moverAddress (InetSocketAddress (Maddr, port));

  Config::SetDefault("ns3::UdpClient::MaxPackets", UintegerValue(packetcount));
  Config::SetDefault("ns3::UdpClient::PacketSize", UintegerValue(packetsize));
  Config::SetDefault("ns3::UdpClient::Interval",   TimeValue (MicroSeconds (interval)));





  Ptr<UdpClient> UdpSendApp = CreateObject<UdpClient>();
  UdpSendApp -> SetRemote(Maddr, port);
  UdpSendApp -> SetStartTime(Seconds(0.0));
  UdpSendApp -> SetStopTime(Seconds(endtime));
 // lowerleft->AddApplication(UdpSendApp);



  // create animation file, to be run with 'netanim'
   //AnimationInterface anim (tracebase + ".xml");


  AnimationInterface anim ("wirelessExample2.xml");
  anim.EnablePacketMetadata ();
  anim.SetMobilityPollInterval(Seconds(0.1));




  // adding the clint application to a random number of nodes
  srand((unsigned)time(0));

  int totalNodes = rows * columns;
  int numClients =  3;//rand() % totalNodes+1;


  for(int i=0; i< numClients; i++){

	  Ptr<Node> clientNode = nodeContainer.Get(rand() % totalNodes);
	  anim.UpdateNodeColor(clientNode,0,204,0);
	  clientNode ->AddApplication(UdpSendApp); // add client app



  }






  // Tracing
  //wifiPhyHelper.EnablePcap (tracebase, devices);

  AsciiTraceHelper ascii;
  //wifiPhyHelper.EnableAsciiAll (ascii.CreateFileStream ("wirelessExample2.tr"));
// wifiPhyHelper.EnableAsciiAll (ascii.CreateFileStream (tracebase + ".tr"));





  // uncomment the next line to verify that node 'mover' is actually moving
  //Simulator::Schedule(Seconds(position_interval), &printPosition);

  Simulator::Schedule(Seconds(endtime), &stopMover);

  Simulator::Stop(Seconds (endtime+60));
  Simulator::Run ();
  Simulator::Destroy ();

  int pktsRecd = UdpRecvApp->GetReceived();
  std::cout << numClients << " node Clients created"<< std::endl;
  std::cout << "packets received: " << pktsRecd << std::endl;
  std::cout << "packets recorded as lost: " << (UdpRecvApp->GetLost()) << std::endl;
  std::cout << "packets actually lost: " << (packetcount - pktsRecd) << std::endl;

  return 0;
}
