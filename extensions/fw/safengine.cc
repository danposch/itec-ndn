#include "safengine.h"

using namespace nfd;
using namespace nfd::fw;

SAFEngine::SAFEngine(const FaceTable& table, unsigned int prefixComponentNumber)
{
  initFaces(table);
  this->prefixComponentNumber = prefixComponentNumber;

  updateEventFWT = ns3::Simulator::Schedule(
        ns3::Seconds(ParameterConfiguration::getInstance ()->getParameter ("UPDATE_INTERVALL")), &SAFEngine::update, this);
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

void SAFEngine::update ()
{
  //NS_LOG_DEBUG("FWT UPDATE at SimTime " << Simulator::Now ().GetSeconds () << " for node: '" <<   Names::FindName(node) << "'\n");
  for(SAFEntryMap::iterator it = entryMap.begin (); it != entryMap.end (); ++it)
  {
    it->second->update();
  }

  updateEventFWT = ns3::Simulator::Schedule(
        ns3::Seconds(ParameterConfiguration::getInstance ()->getParameter ("UPDATE_INTERVALL")), &SAFEngine::update, this);
}

void SAFEngine::logSatisfiedInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  std::string prefix = extractContentPrefix(pitEntry->getName());
  SAFEntryMap::iterator it = entryMap.find (prefix);
  if(it == entryMap.end ())
    fprintf(stderr,"Error in SAFEntryLookUp");
  else
    it->second->logSatisfiedInterest(pitEntry,inFace,data);
}

void SAFEngine::logExpiredInterest(shared_ptr< pit::Entry > pitEntry)
{
  std::string prefix = extractContentPrefix(pitEntry->getName());
  SAFEntryMap::iterator it = entryMap.find (prefix);
  if(it == entryMap.end ())
    fprintf(stderr,"Error in SAFEntryLookUp");
  else
    it->second->logExpiredInterest(pitEntry);
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
