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
#include "../extensions/utils/parameterconfiguration.h"
#include "../extensions/fw/safmeasurefactory.h"

using namespace ns3;

int main(int argc, char* argv[])
{

  std::string outputFolder = "output/";

  CommandLine cmd;
  cmd.AddValue ("outputFolder", "defines specific output subdir", outputFolder);
  cmd.Parse (argc, argv);

  nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/", nfd::fw::SAFStatisticMeasure::MThroughput);
  nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/voip", nfd::fw::SAFStatisticMeasure::MDelay);
  nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/voip", std::string("MaxDelayMS"), std::string("250"));
  nfd::fw::SAFMeasureFactory::getInstance ()->registerMeasure ("/video", nfd::fw::SAFStatisticMeasure::MDelay);
  nfd::fw::SAFMeasureFactory::getInstance ()->registerAttribute("/video", std::string("MaxDelayMS"), std::string("1000"));

  //parse the topology
  AnnotatedTopologyReader topologyReader ("", 5);
  topologyReader.SetFileName ("topologies/saf_scenario.top");
  topologyReader.Read();

  Ptr<UniformRandomVariable> r = CreateObject<UniformRandomVariable>();
  int simTime = 60; //seconds

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
  fprintf(stderr, "Parsed %d Routers\n", routers.size ());

  NodeContainer videoStreamers;
  nodeIndex = 0;
  nodeNamePrefix = std::string("VideoStreamer");
  Ptr<Node> videoStreamer = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(videoStreamer != NULL)
  {
    videoStreamers.Add (videoStreamer);
    videoStreamer =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }
  fprintf(stderr, "Parsed %d VideoStreamers\n", videoStreamers.size ());

  NodeContainer voipStreamers;
  nodeIndex = 0;
  nodeNamePrefix = std::string("VoIPStreamer");
  Ptr<Node> voipStreamer = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(voipStreamer != NULL)
  {
    voipStreamers.Add (voipStreamer);
    voipStreamer =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }
  fprintf(stderr, "Parsed %d VoIPStreamers\n", voipStreamers.size ());

  NodeContainer dataStreamers;
  nodeIndex = 0;
  nodeNamePrefix = std::string("DataStreamer");
  Ptr<Node> dataStreamer = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(dataStreamer != NULL)
  {
    dataStreamers.Add (dataStreamer);
    dataStreamer =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }
  fprintf(stderr, "Parsed %d DataStreamers\n", dataStreamers.size ());

  NodeContainer providers;
  providers.Add (Names::Find<Node>("VideoSrc"));
  providers.Add (Names::Find<Node>("VoIPSrc"));
  providers.Add (Names::Find<Node>("DataSrc"));
  fprintf(stderr, "Parsed %d Providers\n", providers.size ());

  Ptr<Node> SAFRouter = Names::Find<Node>("RouterE");

  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(1); // disable caches

  ndnHelper.Install (routers);
  ndnHelper.Install (videoStreamers);
  ndnHelper.Install (voipStreamers);
  ndnHelper.Install (dataStreamers);
  ndnHelper.Install (providers);

  ndnHelper.Install (SAFRouter);

  //set prefix components for forwarding
  ParameterConfiguration::getInstance()->setParameter("PREFIX_COMPONENT", 1); // set to prefix componen
  ParameterConfiguration::getInstance()->setParameter("SAF_USE_MULTIPLE_MEASURES", 1);

  //install SAF on routers
  ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::SAF>(SAFRouter,"/");

  // Install NDN applications

  //install video consumers
  ns3::ndn::AppHelper consumerVideoHelper ("ns3::ndn::ConsumerCbr"); //TODO change app
  consumerVideoHelper.SetAttribute ("Frequency", StringValue ("60")); //1 MBit with 2048 byte large chunks
  consumerVideoHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  consumerVideoHelper.SetAttribute ("LifeTime", StringValue("1s"));

  for(int i=0; i<videoStreamers.size (); i++)
  {
    consumerVideoHelper.SetPrefix(std::string("/video/" + boost::lexical_cast<std::string>(i) + "/"));
    ApplicationContainer consumer = consumerVideoHelper.Install (videoStreamers.Get (i));
    consumer.Start (Seconds(r->GetInteger (0,0)));
    consumer.Stop (Seconds(simTime));

    ns3::ndn::L3RateTracer::Install (videoStreamers.Get (i), std::string(outputFolder + "/videostreamer-aggregate-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"), Seconds (simTime));
    ns3::ndn::AppDelayTracer::Install(videoStreamers.Get (i),std::string(outputFolder +"/videostreamer-app-delays-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));
  }

  //install voip consumers
  ns3::ndn::AppHelper consumerVOIPHelper ("ns3::ndn::ConsumerCbr"); //TODO change app
  consumerVOIPHelper.SetAttribute ("Frequency", StringValue ("60")); //30 kbit with 64 byte large chunks
  consumerVOIPHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  consumerVOIPHelper.SetAttribute ("LifeTime", StringValue("1s"));

  for(int i=0; i<voipStreamers.size (); i++)
  {
    consumerVOIPHelper.SetPrefix(std::string("/voip/" + boost::lexical_cast<std::string>(i) + "/"));
    ApplicationContainer consumer = consumerVOIPHelper.Install (voipStreamers.Get (i));
    consumer.Start (Seconds(r->GetInteger (0,0)));
    consumer.Stop (Seconds(simTime));

    ns3::ndn::L3RateTracer::Install (voipStreamers.Get (i), std::string(outputFolder + "/voipstreamer-aggregate-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"), Seconds (simTime));
    ns3::ndn::AppDelayTracer::Install(voipStreamers.Get (i),std::string(outputFolder +"/voipstreamer-app-delays-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));
  }

  //install data consumers
  ns3::ndn::AppHelper consumerDataHelper ("ns3::ndn::ConsumerCbr"); //TODO change app
  consumerDataHelper.SetAttribute ("Frequency", StringValue ("60")); //2 Mbit with 4096 byte large chunks
  consumerDataHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  consumerDataHelper.SetAttribute ("LifeTime", StringValue("1s"));

  for(int i=0; i<dataStreamers.size (); i++)
  {
    consumerDataHelper.SetPrefix(std::string("/data/" + boost::lexical_cast<std::string>(i) + "/"));
    ApplicationContainer consumer = consumerDataHelper.Install (dataStreamers.Get (i));
    consumer.Start (Seconds(r->GetInteger (0,0)));
    consumer.Stop (Seconds(simTime));

    ns3::ndn::L3RateTracer::Install (dataStreamers.Get (i), std::string(outputFolder + "/datastreamer-aggregate-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"), Seconds (simTime));
    ns3::ndn::AppDelayTracer::Install(dataStreamers.Get (i),std::string(outputFolder +"/datastreamer-app-delays-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));
  }

  ns3::ndn::L3RateTracer::Install (SAFRouter, std::string(outputFolder + "/saf-router-aggregate-trace.txt"), Seconds (simTime));

  // Installing global routing interface on all nodes
  ns3::ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  //install producer application on the providers
  ns3::ndn::AppHelper producerHelper ("ns3::ndn::Producer");

  Ptr<Node> videoSrc = Names::Find<Node>("VideoSrc");
  producerHelper.SetPrefix ("/video");
  producerHelper.SetAttribute ("PayloadSize", StringValue("2048"));
  producerHelper.Install (videoSrc);
  ndnGlobalRoutingHelper.AddOrigins("/video", videoSrc);

  Ptr<Node> voipSrc = Names::Find<Node>("VoIPSrc");
  producerHelper.SetPrefix ("/voip");
  producerHelper.SetAttribute ("PayloadSize", StringValue("64"));
  producerHelper.Install (voipSrc);
  ndnGlobalRoutingHelper.AddOrigins("/voip", voipSrc);

  Ptr<Node> dataSrc = Names::Find<Node>("DataSrc");
  producerHelper.SetPrefix ("/data");
  producerHelper.SetAttribute ("PayloadSize", StringValue("4096"));
  producerHelper.Install (dataSrc);
  ndnGlobalRoutingHelper.AddOrigins("/data", dataSrc);

  // Calculate and install FIBs
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes ();

  //clean up the simulation
  Simulator::Stop (Seconds(simTime+1)); //runs for 10 min.
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_UNCOND("Simulation completed!");
  return 0;
}
