#ifndef NETWORKGENERATOR_H
#define NETWORKGENERATOR_H

#include "ndnbritehelper.h"
#include "map"
#include "ns3/random-variable-stream.h"
#include "ns3/point-to-point-module.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "helper/ndn-link-control-helper.hpp"

#include "ns3/names.h"
#include "ns3/log.h"
#include "boost/lexical_cast.hpp"

namespace ns3
{
namespace ndn
{

class NetworkGenerator
{
public:

  enum NodePlacement
  {
    LeafNode,
    ASNode
  };

  /**
   * @brief NetworkGenerator generates a random network using BRITE.
   * @param conf_file the path to a BRITE configuration file.
   */
  NetworkGenerator(std::string conf_file);

  /**
   * @brief NetworkGenerator generates a random network using BRITE.
   * @param conf_file the path to a BRITE configuration file.
   * @param seed_file
   * @param newseed_file
   */
  NetworkGenerator(std::string conf_file,
                   std::string seed_file,
                   std::string newseed_file);


  /**
   * @brief randomlyPlaceNodes adds nodes randomly to the network topology.
   * @param nodeCount the number of nodes to add.
   * @param setIdentifier the identifier the nodes carry. Can be used later to retrieve only those nodes.
   * Attention: this identifer should be unique.
   * @param place defines whether to place a nodes at a LeafNode or at a non LeafNode (ASNode).
   * Note that this classification is not updated when new nodes are entered. This includes only nodes
   * specified by the BRITE config file.
   * @param p2p specifies the connection between the nodes in terms of delay, bandwith, etc.
   */
  void randomlyPlaceNodes(int nodeCount, std::string setIdentifier, NodePlacement place, PointToPointHelper *p2p);

  /**
   * @brief randomlyPlaceNodes adds nodes randomly to the network topology.
   * @param nodeCount the number of nodes to add.
   * @param setIdentifier the identifier the nodes carry. Can be used later to retrieve only those nodes.
   * Attention: this identifer should be unique.
   * @param place defines whether to place a nodes at a LeafNode or at a non LeafNode (ASNode).
   * Note that this classification is not updated when new nodes are entered. This includes only nodes
   * specified by the BRITE config file.
   * @param p2p specifies the connection between the nodes in terms of delay, bandwith, etc.
   */
  void randomlyPlaceNodes(int nodeCount, std::string setIdentifier, NodePlacement place, ns3::PointToPointHelper *p2p, std::vector<int> ASnumbers);

  /**
   * @brief randomlyAddConnectionsBetweenAllAS adds connections between each AS pair with randomized ressources.
   * @param numberOfConnectionsPerAsPair number of connections between each AS pair.
   * @param minBW_kbits max link capacity for a connection.
   * @param maxBw_kbits min link capacity for a connection.
   * @param minDelay min delay for a connection.
   * @param maxDelay max delay for a connection.
   */
  void randomlyAddConnectionsBetweenAllAS(int numberOfConnectionsPerAsPair, int minBW_kbits, int maxBw_kbits, int minDelay, int maxDelay);

  /**
   * @brief randomlyAddConnectionsBetweenTwoAS adds connections between two ASs.
   * @param numberOfConnectionsPerAsPair number of connections between each AS pair.
   * @param minBW_kbits max link capacity for a connection.
   * @param maxBw_kbits min link capacity for a connection.
   * @param minDelay min delay for a connection.
   * @param maxDelay max delay for a connection.
   */
  void randomlyAddConnectionsBetweenTwoAS(int numberOfConnectionsPerAsPair, int minBW_kbits, int maxBw_kbits, int minDelay, int maxDelay);

  /**
   * @brief randomlyAddConnectionsBetweenTwoNodesPerAS adds connections between two nodes within a AS.
   * @param numberOfConnectionsPerAsPair number of additional connections added.
   * @param minBW_kbits max link capacity for a connection.
   * @param maxBw_kbits min link capacity for a connection.
   * @param minDelay min delay for a connection.
   * @param maxDelay max delay for a connection.
   */
  void randomlyAddConnectionsBetweenTwoNodesPerAS(int numberOfConnectionsPerAs, int minBW_kbits, int maxBw_kbits, int minDelay_ms, int maxDelay_ms);

  /**
   * @brief calculateConnectivity calculates the graph connectivity.
   * @return
   */
  double calculateConnectivity();

  int getNumberOfAS();
  int getNumberOfNodesInAS(int ASnumber);

  /**
   * @brief getAllASNodes
   * @return all nodes in of the current topology that are specified by the BRITE config file.
   */
  ns3::NodeContainer getAllASNodes();

  /**
   * @brief getAllASNodesFromAS
   * @param ASnumber
   * @return all nodes of single AS that are specified by the BRITE config file.
   */
  ns3::NodeContainer getAllASNodesFromAS(int ASnumber);

  /**
   * @brief getAllLeafNodes
   * @return all LeafNodes that are specified by the BRITE config file. Note that the intersection of LeafNodes and ASNodes is not NULL!
   */
  ns3::NodeContainer getAllLeafNodes();

  /**
   * @brief getAllLeafNodesFromAS
   * @param ASnumber
   * @return all LeafNodes for the current AS that are specified by the BRITE config file. Note that the intersection of LeafNodes and ASNodes is not NULL!
   */
  ns3::NodeContainer getAllLeafNodesFromAS(int ASnumber);

  /**
   * @brief getCustomNodes
   * @param setIdentifier the identifier used to create the nodes
   * @return returns all randomly added nodes with a given identifier.
   */
  ns3::NodeContainer getCustomNodes(std::string setIdentifier);

  /**
   * @brief creatRandomLinkFailure creates a random linkfailure between a given node
   * specified in the BRITE config file and a noded connected to it.
   * @param minTimestamp the erliest time when the link failure occurs.
   * @param maxTimestamp the latest time when the link failure occurs.
   * @param minDuration the min duration of a link failure
   * @param maxDuration the max duration of a link failure
   */
  void creatRandomLinkFailure(double minTimestamp, double maxTimestamp, double minDuration, double maxDuration);

  void exportTopology(std::string fname, std::string server_identifier = std::string(""), std::string client_identifier = std::string(""));


protected:
  NDNBriteHelper *briteHelper;

  bool nodesConnected(Ptr<Node> n1, Ptr<Node> n2);
  uint64_t getBandwidth(Ptr<Node> n1, Ptr<Node> n2);
  NodeContainer getPairOfUnconnectedNodes(int as1, int as2);
  NodeContainer removeNode(NodeContainer container, Ptr<Node> node);

  typedef
  std::map<
  std::string /*label*/,
  ns3::NodeContainer/*nodes*/
  > CustomNodesMap;

  CustomNodesMap nodeContainerMap;
  ns3::Ptr<ns3::UniformRandomVariable> rvariable;
};
}
}
#endif // NETWORKGENERATOR_H
