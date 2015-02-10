#include "safengine.h"

using namespace nfd;
using namespace nfd::fw;

SAFEngine::SAFEngine(const FaceTable& table, unsigned int prefixComponentNumber)
{
  initFaces(table);
  this->prefixComponentNumber = prefixComponentNumber;
}

void SAFEngine::initFaces(const nfd::FaceTable& table)
{
  faces.clear ();
  faces.push_back (DROP_FACE_ID);

  //fprintf(stderr, "tabSize = %d\n",table.size ());

  for(nfd::FaceTable::const_iterator it = table.begin (); it != table.end (); ++it)
    faces.push_back((*it)->getId());

  std::sort(faces.begin(), faces.end());
}

int SAFEngine::determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces, shared_ptr<fib::Entry> fibEntry)
{
  //check if content prefix has been seen
  std::string prefix = extractContentPrefix(interest.getName());

  if(entryMap.find(prefix) == entryMap.end ())
  {
    entryMap[prefix] = boost::shared_ptr<SAFEntry>(new SAFEntry(faces, fibEntry));
    /* maybe add token bucket here later*/
  }

  boost::shared_ptr<SAFEntry> entry = entryMap.find(prefix)->second;
  return entry->determineNextHop(interest, originInFaces, alreadyTriedFaces);
}

std::string SAFEngine::extractContentPrefix(nfd::Name name)
{
  std::string prefix = "";
  for(int i=0; i <= prefixComponentNumber; i++)
  {
    prefix.append ("/");
    prefix.append (name.get (i).toUri ());
  }
  return prefix;
}
