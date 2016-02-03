#include "ompif.h"

using namespace nfd;
using namespace nfd::fw;

const Name OMPIF::STRATEGY_NAME("ndn:/localhost/nfd/strategy/ompif");

OMPIF::OMPIF(Forwarder &forwarder, const Name &name) : Strategy(forwarder, name)
{
  prefixComponents = ParameterConfiguration::getInstance ()->getParameter ("PREFIX_COMPONENT");
  type = OMPIFType::Invalid;
}

OMPIF::~OMPIF()
{
}

void OMPIF::afterReceiveInterest(const Face& inFace, const Interest& interest ,shared_ptr<fib::Entry> fibEntry, shared_ptr<pit::Entry> pitEntry)
{
  if(!fibEntry->hasNextHops()) // check if nexthop(s) exist(s)
  {
    //fprintf(stderr, "No next hop for prefix!\n");
    //rejectPendingInterest(pitEntry); this would create a NACK OMPIF does not use them, so just drop.
    return;
  }

  std::string prefix = extractContentPrefix(pitEntry->getInterest().getName());

  //lets start with OMPIF strategy here
  if(fMap.find (prefix) != fMap.end ()) //
  {
    boost::shared_ptr<FaceControllerEntry> entry = fMap[prefix];

    double rvalue = randomVariable.GetValue ();
    int nextHop = entry->determineOutFace(inFace.getId (),rvalue);
    if(nextHop != DROP_FACE_ID)
    {
      DelayFaceMap dMap;
      dMap[nextHop] = ns3::Simulator::Now ();
      pitMap[pitEntry]=dMap;
      sendInterest(pitEntry, getFaceTable ().get (nextHop));
      return;
    }
    //else continue with broadcast
  }

  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  DelayFaceMap dMap;

   for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it)
   {
     shared_ptr<Face> outFace = it->getFace();
     if (pitEntry->canForwardTo(*outFace))
     {
       dMap[outFace->getId()] = ns3::Simulator::Now ();
        this->sendInterest(pitEntry, outFace);
     }
   }

   if(pitEntry->hasUnexpiredOutRecords())
     pitMap[pitEntry]=dMap; //only store in measurement map if forwarding was possible
}

void OMPIF::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,const Face& inFace, const Data& data)
{
  PitMap::iterator pit = pitMap.find (pitEntry);
  if(pit == pitMap.end ())
  {
    return; //due to late straggle timer of ndnsim forwarder, just return in this case
  }

  DelayFaceMap dmap = pitMap.find (pitEntry)->second;
  for(DelayFaceMap::iterator it = dmap.begin (); it != dmap.end (); it++)
  {
    if(it->first == inFace.getId ())
    {
      std::string prefix = extractContentPrefix(data.getName());
      if(fMap.find (prefix) != fMap.end ())
      {
        fMap[prefix]->satisfiedInterest(inFace.getId (), ns3::Simulator::Now ()-it->second);
      }
      else
      {
        fMap[prefix] = boost::shared_ptr<FaceControllerEntry>(new FaceControllerEntry(prefix));
        fMap[prefix]->addGoodFace(inFace.getId (), ns3::Simulator::Now () - it->second);
      }
      break;
    }
    //fprintf(stderr,"beforeSatisfyInterest face not found in delay map\n");
  }

  pitMap.erase (pit);
  Strategy::beforeSatisfyInterest(pitEntry,inFace,data);
}

void OMPIF::beforeExpirePendingInterest(shared_ptr< pit::Entry > pitEntry)
{
  std::string prefix = extractContentPrefix(pitEntry->getName());

  if(fMap.find (prefix) != fMap.end ()) //
  {
    const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();
    for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
    {
      fMap[prefix]->expiredInterest((*it).getFace()->getId());
    }
  }

  PitMap::iterator it = pitMap.find (pitEntry);
  if(it != pitMap.end ())
    pitMap.erase (it);

  Strategy::beforeExpirePendingInterest(pitEntry);
}

void OMPIF::onUnsolicitedData(const Face& inFace, const Data& data)
{
  std::string prefix = extractContentPrefix(data.getName());
  if(type == OMPIFType::Client)
  {
    //check if data prefix is known
    if(fMap.find (prefix) != fMap.end ()) //
    {
      fMap[prefix]->addAlternativeGoodFace(inFace.getId ());
    }
  } // router does not use multiple faces to ensure node disjointnes

  Strategy::onUnsolicitedData (inFace,data);
}

std::string OMPIF::extractContentPrefix(nfd::Name name)
{
  std::string prefix = "";
  for(int i=0; i <= prefixComponents; i++)
  {
    prefix.append ("/");
    prefix.append (name.get (i).toUri ());
  }
  return prefix;
}

std::vector<int> OMPIF::getAllOutFaces(shared_ptr<pit::Entry> pitEntry)
{
  std::vector<int> faces;
  const nfd::pit::OutRecordCollection records = pitEntry->getOutRecords();

  for(nfd::pit::OutRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
    faces.push_back((*it).getFace()->getId());

  return faces;
}

std::vector<int> OMPIF::getAllInFaces(shared_ptr<pit::Entry> pitEntry)
{
  std::vector<int> faces;
  const nfd::pit::InRecordCollection records = pitEntry->getInRecords();

  for(nfd::pit::InRecordCollection::const_iterator it = records.begin (); it!=records.end (); ++it)
  {
    if(! (*it).getFace()->isLocal())
      faces.push_back((*it).getFace()->getId());
  }
  return faces;
}
