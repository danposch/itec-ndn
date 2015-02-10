#ifndef SAFENGINE_H
#define SAFENGINE_H

#include "fw/face-table.hpp"

#include <vector>

#include "safentry.h"

namespace nfd
{
namespace fw
{

class SAFEngine
{
public:
  SAFEngine(const nfd::FaceTable& table, unsigned int prefixComponentNumber);

  int determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces);

protected:
  void initFaces(const nfd::FaceTable& table);
  std::string extractContentPrefix(nfd::Name name);
  std::vector<int> faces;

  typedef std::map
    < std::string, /*content-prefix*/
      boost::shared_ptr<SAFEntry> /*forwarding prob. table*/
    > SAFEntryMap;

  SAFEntryMap entryMap;

  unsigned int prefixComponentNumber;
};


}
}
#endif // SAFENGINE_H
