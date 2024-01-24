#ifndef HISTOGRAMMANAGER_H
#define HISTOGRAMMANAGER_H

#include <iostream>
#include <vector>

#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TF1.h"
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
  extern TTree *treeTracksTop;
  extern TTree *treeTracksBot;
  extern TTree *treeTracker_Hits;
  extern TTree *treeGenPhotons;
  extern TTree *treePhotons;
  extern TrackClass g_track;
  extern PhotonRecClass g_recPhoton;
  extern PhotonGenClass g_genPhoton;

  extern void bookHistograms( TDirectory *dir);

  extern TF1 *posres_pol1;
  
}
#endif
