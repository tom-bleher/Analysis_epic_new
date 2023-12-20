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
  m_geoSvc = app->template GetService<DD4hep_service>();

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
    
} // End of the Sequential Process Function

//-------------------------------------------------------------------------
void analyzeLumiHits::MCgenAnalysis() {
  
  for( auto particle : MCParticles() ) {
     
    edm4hep::Vector3f p = particle->getMomentum();
    edm4hep::Vector3d v = particle->getVertex(); // Units of mm !!

    // Brem photons
    if( particle->getPDG() == 22 && particle->getGeneratorStatus() == 4 ) {
      // Set the photon energy for the acceptance histograms
      variables::Ephoton = particle->getEnergy();
      variables::ThetaPhoton = atan2( sqrt(pow(p.x,2) + pow(p.y,2)), p.z );
      variables::PhiPhoton = atan2( p.y, p.x );
      variables::Xphoton = v.x;
      variables::Yphoton = v.y;
      ((TH1D *)gHistList->FindObject("hGenPhoton_E"))->Fill( particle->getEnergy() );
      ((TH2D *)gHistList->FindObject("hGenPhoton_xy"))->Fill( v.x, v.y );
    }
    else if( particle->getPDG() == +11 && particle->getGeneratorStatus() == 1 ) {
      variables::Eelectron = particle->getEnergy();
      variables::Xelectron = v.x;
      variables::Yelectron = v.y;

      ((TH1D *)gHistList->FindObject("hGenElectron_E"))->Fill( particle->getEnergy() );
      ((TH2D *)gHistList->FindObject("hGenElectron_xy"))->Fill( v.x, v.y );
    }
    else if( particle->getPDG() == -11 && particle->getGeneratorStatus() == 1 ) {
      variables::Epositron = particle->getEnergy();
      variables::Xpositron = v.x;
      variables::Ypositron = v.y;

      ((TH1D *)gHistList->FindObject("hGenPositron_E"))->Fill( particle->getEnergy() );
      ((TH2D *)gHistList->FindObject("hGenPositron_xy"))->Fill( v.x, v.y );
    }
    else {}
  }

  g_genPhoton.e = variables::Ephoton;
  g_genPhoton.eElec = variables::Eelectron;
  g_genPhoton.ePos = variables::Epositron;
  g_genPhoton.theta = variables::ThetaPhoton;
  g_genPhoton.phi = variables::PhiPhoton;
  g_genPhoton.x = variables::Xphoton;
  g_genPhoton.y = variables::Yphoton;

  treeGenPhotons->Fill();

}

//-------------------------------------------------------------------------
void analyzeLumiHits::FillDiagnosticHistograms() {

  ((TH1D *)gHistList->FindObject("hGenEventCount"))->Fill( variables::Ephoton );
  
}

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void analyzeLumiHits::FinishWithGlobalRootLock() {

  // Do any final calculations here.
}
