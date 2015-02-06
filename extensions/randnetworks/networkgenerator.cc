#include "networkgenerator.h"
#include "ns3/double.h"

using namespace ns3;
using namespace ns3::itec;
using namespace ns3::ndn;

NS_LOG_COMPONENT_DEFINE ("NetworkGenerator");

NetworkGenerator::NetworkGenerator(std::string conf_file)
{
  rvariable = CreateObject<UniformRandomVariable>();
  this->briteHelper = new NDNBriteHelper(conf_file);
  briteHelper->BuildBriteTopology ();

  for(int i=0; i<getAllASNodes ().size (); i++)
  {
    Names::Add (std::string("Node_" + boost::lexical_cast<std::string>(i)), getAllASNodes ().Get (i));
  }
}

void NetworkGenerator::randomlyPlaceNodes(int nodeCount, std::string setIdentifier, NodePlacement place, PointToPointHelper *p2p)
{
  std::vector<int> allAS;

  for(int i=0; i < getNumberOfAS (); i++)
    allAS.push_back (i);

  randomlyPlaceNodes(nodeCount,setIdentifier,place,p2p,allAS);
}

void NetworkGenerator::randomlyPlaceNodes (int nodeCount, std::string setIdentifier, NodePlacement place, PointToPointHelper *p2p, std::vector<int> ASnumbers)
{
  NodeContainer container;

  for(std::vector<int>::iterator it = ASnumbers.begin (); it != ASnumbers.end (); it++)
  {
    if(place == NetworkGenerator::ASNode)
    {
      container.Add (getAllASNodesFromAS(*it));
    }
    else
    {
      container.Add (getAllLeafNodesFromAS(*it));
    }
  }

  if(container.size () == 0)
  {
    NS_LOG_UNCOND("Could not place nodes, as no nodes are provided by the topology.");
    return;
  }

  NodeContainer customNodes;
  customNodes.Create (nodeCount);

  for(int i=0; i<customNodes.size (); i++)
  {
    Names::Add (std::string(setIdentifier + "_" + boost::lexical_cast<std::string>(i)), customNodes.Get (i));

    int rand = rvariable->GetInteger (0,container.size ()-1);
    p2p->Install (customNodes.Get (i), container.Get (rand));
  }
  nodeContainerMap[setIdentifier] = customNodes;
}

void NetworkGenerator::randomlyAddConnectionsBetweenAllAS(int numberOfConnectionsPerAsPair, int minBW_kbits, int maxBw_kbits, int minDelay_ms, int maxDelay_ms)
{
  PointToPointHelper p2p;

  for(int i = 0; i<getNumberOfAS (); i++)
  {
    int j = i+1;
    while(j < getNumberOfAS ())
    {
      for(int c = 0; c < numberOfConnectionsPerAsPair; c++)
      {
        std::string delay(boost::lexical_cast<std::string>(rvariable->GetValue (minDelay_ms,maxDelay_ms)));
        delay.append ("ms");

        std::string bw(boost::lexical_cast<std::string>(rvariable->GetValue (minBW_kbits,maxBw_kbits)));
        bw.append ("Kbps");

        p2p.SetDeviceAttribute ("DataRate", ns3::StringValue (bw));
        p2p.SetChannelAttribute ("Delay", ns3::StringValue (delay));

        int rand_node_i = rvariable->GetInteger (0,getAllASNodesFromAS (i).size ()-1);
        int rand_node_j = rvariable->GetInteger (0,getAllASNodesFromAS (j).size ()-1);
        p2p.Install (getAllASNodesFromAS (i).Get (rand_node_i), getAllASNodesFromAS (j).Get (rand_node_j));
      }
      j++;
    }
  }
}

void NetworkGenerator::randomlyAddConnectionsBetweenTwoAS(int numberOfConnectionsPairs, int minBW_kbits, int maxBw_kbits, int minDelay_ms, int maxDelay_ms)
{
  if(getNumberOfAS() <= 1)
  {
    NS_LOG_UNCOND("Error, at least two AS have to exists to add Connections between ASs!");
    return;
  }

  PointToPointHelper p2p;

  for(int i = 0; i < numberOfConnectionsPairs; i++)
  {
    std::string delay(boost::lexical_cast<std::string>(rvariable->GetValue (minDelay_ms,maxDelay_ms)));
    delay.append ("ms");

    std::string bw(boost::lexical_cast<std::string>(rvariable->GetValue (minBW_kbits,maxBw_kbits)));
    bw.append ("Kbps");

    p2p.SetDeviceAttribute ("DataRate", ns3::StringValue (bw));
    p2p.SetChannelAttribute ("Delay", ns3::StringValue (delay));

    int number_as1 = rvariable->GetInteger (0,getNumberOfAS()-1);
    int number_as2 = number_as1;

    while(number_as2 == number_as1)
      number_as2 = rvariable->GetInteger (0,getNumberOfAS()-1);

    int rand_node_1 = rvariable->GetInteger (0,getAllASNodesFromAS (number_as1).size ()-1);
    int rand_node_2 = rvariable->GetInteger (0,getAllASNodesFromAS (number_as2).size ()-1);

    p2p.Install (getAllASNodesFromAS (number_as1).Get (rand_node_1), getAllASNodesFromAS (number_as2).Get (rand_node_2));
  }
}

void NetworkGenerator::randomlyAddConnectionsBetweenTwoNodesPerAS(int numberOfConnectionsPerAs, int minBW_kbits, int maxBw_kbits, int minDelay_ms, int maxDelay_ms)
{
  PointToPointHelper p2p;

  for(int as = 0; as < getNumberOfAS (); as++)
  {
    for(int i = 0; i < numberOfConnectionsPerAs; i++)
    {
      std::string delay(boost::lexical_cast<std::string>(rvariable->GetValue (minDelay_ms,maxDelay_ms)));
      delay.append ("ms");

      std::string bw(boost::lexical_cast<std::string>(rvariable->GetValue (minBW_kbits,maxBw_kbits)));
      bw.append ("Kbps");

      p2p.SetDeviceAttribute ("DataRate", ns3::StringValue (bw));
      p2p.SetChannelAttribute ("Delay", ns3::StringValue (delay));

      int rand_node_1 = rvariable->GetInteger (0,getAllASNodesFromAS (as).size ()-1);
      int rand_node_2 = rand_node_1;

      while(rand_node_1 == rand_node_2)
        rand_node_2 = rvariable->GetInteger (0,getAllASNodesFromAS (as).size ()-1);

      p2p.Install (getAllASNodesFromAS (as).Get (rand_node_1), getAllASNodesFromAS (as).Get (rand_node_2));
    }
  }
}

int NetworkGenerator::getNumberOfAS ()
{
  return briteHelper->GetNAs ();
}

int NetworkGenerator::getNumberOfNodesInAS (int ASnumber)
{
  if(getNumberOfAS () < ASnumber)
  {
    return briteHelper->GetNNodesForAs (ASnumber);
  }
  return 0;
}

ns3::NodeContainer NetworkGenerator::getAllASNodes()
{
  NodeContainer container;

  for(int as=0; as < getNumberOfAS (); as++)
  {
    container.Add (getAllASNodesFromAS(as));
  }
  return container;
}

ns3::NodeContainer NetworkGenerator::getAllASNodesFromAS(int ASnumber)
{
  NodeContainer container;

  if(getNumberOfAS () < ASnumber)
    return container;

  for(int node=0; node < briteHelper->GetNNodesForAs(ASnumber); node++)
  {
    container.Add (briteHelper->GetNodeForAs (ASnumber,node));
  }

  return container;
}

ns3::NodeContainer NetworkGenerator::getAllLeafNodes()
{
  NodeContainer container;

  for(int as=0; as < getNumberOfAS (); as++)
  {
    container.Add (getAllLeafNodesFromAS(as));
  }

  return container;
}

ns3::NodeContainer NetworkGenerator::getAllLeafNodesFromAS(int ASnumber)
{
  NodeContainer container;

  if(getNumberOfAS () < ASnumber)
    return container;

  for(int node=0; node < briteHelper->GetNLeafNodesForAs (ASnumber); node++)
  {
    container.Add (briteHelper->GetLeafNodeForAs(ASnumber,node));
  }
  return container;
}

ns3::NodeContainer NetworkGenerator::getCustomNodes(std::string setIdentifier)
{
  return nodeContainerMap[setIdentifier];
}

void NetworkGenerator::creatRandomLinkFailure(double minTimestamp, double maxTimestamp, double minDuration, double maxDuration)
{
  int rand = rvariable->GetInteger(0,getNumberOfAS() - 1);

  NodeContainer c = getAllASNodesFromAS(rand);
  rand = rvariable->GetInteger (0, c.size ()-1);

  Ptr<Node> node = c.Get (rand);

  rand = rvariable->GetInteger (0,node->GetNDevices ()-1);
  Ptr<Channel> channel = node->GetDevice (rand)->GetChannel ();

  NodeContainer channelNodes;

  for(int i = 0; i < channel->GetNDevices (); i++)
  {
    Ptr<NetDevice> dev = channel->GetDevice (i);
    channelNodes.Add (dev->GetNode ());
  }

  if(channelNodes.size () != 2)
    NS_LOG_ERROR("Invalid Channel with more than 2 nodes...");
  else
  {
    double startTime = rvariable->GetValue (minTimestamp, maxTimestamp);
    double stopTime = startTime + rvariable->GetValue (minDuration, maxDuration);

    Simulator::Schedule (Seconds (startTime), ndn::LinkControlHelper::FailLink, channelNodes.Get (0), channelNodes.Get (1));
    Simulator::Schedule (Seconds (stopTime), ndn::LinkControlHelper::UpLink,   channelNodes.Get (0), channelNodes.Get (1));

    //fprintf(stderr, "Start LinkFail between %s and %s: %f\n",Names::FindName (channelNodes.Get (0)).c_str (),Names::FindName (channelNodes.Get (1)).c_str (), startTime);
    //fprintf(stderr, "Stop LinkFail between %s and %s: %f\n\n",Names::FindName (channelNodes.Get (0)).c_str (),Names::FindName (channelNodes.Get (1)).c_str (),stopTime);
  }
}

double NetworkGenerator::calculateConnectivity ()
{
  NodeContainer allNodes;
  allNodes.Add (getAllASNodes ());

  Ptr<Node> n;
  double connectivity = 0.0;

  for(int i = 0; i < allNodes.size (); i++)
  {
    n = allNodes.Get (i);
    connectivity += n->GetNDevices (); // degree summation
  }

  connectivity /= allNodes.size ();
  connectivity /= (allNodes.size () - 1);

  return connectivity;
}
