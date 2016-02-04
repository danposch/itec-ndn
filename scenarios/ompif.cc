#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndn-all.hpp"
#include "ns3-dev/ns3/ndnSIM/utils/tracers/ndn-l3-tracer.hpp"
#include "ns3-dev/ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.hpp"
#include "ns3/log.h"

#include "../extensions/randnetworks/networkgenerator.h"
#include "../extensions/fw/competitors/omp-if/ompif-client.h"
#include "../extensions/fw/competitors/omp-if/ompif-router.h"


#include <boost/lexical_cast.hpp>

using namespace ns3;

void parseParameters(int argc, char* argv[])
{
  bool v0 = false, v1 = false, v2 = false;
  bool vN = false;

  ns3::Config::SetDefault("ns3::PointToPointNetDevice::Mtu", StringValue("5000"));

  std::string top_path = "ompif-test.top";

  CommandLine cmd;
  cmd.AddValue ("v0", "Prints all log messages >= LOG_DEBUG. (OPTIONAL)", v0);
  cmd.AddValue ("v1", "Prints all log messages >= LOG_INFO. (OPTIONAL)", v1);
  cmd.AddValue ("v2", "Prints all log messages. (OPTIONAL)", v2);
  cmd.AddValue ("vN", "Disable all internal logging parameters, use NS_LOG instead", vN);
  cmd.AddValue ("top", "Path to the topology file. (OPTIONAL)", top_path);

  cmd.Parse (argc, argv);

  if (vN == false)
  {
    LogComponentEnableAll (LOG_ALL);

    if(!v2)
    {
      LogComponentDisableAll (LOG_LOGIC);
      LogComponentDisableAll (LOG_FUNCTION);
    }
    if(!v1 && !v2)
    {
      LogComponentDisableAll (LOG_INFO);
    }
    if(!v0 && !v1 && !v2)
    {
      LogComponentDisableAll (LOG_DEBUG);
    }
  } else {
    NS_LOG_UNCOND("Disabled internal logging parameters, using NS_LOG as parameter.");
  }
  AnnotatedTopologyReader topologyReader ("", 10);
  NS_LOG_UNCOND("Using topology file " << top_path);
  topologyReader.SetFileName ("topologies/" + top_path);
  topologyReader.Read();
}

int main(int argc, char* argv[])
{
  NS_LOG_COMPONENT_DEFINE ("ContentLookup");

  parseParameters(argc, argv);

  ParameterConfiguration::getInstance ()->setParameter ("PREFIX_COMPONENT",0);

  NodeContainer streamers;
  int nodeIndex = 0;
  std::string nodeNamePrefix("ContentDst");
  Ptr<Node> contentDst = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(contentDst != NULL)
  {
    streamers.Add (contentDst);
    contentDst =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }

  NodeContainer providers;
  nodeIndex = 0;
  nodeNamePrefix = std::string("ContentSrc");
  Ptr<Node> contentSrc = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(contentSrc != NULL)
  {
    providers.Add (contentSrc);
    contentSrc = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }

  NodeContainer routers;
  nodeIndex = 0;
  nodeNamePrefix = std::string("Router");
  Ptr<Node> router = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(router != NULL)
  {
    routers.Add (router);
    router = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }

  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Lru","MaxSize", "1000");

  ndnHelper.Install (providers);
  ndnHelper.Install (streamers);
  ndnHelper.Install (routers);

  ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::OMPIFClient>(streamers,"/");
  ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::OMPIFRouter>(routers,"/");
  //ns3::ndn::StrategyChoiceHelper::Install(routers, "/", "/localhost/nfd/strategy/bestroute");

  // Install NDN applications
  std::string prefix = "/background/";

  ns3::ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix (prefix);
  //consumerHelper.SetAttribute ("Frequency", StringValue ("150")); // ca. 5Mbit/s per streamer
  consumerHelper.SetAttribute ("Frequency", StringValue ("90")); // ca. 4Mbit/s per streamer
  consumerHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  consumerHelper.SetAttribute ("LifeTime", StringValue("1s"));

  for(int i=0; i < streamers.size (); i++)
  {
    //consumerHelper.SetPrefix (prefix + "_c" + boost::lexical_cast<std::string>(i) + "/layer0");
    //consumerHelper.SetPrefix (prefix + "/layer" + boost::lexical_cast<std::string>(i));
    consumerHelper.SetPrefix ("/server"+ boost::lexical_cast<std::string>(i));
    consumerHelper.Install (streamers.Get (i%providers.size ()));
  }

  ns3::ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetPrefix ("/server");
  producerHelper.SetAttribute ("PayloadSize", StringValue("4096"));
  producerHelper.Install (providers);

  for(int i=0; i < providers.size (); i++)
  {
    producerHelper.SetPrefix ("/server"+ boost::lexical_cast<std::string>(i));
    producerHelper.Install (providers.Get (i));
  }

   // Installing global routing interface on all nodes
  ns3::ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  for(int i=0; i < providers.size (); i++)
    ndnGlobalRoutingHelper.AddOrigins("/server"+ boost::lexical_cast<std::string>(i), providers.Get (i));

  // Calculate and install FIBs
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes ();

  NS_LOG_UNCOND("Simulation will be started!");

  Simulator::Stop (Seconds(120)); //runs for 5 min.
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_UNCOND("Simulation completed!");
  return 0;
}
