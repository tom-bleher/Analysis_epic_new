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
      std::shared_ptr<JDD4hep_service> geoSvc ) {

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
    variables::z_cluster = vec.z;
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
    ((TH2D *)gHistList->FindObject("hCALYE"))->Fill(variables::E_cluster, variables::y_cluster);
    ((TH3D *)gHistList->FindObject("hCALYZE"))->Fill( variables::y_cluster, variables::z_cluster,  variables::E_cluster);
  }

}

//-------------------------------------------------------------------------
void CALAnalysis::FillDiagnostics() {
 
  double E_CALhits_total = 0.0;
  double E_CALtophits_total = 0.0;
  double E_CALbothits_total = 0.0;
  
  for( auto hit : m_CALhits ) { 
    edm4hep::Vector3f vec = hit->getPosition();// mm
    E_CALhits_total += hit->getEnergy();
    ((TH1D *)gHistList->FindObject("hEraw"))->Fill( hit->getEnergy() );
    if (vec.y > 0){
      E_CALtophits_total += hit->getEnergy();
    }
    else if ( vec.y < 0){
      E_CALbothits_total += hit->getEnergy();
    }
  }
  
  if( E_CALhits_total > 0 ) { ((TH1D *)gHistList->FindObject("hErawTotal"))->Fill( E_CALhits_total ); }
  if( E_CALtophits_total > 0 ){((TH1D *)gHistList->FindObject("hErawTotalTop"))->Fill( E_CALtophits_total );}
  if( E_CALbothits_total > 0 ){((TH1D *)gHistList->FindObject("hErawTotalBot"))->Fill( E_CALbothits_total );}

  for( auto adc : m_CALadc ) { ((TH1D *)gHistList->FindObject("hADCsignal"))->Fill( adc->getAmplitude() ); }
  
  ((TH1D *)gHistList->FindObject("hProtoClusterCount"))->Fill( m_CALprotoClusters.size() );
  
  ((TH1D *)gHistList->FindObject("hClusterCount"))->Fill( m_CALclusters.size() );
  
  // SJDK - 13/06/23 - Title of this is a little misleading, this is Egamma compared to the TOTAL energy detected in both spectrometers, should make this clearer.
  ((TH2D *)gHistList->FindObject("hCALCluster_Eres"))->Fill( variables::EgammaMC, m_EtopTotal + m_EbotTotal );
  ((TH2D *)gHistList->FindObject("hCAL_Eres"))->Fill( variables::EgammaMC, (variables::EgammaMC - (m_EtopTotal + m_EbotTotal))/variables::EgammaMC );  
  // New histograms for e-/e+ resolution of each spectrometer
  // Sampling fractions currently based upon cluster energies - make raw versions too?
  if (m_EtopTotal > 0){
    ((TH2D *)gHistList->FindObject("hCALTop_Eres"))->Fill( variables::EelecMC, (variables::EelecMC - (m_EtopTotal))/variables::EelecMC );
    ((TH2D *)gHistList->FindObject("hSampFracTop"))->Fill( variables::EelecMC, ( m_EtopTotal/variables::EelecMC ));
  }
  if (m_EbotTotal > 0){
    ((TH2D *)gHistList->FindObject("hCALBot_Eres"))->Fill( variables::EposMC, (variables::EposMC - (m_EbotTotal))/variables::EposMC );
    ((TH2D *)gHistList->FindObject("hSampFracBot"))->Fill( variables::EposMC, ( m_EbotTotal/variables::EposMC ));
  }

  if ( E_CALtophits_total > 0){
    ((TH2D *)gHistList->FindObject("hSampFracTopRaw"))->Fill( variables::EelecMC, ( E_CALtophits_total/variables::EelecMC ));
  }

  if ( E_CALbothits_total > 0){
    ((TH2D *)gHistList->FindObject("hSampFracBotRaw"))->Fill( variables::EposMC, ( E_CALbothits_total/variables::EposMC ));
  }
  
  if( (m_EtopTotal + m_EbotTotal) > 0 ) { 
    ((TH1D *)gHistList->FindObject("hEnergy"))->Fill( m_EtopTotal + m_EbotTotal ); 
  }

  if ( (m_EtopTotal > 0) && (E_CALtophits_total > 0)){
    ((TH2D *)gHistList->FindObject("hCAL_ClusterFracE_Top"))->Fill( variables::EelecMC, ( m_EtopTotal/E_CALtophits_total));
  }
  if ( (m_EbotTotal > 0) && (E_CALtophits_total > 0)){
    ((TH2D *)gHistList->FindObject("hCAL_ClusterFracE_Bot"))->Fill( variables::EposMC, ( m_EbotTotal/E_CALbothits_total));
  }
  
  if( m_EtopTotal > 0 ) { ((TH1D *)gHistList->FindObject("hEup"))->Fill( m_EtopTotal ); }
  if( m_EbotTotal > 0 ) { ((TH1D *)gHistList->FindObject("hEdw"))->Fill( m_EbotTotal ); }

}

//-------------------------------------------------------------------------
void CALAnalysis::FillAcceptances() {

  // SJDK 14/06/23 - This acceptance definition is a bit odd, this is the detection of an electron or positron at the energy of the incoming gamma, add some new histograms
  if( m_EtopTotal > 1 ) {
    ((TH1D *)gHistList->FindObject("hCALTop_Acceptance"))->Fill( variables::EgammaMC );
  }
  if( m_EbotTotal > 1 ) {
    ((TH1D *)gHistList->FindObject("hCALBot_Acceptance"))->Fill( variables::EgammaMC );
  }
  if( m_EtopTotal > 1 && m_EbotTotal > 1) { // Emin ~3.7 GeV
    ((TH1D *)gHistList->FindObject("hCALCoincidence_Acceptance"))->Fill( variables::EgammaMC );
    ((TH2D *)gHistList->FindObject("hCALCoincidence_Acceptance_v2"))->Fill( (variables::EelecMC + variables::EposMC), (m_EtopTotal + m_EbotTotal)  );
  }

  // SJDK 14/06/23 - New acceptance histograms, individual calorimeter plots
  if( m_EtopTotal > 0){
    ((TH1D *)gHistList->FindObject("hCALTop_Acceptance_v2"))->Fill( variables::EelecMC );
    ((TH2D *)gHistList->FindObject("hCALTop_Acceptance_v3"))->Fill( variables::EelecMC, m_EtopTotal );
  }
  if( m_EbotTotal > 0){
    ((TH1D *)gHistList->FindObject("hCALBot_Acceptance_v2"))->Fill( variables::EposMC );
    ((TH2D *)gHistList->FindObject("hCALBot_Acceptance_v3"))->Fill( variables::EposMC, m_EbotTotal );
  }

  // SJDK 22/06/23 - Some more new acceptance histograms, these are filled under the condition that you have something that vaguely looks correct compared to MC. The intention is to divide the Numer by the Denom histogram subsequently

  // Fill numerator only if within +/- tolerance percentage of truth value (see constants.cc), fill denominator with all truth electrons
  if ( ( m_EtopTotal < (variables::EelecMC + (variables::EelecMC * constants::Cal_Single_AcceptanceTol))) && ( m_EtopTotal > (variables::EelecMC - (variables::EelecMC * constants::Cal_Single_AcceptanceTol))) ){
    ((TH1D *)gHistList->FindObject("hCALTop_Acceptance_v4_Numer"))->Fill( variables::EelecMC );
  }
  ((TH1D *)gHistList->FindObject("hCALTop_Acceptance_v4_Denom"))->Fill( variables::EelecMC );
  
  if ( ( m_EbotTotal < (variables::EposMC + (variables::EposMC * constants::Cal_Single_AcceptanceTol))) && ( m_EbotTotal > (variables::EposMC - (variables::EposMC * constants::Cal_Single_AcceptanceTol))) ){
    ((TH1D *)gHistList->FindObject("hCALBot_Acceptance_v4_Numer"))->Fill( variables::EposMC );
  }
  ((TH1D *)gHistList->FindObject("hCALBot_Acceptance_v4_Denom"))->Fill( variables::EposMC );

  
  if ( ( (m_EbotTotal + m_EtopTotal) < (variables::EgammaMC + (variables::EgammaMC * constants::Cal_Coin_AcceptanceTol))) && ( (m_EbotTotal + m_EtopTotal) > (variables::EgammaMC - (variables::EgammaMC * constants::Cal_Coin_AcceptanceTol))) ){
    ((TH1D *)gHistList->FindObject("hCALCoincidence_Acceptance_v3_Numer"))->Fill( variables::EgammaMC );
  }
  ((TH1D *)gHistList->FindObject("hCALCoincidence_Acceptance_v3_Denom"))->Fill( variables::EgammaMC );

}

//-------------------------------------------------------------------------
void CALAnalysis::CollectGoodClusters() {

  // Filter out unwanted low energy clusters
  // apply calibration correction
  // Put good clusters into a edm4eic::Cluster
  // m_GoodClusters.push_back( );

}

#endif
