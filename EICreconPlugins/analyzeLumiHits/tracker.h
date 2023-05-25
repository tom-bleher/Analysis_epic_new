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

#include <edm4hep/MCParticle.h>
#include <edm4hep/SimTrackerHit.h>
#include <edm4hep/SimCalorimeterHit.h>
#include <edm4hep/RawCalorimeterHit.h>

#include <edm4eic/Cluster.h>
#include <edm4eic/RawCalorimeterHit.h>
#include <edm4eic/ProtoCluster.h>

#include <services/geometry/dd4hep/JDD4hep_service.h>

#include "constants.h"
#include "variables.h"
#include "histogramManager.h"

using namespace std;
using namespace histogramManager;

class TrackerAnalysis {
  
  public:

    TrackerAnalysis();

    void Prepare( std::vector<const edm4hep::SimTrackerHit*> &hits, std::shared_ptr<JDD4hep_service> geoSvc );
    void FillTrackerHits();
    void AssembleAllTracks();
    void AssembleTracks(std::vector<TrackClass> *tracks, std::vector<TrackHit> tracker1Hits, std::vector<TrackHit> tracker2Hits, std::vector<TrackHit> tracker3Hits);
    void FillTrackerTrees();
    void FillTrackerHistograms();

    // basic utility functions
    bool PixelOverlap( TrackHit hit, std::vector<TrackHit> trackSet );
    double TrackerErec( double slopeY );
    double DeltaYmagnet( double E, double charge );
    double XatConverter( TrackClass track );
    double YatConverter( TrackClass track );
    double GetPairMass( TrackClass top, TrackClass bot );
    void PrintTrackInfo( std::vector<TrackClass> topTracks, std::vector<TrackClass> botTracks );

    std::vector<const edm4hep::SimTrackerHit*> m_Tracker_hits;

    std::vector<TrackHit> m_TopTracker1Hits;
    std::vector<TrackHit> m_TopTracker2Hits;
    std::vector<TrackHit> m_TopTracker3Hits;
    std::vector<TrackHit> m_BotTracker1Hits;
    std::vector<TrackHit> m_BotTracker2Hits;
    std::vector<TrackHit> m_BotTracker3Hits;

    std::vector<TrackClass> m_AllTopTracks;
    std::vector<TrackClass> m_AllBotTracks;

    // low Chi2 tracks
    std::vector<TrackClass> m_TopTracks;
    std::vector<TrackClass> m_BotTracks;

  protected:
    std::shared_ptr<JDD4hep_service> m_geoSvc;
};
#endif
