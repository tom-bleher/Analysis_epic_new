#ifndef HISTOGRAMMANAGER_H
#define HISTOGRAMMANAGER_H

#include <iostream>
#include <vector>

#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "THashList.h"
#include "TProfile.h"
#include "TDirectory.h"
#include "TTree.h"
#include "variables.h"

using namespace std;

namespace histogramManager {

  // All histograms stored in a THashList
  extern THashList *gHistList;

  extern TTree *treeCAL_Hits;
  extern TTree *treeCAL_RecHits;
  extern TTree *treeCAL_ProtoClusters;
  extern TTree *treeCAL_Clusters;
  extern TTree *treeTracks;
  extern TTree *treeTracker_Hits;

  extern TreeTrackClass tracks;

  extern void bookHistograms( TDirectory *dir);
  
}
#endif
