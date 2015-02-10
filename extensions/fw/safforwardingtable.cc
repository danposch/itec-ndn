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

  for(int i = 0; i < faces.size () ; i++)
  {
    if(faces.at (i) == face_id)
    {
      faceRow = i;
      break;
    }
  }
  return faceRow;
}

int SAFForwardingTable::determineNextHop(const Interest& interest, std::vector<int> originInFaces, std::vector<int> alreadyTriedFaces)
{
  /*//evaluate the forwarding propabilities for all faces not in originInFaces and alreadyTriedFaces
  //copy data structs
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
      fprintf(stderr, "Could not remove inface[i]=" << *i <<". There is something serousliy wrong. Returning the dropping face now...\n");
      return DROP_FACE_ID;
    }
  }*/
}
