#ifndef CAL_CC
#define CAL_CC

#include "cal.h"

// Constructor
//-------------------------------------------------------------------------
CALAnalysis::CALAnalysis() {

  LoadCalibration();

}

// The data structures from the simulation have to be passed for each event
//-------------------------------------------------------------------------
void CALAnalysis::Prepare(std::vector<const edm4hep::SimCalorimeterHit*> &CalHits, 
      std::vector<const edm4hep::RawCalorimeterHit*> &CALadc, 
      std::vector<const edm4eic::CalorimeterHit*> &CALrecHits,
      std::vector<const edm4eic::ProtoCluster*> &CALprotoClusters,
      std::vector<const edm4eic::Cluster*> &CALClusters,
      std::shared_ptr<DD4hep_service> geoSvc ) {

  m_CALhits = CalHits;
  m_CALadc = CALadc;
  m_CALrecHits = CALrecHits;
  m_CALprotoClusters = CALprotoClusters;
  m_CALclusters = CALClusters;
  m_geoSvc = geoSvc;

  m_GoodClusters.clear();

  m_EtopTotal = 0.0;
  m_EbotTotal = 0.0;
  for( auto cluster : m_CALclusters ) {
    edm4hep::Vector3f vec = cluster->getPosition();// mm

    if( cluster->getNhits() < variables::Nhits_min ) { continue; }
    
    if( vec.y > variables::LumiSpecCAL_FiveSigma ) {
      m_EtopTotal = cluster->getEnergy();
    }
    if( vec.y < -variables::LumiSpecCAL_FiveSigma ) {
      m_EbotTotal = cluster->getEnergy();
    }
  }

}

//-------------------------------------------------------------------------
void CALAnalysis::LoadCalibration() {

  // Load the calibration matrix from the utilities directory
  // Check that it exists.  If not there, issue a warning or abort the code
  // m_calibration = ...

}

//-------------------------------------------------------------------------
void CALAnalysis::FillTrees() {

  ///////////////////////////////////////////////////////////////////////
  // CAL G4 Hits Tree
  map<string, int> CALfield_idx_Map{ {"sector", 0}, {"module", 0}, {"fiber_x", 0}, {"fiber_y", 0}};

  for( auto hit : m_CALhits ) {

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
  for( auto hit : m_CALrecHits ) {
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
  for( auto cluster : m_CALclusters ) {
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
void CALAnalysis::FillDiagnostics() {
 
  double E_CALhits_total = 0.0;
  for( auto hit : m_CALhits ) { 
    E_CALhits_total += hit->getEnergy();
    ((TH1D *)gHistList->FindObject("hEraw"))->Fill( hit->getEnergy() ); 
  }
  
  if( E_CALhits_total > 0 ) { ((TH1D *)gHistList->FindObject("hErawTotal"))->Fill( E_CALhits_total ); }

  for( auto adc : m_CALadc ) { ((TH1D *)gHistList->FindObject("hADCsignal"))->Fill( adc->getAmplitude() ); }
  
  ((TH1D *)gHistList->FindObject("hProtoClusterCount"))->Fill( m_CALprotoClusters.size() );
  
  ((TH1D *)gHistList->FindObject("hClusterCount"))->Fill( m_CALclusters.size() );

  ((TH2D *)gHistList->FindObject("hCALCluster_Eres"))->Fill( variables::Ephoton, m_EtopTotal + m_EbotTotal );

  if( (m_EtopTotal + m_EbotTotal) > 0 ) { 
    ((TH1D *)gHistList->FindObject("hEnergy"))->Fill( m_EtopTotal + m_EbotTotal ); 
  }
  
  if( m_EtopTotal > 0 ) { ((TH1D *)gHistList->FindObject("hEup"))->Fill( m_EtopTotal ); }
  if( m_EbotTotal > 0 ) { ((TH1D *)gHistList->FindObject("hEdw"))->Fill( m_EbotTotal ); }

}

//-------------------------------------------------------------------------
void CALAnalysis::FillAcceptances() {

  if( m_EtopTotal > 1 ) {
    ((TH1D *)gHistList->FindObject("hCALTop_Acceptance"))->Fill( variables::Ephoton );
  }
  if( m_EbotTotal > 1 ) {
    ((TH1D *)gHistList->FindObject("hCALBot_Acceptance"))->Fill( variables::Ephoton );
  }
  if( m_EtopTotal > 1 && m_EbotTotal > 1) { // Emin ~3.7 GeV
    ((TH1D *)gHistList->FindObject("hCALCoincidence_Acceptance"))->Fill( variables::Ephoton );
  }

}

//-------------------------------------------------------------------------
void CALAnalysis::CollectGoodClusters() {

  // Filter out unwanted low energy clusters
  // apply calibration correction
  // Put good clusters into a edm4eic::Cluster
  // m_GoodClusters.push_back( );

}

#endif
