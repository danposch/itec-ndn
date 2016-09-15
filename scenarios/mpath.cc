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

#include "ns3-dev/ns3/ndnSIM/utils/tracers/ndn-l3-tracer.hpp"
#include "ns3-dev/ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.hpp"
#include "ns3-dev/ns3/ndnSIM/utils/tracers/ndn-cs-tracer.hpp"

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

using namespace ns3;

int main(int argc, char* argv[])
{
  std::string useRequestAggregation = "true";
  std::string outputFolder = "output/";
  std::string topologyFile = "topologies/mpath.top";

  CommandLine cmd;
  cmd.AddValue ("requestAgr", "todo", useRequestAggregation);
  cmd.Parse (argc, argv);

   //parse the topology
  AnnotatedTopologyReader topologyReader ("", 5);
  topologyReader.SetFileName (topologyFile);
  topologyReader.Read();

  Ptr<UniformRandomVariable> r = CreateObject<UniformRandomVariable>();
  double simTime = 600.0; //seconds

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

  NodeContainer dataStreamersA;
  nodeIndex = 0;
  nodeNamePrefix = std::string("DataStreamer");
  Ptr<Node> dataStreamer = Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  while(dataStreamer != NULL)
  {
    dataStreamersA.Add (dataStreamer);
    dataStreamer =  Names::Find<Node>(nodeNamePrefix +  boost::lexical_cast<std::string>(nodeIndex++));
  }

  NodeContainer providers;
  providers.Add (Names::Find<Node>("Provider"));

  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Fifo","MaxSize", "1");

  ndnHelper.Install (dataStreamersA);
  ndnHelper.Install (providers);
  ndnHelper.Install (routers);

  ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::OMCCRF>(routers,"/");
  ns3::ndn::StrategyChoiceHelper::Install<nfd::fw::OMCCRF>(dataStreamersA,"/");

  // Install NDN applications
  //install data consumers
  ns3::ndn::AppHelper consumerDataHelper ("ns3::ndn::ConsumerCbr");
  consumerDataHelper.SetAttribute ("Frequency", StringValue ("280")); //5 Mbit with 2048 byte large chunks
  consumerDataHelper.SetAttribute ("Randomize", StringValue ("uniform"));
  consumerDataHelper.SetAttribute ("LifeTime", StringValue("2s"));

  for(int i=0; i<dataStreamersA.size (); i++)
  {
    if(useRequestAggregation.compare("true") == 0)
      consumerDataHelper.SetPrefix(std::string("/data/A"));
    else
      consumerDataHelper.SetPrefix(std::string("/data/A/"+boost::lexical_cast<std::string>(i)));

    ApplicationContainer consumer = consumerDataHelper.Install (dataStreamersA.Get (i));
    consumer.Start (Seconds(r->GetInteger (0,1)));
    consumer.Stop (Seconds(simTime));

    ns3::ndn::L3RateTracer::Install (dataStreamersA.Get (i), std::string(outputFolder + "/datastreamer-aggregate-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"), Seconds (simTime));
    ns3::ndn::AppDelayTracer::Install(dataStreamersA.Get (i),std::string(outputFolder +"/datastreamer-app-delays-trace_"  + boost::lexical_cast<std::string>(i)).append(".txt"));
  }

  // Installing global routing interface on all nodes
  ns3::ndn::ExtendedGlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  ns3::ndn::CsTracer::Install(routers, std::string(outputFolder + "/cs-trace.txt"), Seconds(1.0));

  ns3::ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  Ptr<Node> dataSrc = Names::Find<Node>("Provider");
  producerHelper.SetAttribute ("PayloadSize", StringValue("2048"));
  std::string pref = "/data";
  producerHelper.SetPrefix (pref);
  producerHelper.Install (dataSrc);
  ndnGlobalRoutingHelper.AddOrigins(pref, dataSrc);

  // Calculate and install FIBs
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes ();

  //clean up the simulation
  Simulator::Stop (Seconds(simTime+0.1)); //runs for 10 min.
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_UNCOND("Simulation completed!");
  return 0;
}

