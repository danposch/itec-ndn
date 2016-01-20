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
#include "NFD/daemon/fw/broadcast-strategy.hpp"

#include "ns3/ndnSIM/utils/tracers/ndn-dashplayer-tracer.hpp"

#include "../extensions/fw/competitors/rfa/OMCCRF.h"
#include "../extensions/fw/competitors/inrr/oracle.h"
#include "../extensions/fw/competitors/inrr/oraclecontainer.h"
#include "../extensions/utils/extendedglobalroutinghelper.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  // BRITE needs a configuration file to build its graph.
  std::string confFile = "brite_configs/fixed.conf";
  std::string strategy = "saf";
  std::string route = "all";
  std::string outputFolder = "output/";
  std::string connectivity = "medium";
  int totalLinkFailures = 0;
  std::string export_top_file = "/home/dposch/top.csv";
  std::string adaptation = "buffer";
  std::string client_delay = "no-delay";

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
  cmd.AddValue ("adaptation", "Adaptation Strategy used by Client", adaptation);
  cmd.AddValue ("delay-model", "Client Start Time delay Model", client_delay);

  cmd.Parse (argc,argv);

  /*ns3::Config::SetDefault ("ns3::DropTailQueue::Mode", StringValue("QUEUE_MODE_PACKETS"));
  ns3::Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue("50"));*/

  ns3::Config::SetDefault("ns3::PointToPointNetDevice::Mtu", StringValue("4096"));

  //ensure same topology every run
  uint64_t orig_seed = RngSeedManager::GetSeed();
  uint64_t orig_run = RngSeedManager::GetRun();

  //This numbers determine the used scenario. Warning this may change on different systems...
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun(2); // 0 2
  //RngSeedManager::SetRun(totalLinkFailures);

  //ns3::ndn::NetworkGenerator gen(confFile, seedFile , newSeedFile);
  ns3::ndn::NetworkGenerator gen(confFile);

  int min_bw_as = 2000;
  int max_bw_as = 2000;

  int min_bw_leaf = 2000;
  int max_bw_leaf = 2000;
  int additional_random_connections_as = gen.getNumberOfAS ();
  int additional_random_connections_leaf = gen.getAllASNodesFromAS (0).size () / 3;

  gen.randomlyAddConnectionsBetweenTwoAS (additional_random_connections_as,min_bw_as,max_bw_as,5,20);
  gen.randomlyAddConnectionsBetweenTwoNodesPerAS(additional_random_connections_leaf,min_bw_leaf,max_bw_leaf,5,20);

  /*for(int i = 0; i < totalLinkFailures; i++)
    gen.creatRandomLinkFailure(0, simTime, 0, simTime/10);*/

  //2. create server and clients nodes
  PointToPointHelper *p2p = new PointToPointHelper;
  p2p->SetChannelAttribute ("Delay", StringValue ("2ms"));

  p2p->SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  gen.randomlyPlaceNodes (5, "Server", ns3::ndn::NetworkGenerator::ASNode, p2p);

  p2p->SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  gen.randomlyPlaceNodes (25, "Client", ns3::ndn::NetworkGenerator::ASNode, p2p);
  //gen.randomlyPlaceNodes (12, "Client",ns3::ndn::NetworkGenerator::LeafNode, p2p);

  RngSeedManager::SetSeed(orig_seed);
  RngSeedManager::SetRun(orig_run);
  RngSeedManager::SetSeed (RngSeedManager::GetRun());

  //4. setup and install strategy for server/clients
  NodeContainer server = gen.getCustomNodes ("Server");
  NodeContainer client = gen.getCustomNodes ("Client");

  //3. install helper on network nodes
  ns3::ndn::StackHelper ndnHelper;
  //ndnHelper.SetOldContentStore ("ns3::ndn::cs::Lru","MaxSize", "25000"); // all entities can store up to xk chunks in cache (about 100MB)
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Lru","MaxSize", "12500"); // all entities can store up to xk chunks in cache (about 50MB)
  ndnHelper.Install(gen.getAllASNodes ());// install all on network nodes...

  // install helper on client / server
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "1"); // all entities can store up to xk chunks in cache (about 0MB)
  ndnHelper.Install (server);
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "1250"); // all entities can store up to 1k chunks in cache (about 5MB)
  ndnHelper.Install (client);

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
    fprintf(stderr, "Invalid Strategy!\n");
    exit(-1);
  }

  //install cstore tracers
  NodeContainer routers = gen.getAllASNodes ();
  ns3::ndn::CsTracer::Install(routers, std::string(outputFolder + "/cs-trace.txt"), Seconds(1.0));

  ns3::ndn::ExtendedGlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  ns3::ndn::AppHelper producerHelper ("ns3::ndn::FileServer");
  producerHelper.SetAttribute("ContentDirectory", StringValue("/home/dposch/data/concatenated/"));
  //producerHelper.SetAttribute ("MaxPayloadSize", StringValue("4096"));

  for(int i=0; i<server.size (); i++)
  {
    std::string p = std::string("/Server_" + boost::lexical_cast<std::string>(i));
    //fprintf(stderr, "Server_%d offers prefix %s\n", i, p.c_str());

    producerHelper.SetPrefix (p);
    producerHelper.Install (Names::Find<Node>(std::string("Server_" + boost::lexical_cast<std::string>(i))));

    ndnGlobalRoutingHelper.AddOrigin(p, Names::Find<Node>(std::string("Server_" + boost::lexical_cast<std::string>(i))));
  }

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::FileConsumerCbr::MultimediaConsumer");
  //consumerHelper.SetAttribute("MaxPayloadSize", StringValue("4096"));
  consumerHelper.SetAttribute("AllowUpscale", BooleanValue(true));
  consumerHelper.SetAttribute("AllowDownscale", BooleanValue(false));
  consumerHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
  consumerHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
  consumerHelper.SetAttribute("StartRepresentationId", StringValue("auto"));
  //consumerHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::SVCRateBasedAdaptationLogic"));
  consumerHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(50));
  consumerHelper.SetAttribute("TraceNotDownloadedSegments", BooleanValue(true));
  consumerHelper.SetAttribute("StartUpDelay", DoubleValue(2.0));

  if(adaptation.compare ("buffer") == 0)
    consumerHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::SVCBufferBasedAdaptationLogic"));
  else if(adaptation.compare ("rate") == 0)
    consumerHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::SVCRateBasedAdaptationLogic"));
  else if(adaptation.compare ("nologic") == 0)
    consumerHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::SVCNoAdaptationLogic"));
  else
  {
    fprintf(stderr, "Invalid adaptation selected\n");
    exit(-1);
  }

  //export top
  //gen.exportTopology (export_top_file, "Server", "Client");
  //ofstream file;
  //file.open (export_top_file.c_str (), ios::app);
  //print heading
  //file << "#properties (Client, Server)\n";

  //2876sec duration of concatenated dataset
  int simTime = 2880;
  Ptr<RandomVariableStream> r;
  int bound = 1;
  if(client_delay.compare ("exponential") == 0)
  {
    r = CreateObject<ExponentialRandomVariable>();
    r->SetAttribute ("Mean", DoubleValue (60));
    bound = 180;
    r->SetAttribute ("Bound", DoubleValue (bound));
  }
  else if (client_delay.compare ("no-delay") == 0)
  {
    r = CreateObject<UniformRandomVariable>();
    r->SetAttribute ("Min", DoubleValue (0));
    r->SetAttribute ("Max", DoubleValue (bound));
  }
  else
  {
    fprintf(stderr, "Invalid Delay Model choosen\n");
  }
  simTime += bound;

  for(int i=0; i<client.size (); i++)
  {
    std::string mpd("/Server_" + boost::lexical_cast<std::string>(i%server.size ())
                                  +"/concatenated-3layers-server" + boost::lexical_cast<std::string>(i%server.size ()) + ".mpd.gz");

    consumerHelper.SetAttribute("MpdFileToRequest", StringValue(mpd.c_str()));
    //consumerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i%server.size ()) + "/layer0"));
    ApplicationContainer consumer = consumerHelper.Install (Names::Find<Node>(std::string("Client_" + boost::lexical_cast<std::string>(i))));
    consumer.Start (Seconds(r->GetValue()));
    consumer.Stop (Seconds(simTime));

    ns3::ndn::DASHPlayerTracer::Install(Names::Find<Node>(std::string("Client_") + boost::lexical_cast<std::string>(i)),
                                 std::string(outputFolder +"/dashplayer-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));

    /*ns3::ndn::L3RateTracer::Install (Names::Find<Node>(std::string("Client_") + boost::lexical_cast<std::string>(i)),
                                     std::string(outputFolder + "/aggregate-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"), Seconds (simTime));

    ns3::ndn::AppDelayTracer::Install(Names::Find<Node>(std::string("Client_") + boost::lexical_cast<std::string>(i)),
                                 std::string(outputFolder +"/app-delays-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));*/

    /*file << "(" << boost::lexical_cast<std::string>(client.Get (i)->GetId ()) << ","
                << boost::lexical_cast<std::string>(server.Get (i%server.size ())->GetId ()) << ")\n";*/
  }
  //file.close ();

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
  Simulator::Stop (Seconds (simTime+1.0)); // stop sim 1 sec after apps have been stopped
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

