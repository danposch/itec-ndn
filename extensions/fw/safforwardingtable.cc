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

  int ilayer = SAFForwardingTable::determineContentLayer(interest);

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

int SAFForwardingTable::determineContentLayer(const Interest& interest)
{
  //TODO implement.
  return 0;
}
