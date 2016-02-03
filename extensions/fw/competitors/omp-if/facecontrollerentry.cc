#include "facecontrollerentry.h"

using namespace nfd;
using namespace nfd::fw;

FaceControllerEntry::FaceControllerEntry(std::string prefix)
{
  this->prefix = prefix;
}

FaceControllerEntry::FaceControllerEntry(const FaceControllerEntry &other)
{
  this->prefix = other.prefix;
  this->map = other.map;
}

FaceControllerEntry FaceControllerEntry::operator=(const FaceControllerEntry& other)
{
  this->prefix = other.prefix;
  this->map = other.map;

  return *this;
}

std::string FaceControllerEntry::getPrefix()
{
  return prefix;
}

int FaceControllerEntry::determineOutFace (int inFace_id, double rvalue)
{
  if (map.size () == 1)
  {
    return map.begin ()->first;
  }
  else if(map.size () > 0)
  {
    int64_t w_sum;
    for(GoodFaceMap::iterator it = map.begin (); it != map.end (); it++)
    {
      w_sum+=it->second.GetMilliSeconds (); //sum weights
    }

    double sum = 0.0;
    for(GoodFaceMap::iterator it = map.begin (); it != map.end (); it++)
    {
      if(rvalue <= ((double) it->second.GetMilliSeconds ()) / (double) (w_sum)) //inverse transfrom sampling on normalized values
      {
        return it->first;
      }
    }
  }

  return DROP_FACE_ID;
}

void FaceControllerEntry::expiredInterest(int face_id)
{
  GoodFaceMap::iterator it = map.find (face_id);
  if(it != map.end ())
  {
    map.erase (it);
  }
}

void FaceControllerEntry::satisfiedInterest(int face_id, ns3::Time delay)
{
  GoodFaceMap::iterator it = map.find (face_id);
  if(it != map.end ())
  {
    //there is no averaging mechanism suggested in the paper, thus we use an exponential moving average
    map[face_id] = 0.95*map[face_id] + 0.05*delay;
  }
}

void FaceControllerEntry::addAlternativeGoodFace(int face_id)
{
  //check if face is known
  if(map.find (face_id) != map.end ())
    return; //already known

  //else add alternative path with some default rtt-delay
  //we can not estimate the delay, and the paper does not provide a default value so we just take 2 sec as default!
  map[face_id] = ns3::Time(DEFAULT_DELAY);
}

void FaceControllerEntry::addGoodFace (int face_id, ns3::Time delay)
{
  map[face_id] = delay;
}
