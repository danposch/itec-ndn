#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndn-all.hpp"

#include "../extensions/randnetworks/networkgenerator.h"

using namespace ns3;
using namespace ns3::ndn;
using namespace ns3::itec;

int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.Parse(argc, argv);

  // BRITE needs a configuration file to build its graph.
  std::string confFile = "brite_configs/example.conf";

  NetworkGenerator gen(confFile);
  fprintf(stderr, "Number of ASs = %d\n",gen.getNumberOfAS ());
  fprintf(stderr, "Number of ASNodes = %d\n",gen.getAllASNodes ().size ());
  fprintf(stderr, "Number of LeafNodes = %d\n",gen.getAllLeafNodes ().size ());

  int min_bw_as = 3000;
  int max_bw_as = 5000;
  int min_bw_leaf = 2000;
  int max_bw_leaf = 4000;

  int additional_random_connections_as = gen.getNumberOfAS ()/2;
  int additional_random_connections_leaf = gen.getAllASNodesFromAS (0).size () / 3;

  gen.randomlyAddConnectionsBetweenTwoAS (additional_random_connections_as,min_bw_as,max_bw_as,5,20);
  gen.randomlyAddConnectionsBetweenTwoNodesPerAS(additional_random_connections_leaf,min_bw_leaf,max_bw_leaf,5,20);
  fprintf(stderr, "graph connectivity = %f\n",gen.calculateConnectivity());

  int simTime = 600;

  int totalLinkFailures = 10;
  for(int i = 0; i < totalLinkFailures; i++)
    gen.creatRandomLinkFailure(0, simTime, 0, simTime/10);

  StackHelper ndnHelper;
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "25000"); // all entities can store up to 1k chunks in cache (about 100MB)

  //2. create server and clients nodes
  PointToPointHelper *p2p = new PointToPointHelper;
  p2p->SetChannelAttribute ("Delay", StringValue ("2ms"));

  p2p->SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  gen.randomlyPlaceNodes (3, "Server",NetworkGenerator::ASNode, p2p);

  p2p->SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
  gen.randomlyPlaceNodes (9, "Client",NetworkGenerator::LeafNode, p2p);

  //3. install strategies for network nodes
  ndnHelper.Install(gen.getAllASNodes ());// install all on network nodes...

  //4. setup and install strategy for server/clients
  NodeContainer server = gen.getCustomNodes ("Server");
  NodeContainer client = gen.getCustomNodes ("Client");

  ndnHelper.Install (client);
  ndnHelper.Install (server);

  StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("4096"));
  for(int i=0; i<server.size (); i++)
  {
    producerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i) + "/layer0"));
    producerHelper.Install (Names::Find<Node>(std::string("Server_" + boost::lexical_cast<std::string>(i))));

    ndnGlobalRoutingHelper.AddOrigin(std::string("/Server_" + boost::lexical_cast<std::string>(i)),
                                      Names::Find<Node>(std::string("Server_" + boost::lexical_cast<std::string>(i))));
  }

  AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue ("30")); // X interests a second roughly 1 MBIT
  consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  for(int i=0; i<client.size (); i++)
  {
    consumerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i%server.size ()) + "/layer0"));
    consumerHelper.Install (Names::Find<Node>(std::string("Client_" + boost::lexical_cast<std::string>(i))));
  }

   // Calculate and install FIBs
  GlobalRoutingHelper::CalculateRoutes ();
  //GlobalRoutingHelper::CalculateAllPossibleRoutes ();

  // Run the simulator
  Simulator::Stop (Seconds (simTime+0.5)); // 10 min
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
