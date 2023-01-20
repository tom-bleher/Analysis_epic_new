//
// Template for this file generated with eicmkplugin.py
//

#include <algorithm>
#include <bitset>
#include <spdlog/spdlog.h>

#include <JANA/JEventProcessorSequentialRoot.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TFile.h>

#include <edm4hep/SimTrackerHit.h>
#include <edm4hep/SimCalorimeterHit.h>
#include <edm4hep/RawCalorimeterHit.h>

#include <edm4eic/Cluster.h>
#include <edm4eic/RawCalorimeterHit.h>

#include <services/geometry/dd4hep/JDD4hep_service.h>

using namespace std;

class analyzeLumiHits: public JEventProcessorSequentialRoot {
  private:

    static const int maxModules = 3;
    static const int maxSectors = 2;
    
    // spectrometer dimensions/placements in cm
    double LumiSpecMag_Z = -5600;
    double LumiSpecMag_DZ = 78;
    double LumiSpecCAL_Z = -6500;
    double LumiSpecCALTower_DZ = 20;
    double pT = 0.117; // GeV. 0.3*B(T)*dZ(m)

    double LumiSpecTracker_Z1;
    double LumiSpecTracker_Z2;
    double LumiSpecTracker_Z3;

    double Einput;
    int Ntrackers;

    // Declare histogram and tree pointers here. e.g.
    TH1D *hCAL_Acceptance = nullptr;
    TH1D* hEraw  	= nullptr;

    TH2D *hGlobalXY[maxModules][maxSectors] = {{nullptr}};
    //TH2D *hLocalXY[maxModules][maxSectors] = {{nullptr}};

    TH1D* hEup 	        = nullptr;
    TH1D* hEdw	        = nullptr;
    TH1D* hEnergy 	= nullptr;

    TH2D *hTrackers_Eres;
    TH2D *hCAL_Eres;
    TH2D *hTrackers_E;
    TH2D *hTrackersTop_E;
    TH2D *hTrackersBot_E;

    TH1D *hADCsignal;
    TH1D *hRecClusterEnergy;

    // Data objects we will need from JANA e.g.
    PrefetchT<edm4hep::SimCalorimeterHit> CALhits   = {this, "EcalLumiSpecHits"};
    PrefetchT<edm4hep::SimTrackerHit> Trackerhits   = {this, "TrackerLumiSpecHits"};
    PrefetchT<edm4hep::RawCalorimeterHit> ADCsignal = {this, "EcalLumiSpecRawHits"};
    PrefetchT<edm4eic::Cluster> Clusters            = {this, "EcalLumiSpecClusters"};

  public:
    analyzeLumiHits() { SetTypeName(NAME_OF_THIS); }

    void InitWithGlobalRootLock() override;
    void ProcessSequential(const std::shared_ptr<const JEvent>& event) override;
    void FinishWithGlobalRootLock() override;
    
    double TrackerErec( double y[maxModules][maxSectors] );
  protected:

    std::shared_ptr<JDD4hep_service> m_geoSvc;
};
