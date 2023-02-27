
#include "analyzeLumiHits.h"
#include <services/rootfile/RootFile_service.h>

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
  //rootfile->mkdir("LumiHits")->cd();

  // Create histograms here. e.g.
  hCAL_Acceptance = new TH1D("hCAL_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50);
  hTrackers_Eres = new TH2D("hTrackers_Eres","Tracker E resolution;E_{#gamma} (GeV);(E_{gen}-E_{rec})/E_{gen}",200,0,50, 2000,-1,1);
  
  hTrackers_E = new TH2D("hTrackers_E","Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 2500,0,50);
  hTrackersTop_E = new TH2D("hTrackersTop_E","Top Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 2500,0,50);
  hTrackersBot_E = new TH2D("hTrackersBot_E","Bottom Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 2500,0,50);

  for( int i=0; i < maxModules; i++ ) {
    for( int j=0; j < maxSectors; j++ ) {
      hGlobalXY[i][j] = new TH2D( 
          Form("hGlobalXY_%d%d", i,j), "Global Tracker Hits X vs Y;X (cm);Y (cm)", 120,-30,30, 120,-30,30);
      //hLocalXY[i][j] = new TH2D( 
      //    Form("hLocalXY_%d%d", i,j), "Global Tracker Hits X vs Y;X (cm);Y (cm)", 120,-30,30, 120,-30,30);
    }
  }

  hEraw  = new TH1D("Eraw",  "hit energy (raw)", 2500, 0, 50);
  hErawTotal  = new TH1D("ErawTotal",  "summed hit energy (raw)", 2500, 0, 50);
  hEup  = new TH1D("hEup", "Upper CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50);
  hEdw  = new TH1D("hEdw", "Lower CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50);
  hEnergy  = new TH1D("hEnergy", "CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50);
  hCAL_Eres = new TH2D("hCAL_Eres","CAL E resolution;E_{#gamma} (GeV);E_{rec}", 200,0,50, 2500,0,50);

  hClusterCount = new TH1D("hClusterCount", "Number of clusters / event", 20,-0.5,19.5);

  // spectrometer dimensions/placements in cm
  double SpecMag_to_SpecCAL_DZ = (LumiSpecMag_Z - LumiSpecMag_DZ/2.0) - (LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0);
  LumiSpecTracker_Z1 = LumiSpecMag_Z - LumiSpecMag_DZ/2.0 - 5/6.0*SpecMag_to_SpecCAL_DZ;
  LumiSpecTracker_Z2 = LumiSpecMag_Z - LumiSpecMag_DZ/2.0 - 11/12.0*SpecMag_to_SpecCAL_DZ;
  LumiSpecTracker_Z3 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 1;

  hADCsignal  = new TH1D("hADCsignal", "ADC signal", 16385,-0.5,16384.5);

  hCALCluster_Eres = new TH2D("hCALCluster_Eres", "Egen vs Cluster based photon Erec", 200,0,50, 2500,0,50);
 
  tree_Hits = new TTree("tree_Hits","Hits");
  tree_Hits->Branch("E", &E_hit);
  tree_Hits->Branch("x", &x_hit);
  tree_Hits->Branch("y", &y_hit);
  tree_Hits->Branch("r", &r_hit);

  tree_RecHits = new TTree("tree_RecHits","RecHits");
  tree_RecHits->Branch("E", &E_hit);
  tree_RecHits->Branch("x", &x_hit);
  tree_RecHits->Branch("y", &y_hit);
  tree_RecHits->Branch("r", &r_hit);
  tree_RecHits->Branch("t", &t_hit);

  tree_Clusters = new TTree("tree_Clusters","Clusters");
  tree_Clusters->Branch("Nhits", &Nhits_cluster);
  tree_Clusters->Branch("E", &E_cluster);
  tree_Clusters->Branch("x", &x_cluster);
  tree_Clusters->Branch("y", &y_cluster);
  tree_Clusters->Branch("r", &r_cluster);
  tree_Clusters->Branch("t", &t_cluster);
  
  tree_MergedClusters = new TTree("tree_MergedClusters","MergedClusters");
  tree_MergedClusters->Branch("Nhits", &Nhits_cluster);
  tree_MergedClusters->Branch("E", &E_cluster);
  tree_MergedClusters->Branch("x", &x_cluster);
  tree_MergedClusters->Branch("y", &y_cluster);
  tree_MergedClusters->Branch("r", &r_cluster);
  tree_MergedClusters->Branch("t", &t_cluster);

}

//-------------------------------------------
// ProcessSequential
//-------------------------------------------
void analyzeLumiHits::ProcessSequential(const std::shared_ptr<const JEvent>& event) {

  //cout<<"New Event"<<endl;
  int hitcount = 0;

  // CALs
  int counts_CALup	= 0;
  int counts_CALdw	= 0;
  double E_CALup  	= 0.0;
  double E_CALdw  	= 0.0;
  double E_total        = 0.0;

  // Trackers
  int counts_Tr[maxModules][maxSectors] = {0};
  //global position
  double gpos_x[maxModules][maxSectors] = {0.0};
  double gpos_y[maxModules][maxSectors] = {0.0};
  //local position
  double lpos_x[maxModules][maxSectors] = {0.0};
  double lpos_y[maxModules][maxSectors] = {0.0};

  auto app = GetApplication();
  m_geoSvc = app->template GetService<JDD4hep_service>();

  Einput = 0;
  Ntrackers = 0;
  app->SetDefaultParameter("analyzeLumiHits:Egen", Einput);
  app->SetDefaultParameter("analyzeLumiHits:Ntrackers", Ntrackers);
  //cout<<"E gen = "<<Einput<<"   Ntrackers = "<<Ntrackers<<endl;

  //Calorimeter Input___________________________________________
  for( auto hit : CAL_hits()  ) {

    hitcount++;

    const auto id       = hit->getCellID();

    //m_geoSvc 		= app->template GetService<JDD4hep_service>();
    auto id_dec 	= m_geoSvc->detector()->readout( "LumiSpecCALHits" ).idSpec().decoder();

    int sector_idx 	= id_dec->index( "sector" ); //Top (0) and Bottom (1)
    int module_idx 	= id_dec->index( "module" ); //8x8 Matrix of bars (0-127)

    const int sec_id 	= (int) id_dec->get( id, sector_idx );
    const int mod_id 	= (int) id_dec->get( id, module_idx );

    hEraw->Fill( hit->getEnergy() );

    E_total += hit->getEnergy();

    if(sec_id == 0) { 
      E_CALup += hit->getEnergy(); 
      counts_CALup++; 
    }
    if(sec_id == 1) {
      E_CALdw += hit->getEnergy(); 
      counts_CALdw++;
    }
  } //Calorimeter hits close

  if( E_total > 0 ) { hErawTotal->Fill( E_total ); }

  //Tracker Input Section______________________________________
  for( auto hit : Tracker_hits() ){

    const auto id 	= hit->getCellID();

    //m_geoSvc 		= app->template GetService<JDD4hep_service>();
    auto id_dec 	= m_geoSvc->detector()->readout( "LumiSpecTrackerHits" ).idSpec().decoder();

    int sector_idx 	= id_dec->index( "sector" ); //Top (0) and Bottom Layer (1)
    int module_idx 	= id_dec->index( "module" ); //Front(0) and Back (1)

    const int sec_id 	= (int) id_dec->get( id, sector_idx );
    const int mod_id 	= (int) id_dec->get( id, module_idx );

    //for global positions
    const auto gpos 	= m_geoSvc->cellIDPositionConverter()->position(id);

    //for local positions
    const auto volman 	= m_geoSvc->detector()->volumeManager();
    const auto alignment = volman.lookupDetElement(id).nominal();
    const auto lpos 	= alignment.worldToLocal( dd4hep::Position( gpos.x(), gpos.y(), gpos.z() ) );

    if( sec_id < maxSectors && mod_id < maxModules ) {
      gpos_x[mod_id][sec_id] += gpos.x();
      gpos_y[mod_id][sec_id] += gpos.y();

      lpos_x[mod_id][sec_id] += lpos.x();
      lpos_y[mod_id][sec_id] += lpos.y();

      counts_Tr[mod_id][sec_id]++;
    }
  } //Tracker hits close

  // Normalization and histograming Section________________________________________
  bool GoodCALsHit = true;
  bool AllTrackersHit = true;
  bool Mod1TrackerHit = true;
  bool Mod2TrackerHit = true;
  bool Mod3TrackerHit = true;

  // TODO: temporary hard cut on lower E limit.  Should use a clusterizer instead
  if( E_CALup < 2.5 || E_CALdw < 2.5 ) { GoodCALsHit = false; }

  for( int i=0; i < maxModules; i++ ) {
    for( int j=0; j < maxSectors; j++ ) {

      if( counts_Tr[i][j] <= 0 ) { 
        
        // i==0 corresponds to tracker closest to IP
        if( i >= (maxModules - Ntrackers) ) { AllTrackersHit = false; }
        if( i == 0 ) { Mod1TrackerHit = false; }
        if( i == 1 ) { Mod2TrackerHit = false; }
        if( i == 2 ) { Mod3TrackerHit = false; }

        continue;
      }

      gpos_x[i][j] /= counts_Tr[i][j];
      gpos_y[i][j] /= counts_Tr[i][j];
      lpos_x[i][j] /= counts_Tr[i][j];
      lpos_y[i][j] /= counts_Tr[i][j];
    }
  }

  if( GoodCALsHit ) {
    hCAL_Acceptance->Fill( Einput );
  }

  if( AllTrackersHit ){

    double E_trackers = TrackerErec( gpos_y );
    cout<<"Egen = "<<Einput<<"  E_trackers = "<<E_trackers<<endl;
    hTrackers_Eres->Fill( Einput, (Einput - E_trackers)/Einput );
    hTrackers_E->Fill( Einput, E_trackers );

    // Fill the energy histograms
    if( (E_CALup > 0) && (E_CALdw > 0) ){
      hEnergy->Fill( E_CALup + E_CALdw );
      hCAL_Eres->Fill( Einput, E_CALup + E_CALdw );
      hEup->Fill( E_CALup );
      hEdw->Fill( E_CALdw );

      // Fill of XY-histograms.
      for( int i=0; i < maxModules; i++ ) {
        for( int j=0; j < maxSectors; j++ ) {
          if( counts_Tr[i][j] <= 0 ) { continue; }
          hGlobalXY[i][j]->Fill( gpos_x[i][j], gpos_y[i][j] ); 
          //hLocalXY[i][j]->Fill( lpos_x[i][j], lpos_y[i][j] );
        } // j
      } // i

    } //Energy check

  } // AllTrackersHit

  // Digitized ADC raw hits
  for( auto adc : CAL_adc() ) hADCsignal->Fill( adc->getAmplitude() );

  ///////////////////////////////////////////////////////////////////////
  // G4 Hits
  for( auto hit : CAL_hits() ) {
    E_hit = hit->getEnergy() / dd4hep::GeV;
    edm4hep::Vector3f vec = hit->getPosition();// mm
    x_hit = vec.x / dd4hep::mm;
    y_hit = vec.y / dd4hep::mm;
    r_hit = sqrt( pow(x_hit, 2) + pow(y_hit, 2) );

    tree_Hits->Fill();
  }

  ///////////////////////////////////////////////////////////////////////
  // Rec Hits
  for( auto hit : CAL_rechits() ) {
    E_hit = hit->getEnergy() / dd4hep::GeV;
    edm4hep::Vector3f vec = hit->getPosition();// mm
    x_hit = vec.x / dd4hep::mm;
    y_hit = vec.y / dd4hep::mm;
    r_hit = sqrt( pow(x_hit, 2) + pow(y_hit, 2) );
    t_hit = hit->getTime();

    tree_RecHits->Fill();
  }

  ///////////////////////////////////////////////////////////////////////
  // Reconstructed Clusters
  double top_E = 0, bottom_E = 0;
  double photon_E = 0;

  for( auto cluster : CAL_clusters() ) {
    Nhits_cluster = cluster->getNhits();
    E_cluster = cluster->getEnergy() / dd4hep::GeV;
    edm4hep::Vector3f vec = cluster->getPosition();// mm
    x_cluster = vec.x / dd4hep::mm;
    y_cluster = vec.y / dd4hep::mm;
    r_cluster = sqrt( pow(x_cluster, 2) + pow(y_cluster, 2) );
    t_cluster = cluster->getTime();

    tree_Clusters->Fill();

    if( Nhits_cluster < Nhits_min ) { continue; }

    if( y_cluster > LumiSpecCAL_DXY + LumiSpecCAL_FiveSigma ) {
      top_E = E_cluster;
    }
    if( y_cluster < -(LumiSpecCAL_DXY + LumiSpecCAL_FiveSigma) ) {
      bottom_E = E_cluster;
    }
  }

  hClusterCount->Fill( CAL_clusters().size() );

  if( top_E > 0 && bottom_E > 0 ) { 
    photon_E = top_E + bottom_E; 
    hCALCluster_Eres->Fill( Einput, photon_E );
  }

  /////////////////////////////////////////////////////////////////////////
  //// Merged Clusters
  for( auto cluster : CAL_mergedClusters() ) {
    Nhits_cluster = cluster->getNhits();
    E_cluster = cluster->getEnergy() / dd4hep::GeV;
    edm4hep::Vector3f vec = cluster->getPosition();// mm
    x_cluster = vec.x / dd4hep::mm;
    y_cluster = vec.y / dd4hep::mm;
    r_cluster = sqrt( pow(x_cluster, 2) + pow(y_cluster, 2) );
    t_cluster = cluster->getTime();
    
    tree_MergedClusters->Fill();
  }

  //End of the Sequential Process Function
} //sequence close

double analyzeLumiHits::TrackerErec( double y[maxModules][maxSectors] ) {

  // calculate electron track slope
  double slopes[maxSectors] = {0};

  for( int sec = 0; sec < maxSectors; sec++ ) {
    double sumZY = 0;
    double sumZZ = 0;
    double sumY = 0;
    double sumZ = 0;

    double Zs[3] = {LumiSpecTracker_Z1, LumiSpecTracker_Z2, LumiSpecTracker_Z3};

    for( int mod = maxModules - Ntrackers; mod < maxModules; mod++ ) {
      sumZY += Zs[mod] * y[mod][sec];
      sumZZ += Zs[mod] * Zs[mod];
      sumY += y[mod][sec];
      sumZ += Zs[mod];
    }
 
    // least-squares regression formula
    if( Ntrackers > 1 ) {
      slopes[sec] = (Ntrackers * sumZY - sumZ*sumY) / (Ntrackers * sumZZ - sumZ*sumZ);
    }
    else if( Ntrackers == 1 ) {
      double Zrel = fabs(sumZ) - fabs(LumiSpecMag_Z) + fabs(LumiSpecMag_DZ/2.0);
      slopes[sec] = tan( asin( sumY / (Zrel - LumiSpecMag_DZ/2.0) ) );
    }
    else {
      slopes[sec] = 0;
    }
  }

  double Etop = pT / fabs( sin( atan(slopes[0]) ) ); 
  double Ebot = pT / fabs( sin( atan(slopes[1]) ) ); 
  
  hTrackersTop_E->Fill( Einput, Etop );
  hTrackersBot_E->Fill( Einput, Ebot );
  
  return (Etop + Ebot);
}

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void analyzeLumiHits::FinishWithGlobalRootLock() {

  // Do any final calculations here.
}


