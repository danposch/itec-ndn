#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4.h"

#include "boost/lexical_cast.hpp"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TCPMPATH");

int
main (int argc, char *argv[])
{

  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId()));

  double simTime = 600.0;

  bool tracing = true;
  uint32_t maxBytes = 0;

// Allow the user to override any of the defaults at
// run-time, via command-line arguments
//
  CommandLine cmd;
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.Parse (argc, argv);

// Explicitly create the nodes required by the topology (shown above).
  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  nodes.Create (8);

  //name the nodes
  for(int i = 0; i < 6; i++) //add the router names
  {
     Names::Add (std::string("Router" + boost::lexical_cast<std::string>(i)), nodes.Get (i));
  }

  //add the provider
  Names::Add (std::string("Provider"), nodes.Get (6));
  Names::Add (std::string("DataStreamer0"), nodes.Get (7));

  NS_LOG_INFO ("Create channels.");
  //Explicitly create the point-to-point link required by the topology
  PointToPointHelper pointToPoint;
  std::vector<NetDeviceContainer> netContainers;

  pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxPackets", StringValue ("30"));

  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2000Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("20ms"));

  netContainers.push_back (NetDeviceContainer());
  netContainers.at (netContainers.size ()-1).Add (pointToPoint.Install (Names::Find<Node>(std::string("Router2")), Names::Find<Node>(std::string("Router4"))));

  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("3000Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  netContainers.push_back (NetDeviceContainer());
  netContainers.at (netContainers.size ()-1).Add(pointToPoint.Install (Names::Find<Node>(std::string("Router1")), Names::Find<Node>(std::string("Router3"))));


  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5000Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));

  netContainers.push_back (NetDeviceContainer());
  netContainers.at (netContainers.size ()-1).Add(pointToPoint.Install (Names::Find<Node>(std::string("Router1")), Names::Find<Node>(std::string("Router0"))));

  netContainers.push_back (NetDeviceContainer());
  netContainers.at (netContainers.size ()-1).Add(pointToPoint.Install (Names::Find<Node>(std::string("Router2")), Names::Find<Node>(std::string("Router0"))));

  netContainers.push_back (NetDeviceContainer());
  netContainers.at (netContainers.size ()-1).Add(pointToPoint.Install (Names::Find<Node>(std::string("Router5")), Names::Find<Node>(std::string("Router3"))));

  netContainers.push_back (NetDeviceContainer());
  netContainers.at (netContainers.size ()-1).Add(pointToPoint.Install (Names::Find<Node>(std::string("Router5")), Names::Find<Node>(std::string("Router4"))));

  netContainers.push_back (NetDeviceContainer());
  netContainers.at (netContainers.size ()-1).Add(pointToPoint.Install (Names::Find<Node>(std::string("Router5")), Names::Find<Node>(std::string("Provider"))));

  netContainers.push_back (NetDeviceContainer());
  netContainers.at (netContainers.size ()-1).Add(pointToPoint.Install (Names::Find<Node>(std::string("Router0")), Names::Find<Node>(std::string("DataStreamer0"))));

// Install the internet stack on the nodes
  InternetStackHelper internet;
  internet.Install (nodes);

// We've got the "hardware" in place.  Now we need to add IP addresses.
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;

  for(int i = 0; i < netContainers.size (); i++)
  {
    ipv4.SetBase (std::string("192.168."+ boost::lexical_cast<std::string>(i)+".0").c_str(), "255.255.255.0");
    ipv4.Assign (netContainers.at (i));
  }
  //Turn on global static routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_INFO ("Create Applications.");
// Create a BulkSendApplication and install it on node 0
  uint16_t port = 1234;

  //Ptr<Node> n = Names::Find<Node>(std::string("DataStreamerA5"));

  ApplicationContainer sinkApps;

  for (int  i = 0; i < 1 ; i++)
  {
    BulkSendHelper source("ns3::TcpSocketFactory",
                           InetSocketAddress (Names::Find<Node>(std::string("DataStreamer"+boost::lexical_cast<std::string>(i)))->GetObject<Ipv4>()->GetAddress (1,0).GetLocal (), port));
    // Set the amount of data to send in bytes.  Zero is unlimited.
    source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    ApplicationContainer sourceApps = source.Install (Names::Find<Node>(std::string("Provider")));
    sourceApps.Start (Seconds (0.0));
    sourceApps.Stop (Seconds (simTime));

  // Create a PacketSinkApplication and install it on node 1
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                           InetSocketAddress (Ipv4Address::GetAny (), port));
    sinkApps.Add (sink.Install (Names::Find<Node>(std::string("DataStreamer"+boost::lexical_cast<std::string>(i)))));

  }

  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (simTime));

//
// Set up tracing if enabled
//
  if (tracing)
    {
      AsciiTraceHelper ascii;
      //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
      pointToPoint.EnablePcapAll ("tcp-bulk-send", false);
    }

// Now, do the actual simulation.
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (simTime+0.01));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  double totalBytes  = 0.0;
  for (int i = 0; i < sinkApps.GetN (); i++)
  {
    totalBytes += (DynamicCast<PacketSink> (sinkApps.Get(i)))->GetTotalRx ();
  }
//  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
//  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;

  fprintf(stderr, "avg kbps = %f\n", (totalBytes / simTime / 1000)*8);
}
