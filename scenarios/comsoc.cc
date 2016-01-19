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
#include <fstream>
#include <string>

using namespace ns3;

typedef std::map<int /*client*/,int /*server*/> ClientServerPairs;

int main (int argc, char *argv[])
{
  // BRITE needs a configuration file to build its graph.
  std::string confFile = "comsoc_tops/LowBW_LowCon_0.top";
  std::string strategy = "bestRoute";
  std::string route = "all";
  std::string outputFolder = "output/";
  std::string content_popularity = "uniform";

  /*LogComponentEnableAll (LOG_ALL);
  LogComponentDisableAll (LOG_LOGIC);
  LogComponentDisableAll (LOG_FUNCTION);
  LogComponentDisableAll (LOG_INFO);*/

  CommandLine cmd;
  cmd.AddValue ("comsocConfigFile", "BRITE conf file", confFile);
  cmd.AddValue ("fw-strategy", "Forwarding Strategy", strategy);
  cmd.AddValue ("route", "defines if you use a single route or all possible routes", route);
  cmd.AddValue ("outputFolder", "defines specific output subdir", outputFolder);
  cmd.AddValue ("content-popularity", "Defines the model for the content popularity", content_popularity);

  cmd.Parse (argc,argv);

  Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (0.00));
  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

  //parse comsoc topology (quick and dirty..)
  std::ifstream file(confFile);
  std::string line;

  std::getline(file,line);
  if(!boost::starts_with(line,"#number of nodes"))
  {
    fprintf(stderr, "Invalid comsoc topolgy!\n");
    exit(0);
  }

  int nr_nodes = 0;
  std::getline(file,line);
  nr_nodes = boost::lexical_cast<int>(line);

  //fprintf(stderr, "ComsocTop: %d nodes\n",nr_nodes);

  std::getline(file,line);
  if(!boost::starts_with(line,"#nodes setting (n1,n2,bandwidth in kbits a -> b, bandwidth in kbits a <- b, delay a -> b in ms, delay b -> a in ms)"))
  {
    fprintf(stderr, "Invalid comsoc topolgy!\n");
    exit(0);
  }

  NodeContainer nodes;
  ClientServerPairs pairs;

  nodes.Create (nr_nodes);

  for(int i=0; i<nodes.size (); i++)
  {
    Names::Add (std::string("Node_" + boost::lexical_cast<std::string>(i)), nodes.Get (i));
  }

  PointToPointHelper *p2p = new PointToPointHelper;

  std::getline(file,line);
  std::vector<std::string> attributes;
  int n1, n2, bw_n1_n2, bw_n2_n1, delay_n1_n2, delay_n2_n1;

   ObjectFactory m_queueFactory;
   ObjectFactory m_channelFactory;
   ObjectFactory m_deviceFactory;
   m_queueFactory.SetTypeId ("ns3::DropTailQueue");
   m_deviceFactory.SetTypeId ("ns3::PointToPointNetDevice");
   m_channelFactory.SetTypeId ("ns3::PointToPointChannel");

  while(!boost::starts_with(line, "#properties (Client, Server)")) // create the connections
  {
    boost::split(attributes, line, boost::is_any_of(","));

    if(attributes.size () < 6)
    {
      fprintf(stderr,"Invalid Link Specification\n");
      exit(0);
    }

    n1 = boost::lexical_cast<int>(attributes.at (0));
    n2 = boost::lexical_cast<int>(attributes.at (1));
    bw_n1_n2 = boost::lexical_cast<int>(attributes.at (2));
    bw_n2_n1 = boost::lexical_cast<int>(attributes.at (3));
    delay_n1_n2 = boost::lexical_cast<int>(attributes.at (4));
    delay_n2_n1 = boost::lexical_cast<int>(attributes.at (5));

    std::string delay = boost::lexical_cast<std::string>((delay_n1_n2+delay_n2_n1)/2);
    delay = delay.append("ms");
    m_channelFactory.Set ("Delay", StringValue(delay));

    std::string rate = boost::lexical_cast<std::string>(bw_n1_n2);
    rate = rate.append ("kbps");
    m_deviceFactory.Set ("DataRate", StringValue(rate));

    Ptr<Node> a = nodes.Get (n1);
    Ptr<Node> b = nodes.Get (n2);

    Ptr<PointToPointNetDevice> devA = m_deviceFactory.Create<PointToPointNetDevice> ();
    devA->SetAddress (Mac48Address::Allocate ());
    a->AddDevice (devA);
    Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
    devA->SetQueue (queueA);

    rate = boost::lexical_cast<std::string>(bw_n2_n1);
    rate = rate.append ("kbps");
    m_deviceFactory.Set ("DataRate", StringValue(rate));

    Ptr<PointToPointNetDevice> devB = m_deviceFactory.Create<PointToPointNetDevice> ();
    devB->SetAddress (Mac48Address::Allocate ());
    b->AddDevice (devB);
    Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
    devB->SetQueue (queueB);

    Ptr<PointToPointChannel> channel = m_channelFactory.Create<PointToPointChannel> ();
    devA->Attach (channel);
    devB->Attach (channel);

    std::getline(file,line);
  }

  if(!boost::starts_with(line,"#properties (Client, Server)"))
  {
    fprintf(stderr, "Invalid comsoc topolgy!\n");
    exit(0);
  }

  std::getline(file,line);
  int c_id = 0, s_id = 0;

  while(!boost::starts_with(line, "#eof //do not delete this")) // server/clients nodes
  {
    boost::split(attributes, line, boost::is_any_of(","));

    if(attributes.size () < 2)
    {
      fprintf(stderr,"Invalid Properties Specification\n");
      exit(0);
    }
    c_id = boost::lexical_cast<int>(attributes.at (0));
    s_id = boost::lexical_cast<int>(attributes.at (1));
    pairs[c_id] = s_id;

    std::getline(file,line);
  }

  /*fprintf(stderr,"Pairs:\n");
  for(ClientServerPairs::iterator it = pairs.begin (); it != pairs.end (); it++)
    fprintf(stderr, "(%d,%d)\n", it->first, it->second);*/

  int simTime = 1800; //seconds

  //4. setup and install strategy for server/clients

  //3. install helper on network nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Lru","MaxSize", "65536"); // cache size 250 MB assuming 4kb large data packets
  ndnHelper.Install(nodes);// install all on network nodes...

  if(strategy.compare ("saf") == 0)
    ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::SAF>(nodes,"/");
  else if(strategy.compare ("bestRoute") == 0)
    ns3::ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/best-route");
  else if(strategy.compare ("ncc") == 0)
    ns3::ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/ncc");
  else if(strategy.compare ("broadcast") == 0)
    ns3::ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/broadcast");
  else if (strategy.compare ("omccrf") == 0)
    ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::OMCCRF>(nodes,"/");
  else if (strategy.compare ("oracle") == 0)
  {
    fprintf(stderr, "Not Supported for this Scenario\n");
    exit(-1);
  }
  else
  {
    fprintf(stderr, "Invalid Strategy::%s!\n",strategy.c_str ());
    exit(-1);
  }

  //install cstore tracers
  ns3::ndn::CsTracer::Install(nodes, std::string(outputFolder + "/cs-trace.txt"), Seconds(1.0));

  ns3::ndn::ExtendedGlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  std::vector<int> producers_already_seen;
  ns3::ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("4096"));
  producerHelper.SetAttribute ("Freshness", StringValue("300s"));

  ns3::ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue ("60")); // X interests a second roughly 2 MBIT
  consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  consumerHelper.SetAttribute ("LifeTime", StringValue("1s"));

  Ptr<UniformRandomVariable> r = CreateObject<UniformRandomVariable>();
  r->SetAttribute ("Min", DoubleValue (0));
  r->SetAttribute ("Max", DoubleValue (1));

  for(ClientServerPairs::iterator it = pairs.begin (); it != pairs.end (); it++)
  {
    //fprintf(stderr, "(%d,%d)\n", it->first, it->second);

    c_id = it->first;
    s_id = it->second;

    if(std::find(producers_already_seen.begin (), producers_already_seen.end (), s_id) == producers_already_seen.end ())
    {
      producerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(s_id)));
      producerHelper.Install (nodes.Get (s_id));
      ndnGlobalRoutingHelper.AddOrigin(std::string("/Server_" + boost::lexical_cast<std::string>(s_id)),nodes.Get (s_id));
      ns3::ndn::L3RateTracer::Install (nodes.Get (s_id), std::string(outputFolder + "/server_aggregate-trace_"  + boost::lexical_cast<std::string>(s_id)).append(".txt"), Seconds (simTime));
    }

    consumerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(s_id)));
    ApplicationContainer consumer = consumerHelper.Install (nodes.Get (c_id));
    consumer.Start (Seconds(r->GetValue()*5.0));
    consumer.Stop (Seconds(simTime));

    ns3::ndn::L3RateTracer::Install (nodes.Get (c_id), std::string(outputFolder + "/consumer_aggregate-trace_"  + boost::lexical_cast<std::string>(c_id)).append(".txt"), Seconds (simTime));
    ns3::ndn::AppDelayTracer::Install(nodes.Get (c_id), std::string(outputFolder +"/consumer_app-delays-trace_"  + boost::lexical_cast<std::string>(c_id)).append(".txt"));
  }

  bool isRouter;
  for(int index = 0; index < nodes.size (); index++)
  {
    bool isRouter = true;
    for(ClientServerPairs::iterator it = pairs.begin (); it != pairs.end (); it++)
    {
      if(it->first == index || it->second == index)
      {
        isRouter = false;
        break;
      }
    }
    if(isRouter)
    {
      ns3::ndn::L3RateTracer::Install (nodes.Get (index), std::string(outputFolder + "/router_aggregate-trace_"  + boost::lexical_cast<std::string>(index)).append(".txt"), Seconds (simTime));
    }
  }

  /*if(strategy.compare ("oracle") == 0) // calc all routs ammoung the nodes
  {
    ndnGlobalRoutingHelper.AddOriginsForAllUsingNodeIds ();
  }*/

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
