/**
 * Copyright (c) 2015 Daniel Posch (Alpen-Adria Universität Klagenfurt)
 *
 * This file is part of the ndnSIM extension for Stochastic Adaptive Forwarding (SAF).
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndn-all.hpp"

#include "../extensions/fw/saf.h"
#include "../extensions/fw/competitors/omp-if/ompif-router.h"
#include "../extensions/fw/competitors/omp-if/ompif-client.h"
#include "../extensions/fw/competitors/rfa/OMCCRF.h"
#include "../extensions/fw/competitors/inrr/oracle.h"
#include "../extensions/fw/competitors/inrr/oraclecontainer.h"
#include "../extensions/utils/parameterconfiguration.h"
#include "../extensions/fw/safmeasurefactory.h"
#include "ns3/ndnSIM/utils/tracers/ndn-dashplayer-tracer.hpp"
#include "../extensions/utils/extendedglobalroutinghelper.h"
#include "../extensions/utils/prefixtracer.h"

using namespace ns3;

int main(int argc, char* argv[])
{

  //nessacary as the video consumer calcs max packet per second as link capacity / mtu.
  ns3::Config::SetDefault("ns3::PointToPointNetDevice::Mtu", StringValue("4096"));

  /*LogComponentEnableAll (LOG_ALL);
  LogComponentDisableAll (LOG_LOGIC);
  LogComponentDisableAll (LOG_FUNCTION);
  LogComponentDisableAll (LOG_INFO);*/

  std::string outputFolder = "output/";
  std::string strategy = "bestRoute";
  std::string topologyFile = "topologies/saf_scenario.top";

  CommandLine cmd;
  cmd.AddValue ("outputFolder", "defines specific output subdir", outputFolder);
  cmd.AddValue ("topology", "path to the required topology file", topologyFile);
  cmd.AddValue ("fw-strategy", "Forwarding Strategy", strategy);
  cmd.Parse (argc, argv);

  ParameterConfiguration::getInstance ()->setParameter ("RELIABILITY_THRESHOLD_MIN", 0.9);
  ParameterConfiguration::getInstance ()->setParameter ("UPDATE_INTERVALL", 0.5);
  //nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/", nfd::fw::SAFStatisticMeasure::MHop);
  //nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/", std::string("MaxHops"), std::string("5"));
  nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/", nfd::fw::SAFStatisticMeasure::MThroughput);
  //nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/voip", nfd::fw::SAFStatisticMeasure::MDelay);
  //nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/voip", std::string("MaxDelayMS"), std::string("250"));
  //nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/video", nfd::fw::SAFStatisticMeasure::MDelay);
  //nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/video", std::string("MaxDelayMS"), std::string("1000"));

  if(strategy.compare ("saf_caa") == 0)
  {
    fprintf(stderr, "Enabling Context Aware Adaptation\n");
    ParameterConfiguration::getInstance ()->setParameter ("CONTENT_AWARE_ADAPTATION", 1);
    /*nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/voip", nfd::fw::SAFStatisticMeasure::MWeightedThrouput);
    nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/voip", std::string("SatisfiedWeight"), std::string("1"));
    nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/voip", std::string("UnsatisfiedWeight"), std::string("1"));

    nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/video", nfd::fw::SAFStatisticMeasure::MWeightedThrouput);
    nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/video", std::string("SatisfiedWeight"), std::string("1"));
    nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/video", std::string("UnsatisfiedWeight"), std::string("1"));

    nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/data", nfd::fw::SAFStatisticMeasure::MWeightedThrouput);
    nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/data", std::string("SatisfiedWeight"), std::string("1"));
    nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/data", std::string("UnsatisfiedWeight"), std::string("1"));*/
  }

  //parse the topology
  AnnotatedTopologyReader topologyReader ("", 5);
  topologyReader.SetFileName (topologyFile);
  topologyReader.Read();

  Ptr<UniformRandomVariable> r = CreateObject<UniformRandomVariable>();
  int simTime = 2880; //seconds

  //grep the nodes
  NodeContainer routers;
  int nodeIndex = 0;
  std::string nodeNamePrefix("Router");
  Ptr<Node> router = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(router != NULL)
  {
    routers.Add (router);
    router =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }

  nodeIndex = 0;
  nodeNamePrefix = std::string("RouterNet");
  router = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(router != NULL)
  {
    routers.Add (router);
    router =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }

  routers.Add (Names::Find<Node>("RouterA"));
  //fprintf(stderr, "Parsed %d Routers\n", routers.size ());

  NodeContainer videoStreamers;
  nodeIndex = 0;
  nodeNamePrefix = std::string("VideoStreamer");
  Ptr<Node> videoStreamer = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(videoStreamer != NULL)
  {
    videoStreamers.Add (videoStreamer);
    videoStreamer =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }
  //fprintf(stderr, "Parsed %d VideoStreamers\n", videoStreamers.size ());

  NodeContainer voipStreamers;
  nodeIndex = 0;
  nodeNamePrefix = std::string("VoIPStreamer");
  Ptr<Node> voipStreamer = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(voipStreamer != NULL)
  {
    voipStreamers.Add (voipStreamer);
    voipStreamer =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }
  //fprintf(stderr, "Parsed %d VoIPStreamers\n", voipStreamers.size ());

  NodeContainer dataStreamers;
  nodeIndex = 0;
  nodeNamePrefix = std::string("DataStreamer");
  Ptr<Node> dataStreamer = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(dataStreamer != NULL)
  {
    dataStreamers.Add (dataStreamer);
    dataStreamer =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }
  //fprintf(stderr, "Parsed %d DataStreamers\n", dataStreamers.size ());

  NodeContainer providers;
  providers.Add (Names::Find<Node>("VideoSrc"));
  providers.Add (Names::Find<Node>("VoIPSrc"));
  providers.Add (Names::Find<Node>("DataSrc"));
  //fprintf(stderr, "Parsed %d Providers\n", providers.size ());

  Ptr<Node> SAFRouter = Names::Find<Node>("RouterE");

  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(1); // disable caches

  ndnHelper.Install (routers);
  ndnHelper.Install (videoStreamers);
  ndnHelper.Install (voipStreamers);
  ndnHelper.Install (dataStreamers);
  ndnHelper.Install (providers);

  ns3::ndn::StrategyChoiceHelper::Install((routers), "/", "/localhost/nfd/strategy/best-route");
  ns3::ndn::StrategyChoiceHelper::Install(videoStreamers, "/", "/localhost/nfd/strategy/best-route");
  ns3::ndn::StrategyChoiceHelper::Install(voipStreamers, "/", "/localhost/nfd/strategy/best-route");
  ns3::ndn::StrategyChoiceHelper::Install(dataStreamers, "/", "/localhost/nfd/strategy/best-route");
  ns3::ndn::StrategyChoiceHelper::Install(providers, "/", "/localhost/nfd/strategy/best-route");

  ndnHelper.Install (SAFRouter);

  //set prefix components for forwarding
  ParameterConfiguration::getInstance()->setParameter("PREFIX_COMPONENT", 0); // set to prefix componen

  //install SAF on routers

  if(strategy.compare ("saf") == 0 || strategy.compare ("saf_caa") == 0)
    ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::SAF>(SAFRouter,"/");
  else if(strategy.compare ("bestRoute") == 0)
    ns3::ndn::StrategyChoiceHelper::Install(SAFRouter, "/", "/localhost/nfd/strategy/best-route");
  else if(strategy.compare ("ncc") == 0)
    ns3::ndn::StrategyChoiceHelper::Install(SAFRouter, "/", "/localhost/nfd/strategy/ncc");
  else if(strategy.compare ("broadcast") == 0)
    ns3::ndn::StrategyChoiceHelper::Install(SAFRouter, "/", "/localhost/nfd/strategy/broadcast");
  else if(strategy.compare ("ompif") == 0)
  {
    ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::OMPIFRouter>(SAFRouter,"/");
  }
  else if (strategy.compare ("omccrf") == 0)
    ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::OMCCRF>(SAFRouter,"/");
  else if (strategy.compare ("oracle") == 0)
  {
    NodeContainer allNodes;
    allNodes.Add (routers);
    allNodes.Add (videoStreamers);
    allNodes.Add (voipStreamers);
    allNodes.Add (dataStreamers);
    allNodes.Add (providers);
    allNodes.Add (SAFRouter);

    //somehow distribute the knowlege of the nodes to the strategys...
    for(int i = 0; i < allNodes.size(); i++)
    {
      std::string oldname = Names::FindName (allNodes.Get (i));
      std::string newname = "StrategyNode" + boost::lexical_cast<std::string>(i);

      Names::Rename (oldname, newname);
      nfd::fw::StaticOracaleContainer::getInstance()->insertNode("StrategyNode" + boost::lexical_cast<std::string>(i), allNodes.Get (i));
      Names::Rename (newname, oldname);

      if(oldname.compare ("RouterE") == 0)
      {
        ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::Oracle>(allNodes.Get (i),"/");
      }
    }
  }
  else
  {
    fprintf(stderr, "Invalid Strategy::%s!\n",strategy.c_str ());
    exit(-1);
  }

  // Install NDN applications

  //install video consumers
  ns3::ndn::AppHelper consumerVideoHelper("ns3::ndn::FileConsumerCbr::MultimediaConsumer");
  consumerVideoHelper.SetAttribute("AllowUpscale", BooleanValue(true));
  consumerVideoHelper.SetAttribute("AllowDownscale", BooleanValue(false));
  consumerVideoHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
  consumerVideoHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
  consumerVideoHelper.SetAttribute("StartRepresentationId", StringValue("auto"));
  consumerVideoHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::SVCBufferBasedAdaptationLogic"));
  consumerVideoHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(50));
  consumerVideoHelper.SetAttribute("TraceNotDownloadedSegments", BooleanValue(true));
  consumerVideoHelper.SetAttribute("StartUpDelay", DoubleValue(2.0));
  consumerVideoHelper.SetAttribute ("LifeTime", StringValue("1s"));

  for(int i=0; i<videoStreamers.size (); i++)
  {
    //consumerVideoHelper.SetPrefix(std::string("/video/" + boost::lexical_cast<std::string>(i) + "/"));
    std::string mpd("/video/" + boost::lexical_cast<std::string>(i) +"/3layers-video" + boost::lexical_cast<std::string>(i) + ".mpd.gz");
    consumerVideoHelper.SetAttribute("MpdFileToRequest", StringValue(mpd.c_str()));

    ApplicationContainer consumer = consumerVideoHelper.Install (videoStreamers.Get (i));
    consumer.Start (Seconds(r->GetInteger (0,3)));
    consumer.Stop (Seconds(simTime));

    //ns3::ndn::L3RateTracer::Install (videoStreamers.Get (i), std::string(outputFolder + "/videostreamer-aggregate-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"), Seconds (simTime));
    //ns3::ndn::AppDelayTracer::Install(videoStreamers.Get (i),std::string(outputFolder +"/videostreamer-app-delays-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));
    ns3::ndn::DASHPlayerTracer::Install(videoStreamers.Get (i), std::string(outputFolder +"/videostreamer-dashplayer-trace_" + boost::lexical_cast<std::string>(i)).append(".txt"));
  }

  //install voip consumers
  ns3::ndn::AppHelper consumerVOIPHelper ("ns3::ndn::VoIPClient");
  //we assume voip packets (G.711) with 10ms speech per packet ==> 100 packets per second
  consumerVOIPHelper.SetAttribute ("Frequency", StringValue ("100"));
  consumerVOIPHelper.SetAttribute ("Randomize", StringValue ("none"));
  consumerVOIPHelper.SetAttribute ("LifeTime", StringValue("0.050s"));
  consumerVOIPHelper.SetAttribute ("LookaheadLiftime", IntegerValue(250)); //250ms
  //consumerVOIPHelper.SetAttribute ("JitterBufferSize", StringValue("100ms"));

  for(int i=0; i<voipStreamers.size (); i++)
  {
    consumerVOIPHelper.SetPrefix(std::string("/voip/" + boost::lexical_cast<std::string>(i) + "/"));
    consumerVOIPHelper.SetAttribute ("BurstLogFile", StringValue(outputFolder+"/voipstreamer-burst-trace_"+ boost::lexical_cast<std::string>(i)+".txt"));

    ApplicationContainer consumer = consumerVOIPHelper.Install (voipStreamers.Get (i));
    consumer.Start (Seconds(r->GetInteger (0,3)));
    consumer.Stop (Seconds(simTime));

    ns3::ndn::L3RateTracer::Install (voipStreamers.Get (i), std::string(outputFolder + "/voipstreamer-aggregate-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"), Seconds (simTime));
    ns3::ndn::AppDelayTracer::Install(voipStreamers.Get (i),std::string(outputFolder +"/voipstreamer-app-delays-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));
  }

  //install data consumers
  ns3::ndn::AppHelper consumerDataHelper ("ns3::ndn::ConsumerCbr");
  consumerDataHelper.SetAttribute ("Frequency", StringValue ("90")); //3 Mbit with 4096 byte large chunks
  consumerDataHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  consumerDataHelper.SetAttribute ("LifeTime", StringValue("2s"));

  for(int i=0; i<dataStreamers.size (); i++)
  {
    consumerDataHelper.SetPrefix(std::string("/data/" + boost::lexical_cast<std::string>(i) + "/"));
    ApplicationContainer consumer = consumerDataHelper.Install (dataStreamers.Get (i));
    consumer.Start (Seconds(r->GetInteger (0,3)));
    consumer.Stop (Seconds(simTime));

    ns3::ndn::L3RateTracer::Install (dataStreamers.Get (i), std::string(outputFolder + "/datastreamer-aggregate-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"), Seconds (simTime));
    ns3::ndn::AppDelayTracer::Install(dataStreamers.Get (i),std::string(outputFolder +"/datastreamer-app-delays-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));
  }

  ns3::ndn::L3RateTracer::Install (SAFRouter, std::string(outputFolder + "/saf-router-aggregate-trace.txt"), Seconds (simTime));
  //ns3::ndn::PrefixTracer::Install (SAFRouter, std::string(outputFolder + "/saf-router-prefix-trace.txt"), Seconds (simTime));

  // Installing global routing interface on all nodes
  ns3::ndn::ExtendedGlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  //install producer application on the providers
  ns3::ndn::AppHelper videoProducerHelper ("ns3::ndn::FileServer");
  videoProducerHelper.SetAttribute("ContentDirectory", StringValue("/home/dposch/data/concatenated/"));
  //videoProducerHelper.SetAttribute ("MaxPayloadSize", StringValue("2048"));

  Ptr<Node> videoSrc = Names::Find<Node>("VideoSrc");
  for(int i=0; i < videoStreamers.size (); i++)
  {
    std::string pref = "/video/"+boost::lexical_cast<std::string>(i);
    videoProducerHelper.SetPrefix (pref);
    videoProducerHelper.Install (videoSrc);
    ndnGlobalRoutingHelper.AddOrigins(pref, videoSrc);
  }

  ns3::ndn::AppHelper producerHelper ("ns3::ndn::VoIPProducer");
  Ptr<Node> voipSrc = Names::Find<Node>("VoIPSrc");
  producerHelper.SetAttribute ("PayloadSize", StringValue("80")); // 80 byte per G.177 packet 64kbits/100 ==> 640bit/s
  for(int i=0; i < voipStreamers.size (); i++)
  {
    std::string pref = "/voip/"+boost::lexical_cast<std::string>(i);
    producerHelper.SetPrefix (pref);
    producerHelper.Install (voipSrc);
    ndnGlobalRoutingHelper.AddOrigins(pref, voipSrc);
  }

  Ptr<Node> dataSrc = Names::Find<Node>("DataSrc");
  producerHelper.SetAttribute ("PayloadSize", StringValue("4096"));
  for(int i=0; i < dataStreamers.size (); i++)
  {
    std::string pref = "/data/"+boost::lexical_cast<std::string>(i);
    producerHelper.SetPrefix (pref);
    producerHelper.Install (dataSrc);
    ndnGlobalRoutingHelper.AddOrigins(pref, dataSrc);
  }

  if(strategy.compare ("oracle") == 0) // calc all routs ammoung the nodes
  {
    ndnGlobalRoutingHelper.AddOriginsForAllUsingNodeIds ();
  }

  // Calculate and install FIBs
  //ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes ();
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes ();

  //clean up the simulation
  Simulator::Stop (Seconds(simTime+1));
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_UNCOND("Simulation completed!");
  return 0;
}
