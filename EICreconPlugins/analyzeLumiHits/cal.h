#ifndef CAL_H
#define CAL_H

#include <algorithm>
#include <bitset>

#include <TH2D.h>
#include <TProfile.h>
#include <TFile.h>
#include <TTree.h>
#include <TLorentzVector.h>
#include <THashList.h>

#include <edm4hep/MCParticle.h>
#include <edm4hep/SimTrackerHit.h>
#include <edm4hep/SimCalorimeterHit.h>
#include <edm4hep/RawCalorimeterHit.h>

#include <edm4eic/Cluster.h>
#include <edm4eic/RawCalorimeterHit.h>
#include <edm4eic/ProtoCluster.h>

#include <services/geometry/dd4hep/DD4hep_service.h>

#include "constants.h"
#include "variables.h"
#include "histogramManager.h"

using namespace std;
using namespace histogramManager;

class CALAnalysis {

  public:

  CALAnalysis();

  void Prepare( 
      std::vector<const edm4hep::SimCalorimeterHit*> &CALHits, 
      std::vector<const edm4hep::RawCalorimeterHit*> &CALadc, 
      std::vector<const edm4eic::CalorimeterHit*> &CALrecHits,
      std::vector<const edm4eic::ProtoCluster*> &CALprotoClusters,
      std::vector<const edm4eic::Cluster*> &CALClusters,
      std::vector<const edm4hep::SimTrackerHit*> &tracker_hits,
      std::shared_ptr<DD4hep_service> geoSvc );

  void LoadCalibration();
  void FillTrees();
  void FillDiagnostics();
  void FillAcceptances();
  void CollectGoodClusters();

  TH2D *m_calibration;
  std::vector<edm4eic::Cluster> m_GoodClusters; // our list of corrected clusters

  std::vector<const edm4hep::SimCalorimeterHit*> m_CALhits;
  std::vector<const edm4hep::RawCalorimeterHit*> m_CALadc;
  std::vector<const edm4eic::CalorimeterHit*> m_CALrecHits;
  std::vector<const edm4eic::ProtoCluster*> m_CALprotoClusters;
  std::vector<const edm4eic::Cluster*> m_CALclusters;
  std::vector<const edm4hep::SimTrackerHit*> m_edm4hepTrackerHits;

  double m_EtopTotal = 0.0;
  double m_EbotTotal = 0.0;

  double m_E_CALhits_total = 0.0;
  double m_E_CALtophits_total = 0.0;
  double m_E_CALbothits_total = 0.0;
  double m_CALtophits_total = 0.0;
  double m_CALbothits_total = 0.0;

  double xhit_TopTrack2;
  double yhit_TopTrack2;
  double xhit_BotTrack2;
  double yhit_BotTrack2;

  protected:
    std::shared_ptr<DD4hep_service> m_geoSvc;
};
#endif
