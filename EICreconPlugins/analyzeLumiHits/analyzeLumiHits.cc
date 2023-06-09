#include <services/rootfile/RootFile_service.h>

#include "histogramManager.h"
#include "constants.h"
#include "variables.h"
#include "tracker.h"
#include "cal.h"

#include "analyzeLumiHits.h"

using namespace histogramManager;

// The following just makes this a JANA plugin
extern "C" {
  void InitPlugin(JApplication *app) {
    InitJANAPlugin(app);
    app->Add(new analyzeLumiHits);
  }
}

//-------------------------------------------
// InitWithGlobalRootLock
//-------------------------------------------
void analyzeLumiHits::InitWithGlobalRootLock(){

  auto rootfile_svc = GetApplication()->GetService<RootFile_service>();
  auto rootfile = rootfile_svc->GetHistFile();

  // Create histograms and TTrees
  bookHistograms( rootfile );

  CAL = new CALAnalysis();
  tracker = new TrackerAnalysis();

}

//-------------------------------------------
// ProcessSequential
//-------------------------------------------
void analyzeLumiHits::ProcessSequential(const std::shared_ptr<const JEvent>& event) {

  //cout<<"New Event"<<endl;

  auto app = GetApplication();
  m_geoSvc = app->template GetService<JDD4hep_service>();

  // example of how to get a parameter variable
  //app->GetParameter("analyzeLumiHits:Egen", variables::Einput);

  MCgenAnalysis();

  FillDiagnosticHistograms();

  /////////////////////////////
  // CAL Studies
  CAL->Prepare( CAL_hits(), CAL_adc(), CAL_rechits(), CAL_protoClusters(), CAL_clusters(), m_geoSvc );
  CAL->FillTrees();
  CAL->FillDiagnostics();
  CAL->FillAcceptances();
  CAL->CollectGoodClusters();


  /////////////////////////////
  // Tracker Studies
  tracker->Prepare( Tracker_hits(), m_geoSvc );
  tracker->FillTrackerHits();
  tracker->AssembleAllTracks();
  tracker->FillTrackerTrees();
  tracker->FillTrackerHistograms();

  /////////////////////////////
  // CAL + Tracker Studies
  //
  // Track / Cluster matching

    
} // End of the Sequential Process Function

//-------------------------------------------------------------------------
void analyzeLumiHits::MCgenAnalysis() {
  
  for( auto particle : MCParticles() ) {
     
    edm4hep::Vector3f p = particle->getMomentum();
    edm4hep::Vector3d v = particle->getVertex(); // Units of mm !!
    
    // Brem photons
    if( particle->getPDG() == 22 ) {
      // Set the photon energy for the acceptance histograms
      variables::Einput = particle->getEnergy();
      
      ((TH1D *)gHistList->FindObject("hGenPhoton_E"))->Fill( particle->getEnergy() );
      ((TH2D *)gHistList->FindObject("hGenPhoton_xy"))->Fill( v.x, v.y );
    }
    else if( particle->getPDG() == +11 && particle->getGeneratorStatus() == 1 ) {
      ((TH1D *)gHistList->FindObject("hGenElectron_E"))->Fill( particle->getEnergy() );
      ((TH2D *)gHistList->FindObject("hGenElectron_xy"))->Fill( v.x, v.y );
    }
    else if( particle->getPDG() == -11 && particle->getGeneratorStatus() == 1 ) {
      ((TH1D *)gHistList->FindObject("hGenPositron_E"))->Fill( particle->getEnergy() );
      ((TH2D *)gHistList->FindObject("hGenPositron_xy"))->Fill( v.x, v.y );
    }
    else {}
  }
}

//-------------------------------------------------------------------------
void analyzeLumiHits::FillDiagnosticHistograms() {

  ((TH1D *)gHistList->FindObject("hGenEventCount"))->Fill( variables::Einput );
  
}

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void analyzeLumiHits::FinishWithGlobalRootLock() {

  // Do any final calculations here.
}
