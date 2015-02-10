#ifndef SAFFORWARDINGTABLE_H
#define SAFFORWARDINGTABLE_H

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include "ns3/random-variable.h"

#include "../utils/parameterconfiguration.h"
#include "safstatisticmeasure.h"

#include "fw/face-table.hpp"
#include "iostream"

namespace nfd
{
namespace fw
{

class SAFForwardingTable
{
public:
  SAFForwardingTable(std::vector<int> faceIds, std::vector<int> preferedFacesIds = std::vector<int>());
  int determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces);

  void update(boost::shared_ptr<SAFStatisticMeasure> smeasure);
  double getCurrentReliability(){return this->curReliability;}

  protected:

  void initTable();
  int determineRowOfFace(int face_uid, boost::numeric::ublas::matrix<double> tab, std::vector<int> faces);
  int determineRowOfFace(int face_uid);
  boost::numeric::ublas::matrix<double> removeFaceFromTable (int faceId, boost::numeric::ublas::matrix<double> tab, std::vector<int> faces);
  boost::numeric::ublas::matrix<double> normalizeColumns(boost::numeric::ublas::matrix<double> m);

  int chooseFaceAccordingProbability(boost::numeric::ublas::matrix<double> m, int ilayer, std::vector<int> faceList);

  double calcWeightedUtilization(int faceId, int layer, boost::shared_ptr<SAFStatisticMeasure> smeasure); // u(F_i)
  double getSumOfWeightedForwardingProbabilities(std::vector<int> set_of_faces, int layer, boost::shared_ptr<SAFStatisticMeasure> smeasure);

  void updateColumn(std::vector<int> faces, int layer, boost::shared_ptr<SAFStatisticMeasure> smeasure, double utf, bool shift_traffic);
  void probeColumn(std::vector<int> faces, int layer, boost::shared_ptr<SAFStatisticMeasure> smeasure, bool useDroppingProbabilityFromFWT);
  void shiftDroppingTraffic(std::vector<int> faces, int layer, boost::shared_ptr<SAFStatisticMeasure> smeasure);

  void decreaseReliabilityThreshold();
  void increaseReliabilityThreshold();
  void updateReliabilityThreshold(bool mode);

  boost::numeric::ublas::matrix<double> table;
  std::vector<int> faces;
  std::vector<int> preferedFaces;
  double curReliability;
  ns3::UniformVariable randomVariable;
};

}
}
#endif // SAFFORWARDINGTABLE_H
