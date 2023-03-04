//
// Template for this file generated with eicmkplugin.py
//

#include <algorithm>
#include <bitset>
#include <spdlog/spdlog.h>

#include <JANA/JEventProcessorSequentialRoot.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TFile.h>
#include <TTree.h>

// Include appropirate class headers. e.g.
// #include <edm4hep/SimCalorimeterHit.h>
// #include <detectors/BEMC/BEMCRawCalorimeterHit.h>

#include <edm4hep/SimTrackerHit.h>
#include <edm4hep/SimCalorimeterHit.h>
#include <edm4hep/RawCalorimeterHit.h>

#include <edm4eic/Cluster.h>
#include <edm4eic/ProtoCluster.h>
#include <edm4eic/CalorimeterHit.h>
#include <edm4eic/RawCalorimeterHit.h>

#include <services/geometry/dd4hep/JDD4hep_service.h>

using namespace std;

class testProcessor: public JEventProcessorSequentialRoot {
  private:

    static const int maxModules = 3;
    static const int maxSectors = 2;
   
    int Nhits_min = 15;

    // spectrometer dimensions/placements in cm
    double LumiSpecMag_Z = -5600;
    double LumiSpecMag_DZ = 78;
    double LumiSpecCAL_Z = -6500;
    double LumiSpecCALTower_DZ = 20;
    double LumiSpecCAL_DXY = 20;
    double LumiSpecCAL_FiveSigma = 6.9;
    double pT = 0.117; // GeV. 0.3*B(T)*dZ(m)

    double LumiSpecTracker_Z1;
    double LumiSpecTracker_Z2;
    double LumiSpecTracker_Z3;

    double Einput;
    int Ntrackers;

    // Declare histogram and tree pointers here. e.g.
    TH1D *hCAL_Acceptance = nullptr;
    TH1D* hEraw  	= nullptr;
    TH1D* hErawTotal  	= nullptr;

    TH2D *hGlobalXY[maxModules][maxSectors] = {{nullptr}};
    //TH2D *hLocalXY[maxModules][maxSectors] = {{nullptr}};

    TH1D* hEup 	        = nullptr;
    TH1D* hEdw	        = nullptr;
    TH1D* hEnergy 	= nullptr;

    TH1D *hClusterCount;

    TH2D *hTrackers_Eres;
    TH2D *hCAL_Eres;
    TH2D *hTrackers_E;
    TH2D *hTrackersTop_E;
    TH2D *hTrackersBot_E;

    TH1D *hADCsignal;
    TH2D *hCALCluster_Eres;

    TTree *tree_Hits;
    TTree *tree_RecHits;
    TTree *tree_ProtoClusters;
    TTree *tree_Clusters;
    TTree *tree_MergedClusters;

    double E_hit;
    double x_hit;
    double y_hit;
    double r_hit;
    double t_hit;

    int Nhits_cluster;
    double E_cluster;
    double x_cluster;
    double y_cluster;
    double r_cluster;
    double t_cluster;
    double ETrue_cluster;

    TFile *file_in = new TFile("TProfile2DCAlibrationMatrix.root","READ");
    TProfile2D *hCALCalibration = (TProfile2D*)file_in->Get("hEfficiencyVsCentroid");

    // Data objects we will need from JANA e.g.
    PrefetchT<edm4hep::SimCalorimeterHit> CAL_hits      = {this, "EcalLumiSpecHits"};
    PrefetchT<edm4hep::RawCalorimeterHit> CAL_adc       = {this, "EcalLumiSpecRawHits"};
    PrefetchT<edm4eic::CalorimeterHit> CAL_rechits      = {this, "EcalLumiSpecRecHits"};
    PrefetchT<edm4eic::ProtoCluster> CAL_protoClusters  = {this, "EcalLumiSpecIslandProtoClusters"};
    PrefetchT<edm4eic::Cluster> CAL_clusters            = {this, "EcalLumiSpecClusters"};
    PrefetchT<edm4eic::Cluster> CAL_mergedClusters      = {this, "EcalLumiSpecMergedClusters"};
    
  public:
    testProcessor() { SetTypeName(NAME_OF_THIS); }

    void InitWithGlobalRootLock() override;
    void ProcessSequential(const std::shared_ptr<const JEvent>& event) override;
    void FinishWithGlobalRootLock() override;
   
    double TrackerErec( double y[maxModules][maxSectors] );
    double ClusterEnergyCalibration(double x_cluster, double y_cluster);
  protected:

    std::shared_ptr<JDD4hep_service> m_geoSvc;
};
