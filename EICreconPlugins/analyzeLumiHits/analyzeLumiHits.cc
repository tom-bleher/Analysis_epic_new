#include "histogramManager.h"
#include "tracker.h"
#include "constants.h"
#include "variables.h"
#include <services/rootfile/RootFile_service.h>

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

  tracker = new TrackerAnalysis( Tracker_hits );

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

  FillTrees();

  FillDiagnosticHistograms();

  /////////////////////////////
  // CAL Studies


  /////////////////////////////
  // Tracker Studies
  tracker->FillTrackerHits();
  tracker->AssembleAllTracks();
 
  tracker->FillTrackerTrees();
  tracker->FillTrackerHistograms();

    
} // End of the Sequential Process Function

//-------------------------------------------------------------------------
void analyzeLumiHits::MCgenAnalysis() {
  
  for( auto particle : MCParticles() ) {
     
    edm4hep::Vector3f p = particle->getMomentum();
    edm4hep::Vector3d v = particle->getVertex(); // Units of mm !!
    
    // Brem photons
    if( particle->getPDG() == 22 ) {
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
void analyzeLumiHits::FillTrees() {

  ///////////////////////////////////////////////////////////////////////
  // CAL G4 Hits Tree
  map<string, int> CALfield_idx_Map{ {"sector", 0}, {"module", 0}, {"fiber_x", 0}, {"fiber_y", 0}};

  for( auto hit : CAL_hits() ) {

    const auto id = hit->getCellID();
    auto id_dec   = m_geoSvc->detector()->readout( "LumiSpecCALHits" ).idSpec().decoder();

    // field of readout fields
    vector<dd4hep::BitFieldElement> hitFields = id_dec->fields();

    // try to find the expected fields and store field index
    for( auto field : hitFields ) {
      if( CALfield_idx_Map.find( field.name() ) != CALfield_idx_Map.end() ) {
        CALfield_idx_Map[ field.name() ] = id_dec->index( field.name() );
      }
    }

    // look up sector,module,fiber... id of this hit 
    variables::sec_id 	= (int) id_dec->get( id, CALfield_idx_Map["sector"] ); // Top (0) and Bottom (1)
    variables::mod_id 	= (int) id_dec->get( id, CALfield_idx_Map["module"] ); // 10x10 Matrix of bars
    variables::fiber_x_id  = (int) id_dec->get( id, CALfield_idx_Map["fiber_x"] );
    variables::fiber_y_id  = (int) id_dec->get( id, CALfield_idx_Map["fiber_y"] );

    edm4hep::Vector3f vec = hit->getPosition();// mm
    variables::E_hit = hit->getEnergy();
    variables::x_hit = vec.x;
    variables::y_hit = vec.y;
    variables::r_hit = sqrt( pow(variables::x_hit, 2) + pow(variables::y_hit, 2) );

    treeCAL_Hits->Fill();
  }

  ///////////////////////////////////////////////////////////////////////
  // CAL Rec Hits
  for( auto hit : CAL_rechits() ) {
    edm4hep::Vector3f vec = hit->getPosition();// mm
    variables::E_hit = hit->getEnergy();
    variables::x_hit = vec.x;
    variables::y_hit = vec.y;
    variables::r_hit = sqrt( pow(variables::x_hit, 2) + pow(variables::y_hit, 2) );
    variables::t_hit = hit->getTime();

    treeCAL_RecHits->Fill();
  }

  ///////////////////////////////////////////////////////////////////////
  // CAL Clusters
  for( auto cluster : CAL_clusters() ) {
    edm4hep::Vector3f vec = cluster->getPosition();// mm
    variables::Nhits_cluster = cluster->getNhits();
    variables::E_cluster = cluster->getEnergy();
    variables::x_cluster = vec.x;
    variables::y_cluster = vec.y;
    variables::r_cluster = sqrt( pow(vec.x, 2) + pow(vec.y, 2) );
    variables::t_cluster = cluster->getTime();
    if( cluster->shapeParameters_size() > 0 ) {
      variables::Radius_cluster = cluster->getShapeParameters(0);
      variables::Dispersion_cluster = cluster->getShapeParameters(1);
      variables::SigmaThetaPhi1_cluster = cluster->getShapeParameters(2);
      variables::SigmaThetaPhi2_cluster = cluster->getShapeParameters(3);
    } else {// TODO: find out why this case happens
      variables::Radius_cluster = 0;
      variables::Dispersion_cluster = 0;
      variables::SigmaThetaPhi1_cluster = 0;
      variables::SigmaThetaPhi2_cluster = 0;
    }
   
    treeCAL_Clusters->Fill();
  }
  
}

//-------------------------------------------------------------------------
void analyzeLumiHits::FillDiagnosticHistograms() {

  ((TH1D *)gHistList->FindObject("hGenEventCount"))->Fill( variables::Einput );

  double E_CALhits_total = 0.0;
  for( auto hit : CAL_hits() ) { 
    E_CALhits_total += hit->getEnergy();
    ((TH1D *)gHistList->FindObject("hEraw"))->Fill( hit->getEnergy() ); 
  }
  if( E_CALhits_total > 0 ) { ((TH1D *)gHistList->FindObject("hErawTotal"))->Fill( E_CALhits_total ); }

  ///////////////////////////////////////////////////////////////////////
  // CAL Digitized ADC raw hits
  for( auto adc : CAL_adc() ) ((TH1D *)gHistList->FindObject("hADCsignal"))->Fill( adc->getAmplitude() );
  
  ///////////////////////////////////////////////////////////////////////
  // CAL Proto Island Clusters
  ((TH1D *)gHistList->FindObject("hProtoClusterCount"))->Fill( CAL_protoClusters().size() );

  ///////////////////////////////////////////////////////////////////////
  // CAL Reconstructed Clusters
  double E_CALup  	= 0.0;
  double E_CALdw  	= 0.0;
  for( auto cluster : CAL_clusters() ) {
    edm4hep::Vector3f vec = cluster->getPosition();// mm

    if( cluster->getNhits() < variables::Nhits_min ) { continue; }

    if( vec.y > variables::LumiSpecCAL_FiveSigma ) {
      E_CALup = cluster->getEnergy();
    }
    if( vec.y < -variables::LumiSpecCAL_FiveSigma ) {
      E_CALdw = cluster->getEnergy();
    }
  }

  ((TH1D *)gHistList->FindObject("hClusterCount"))->Fill( CAL_clusters().size() );

  if( E_CALup > 1 ) {
    ((TH1D *)gHistList->FindObject("hCALTop_Acceptance"))->Fill( variables::Einput );
  }
  if( E_CALdw > 1 ) {
    ((TH1D *)gHistList->FindObject("hCALBot_Acceptance"))->Fill( variables::Einput );
  }
  if( E_CALup > 1 && E_CALdw > 1) { // Emin ~3.7 GeV
    ((TH2D *)gHistList->FindObject("hCALCluster_Eres"))->Fill( variables::Einput, E_CALup + E_CALdw );
    ((TH1D *)gHistList->FindObject("hCALCoincidence_Acceptance"))->Fill( variables::Einput );
  }

  // Fill the energy histograms
  if( E_CALup > 0 && E_CALdw > 0 ){
    ((TH1D *)gHistList->FindObject("hEnergy"))->Fill( E_CALup + E_CALdw );
    ((TH2D *)gHistList->FindObject("hCAL_Eres"))->Fill( variables::Einput, E_CALup + E_CALdw );
  }
  if( E_CALup > 0 ) { ((TH1D *)gHistList->FindObject("hEup"))->Fill( E_CALup ); }
  if( E_CALdw > 0 ) { ((TH1D *)gHistList->FindObject("hEdw"))->Fill( E_CALdw ); }

}

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void analyzeLumiHits::FinishWithGlobalRootLock() {

  // Do any final calculations here.
}
