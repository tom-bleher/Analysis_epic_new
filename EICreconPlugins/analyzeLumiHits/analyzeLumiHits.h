//
// Template for this file generated with eicmkplugin.py
//

#include <algorithm>
#include <bitset>
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <iostream>

#include <JANA/JEventProcessorSequentialRoot.h>
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
//#include <edm4eic/RawCalorimeterHit.h>
#include <edm4eic/ProtoCluster.h>

#include <services/geometry/dd4hep/DD4hep_service.h>

using namespace std;

class analyzeLumiHits: public JEventProcessorSequentialRoot {
  private:
    // Data objects we will need from JANA e.g.
    PrefetchT<edm4hep::MCParticle> MCParticles          = {this, "MCParticles"};
    //PrefetchT<edm4hep::SimCalorimeterHit> CAL_hits      = {this, "EcalLumiSpecHits"};
    PrefetchT<edm4hep::SimCalorimeterHit> CAL_hits      = {this, "EcalLumiSpecHits"};
    PrefetchT<edm4hep::RawCalorimeterHit> CAL_adc       = {this, "EcalLumiSpecRawHits"};
    PrefetchT<edm4eic::CalorimeterHit> CAL_rechits      = {this, "EcalLumiSpecRecHits"};
    PrefetchT<edm4eic::ProtoCluster> CAL_protoClusters  = {this, "EcalLumiSpecIslandProtoClusters"};
    PrefetchT<edm4eic::Cluster> CAL_clusters            = {this, "EcalLumiSpecClusters"};
    
    PrefetchT<edm4hep::SimTrackerHit> Tracker_hits      = {this, "LumiSpecTrackerHits"};
    
    TrackerAnalysis *tracker;
    CALAnalysis *CAL;

  public:
    analyzeLumiHits() { SetTypeName(NAME_OF_THIS); }

    void InitWithGlobalRootLock() override;
    void ProcessSequential(const std::shared_ptr<const JEvent>& event) override;
    void FinishWithGlobalRootLock() override;

    void MCgenAnalysis();
    void FillDiagnosticHistograms();

  protected:
    std::shared_ptr<DD4hep_service> m_geoSvc;
};

