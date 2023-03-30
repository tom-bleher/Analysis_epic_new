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
#include <TTree.h>
#include <TLorentzVector.h>

#include <edm4hep/SimTrackerHit.h>
#include <edm4hep/SimCalorimeterHit.h>
#include <edm4hep/RawCalorimeterHit.h>

#include <edm4eic/Cluster.h>
#include <edm4eic/RawCalorimeterHit.h>
#include <edm4eic/ProtoCluster.h>

#include <services/geometry/dd4hep/JDD4hep_service.h>

#include "../../utilities/constants.h"

using namespace std;

typedef std::tuple<double,double,double> MyHit;

struct TrackClass {
  double charge;
  double X0;
  double Y0;
  double slopeX;
  double slopeY;
  double theta;
  double phi;
  double Chi2;
};

struct TreeTrackClass {
  vector<double> X0_e;
  vector<double> Y0_e;
  vector<double> slopeX_e;
  vector<double> slopeY_e;

  vector<double> X0_p;
  vector<double> Y0_p;
  vector<double> slopeX_p;
  vector<double> slopeY_p;
};

class analyzeLumiHits: public JEventProcessorSequentialRoot {
  private:

    static const int maxModules = 3;
    static const int maxSectors = 2;
   
    int Nhits_min = 1; // 15

    // spectrometer dimensions/placements in cm
    double LumiSpecMag_Z = -5600;
    double LumiSpecMag_DZ = 78;
    double LumiSpecCAL_Z = -6500;
    double LumiSpecCALTower_DZ = 20;
    double LumiSpecCAL_DXY = 20;
    double LumiSpecCAL_FiveSigma = 6.9;
    double LumiConverter_Z = LumiSpecMag_Z + LumiSpecMag_DZ/2.0;
    double LumiSpecMagEnd_Z = LumiSpecMag_Z - LumiSpecMag_DZ/2.0;
    double pT = 0.117; // GeV. 0.3*B(T)*dZ(m)
    // cyclotron radius = speed / cyclotron frequency -> p/(q*B) = E/(c*q*B) in ultrarelativistic limit
    double RmagPreFactor = 667.079; // (J/GeV)/(c * q * B), multiply this by E in GeV to get R in cm

    double LumiSpecTracker_Z1;
    double LumiSpecTracker_Z2;
    double LumiSpecTracker_Z3;
    vector<double> Tracker_Zs;
    double Tracker_meanZ;

    double Tracker_pixelSize = 0.005; // cm
    //maximal reduced chi^2 for tracks
    double max_chi2ndf = 0.01;
    double Tracker_sigma = 0.19; // cm from reconstructed photon origins decaying into 2 electrons.
    
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

    TH1D *hProtoClusterCount;
    TH1D *hClusterCount;

    TH1D *hTrackChi2;
    TH1D *hTrackersSlope;

    TH2D *hTrackers_Eres;
    TH2D *hCAL_Eres;
    TH2D *hTrackers_E;
    TH2D *hTrackersTop_E;
    TH2D *hTrackersBot_E;

    TH1D *hTrackers_X_allPairs;
    TH1D *hTrackers_Y_allPairs;
    TH1D *hTrackers_X;
    TH1D *hTrackers_Y;
    TH2D *hTrackers_X_BotVsTop;
    TH2D *hTrackers_Y_BotVsTop;

    TH1D *hTrackers_InvMass_allPairs;
    TH1D *hTrackers_InvMass;

    TH1D *hADCsignal;
    TH2D *hCALCluster_Eres;

    TTree *tree_Hits;
    TTree *tree_RecHits;
    TTree *tree_ProtoClusters;
    TTree *tree_Clusters;
    TTree *tree_Tracks;

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

    TreeTrackClass treeTracks;
    
    // Data objects we will need from JANA e.g.
    PrefetchT<edm4hep::SimCalorimeterHit> CAL_hits      = {this, "LumiSpecCALHits"};
    PrefetchT<edm4hep::RawCalorimeterHit> CAL_adc       = {this, "EcalLumiSpecRawHits"};
    PrefetchT<edm4eic::CalorimeterHit> CAL_rechits      = {this, "EcalLumiSpecRecHits"};
    PrefetchT<edm4eic::ProtoCluster> CAL_protoClusters  = {this, "EcalLumiSpecIslandProtoClusters"};
    PrefetchT<edm4eic::Cluster> CAL_clusters            = {this, "EcalLumiSpecClusters"};
    
    PrefetchT<edm4hep::SimTrackerHit> Tracker_hits      = {this, "LumiSpecTrackerHits"};

  public:
    analyzeLumiHits() { SetTypeName(NAME_OF_THIS); }

    void InitWithGlobalRootLock() override;
    void ProcessSequential(const std::shared_ptr<const JEvent>& event) override;
    void FinishWithGlobalRootLock() override;
    bool PixelOverlap( MyHit hit, vector<MyHit> trackSet );
    void AssembleTracks( vector<TrackClass> *tracks, vector<vector<MyHit>> hitSet );

    double TrackerErec( double slopeY );
    double DeltaYmagnet( double E, double charge );
    double XatConverter( TrackClass track );
    double YatConverter( TrackClass track );
    double GetPairMass( TrackClass top, TrackClass bot );
    void PrintTrackInfo( vector<TrackClass> topTracks, vector<TrackClass> botTracks );
  protected:

    std::shared_ptr<JDD4hep_service> m_geoSvc;
};


