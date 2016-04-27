#include "adaptationengine.h"

using namespace nfd;
using namespace nfd::fw;


AdaptationEngine::AdaptationEngine(std::vector<int> faces)
{
  updateFaces (faces);
}

void AdaptationEngine::registerSMeasure(std::string prefix, boost::shared_ptr<SAFStatisticMeasure> smeasure, boost::shared_ptr<SAFForwardingTable> ftable)
{
  smap[prefix].smeasure = smeasure;
  smap[prefix].ftable = ftable;
}

void AdaptationEngine::setAndUpdateWeights()
{
  //fprintf(stderr, "Set and update weights\n");

  //first get for all faces, all reliable contents ("prefixes")

  for(std::vector<int>::iterator it = faces.begin (); it != faces.end (); it++)
  {
    if(*it == -1) //skip dropping face
      continue;

    std::vector<std::string> relevant, irrelevant;

    //get reliable contents for face: *it:
    for(SMeasureMap::iterator sit = smap.begin (); sit!=smap.end (); sit++)
    {
      //we need content that has U > 0 and S > 0
      if( (sit->second.smeasure->getU(*it, 0) <= 0) || (sit->second.smeasure->getS(*it, 0) <= 0) ) //we only consider layer 0
      {
        irrelevant.push_back (sit->first);
        continue;
      }

      //we need content /face that is reliable with respecct to the current reliability threashold
      double face_r = sit->second.smeasure->getFaceReliability(*it, 0); //we only consider layer 0
      if(sit->second.ftable->getCurrentReliability()[0] < face_r)  //we only consider layer 0
      {
        irrelevant.push_back (sit->first);
        continue;
      }

      //add to relevant contents
      relevant.push_back (sit->first);
    }

    // now we can cal the weights
    if(relevant.size () > 1)
    {
      relevant = sortRelevantsByPriority(relevant);
      double* wl = new double[relevant.size ()];
      double* wu = new double[relevant.size ()];
      double* w = new double[relevant.size ()];

      wl[0] = 1;
      for(int j = 0; j < relevant.size () - 1; j++)
      {
        wl[j+1] = std::max(
                          ((double)(smap[relevant[j+1]].smeasure->getS(*it,0) * smap[relevant[j]].smeasure->getU(*it,0)*wl[j]))
                          /
                          ((double)(smap[relevant[j]].smeasure->getS(*it,0) *  smap[relevant[j+1]].smeasure->getU(*it,0)))
                          ,
                          ((double)(smap[relevant[j+1]].smeasure->getS(*it,0) - smap[relevant[j+1]].smeasure->getS(*it,0)* smap[relevant[j+1]].ftable->getCurrentReliability()[0]))
                          /
                          ((double)(smap[relevant[j+1]].smeasure->getU(*it,0)* smap[relevant[j+1]].ftable->getCurrentReliability()[0]))
                          );
      }

      wu[relevant.size ()-1] = wl[relevant.size ()-1] + 1;
      for(int j=relevant.size ()-1; j > 1; j--)
      {
        wu[j-1] = ((double)(smap[relevant[j-1]].smeasure->getS(*it,0) * smap[relevant[j]].smeasure->getU(*it,0)*wu[j]))
                  /
                  ((double)(smap[relevant[j]].smeasure->getS(*it,0) *  smap[relevant[j-1]].smeasure->getU(*it,0)));
      }

      w[0] = wl[0];
      w[relevant.size ()-1] = wu[relevant.size ()-1];

      for(int i = 1; i < relevant.size () - 1; i++)
        w[i] = (wl[i] + wu[i]) / 2;


      //debug check
      /*for(int i = 0; i < relevant.size (); i++)
        fprintf(stderr, "R[%d][%s]=%f      w[%f]\n", *it, relevant[i].c_str(),
                ((double)(smap[relevant[i]].smeasure->getS(*it,0)))/(smap[relevant[i]].smeasure->getS(*it,0) + (smap[relevant[i]].smeasure->getU(*it,0)*w[i]))
                , w[i]);
      fprintf(stderr, "\n");*/

      //set new weights
      for(int i = 0; i < relevant.size (); i++)
      {
        smap[relevant[i]].smeasure->setUnsatisfiedWeight(*it, 0, w[i]);
      }

      delete[] wl;
      delete[] wu;
      delete[] w;
    }
    else // only 1 relevant content
    {
      irrelevant.insert (irrelevant.end (), relevant.begin (), relevant.end ());
    }

    // Set weights for irrelevant contents to 1.0 ?
    for(int i = 0; i < irrelevant.size (); i++)
    {
      smap[irrelevant[i]].smeasure->setUnsatisfiedWeight(*it, 0, 1.0);
    }
  }
}

std::vector<std::string> AdaptationEngine::sortRelevantsByPriority(std::vector<std::string> vec)
{
  std::vector<std::string> result;

  /*for (auto i = vec.begin(); i != vec.end(); ++i)
      std::cout << *i << ' ';
  std::cout << std::endl;*/

  //todo implent something more sophisticated here:
  if (std::find(vec.begin(), vec.end(), "/voip") != vec.end())
  {
    result.push_back ("/voip");
  }

  if (std::find(vec.begin(), vec.end(), "/video") != vec.end())
  {
    result.push_back ("/video");
  }

  if (std::find(vec.begin(), vec.end(), "/data") != vec.end())
  {
    result.push_back ("/data");
  }

  //default keep current order
  for(std::vector<std::string>::iterator it = vec.begin (); it != vec.end (); it ++)
  {
    if (std::find(result.begin(), result.end(), *it) == result.end())
      result.push_back (*it);
  }

  /*for (auto i = result.begin(); i != result.end(); ++i)
      std::cout << *i << ' ';
  std::cout << std::endl;
  std::cout << std::endl;*/

  return result;
}

void AdaptationEngine::updateFaces (std::vector<int> faces)
{
  this->faces = faces;
}
