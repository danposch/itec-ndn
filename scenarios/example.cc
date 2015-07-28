#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndn-all.hpp"
#include "ns3-dev/ns3/ndnSIM/utils/tracers/ndn-l3-tracer.hpp"
#include "ns3-dev/ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.hpp"
#include "ns3-dev/ns3/ndnSIM/utils/tracers/ndn-cs-tracer.hpp"

#include "../extensions/randnetworks/networkgenerator.h"
#include "../extensions/fw/saf.h"
#include "../extensions/fw/OMCCRF.h"
#include "../extensions/fw/oracle.h"
#include "../extensions/fw/oraclecontainer.h"
#include "NFD/daemon/fw/broadcast-strategy.hpp"
#include "../extensions/utils/extendedglobalroutinghelper.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  // BRITE needs a configuration file to build its graph.
  std::string confFile = "brite_configs/brite_low_bw.conf";
  std::string strategy = "bestRoute";
  std::string route = "all";
  std::string outputFolder = "output/";
  std::string connectivity = "medium";
  int totalLinkFailures = 0;
  double min_link_error_rate = 0.0;
  double max_link_error_rate = 0.0;

  /*LogComponentEnableAll (LOG_ALL);
  LogComponentDisableAll (LOG_LOGIC);
  LogComponentDisableAll (LOG_FUNCTION);
  LogComponentDisableAll (LOG_INFO);*/

  CommandLine cmd;
  cmd.AddValue ("briteConfFile", "BRITE conf file", confFile);
  cmd.AddValue ("connectivity", "low, medium, high", connectivity);
  cmd.AddValue ("fw-strategy", "Forwarding Strategy", strategy);
  cmd.AddValue ("route", "defines if you use a single route or all possible routes", route);
  cmd.AddValue ("outputFolder", "defines specific output subdir", outputFolder);
  cmd.AddValue ("linkFailures", "defines number of linkfailures events", totalLinkFailures);
  cmd.AddValue ("min_link_error_rate", "Minimum error rate on network links", min_link_error_rate);
  cmd.AddValue ("max_link_error_rate", "MaxiumumError rate on network links", max_link_error_rate);

  cmd.Parse (argc,argv);

  Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (0.00));
  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

  ns3::Config::SetDefault("ns3::PointToPointNetDevice::Mtu", StringValue("5000"));
  ns3::ndn::NetworkGenerator gen(confFile);

  int min_bw_as = -1;
  int max_bw_as = -1;
  int min_bw_leaf = -1;
  int max_bw_leaf = -1;
  int additional_random_connections_as = -1;
  int additional_random_connections_leaf = - 1;

  if(confFile.find ("low_bw") != std::string::npos)
  {
    min_bw_as = 2000;
    max_bw_as = 4000;

    min_bw_leaf = 1000;
    max_bw_leaf = 2000;
  }
  else if(confFile.find ("medium_bw") != std::string::npos)
  {
    min_bw_as = 3000;
    max_bw_as = 5000;

    min_bw_leaf = 2000;
    max_bw_leaf = 4000;
  }
  else if (confFile.find ("high_bw") != std::string::npos)
  {
    min_bw_as = 4000;
    max_bw_as = 6000;

    min_bw_leaf = 3000;
    max_bw_leaf = 5000;
  }

  if(connectivity.compare ("low") == 0)
  {
    additional_random_connections_as = gen.getNumberOfAS () / 2;
    additional_random_connections_leaf = gen.getAllASNodesFromAS (0).size () / 3;
  }
  else if(connectivity.compare ("medium") == 0)
  {
    additional_random_connections_as = gen.getNumberOfAS ();
    additional_random_connections_leaf = gen.getAllASNodesFromAS (0).size () / 2;
  }
  else if (connectivity.compare ("high") == 0)
  {
    additional_random_connections_as = gen.getNumberOfAS () *2;
    additional_random_connections_leaf = gen.getAllASNodesFromAS (0).size ();
  }

  if(min_bw_as == -1 || max_bw_as == -1 || additional_random_connections_as == -1 || additional_random_connections_leaf == -1)
  {
    fprintf(stderr, "check scenario setting\n");
    exit(0);
  }

  gen.randomlyAddConnectionsBetweenTwoAS (additional_random_connections_as,min_bw_as,max_bw_as,5,20);
  gen.randomlyAddConnectionsBetweenTwoNodesPerAS(additional_random_connections_leaf,min_bw_leaf,max_bw_leaf,5,20);

  int simTime = 1800;

  for(int i = 0; i < totalLinkFailures; i++)
    gen.creatRandomLinkFailure(0, simTime, 0, simTime/10);

  //introduce error model if needed
  //gen.introduceError (min_link_error_rate, max_link_error_rate);

  //2. create server and clients nodes
  PointToPointHelper *p2p = new PointToPointHelper;
  p2p->SetChannelAttribute ("Delay", StringValue ("2ms"));

  p2p->SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  gen.randomlyPlaceNodes (10, "Server",ns3::ndn::NetworkGenerator::ASNode, p2p);

  p2p->SetDeviceAttribute ("DataRate", StringValue ("3Mbps"));
  gen.randomlyPlaceNodes (100, "Client",ns3::ndn::NetworkGenerator::LeafNode, p2p);

  //4. setup and install strategy for server/clients
  NodeContainer server = gen.getCustomNodes ("Server");
  NodeContainer client = gen.getCustomNodes ("Client");

  //3. install helper on network nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Lru","MaxSize", "6250"); // all entities can store up to 1k chunks in cache (about 25MB)
  ndnHelper.Install(gen.getAllASNodes ());// install all on network nodes...

  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "1250"); // all entities can store up to 1k chunks in cache (about 5MB)
  ndnHelper.Install (client);
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "1");
  ndnHelper.Install (server);

  if(strategy.compare ("saf") == 0)
    ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::SAF>(gen.getAllASNodes (),"/");
  else if(strategy.compare ("bestRoute") == 0)
    ns3::ndn::StrategyChoiceHelper::Install(gen.getAllASNodes (), "/", "/localhost/nfd/strategy/best-route");
  else if(strategy.compare ("ncc") == 0)
    ns3::ndn::StrategyChoiceHelper::Install(gen.getAllASNodes (), "/", "/localhost/nfd/strategy/ncc");
  else if(strategy.compare ("broadcast") == 0)
    ns3::ndn::StrategyChoiceHelper::Install(gen.getAllASNodes (), "/", "/localhost/nfd/strategy/broadcast");
  else if (strategy.compare ("omccrf") == 0)
    ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::OMCCRF>(gen.getAllASNodes (),"/");
  else if (strategy.compare ("oracle") == 0)
  {
    //ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::Oracle>(gen.getAllASNodes (),"/");

    //somehow distribute the knowlege of the nodes to the strategys...
    NodeContainer c = gen.getAllASNodes();
    for(int i = 0; i < c.size(); i++)
    {
      //fprintf(stderr, "Name = %s\n", Names::FindName (c.Get (i)).c_str ());
      Names::Rename (Names::FindName (c.Get (i)), "StrategyNode" + boost::lexical_cast<std::string>(i));

      nfd::fw::StaticOracaleContainer::getInstance()->insertNode("StrategyNode" + boost::lexical_cast<std::string>(i), c.Get (i));
      ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::Oracle>(c.Get (i),"/");
    }

    for(int i = 0; i < client.size (); i++)// register clients as potential chaches for the oracle
    {
      std::string oldname = Names::FindName (client.Get (i));
      std::string newname = "StrategyNode" + boost::lexical_cast<std::string>(i + gen.getAllASNodes ().size ());
      Names::Rename (oldname, newname);
      nfd::fw::StaticOracaleContainer::getInstance()->insertNode(newname, client.Get (i));
      ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::Oracle>(client.Get (i),"/");
      Names::Rename (newname, oldname);
    }

  }
  else
  {
    fprintf(stderr, "Invalid Strategy::%s!\n",strategy.c_str ());
    exit(-1);
  }

  //install cstore tracers
  NodeContainer routers = gen.getAllASNodes ();
  ns3::ndn::CsTracer::Install(routers, std::string(outputFolder + "/cs-trace.txt"), Seconds(1.0));

  ns3::ndn::ExtendedGlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  ns3::ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("4096"));
  for(int i=0; i<server.size (); i++)
  {
    producerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i) + "/layer0"));
    producerHelper.Install (Names::Find<Node>(std::string("Server_" + boost::lexical_cast<std::string>(i))));

    ndnGlobalRoutingHelper.AddOrigin(std::string("/Server_" + boost::lexical_cast<std::string>(i)),
                                      Names::Find<Node>(std::string("Server_" + boost::lexical_cast<std::string>(i))));
  }

  ns3::ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue ("30")); // X interests a second roughly 1 MBIT
  consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  consumerHelper.SetAttribute ("LifeTime", StringValue("1s"));

  Ptr<UniformRandomVariable> r = CreateObject<UniformRandomVariable>();
  for(int i=0; i<client.size (); i++)
  {
    consumerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i%server.size ()) + "/layer0"));
    ApplicationContainer consumer = consumerHelper.Install (Names::Find<Node>(std::string("Client_" + boost::lexical_cast<std::string>(i))));

    consumer.Start (Seconds(r->GetInteger (0,30)));
    consumer.Stop (Seconds(simTime));

    ns3::ndn::L3RateTracer::Install (Names::Find<Node>(std::string("Client_") + boost::lexical_cast<std::string>(i)),
                                     std::string(outputFolder + "/aggregate-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"), Seconds (simTime));

    ns3::ndn::AppDelayTracer::Install(Names::Find<Node>(std::string("Client_") + boost::lexical_cast<std::string>(i)),
                                 std::string(outputFolder +"/app-delays-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));
  }

  if(strategy.compare ("oracle") == 0) // calc all routs ammoung the nodes
  {
    ndnGlobalRoutingHelper.AddOriginsForAllUsingNodeIds ();
  }

   // Calculate and install FIBs
  if(route.compare ("all") == 0)
    ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes ();
  else if(route.compare ("single") == 0)
    ns3::ndn::GlobalRoutingHelper::CalculateRoutes ();
  else
  {
    fprintf(stderr, "Invalid routing algorithm\n");
    exit(-1);
  }

  // Run the simulator
  Simulator::Stop (Seconds (simTime+0.1)); // 10 min
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
