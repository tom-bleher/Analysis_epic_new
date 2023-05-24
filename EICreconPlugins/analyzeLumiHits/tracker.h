#ifndef TRACKER_H
#define TRACKER_H

#include <algorithm>
#include <bitset>
//#include <spdlog/spdlog.h>

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
#include <edm4eic/RawCalorimeterHit.h>
#include <edm4eic/ProtoCluster.h>

#include <services/geometry/dd4hep/JDD4hep_service.h>

using namespace std;
using namespace histogramManager;

class TrackerAnalysis : public JEventProcessorSequentialRoot {
  
  public:

    TrackerAnalysis( JEventProcessorSequentialRoot::PrefetchT<edm4hep::SimTrackerHit>& hits );
    
    PrefetchT<edm4hep::SimTrackerHit> Tracker_hits = {this, "LumiSpecTrackerHits"};

    vector<MyHit> TopTracker1Hits;
    vector<MyHit> TopTracker2Hits;
    vector<MyHit> TopTracker3Hits;
    vector<MyHit> BotTracker1Hits;
    vector<MyHit> BotTracker2Hits;
    vector<MyHit> BotTracker3Hits;

    vector<TrackClass> AllTopTracks;
    vector<TrackClass> AllBotTracks;

    // low Chi2 tracks
    vector<TrackClass> TopTracks;
    vector<TrackClass> BotTracks;

    void FillTrackerHits();
    void AssembleAllTracks();
    void AssembleTracks(vector<TrackClass> *tracks, vector<MyHit> tracker1Hits, vector<MyHit> tracker2Hits, vector<MyHit> tracker3Hits);
    void FillTrackerTrees();
    void FillTrackerHistograms();

    // basic utility functions
    bool PixelOverlap( MyHit hit, vector<MyHit> trackSet );
    double TrackerErec( double slopeY );
    double DeltaYmagnet( double E, double charge );
    double XatConverter( TrackClass track );
    double YatConverter( TrackClass track );
    double GetPairMass( TrackClass top, TrackClass bot );
    void PrintTrackInfo( vector<TrackClass> topTracks, vector<TrackClass> botTracks );

  protected:
    std::shared_ptr<JDD4hep_service> m_geoSvc;
};
#endif
