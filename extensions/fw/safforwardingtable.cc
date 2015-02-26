#include "safforwardingtable.h"

using namespace nfd;
using namespace nfd::fw;
using namespace boost::numeric::ublas;

NS_LOG_COMPONENT_DEFINE("SAFForwardingTable");

SAFForwardingTable::SAFForwardingTable(std::vector<int> faceIds, std::map<int, int> preferedFacesIds)
{
  for(int i = 0; i < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); i++)
    curReliability[i]=ParameterConfiguration::getInstance ()->getParameter ("RELIABILITY_THRESHOLD_MAX");

  this->faces = faceIds;
  this->preferedFaces = preferedFacesIds;
  initTable ();
}

std::map<int, double> SAFForwardingTable::calcInitForwardingProb(std::map<int, int> preferedFacesIds, double gamma)
{
  std::map<int, double> res;
  double sum = 0;

  //use gamma to modify the weight.
  for(std::map<int,int>::iterator it = preferedFacesIds.begin (); it != preferedFacesIds.end (); it++)
    res[it->first] = pow((double)it->second, gamma);

  //calc the sum of the values
  for(std::map<int,double>::iterator it = res.begin (); it != res.end (); it++)
    sum += it->second;

  //divide each value by the sum
  for(std::map<int,double>::iterator it = res.begin (); it != res.end (); it++)
    res[it->first] = sum / (double) it->second;

  sum = 0;
  //calc the normalize factor
  for(std::map<int,double>::iterator it = res.begin (); it != res.end (); it++)
    sum += it->second;

  //normalize
  for(std::map<int,double>::iterator it = res.begin (); it != res.end (); it++)
    res[it->first] = it->second / sum;

  return res;
}

void SAFForwardingTable::initTable ()
{
  std::sort(faces.begin(), faces.end());//order

  table = matrix<double> (faces.size () /*rows*/, (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS") /*columns*/);

  std::map<int, double> initValues = calcInitForwardingProb (preferedFaces, 2.0);

  // fill matrix column-wise /* table(i,j) = i-th row, j-th column*/
  for (unsigned j = 0; j < table.size2 (); ++j) /* columns */
  {
    for (unsigned i = 0; i < table.size1 (); ++i) /* rows */
    {
      if(faces.at (i) == DROP_FACE_ID)
        table(i,j) = 0.0;
      else if(initValues.size ()== 0)
      {
        table(i,j) = (1.0 / ((double)faces.size () - 1.0)); /*set default value to 1 / (d - 1) */
      }
      else
      {
        std::map<int,double>::iterator it = initValues.find(faces.at (i));
        if( it != initValues.end ())// && it->first == faceId)
        {
          table(i,j) = it->second;
          //table(i,j) = (1.0 / ((double)preferedFaces.size ())); // preferedFaces dont include the dropping face.
          //table(i,j) = 1.0;
        }
        else
        {
          table(i,j) = 0;
        }
      }
    }
  }
}

int SAFForwardingTable::determineNextHop(const Interest& interest, std::vector<int> alreadyTriedFaces)
{
  //create a copy of the table
  matrix<double> tmp_matrix(table);
  std::vector<int> face_list(faces);

  int ilayer = SAFStatisticMeasure::determineContentLayer(interest);
  //lets check if sum(Fi in alreadyTriedFaces > R)
  double fw_prob = 0.0;
  int row = -1;
  for(std::vector<int>::iterator i = alreadyTriedFaces.begin (); i != alreadyTriedFaces.end ();++i)
  {
    row = determineRowOfFace(*i, tmp_matrix, face_list);
    if(row != FACE_NOT_FOUND)
    {
      fw_prob += tmp_matrix(row,ilayer);
      //fprintf(stderr, "face %d\n has been alread tried\n, i");
    }
    else
    {
      fprintf(stderr, "Could not find tried face[i]=%d. Returning the dropping face..\n", *i);
      return DROP_FACE_ID;
    }
  }

  if(fw_prob >= curReliability[ilayer] && fw_prob != 0) // in this case we drop...
  {
    //fprintf(stderr,"fw_prob >= curReliability[ilayer] && fw_prob != 0\n"); //TODO REMOVE
    //fprintf(stderr, "fw_prob = %f\n", fw_prob);
    //fprintf(stderr, " curReliability[ilayer] = %f\n",  curReliability[ilayer]);
    return DROP_FACE_ID;
  }

  //ok now remove the alreadyTriedFaces
  int offset = 0;
  for(std::vector<int>::iterator i = alreadyTriedFaces.begin (); i != alreadyTriedFaces.end ();++i)
  {
    offset = determineRowOfFace(*i, tmp_matrix, face_list);
    if(offset != FACE_NOT_FOUND)
    {
      //then remove the row and the face from the list
      tmp_matrix = removeFaceFromTable(*i, tmp_matrix, face_list);
      face_list.erase (face_list.begin ()+offset);
    }
    else// if(*i != inFaces->GetId ())
    {
     fprintf(stderr, "Could not remove tired face[i]=%d. Returning the dropping face..\n", *i);
      return DROP_FACE_ID;
    }
  }

  // choose one face as outgoing according to the probability
  return chooseFaceAccordingProbability(tmp_matrix, ilayer, face_list);
}

void SAFForwardingTable::update(boost::shared_ptr<SAFStatisticMeasure> stats)
{
  std::vector<int> r_faces;  /*reliable faces*/
  std::vector<int> ur_faces; /*unreliable faces*/
  std::vector<int> p_faces;  /*probing faces*/

  NS_LOG_DEBUG("FWT Before Update:\n" << table); /* prints matrix line by line ( (first line), (second line) )*/

  for(int layer = 0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); layer++) // for each layer
  {
    NS_LOG_DEBUG("Updating Layer[" << layer << "] with reliability_t=" << curReliability[layer]);

    //determine the set of (un)reliable faces
    r_faces = stats->getReliableFaces (layer, curReliability[layer]);
    ur_faces = stats->getUnreliableFaces (layer, curReliability[layer]);

    //seperate reliable faces from probing faces
    for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end();)
    {
      if(stats->getForwardedInterests (*it, layer) == 0)
      {
        p_faces.push_back (*it);
        r_faces.erase (it);
      }
      else
        ++it;
    }

    for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end(); ++it)
      NS_LOG_DEBUG("Reliable Face[" << *it << "]=" << stats->getFaceReliability(*it,layer)
                   << "\t "<< stats->getForwardedInterests (*it,layer) << " interest forwarded");
    for(std::vector<int>::iterator it = ur_faces.begin(); it != ur_faces.end(); ++it)
      NS_LOG_DEBUG("Unreliable Face[" << *it << "]=" << stats->getFaceReliability(*it,layer)
                   << "\t "<< stats->getForwardedInterests (*it,layer) << " interest forwarded");
    for(std::vector<int>::iterator it = p_faces.begin(); it != p_faces.end(); ++it)
      NS_LOG_DEBUG("Probe Face[" << *it << "]=" << stats->getFaceReliability(*it,layer)
                   << "\t "<< stats->getForwardedInterests (*it,layer) << " interest forwarded");
    NS_LOG_DEBUG("Drop Face[" << DROP_FACE_ID << "]=" << stats->getFaceReliability(DROP_FACE_ID,layer)
                 << "\t "<< stats->getForwardedInterests (DROP_FACE_ID,layer) << " interest forwarded");

    // ok treat the unreliable faces first...
    double utf = table(determineRowOfFace (DROP_FACE_ID),layer);
    double utf_face = 0.0;
    for(std::vector<int>::iterator it = ur_faces.begin (); it != ur_faces.end (); it++)
    {
      utf_face=stats->getAlpha (*it, layer) * stats->getUT(*it, layer);

      if(utf_face <= table(determineRowOfFace (*it),layer))
      {
        NS_LOG_DEBUG("Face[" << *it <<"]: Removing alpha[" << *it <<"]*UT[" << *it << "]="
                 << stats->getAlpha(*it, layer) << "*" << stats->getUT(*it, layer) << "=" << utf_face);
      }
      else
      {
        utf_face = table(determineRowOfFace (*it),layer);
        NS_LOG_DEBUG("Face[" << *it <<"]: Removing all =" << utf_face);
      }

      // remove traffic and store removed fraction
      table(determineRowOfFace (*it),layer) -= utf_face;
      utf += utf_face;
    }

    NS_LOG_DEBUG("Total UTF = " << utf);

    if(utf > 0)
    {
      // try to split the utf on r_faces
      std::map<int/*faceId*/, double /*traffic that can be shifted to face*/> ts;
      double ts_sum = 0.0;
      for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end(); ++it) // for each r_face
      {
        NS_LOG_DEBUG("Face[" << *it <<"]: getS() / curReliability[*it] = "
                     << ((double)stats->getS (*it, layer)) << "/" << curReliability[layer]);
        ts[*it] = ((double)stats->getS (*it, layer)) / curReliability[layer];
        ts[*it] -= stats->getForwardedInterests (*it, layer);
        ts_sum += ts[*it];
        NS_LOG_DEBUG("Face[" << *it <<"]: still can take " <<  ts[*it] << " more Interests");
      }
      NS_LOG_DEBUG("Total Interests that can be taken by F_R = " << ts_sum);

      // find the minium fraction that can be AND should be shifted
      double min_fraction = (ts_sum / (double) stats->getTotalForwardedInterests (layer));
      if(min_fraction > utf)
        min_fraction = utf;

      NS_LOG_DEBUG("Total fraction that will be shifted to F_R= " << min_fraction);

      //now shift traffic to r_faces
      for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end(); ++it) // for each r_face
      {
        NS_LOG_DEBUG("Face[" << *it <<"]: Adding (min_fraction*ts[" << *it << "] / I) / (ts_sum / I)="
                   << "(" << min_fraction << "*" << ts[*it] << "/" << stats->getTotalForwardedInterests (layer) << ") / (" <<
                   ts_sum << "/" << stats->getTotalForwardedInterests (layer) << ")=" <<
                   (min_fraction * ts[*it] / (double) stats->getTotalForwardedInterests (layer))
                   / (ts_sum / (double) stats->getTotalForwardedInterests (layer)));

        table(determineRowOfFace (*it),layer) += (min_fraction * ts[*it] / (double) stats->getTotalForwardedInterests (layer))
            / (ts_sum / (double) stats->getTotalForwardedInterests (layer));
      }

      //lets check if something has to be shifted to the dropping face
      utf -= min_fraction;
      NS_LOG_DEBUG("UTF remaining for the Dropping Face = "<< utf);
      table(determineRowOfFace (DROP_FACE_ID), layer) = utf;
    }

    // if we still have utf we do some probing
    if(utf > 0)
    {
      probeColumn (p_faces, layer, stats);

      if(table(determineRowOfFace (DROP_FACE_ID), layer) > (1-curReliability[layer]))
        decreaseReliabilityThreshold (layer);
    }
    else
      increaseReliabilityThreshold (layer);

  }
  //finally just normalize to remove the rounding errors
  table = normalizeColumns(table);
  NS_LOG_DEBUG("FWT After Update:\n" << table); /* prints matrix line by line ( (first line), (second line) )*/
}

void SAFForwardingTable::probeColumn(std::vector<int> faces, int layer, boost::shared_ptr<SAFStatisticMeasure> stats)
{
  if(faces.size () == 0)
    return;

   //double probe = table(determineRowOfFace (DROP_FACE_ID), layer) * ParameterConfiguration::getInstance ()->getParameter ("PROBING_TRAFFIC");
  double probe = table(determineRowOfFace (DROP_FACE_ID), layer) * stats->getRho (layer);

  if(probe < 0.001) // if probe is zero return
    return;

  /*NS_LOG_DEBUG("Probing! Probe Size = p(F_D) * rho = " << table(determineRowOfFace (DROP_FACE_ID), layer)
               << "*" << ParameterConfiguration::getInstance ()->getParameter ("PROBING_TRAFFIC") << "=" << probe);*/
  NS_LOG_DEBUG("Probing! Probe Size = p(F_D) * rho = " << table(determineRowOfFace (DROP_FACE_ID), layer)
                 << "*" << stats->getRho (layer) << "=" << probe);

  //remove the probing traffic from F_D
  table(determineRowOfFace (DROP_FACE_ID), layer) -= probe;

  double normFactor = 0.0; // optimization for layers > 0
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    normFactor += table(determineRowOfFace (*it), 0);
  }

  //split the probe (forwarding probabilties)....
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
  {
    if(layer == 0 || normFactor == 0)
    {
      table(determineRowOfFace (*it), layer) += (probe / ((double)faces.size ()));
    }
    else
    {
      table(determineRowOfFace (*it), layer) += (probe * (table(determineRowOfFace (*it), 0) / normFactor));
    }
  }
}

int SAFForwardingTable::determineRowOfFace(int face_uid)
{
  return determineRowOfFace (face_uid, table, faces);
}

int SAFForwardingTable::determineRowOfFace(int face_id, boost::numeric::ublas::matrix<double> tab, std::vector<int> faces)
{
  // check if table fits to faces
  if(tab.size1 () != faces.size ())
  {
    fprintf(stderr, "Error the number of faces dont correspond to the table!\n");
    return FACE_NOT_FOUND;
  }

  //determine row of face
  int faceRow = FACE_NOT_FOUND;
  std::sort(faces.begin(), faces.end());//order

  int rowCounter = 0;
  for(std::vector<int>::iterator i = faces.begin (); i != faces.end() ; ++i)
  {
    //fprintf(stderr, "*i=%d ; face_id=%d\n",*i,face_id);
    if(*i == face_id)
    {
      faceRow = rowCounter;
      break;
    }
    rowCounter++;
  }
  return faceRow;
}

matrix<double> SAFForwardingTable::removeFaceFromTable (int faceId, matrix<double> tab, std::vector<int> faces)
{
  int faceRow = determineRowOfFace (faceId, tab, faces);

  if(faceRow == FACE_NOT_FOUND)
  {
    fprintf(stderr, "Could not remove Face from Table as it does not exist");
    return tab;
  }

  //fprintf(stderr, "facerow=%d\ņ",faceRow);
  //fprintf(stderr, "tab.size1=%d ; tab.size2=%d\n",tab.size1 (), tab.size2 ());

  matrix<double> m (tab.size1 () - 1, tab.size2 ());
  for (unsigned j = 0; j < tab.size2 (); ++j) /* columns */
  {
    for (unsigned i = 0; i < tab.size1 (); ++i) /* rows */
    {
      if(i < faceRow)
      {
        m(i,j) = tab(i,j);
      }
      /*else if(faceRow == i)
      {
        // skip i-th row.
      }*/
      else if (i > faceRow)
      {
        m(i-1,j) = tab(i,j);
      }
    }
  }
  return normalizeColumns (m);
}

matrix<double> SAFForwardingTable::normalizeColumns(matrix<double> m)
{
  for (unsigned j = 0; j < m.size2 (); ++j) /* columns */
  {
    double colSum= 0;
    for (unsigned i = 0; i < m.size1 (); ++i) /* rows */
    {
      if(m(i,j) > 0)
        colSum += m(i,j);
    }
    if(colSum == 0) // means we have removed the only face that was able to transmitt the traffic
    {
      //split probabilities
      for (unsigned i = 0; i < m.size1 (); ++i) /* rows */
        m(i,j) = 1.0 /((double)m.size1 ());
    }
    else
    {
      for (unsigned i = 0; i < m.size1 (); ++i) /* rows */
      {
        if(m(i,j) < 0)
          m(i,j) = 0;
        else
          m(i,j) /= colSum;
      }
    }
  }
  return m;
}

int SAFForwardingTable::chooseFaceAccordingProbability(matrix<double> m, int ilayer, std::vector<int> faceList)
{
  double rvalue = randomVariable.GetValue ();
  double sum = 0.0;

  if(faceList.size () != m.size1 ())
  {
    fprintf(stderr, "Error ForwardingMatrix invalid cant choose Face\n!");
    return DROP_FACE_ID;
  }

  for(int i = 0; i < m.size1 (); i++)
  {
    sum += m(i, ilayer);
    if(rvalue <= sum)
    {
      return faceList.at (i);
    }
  }
  //error case
  return DROP_FACE_ID;
}

void SAFForwardingTable::increaseReliabilityThreshold(int layer)
{
  updateReliabilityThreshold (layer, true);
}

void SAFForwardingTable::decreaseReliabilityThreshold(int layer)
{
  updateReliabilityThreshold (layer, false);
}

void SAFForwardingTable::updateReliabilityThreshold(int layer, bool increase)
{
  ParameterConfiguration *p = ParameterConfiguration::getInstance ();

  double new_t = 0.0;

  if(increase)
    new_t = curReliability[layer] + ((p->getParameter("RELIABILITY_THRESHOLD_MAX") - curReliability[layer]) * p->getParameter("ALPHA"));
  else
    new_t = curReliability[layer] - ((curReliability[layer] - p->getParameter("RELIABILITY_THRESHOLD_MIN")) * p->getParameter("ALPHA"));

  if(new_t > p->getParameter("RELIABILITY_THRESHOLD_MAX"))
    new_t = p->getParameter("RELIABILITY_THRESHOLD_MAX");

  if(new_t < p->getParameter("RELIABILITY_THRESHOLD_MIN"))
    new_t = p->getParameter("RELIABILITY_THRESHOLD_MIN");

  if(new_t != curReliability[layer])
  {
  if(increase)
    NS_LOG_DEBUG("Increasing reliability[" << layer << "]=" << new_t);
  else
    NS_LOG_DEBUG("Decreasing reliability[" << layer << "]=" << new_t);
  }

  curReliability[layer] = new_t;
}
