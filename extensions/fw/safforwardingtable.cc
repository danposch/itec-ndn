#include "safforwardingtable.h"

using namespace nfd;
using namespace nfd::fw;
using namespace boost::numeric::ublas;

SAFForwardingTable::SAFForwardingTable(std::vector<int> faceIds, std::vector<int> preferedFacesIds)
{
  this->curReliability = ParameterConfiguration::getInstance ()->getParameter("RELIABILITY_THRESHOLD_MIN");
  this->faces = faceIds;
  this->preferedFaces = preferedFacesIds;
  initTable ();
}

void SAFForwardingTable::initTable ()
{
  std::sort(faces.begin(), faces.end());//order

  table = matrix<double> (faces.size () /*rows*/, (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS") /*columns*/);

  // fill matrix column-wise /* table(i,j) = i-th row, j-th column*/
  for (unsigned j = 0; j < table.size2 (); ++j) /* columns */
  {
    for (unsigned i = 0; i < table.size1 (); ++i) /* rows */
    {
      if(faces.at (i) == DROP_FACE_ID)
        table(i,j) = 0.0;
      else if(preferedFaces.size () == 0)
      {
        table(i,j) = (1.0 / ((double)faces.size () - 1.0)); /*set default value to 1 / (d - 1) */
      }
      else
      {
        if(std::find(preferedFaces.begin (), preferedFaces.end (), faces.at (i)) != preferedFaces.end ())
        {
          table(i,j) = (1.0 / ((double)preferedFaces.size ())); // preferedFaces dont include the dropping face.
        }
        else
        {
          table(i,j) = 0;
        }
      }
    }
  }

  //std::cout << table << std::endl; /* prints matrix line by line ( (first line), (second line) )*/
}

int SAFForwardingTable::determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces)
{
  //create a copy of the table
  matrix<double> tmp_matrix(table);
  std::vector<int> face_list(faces);
  int offset = 0;

  //first remove the inface(s)
  for(std::vector<int>::iterator i = originInFaces.begin (); i != originInFaces.end ();++i)
  {
    offset = determineRowOfFace(*i, tmp_matrix, face_list);
    if(offset != FACE_NOT_FOUND)
    {
      //then remove the row and the face from the list
      tmp_matrix = removeFaceFromTable(*i, tmp_matrix, face_list);
      face_list.erase (face_list.begin ()+offset);
    }
    else
    {
      fprintf(stderr, "Could not remove inface[i]=%d. Returning the dropping face..\n", *i);
      return DROP_FACE_ID;
    }
  }

  int ilayer = SAFStatisticMeasure::determineContentLayer(interest);

  //lets check if sum(Fi in alreadyTriedFaces > R)
  double fw_prob = 0.0;
  int row = -1;
  for(std::vector<int>::iterator i = alreadyTriedFaces.begin (); i != alreadyTriedFaces.end ();++i)
  {
    row = determineRowOfFace(*i, tmp_matrix, face_list);
    if(row != FACE_NOT_FOUND)
      fw_prob += tmp_matrix(row,ilayer);
    else
    {
      fprintf(stderr, "Could not find face[i]=%d. Returning the dropping face..\n", *i);
      return DROP_FACE_ID;
    }
  }

  if(fw_prob >= curReliability) // in this case we drop...
    return DROP_FACE_ID;

  //ok now remove the alreadyTriedFaces
  offset = 0;
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
     fprintf(stderr, "Could not remove face[i]=%d. Returning the dropping face..\n", *i);
      return DROP_FACE_ID;
    }
  }

  // choose one face as outgoing according to the probability
  return chooseFaceAccordingProbability(tmp_matrix, ilayer, face_list);
}

void SAFForwardingTable::update(boost::shared_ptr<SAFStatisticMeasure> stats)
{
  std::vector<int> r_faces;
  std::vector<int> ur_faces;

  for(int layer = 0; layer < (int)ParameterConfiguration::getInstance ()->getParameter ("MAX_LAYERS"); layer++) // for each layer
  {
    //determine the set of (un)reliable faces
    r_faces = stats->getReliableFaces (layer, curReliability);
    ur_faces = stats->getUnreliableFaces (layer, curReliability);

    double utf = stats->getUnsatisfiedTrafficFractionOfUnreliableFaces (layer);
    utf *= ParameterConfiguration::getInstance ()->getParameter ("ALPHA");

    //check if we need to shift traffic
    if(utf > 0)
    {
      //determine the relialbe faces act forwarding Prob > 0
      double r_faces_actual_fowarding_prob = 0.0;
      for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end(); ++it) // for each r_face
      {
        r_faces_actual_fowarding_prob += stats->getActualForwardingProbability (*it,layer);
      }

      //if we have no relible faces, or no interests can be forwarded to reliable faces
      if(r_faces.size () == 0 || r_faces_actual_fowarding_prob == 0.0) // we drop everything in this case
      {
        table(determineRowOfFace(DROP_FACE_ID), layer) = calcWeightedUtilization(DROP_FACE_ID,layer,stats)+ utf;
        updateColumn (ur_faces, layer, stats, utf, false);
        probeColumn(r_faces, layer, stats, true);

        //lower reliability
        if(table(determineRowOfFace (DROP_FACE_ID),layer) > (1.0-curReliability) )
          decreaseReliabilityThreshold();
      }
      else
      {
        //fprintf(stderr, "CASE 2\n");
        //add traffic to relialbe faces
        updateColumn (r_faces, layer, stats, utf, true);
        //remove traffic from unreliable faces
        updateColumn (ur_faces, layer, stats, utf, false);
        //set dropping face
        table(determineRowOfFace(DROP_FACE_ID), layer) = calcWeightedUtilization(DROP_FACE_ID,layer,stats);
       }
    }
    else/*(delta == 0* &&)*/ // utf == 0 and layer has not been jammed last time
    {

      if(stats->getTotalForwardedInterests (layer) == 0)
      {
        double probe = table(determineRowOfFace (DROP_FACE_ID), layer) * ParameterConfiguration::getInstance ()->getParameter ("PROBING_TRAFFIC");
         table(determineRowOfFace (DROP_FACE_ID), layer) -= probe;

        //distribute the traffic
        for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end(); ++it) // for each ur_face
          table(determineRowOfFace (*it), layer) += (probe / ((double)r_faces.size ()));
      }
      else if(table(determineRowOfFace(DROP_FACE_ID),layer) == 0.0) // dropping prob == 0 or there are no reliable faces
      {
        if(stats->getTotalForwardedInterests (layer) != 0)
          for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it)
            table(determineRowOfFace(*it), layer) = calcWeightedUtilization(*it,layer,stats);

        //increase Reliability Threshold
         increaseReliabilityThreshold();
      }
      else
      {
        //NS_LOG_DEBUG("WE SHOULD DECREASE DROPPING TRAFFIC\n");

        //check if we should do probing or shifting and probing
        std::vector<int> shift_faces;
        std::vector<int> probe_faces;
        for(std::vector<int>::iterator it = r_faces.begin(); it != r_faces.end(); ++it)
        {
          //if(calcWeightedUtilization (*it,layer,stats) > SHIFT_THRESHOLD) start here
          if(stats->getActualForwardingProbability (*it,layer) > ParameterConfiguration::getInstance()->getParameter ("SHIFT_THRESHOLD"))
            shift_faces.push_back (*it);
          else
            probe_faces.push_back (*it);
        }

        if(shift_faces.size () == 0)
        {
          //needs normalization if, no interests have been transmitted at all.
          // however this might be good, as it lowers, dropping probability, and distributes forwarding prob at all other faces...
          //fprintf(stderr, "CASE 5\n");
          probeColumn(probe_faces, layer, stats, false); // do only probing
        }
        else
        {
          //fprintf(stderr, "CASE 6\n");
          shiftDroppingTraffic(shift_faces, layer, stats); //shift traffic
          probeColumn(probe_faces, layer, stats, true); // and probe then

          //increase Reliability Threshold
          if(table(determineRowOfFace (DROP_FACE_ID),layer) < (1.0-curReliability) )
            increaseReliabilityThreshold();
        }
      }
    }
  }

  //NS_LOG_DEBUG("Forwarding Matrix after update:\n" << table);

  /*std::stringstream ss1;
  ss1 << table;
  std::string s1 = ss1.str();*/

  //finally just normalize to remove the error introduced by threashold RELIABILITY_THRESHOLD
  table = normalizeColumns(table);
}

void SAFForwardingTable::updateColumn(std::vector<int> faces, int layer, boost::shared_ptr<SAFStatisticMeasure> stats, double utf,
                                                  bool shift_traffic/*true -> traffic will be shifted towards faces, false traffic will be shifted away*/)
{
  if(faces.size () == 0)
    return;

  double sum_reliabilities = 0.0;

  if(shift_traffic)
    sum_reliabilities = stats->getSumOfReliabilities (faces, layer);
  else
    sum_reliabilities = stats->getSumOfUnreliabilities (faces, layer);

  double sum_fwProbs = getSumOfWeightedForwardingProbabilities (faces, layer,stats);

  double normalization_value = 0.0;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each r_face
  {
    if(shift_traffic && sum_fwProbs > 0)
    {
      normalization_value +=
        (calcWeightedUtilization(*it,layer,stats) / sum_fwProbs) * (stats->getLinkReliability (*it,layer) / sum_reliabilities);
    }
    else if(shift_traffic && sum_fwProbs == 0)  // special case when forwarding probabilities are all 0 for all non relialbe faces. e.g. (0, 0, 1) where f3() = 1 is the incoming face of the interests
    {
      normalization_value +=
        (1.0 /((double)faces.size ())) * (stats->getLinkReliability (*it,layer) / sum_reliabilities);
    }
    else if(!shift_traffic && sum_fwProbs == 0)
    {
      normalization_value +=
        (1.0 /((double)faces.size ())) * ((1 - stats->getLinkReliability (*it,layer)) / sum_reliabilities);
    }
    else
    {
      normalization_value +=
        (calcWeightedUtilization(*it,layer,stats) / sum_fwProbs) * ((1 - stats->getLinkReliability (*it,layer)) / sum_reliabilities);
    }
  }

  if(normalization_value == 0)
  {
    fprintf(stderr,"Error normalization_value == 0.\n");
  }

  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each r_face
  {

    double weightedUtil = calcWeightedUtilization(*it,layer,stats);

    if(shift_traffic && sum_fwProbs > 0)
    {
      table(determineRowOfFace(*it), layer) = weightedUtil +
        utf * (weightedUtil / sum_fwProbs) * (stats->getLinkReliability (*it,layer) / sum_reliabilities) / normalization_value;
    }
    else if (shift_traffic && sum_fwProbs == 0) // special case
    {
      table(determineRowOfFace(*it), layer) = weightedUtil +
          utf * (1.0 /((double)faces.size ())) * ( stats->getLinkReliability (*it,layer) / sum_reliabilities / normalization_value);
    }
    else if (!shift_traffic && sum_fwProbs == 0) // special case
    {
      table(determineRowOfFace(*it), layer) = weightedUtil -
          utf * (1.0 /((double)faces.size ())) * ( (1 - stats->getLinkReliability (*it,layer)) / sum_reliabilities / normalization_value);
    }
    else
    {
      table(determineRowOfFace(*it), layer) = weightedUtil -
          utf * (weightedUtil / sum_fwProbs) * ( (1 - stats->getLinkReliability (*it,layer)) / sum_reliabilities / normalization_value);
    }
  }
}

void SAFForwardingTable::probeColumn(std::vector<int> faces, int layer, boost::shared_ptr<SAFStatisticMeasure> stats, bool useDroppingProbabilityFromFWT)
{
  if(faces.size () == 0)
    return;

   double probe = 0.0;

  if(useDroppingProbabilityFromFWT)
    probe = table(determineRowOfFace (DROP_FACE_ID), layer) * ParameterConfiguration::getInstance ()->getParameter ("PROBING_TRAFFIC");
  else
    probe = calcWeightedUtilization(DROP_FACE_ID,layer,stats) * ParameterConfiguration::getInstance ()->getParameter ("PROBING_TRAFFIC");

  if(probe < 0.001) // if probe is zero return
    return;

  if(useDroppingProbabilityFromFWT)
    table(determineRowOfFace (DROP_FACE_ID), layer) -= probe;
  else
    table(determineRowOfFace (DROP_FACE_ID), layer) = calcWeightedUtilization(DROP_FACE_ID,layer,stats) - probe;

  //split the probe (forwarding percents)....
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each ur_face
  {
     double normFactor = 0.0;
    for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each ur_face
    {
      normFactor += table(determineRowOfFace (*it), 0);
    }

    for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each ur_face
    {
      //table(determineRowOfFace (*it), layer) = calcWeightedUtilization(*it,layer,stats) + (probe / ((double)faces.size ()));
      if(layer == 0 || normFactor == 0)
        table(determineRowOfFace (*it), layer) = calcWeightedUtilization(*it,layer,stats) + (probe / ((double)faces.size ()));
      else
      {
        //fprintf(stderr, "probefraction =%f\n", (table(determineRowOfFace (*it), 0) / normFactor));
        //fprintf(stderr, "value =%f\n", probe * (table(determineRowOfFace (*it), 0) / normFactor));
        table(determineRowOfFace (*it), layer) =
            calcWeightedUtilization(*it,layer,stats) + (probe * (table(determineRowOfFace (*it), 0) / normFactor));
      }
    }
  }
}

void SAFForwardingTable::shiftDroppingTraffic(std::vector<int> faces, int layer, boost::shared_ptr<SAFStatisticMeasure>  stats)
{
  //calcualte how much traffic we can take
  double interests_to_shift = 0;
  for(std::vector<int>::iterator it = faces.begin(); it != faces.end(); ++it) // for each r_face
  {
    interests_to_shift += stats->getForwardedInterests (*it, layer);
  }

  interests_to_shift /= (double)stats->getTotalForwardedInterests (layer);
  interests_to_shift *= ParameterConfiguration::getInstance ()->getParameter ("SHIFT_TRAFFIC");

  //double dropped_interests = stats->getForwardedInterests(DROP_FACE_ID, layer);
  double dropped_interests = calcWeightedUtilization(DROP_FACE_ID,layer,stats);

  if(dropped_interests <= interests_to_shift)
  {
    interests_to_shift = dropped_interests;
  }

  table(determineRowOfFace(DROP_FACE_ID), layer) = calcWeightedUtilization(DROP_FACE_ID,layer,stats) - interests_to_shift;
  updateColumn (faces, layer,stats,interests_to_shift,true);

}

double SAFForwardingTable::getSumOfWeightedForwardingProbabilities(std::vector<int> set_of_faces, int layer, boost::shared_ptr<SAFStatisticMeasure> stats)
{
  double sum = 0.0;
  for(std::vector<int>::iterator it = set_of_faces.begin(); it != set_of_faces.end(); ++it)
    sum += sum += calcWeightedUtilization(*it,layer,stats);
  return sum;
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
      //split robabilities amoung all other faces
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
      return faceList.at (i);
  }
  //error case
  return DROP_FACE_ID;
}

double SAFForwardingTable::calcWeightedUtilization(int faceId, int layer, boost::shared_ptr<SAFStatisticMeasure> smeasure)
{
  double actual = smeasure->getActualForwardingProbability (faceId,layer);
  double old = table(determineRowOfFace (faceId), layer);

  return old + ( (actual - old) * ParameterConfiguration::getInstance ()->getParameter ("ALPHA") );
}

void SAFForwardingTable::increaseReliabilityThreshold()
{
  updateReliabilityThreshold (true);
}

void SAFForwardingTable::decreaseReliabilityThreshold()
{
  updateReliabilityThreshold (false);
}

void SAFForwardingTable::updateReliabilityThreshold(bool mode)
{
  ParameterConfiguration *p = ParameterConfiguration::getInstance ();

  double new_t = 0.0;

  if(mode)
    new_t = curReliability + ((p->getParameter("RELIABILITY_THRESHOLD_MAX") - curReliability) * p->getParameter("ALPHA"));
  else
    new_t = curReliability - ((curReliability - p->getParameter("RELIABILITY_THRESHOLD_MIN")) * p->getParameter("ALPHA"));

  if(new_t > p->getParameter("RELIABILITY_THRESHOLD_MAX"))
    new_t = p->getParameter("RELIABILITY_THRESHOLD_MAX");

  if(new_t < p->getParameter("RELIABILITY_THRESHOLD_MIN"))
    new_t = p->getParameter("RELIABILITY_THRESHOLD_MIN");

  curReliability = new_t;
}
