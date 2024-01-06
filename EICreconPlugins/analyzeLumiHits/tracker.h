#ifndef TRACKER_H
#define TRACKER_H

#include <algorithm>
#include <bitset>

#include <TH2D.h>
#include <TProfile.h>
#include <TFile.h>
#include <TTree.h>
#include <TLorentzVector.h>
#include <THashList.h>

#include "DD4hep/Detector.h"
#include "DD4hep/DD4hepUnits.h"

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

class TrackerAnalysis {
  
  public:

    TrackerAnalysis(std::shared_ptr<DD4hep_service> geoSvc);

    void Prepare( std::vector<const edm4hep::SimTrackerHit*> &hits, std::shared_ptr<DD4hep_service> geoSvc );
    void CalcBdotDl();
    void InsertSensorNoise();
    //void get_sensitive_volumeIDs(const dd4hep::IDDescriptor &id_desc, const dd4hep::PlacedVolume &pv, std::vector<dd4hep::VolumeID> *result, dd4hep::VolumeID id);
    void FillTrackerHits();
    void ComputeLinearRegressionComponents( vector<TrackClass> *tracks, vector<TrackHit> *hits, double Nmods, int sec, int mod, double *x, double *y, double *z, double *xz, double *yz, double *zz );
    void AssembleAllTracks();
    void AssembleTracks( std::vector<TrackClass> *tracks, int sec_id );
    void FillTrackerTrees();
    void FillTrackerHistograms();

    // basic utility functions
    bool PixelOverlap( TrackHit hit, std::vector<TrackHit> trackSet );
    double TrackerErec( double slopeY );
    void BackPropagate( TrackClass *track );
    std::pair<double,double> DCA( TrackClass top, TrackClass bot );
    double GetPairMass( TrackClass top, TrackClass bot );
    void PrintTrackInfo( std::vector<TrackClass> topTracks, std::vector<TrackClass> botTracks );

    std::vector<const edm4hep::SimTrackerHit*> m_edm4hepTrackerHits;
   
    vector<vector<vector<TrackHit>>> m_TrackerHits;

    std::vector<TrackClass> m_AllTopTracks;
    std::vector<TrackClass> m_AllBotTracks;

    // low Chi2 tracks
    std::vector<TrackClass> m_TopTracks;
    std::vector<TrackClass> m_BotTracks;

    double m_BxDotDz_central;
    double m_Bx_sign;

  private:
    std::vector<float>                          steps, mins, maxs;
    vector< vector< vector< array<float,3>>>>   Bvals_XYZ;
    int                                         ix, iy, iz; // lookup indices
    float                                       idx_1_f, idx_2_f, idx_3_f; // transient float indicies
    float                                       dx, dy, dz; // deltas for interpolation
    ROOT::Math::Translation3D                   coordTranslate_inv;

  protected:
    std::shared_ptr<DD4hep_service> m_geoSvc; // not set unil Prepare()  
};
#endif
