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
  //app->GetParameter("analyzeLumiHits:variable", variable );

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

  // calculate B*dL
  std::cout<<"Calculating B*dL in lumi analyzer magnet"<<std::endl;
  
  double FiveSigmaLimits = 7.5 /5.; // cm
  
  double BxdL_mean = 0;
  double BydL_mean = 0;
  double BzdL_mean = 0;
  double BxdL_mean2 = 0;
  double BydL_mean2 = 0;
  double BzdL_mean2 = 0;
  double entries_mean = 0;

  dd4hep::Detector* description = m_geoSvc->detector();
  
  for( double x = -8; x < 8.1; x += 0.5 ) {
    for( double y = -20; y < 20.1; y += 0.5 ) {

      double BxdL = 0;
      double BydL = 0;
      double BzdL = 0;

      for( double z = -6350; z < -6150; z += 1.0 ) {

        double posV[3] = { x, y, z };
        double bfieldV[3];
        description->field().magneticField( posV  , bfieldV  ) ;

        BxdL += bfieldV[0] / dd4hep::tesla / dd4hep::m; // 1 cm steps
        BydL += bfieldV[1] / dd4hep::tesla / dd4hep::m; // 1 cm steps  
        BzdL += bfieldV[2] / dd4hep::tesla / dd4hep::m; // 1 cm steps  
      }

      if( sqrt(x*x + y*y) < FiveSigmaLimits ) {
          BxdL_mean += BxdL;
          BydL_mean += BydL;
          BzdL_mean += BzdL;
          BxdL_mean2 += pow(BxdL, 2);
          BydL_mean2 += pow(BydL, 2);
          BzdL_mean2 += pow(BzdL, 2);
          entries_mean++;
        }

      ((TH2D *)gHistList->FindObject("hBxdotdL"))->Fill( x, y, BxdL );
      ((TH2D *)gHistList->FindObject("hBydotdL"))->Fill( x, y, BydL );
      ((TH2D *)gHistList->FindObject("hBzdotdL"))->Fill( x, y, BzdL );
    }
  }

  BxdL_mean /= entries_mean;
  BydL_mean /= entries_mean;
  BzdL_mean /= entries_mean;
  BxdL_mean2 /= entries_mean;
  BydL_mean2 /= entries_mean;
  BzdL_mean2 /= entries_mean;

  cout<<"Mean Bx*dL in 5 sigma region: "<<BxdL_mean<<"   Std: "<<sqrt( BxdL_mean2 - pow(BxdL_mean,2) )<<endl;
  cout<<"Mean By*dL in 5 sigma region: "<<BydL_mean<<"   Std: "<<sqrt( BydL_mean2 - pow(BydL_mean,2) )<<endl;
  cout<<"Mean Bz*dL in 5 sigma region: "<<BzdL_mean<<"   Std: "<<sqrt( BzdL_mean2 - pow(BzdL_mean,2) )<<endl;

    
    //printf(" LUMI B FIELD: %+15.8e  %+15.8e  %+15.8e  %+15.8e  %+15.8e  %+15.8e  \n",
  //    posV[0]/dd4hep::cm, posV[1]/dd4hep::cm,  posV[2]/dd4hep::cm,
  //    bfieldV[0]/dd4hep::tesla , bfieldV[1]/dd4hep::tesla, bfieldV[2]/dd4hep::tesla ) ;
}
