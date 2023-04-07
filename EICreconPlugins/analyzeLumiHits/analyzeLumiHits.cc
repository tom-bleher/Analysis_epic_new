
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

  gHistList = new THashList();


  // Create histograms here. e.g.
  gHistList->Add( new TH1D("hCAL_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );
  gHistList->Add( new TH2D("hTrackers_Eres","Tracker E resolution;E_{#gamma} (GeV);(E_{gen}-E_{rec})/E_{gen}",200,0,50, 200,-1,1) );
  
  gHistList->Add( new TH2D("hTrackers_E","Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50) );
  gHistList->Add( new TH2D("hTrackersTop_E","Top Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50) );
  gHistList->Add( new TH2D("hTrackersBot_E","Bottom Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50) );

  gHistList->Add( new TH1D("hTrackers_X_allPairs","X at converter;(x_{electron} + x_{positron})/2 (cm);counts",1000,-5,5) );
  gHistList->Add( new TH1D("hTrackers_Y_allPairs","Y at converter;(y_{electron} + y_{positron})/2 (cm);counts",1000,-5,5) );
  gHistList->Add( new TH1D("hTrackers_DX_allPairs","#DeltaX at converter;x_{electron} - x_{positron} (cm);counts",1000,-5,5) );
  gHistList->Add( new TH1D("hTrackers_DY_allPairs","#DeltaY at converter;y_{electron} - y_{positron} (cm);counts",1000,-5,5) );

  gHistList->Add( new TH1D("hTrackers_X","X at converter;(x_{electron} + x_{positron})/2 (cm);counts",1000,-5,5) );
  gHistList->Add( new TH1D("hTrackers_Y","Y at converter;(y_{electron} + y_{positron})/2 (cm);counts",1000,-5,5) );
  gHistList->Add( new TH2D("hTrackers_X_BotVsTop","X at converter;X positron (cm);X electron (cm)", 1000,-5,5, 1000,-5,5) );
  gHistList->Add( new TH2D("hTrackers_Y_BotVsTop","Y at converter;Y positron (cm);Y electron (cm)", 1000,-5,5, 1000,-5,5) );
  
  gHistList->Add( new TH1D("hTrackers_InvMass_allPairs","tracker pair mass",1000,0,1) );
  gHistList->Add( new TH1D("hTrackers_InvMass","tracker pair mass",1000,0,1) );

  for( int i=0; i < maxModules; i++ ) {
    for( int j=0; j < maxSectors; j++ ) {
      gHistList->Add( new TH2D( 
          Form("hGlobalXY_%d_%d", i,j), "Global Tracker Hits X vs Y;X (cm);Y (cm)", 120,-30,30, 120,-30,30) );
    }
  }

  gHistList->Add( new TH1D("hEraw",  "hit energy (raw)", 2500, 0, 50) );
  gHistList->Add( new TH1D("hErawTotal",  "summed hit energy (raw)", 2500, 0, 50) );
  gHistList->Add( new TH1D("hEup", "Upper CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50) );
  gHistList->Add( new TH1D("hEdw", "Lower CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50) );
  gHistList->Add( new TH1D("hEnergy", "CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50) );
  gHistList->Add( new TH2D("hCAL_Eres","CAL E resolution;E_{#gamma} (GeV);E_{rec}", 200,0,50, 2500,0,50) );

  gHistList->Add( new TH1D("hProtoClusterCount", "Number of proto island clusters / event", 20,-0.5,19.5) );
  gHistList->Add( new TH1D("hClusterCount", "Number of clusters / event;# clusters / event", 20,-0.5,19.5) );

  gHistList->Add( new TH1D("hTrackChi2", "Track #chi^{2} / Nlayers;#chi^{2};counts", 1000,0,1) );
  gHistList->Add( new TH1D("hTrackersSlope", "", 1000,0,1) );

  // spectrometer dimensions/placements in cm
  double SpecMag_to_SpecCAL_DZ = (LumiSpecMag_Z - LumiSpecMag_DZ/2.0) - (LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0);
  LumiSpecTracker_Z1 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 21;
  LumiSpecTracker_Z2 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 11;
  LumiSpecTracker_Z3 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 1;

  Tracker_Zs = {LumiSpecTracker_Z1, LumiSpecTracker_Z2, LumiSpecTracker_Z3};
  for( auto el : Tracker_Zs ) { Tracker_meanZ += el; }
  Tracker_meanZ /= double(Tracker_Zs.size()); 

  gHistList->Add( new TH1D("hADCsignal", "ADC signal", 16385,-0.5,16384.5) );

  gHistList->Add( new TH2D("hCALCluster_Eres", "Egen vs Cluster based photon Erec", 200,0,50, 2500,0,50) );
 
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
  
  tree_Tracks = new TTree("tree_Tracks","Tracks");
  tree_Tracks->Branch("Top_X0", &treeTracks.X0_e);
  tree_Tracks->Branch("Top_Y0", &treeTracks.Y0_e);
  tree_Tracks->Branch("Top_slopeX", &treeTracks.slopeX_e);
  tree_Tracks->Branch("Top_slopeY", &treeTracks.slopeY_e);
  tree_Tracks->Branch("Bot_X0", &treeTracks.X0_p);
  tree_Tracks->Branch("Bot_Y0", &treeTracks.Y0_p);
  tree_Tracks->Branch("Bot_slopeX", &treeTracks.slopeX_p);
  tree_Tracks->Branch("Bot_slopeY", &treeTracks.slopeY_p);
}

//-------------------------------------------
// ProcessSequential
//-------------------------------------------
void analyzeLumiHits::ProcessSequential(const std::shared_ptr<const JEvent>& event) {

  //cout<<"New Event"<<endl;

  auto app = GetApplication();
  m_geoSvc = app->template GetService<JDD4hep_service>();

  Einput = 0;
  Ntrackers = 0;
  app->GetParameter("analyzeLumiHits:Egen", Einput);
  app->GetParameter("analyzeLumiHits:Ntrackers", Ntrackers);
  //cout<<"E gen = "<<Einput<<"   Ntrackers = "<<Ntrackers<<endl;

  ///////////////////////////////////////////////////////////////////////
  // Digitized ADC raw hits
  for( auto adc : CAL_adc() ) ((TH1D *)gHistList->FindObject("hADCsignal"))->Fill( adc->getAmplitude() );

  ///////////////////////////////////////////////////////////////////////
  // G4 Hits
  for( auto hit : CAL_hits() ) {
    E_hit = hit->getEnergy();
    edm4hep::Vector3f vec = hit->getPosition();// mm
    x_hit = vec.x;
    y_hit = vec.y;
    r_hit = sqrt( pow(x_hit, 2) + pow(y_hit, 2) );

    tree_Hits->Fill();
  }

  ///////////////////////////////////////////////////////////////////////
  // Rec Hits
  for( auto hit : CAL_rechits() ) {
    E_hit = hit->getEnergy();
    edm4hep::Vector3f vec = hit->getPosition();// mm
    x_hit = vec.x;
    y_hit = vec.y;
    r_hit = sqrt( pow(x_hit, 2) + pow(y_hit, 2) );
    t_hit = hit->getTime();

    tree_RecHits->Fill();
  }

  ///////////////////////////////////////////////////////////////////////
  // Proto Island Clusters

  ((TH1D *)gHistList->FindObject("hProtoClusterCount"))->Fill( CAL_protoClusters().size() );

  ///////////////////////////////////////////////////////////////////////
  // Reconstructed Clusters
  double E_CALup  	= 0.0;
  double E_CALdw  	= 0.0;
  for( auto cluster : CAL_clusters() ) {
    Nhits_cluster = cluster->getNhits();
    E_cluster = cluster->getEnergy();
    edm4hep::Vector3f vec = cluster->getPosition();// mm
    x_cluster = vec.x;
    y_cluster = vec.y;
    r_cluster = sqrt( pow(x_cluster, 2) + pow(y_cluster, 2) );
    t_cluster = cluster->getTime();

    tree_Clusters->Fill();

    if( Nhits_cluster < Nhits_min ) { continue; }

    if( y_cluster > LumiSpecCAL_DXY + LumiSpecCAL_FiveSigma ) {
      E_CALup = E_cluster;
    }
    if( y_cluster < -(LumiSpecCAL_DXY + LumiSpecCAL_FiveSigma) ) {
      E_CALdw = E_cluster;
    }
  }

  ((TH1D *)gHistList->FindObject("hClusterCount"))->Fill( CAL_clusters().size() );

  if( E_CALup > 1 && E_CALdw > 1) { // Emin ~3.7 GeV
    ((TH2D *)gHistList->FindObject("hCALCluster_Eres"))->Fill( Einput, E_CALup + E_CALdw );
    ((TH1D *)gHistList->FindObject("hCAL_Acceptance"))->Fill( Einput );
  }

  ///////////////////////////////////////////////////////////////////////
  //Calorimeter hits
  double E_CALhits_total        = 0.0;
  for( auto hit : CAL_hits()  ) {

    const auto id       = hit->getCellID();

    //m_geoSvc 		= app->template GetService<JDD4hep_service>();
    auto id_dec 	= m_geoSvc->detector()->readout( "LumiSpecCALHits" ).idSpec().decoder();

    int sector_idx 	= id_dec->index( "sector" ); //Top (0) and Bottom (1)
    int module_idx 	= id_dec->index( "module" ); //8x8 Matrix of bars (0-127)

    const int sec_id 	= (int) id_dec->get( id, sector_idx );
    const int mod_id 	= (int) id_dec->get( id, module_idx );

    ((TH1D *)gHistList->FindObject("hEraw"))->Fill( hit->getEnergy() );

    E_CALhits_total += hit->getEnergy();

  } //Calorimeter hits close

  if( E_CALhits_total > 0 ) { ((TH1D *)gHistList->FindObject("hErawTotal"))->Fill( E_CALhits_total ); }

  ///////////////////////////////////////////////////////////////////////
  // Tracker Input Section
  vector<MyHit> TopTracker1;
  vector<MyHit> TopTracker2;
  vector<MyHit> TopTracker3;
  vector<MyHit> BotTracker1;
  vector<MyHit> BotTracker2;
  vector<MyHit> BotTracker3;
  
  //global position
  double gpos_x[maxModules][maxSectors] = {0.0};
  double gpos_y[maxModules][maxSectors] = {0.0};
  int counts_Tr[maxModules][maxSectors] = {0};
  
  for( auto hit : Tracker_hits() ){

    const auto id 	= hit->getCellID();
    
    bool secondary = (hit->getQuality() == 0) ? false : true; // 1 << 30 if produced by secondary (edm4hep docs)

    auto id_dec 	= m_geoSvc->detector()->readout( "LumiSpecTrackerHits" ).idSpec().decoder();

    int sector_idx 	= id_dec->index( "sector" ); //Top (0) and Bottom Layer (1)
    int module_idx 	= id_dec->index( "module" ); //Front(0) and Back (1)

    const int sec_id 	= (int) id_dec->get( id, sector_idx ); // top or bottom
    const int mod_id 	= (int) id_dec->get( id, module_idx ); // layer, closet to furthest from IP

    //for global positions
    const auto gpos 	= m_geoSvc->cellIDPositionConverter()->position(id); // cm

    //for local positions
    const auto volman 	= m_geoSvc->detector()->volumeManager();
    const auto alignment = volman.lookupDetElement(id).nominal();
    const auto lpos 	= alignment.worldToLocal( dd4hep::Position( gpos.x(), gpos.y(), gpos.z() ) );

    if( sec_id < maxSectors && mod_id < maxModules ) {

      MyHit myhit = {gpos.x(), gpos.y(), Tracker_Zs[mod_id]}; // cm
      //cout<<setprecision(6)<<"hit X: "<<get<0>(myhit)<<"   hit Y: "<<get<1>(myhit)<<"   hit Z: "<<get<2>(myhit)<<endl;

      if( sec_id == 0 ) { // top
        if( mod_id == 0 ) { // closest to IP
          if( ! PixelOverlap( myhit, TopTracker1 ) ) { TopTracker1.push_back( myhit ); }
        }
        if( mod_id == 1 ) {
          if( ! PixelOverlap( myhit, TopTracker2 ) ) { TopTracker2.push_back( myhit ); }
        }
        if( mod_id == 2 ) {
          if( ! PixelOverlap( myhit, TopTracker3 ) ) { TopTracker3.push_back( myhit ); }
        }
      }
      if( sec_id == 1 ) { // bottom
        if( mod_id == 0 ) { // closest to IP
          if( ! PixelOverlap( myhit, BotTracker1 ) ) { BotTracker1.push_back( myhit ); }
        }
        if( mod_id == 1 ) {
          if( ! PixelOverlap( myhit, BotTracker2 ) ) { BotTracker2.push_back( myhit ); }
        }
        if( mod_id == 2 ) {
          if( ! PixelOverlap( myhit, BotTracker3 ) ) { BotTracker3.push_back( myhit ); }
        }
      }

      gpos_x[mod_id][sec_id] += gpos.x();
      gpos_y[mod_id][sec_id] += gpos.y();

      counts_Tr[mod_id][sec_id]++;
    }
  } //Tracker hits close

  ///////////////////////////////////////////////////////////////////////
  // Tracking Section
  vector<TrackClass> AllTopTracks;
  vector<TrackClass> AllBotTracks;
  vector<vector<MyHit>> TopHitSet = {TopTracker1, TopTracker2, TopTracker3};
  vector<vector<MyHit>> BotHitSet = {BotTracker1, BotTracker2, BotTracker3};

  AssembleTracks( &AllTopTracks, TopHitSet );
  AssembleTracks( &AllBotTracks, BotHitSet );
  
  for( auto track : AllTopTracks ) { ((TH1D *)gHistList->FindObject("hTrackChi2"))->Fill( track.Chi2/3. ); }
  for( auto track : AllBotTracks ) { ((TH1D *)gHistList->FindObject("hTrackChi2"))->Fill( track.Chi2/3. ); }

  // Remove high Chi2 tracks (likely due to secondary hits)
  vector<TrackClass> TopTracks;
  vector<TrackClass> BotTracks;
  TreeTrackClass tracks;
  for( auto track : AllTopTracks ) { 
    if( track.Chi2/3. < max_chi2ndf ) { 
      TopTracks.push_back( track );
      tracks.X0_e.push_back( XatConverter( track ) );
      tracks.Y0_e.push_back( YatConverter( track ) );
      tracks.slopeX_e.push_back( track.slopeX );
      tracks.slopeY_e.push_back( track.slopeY );
    } 
  }
  for( auto track : AllBotTracks ) { 
    if( track.Chi2/3. < max_chi2ndf ) { 
      BotTracks.push_back( track ); 
      tracks.X0_p.push_back( XatConverter( track ) );
      tracks.Y0_p.push_back( YatConverter( track ) );
      tracks.slopeX_p.push_back( track.slopeX );
      tracks.slopeY_p.push_back( track.slopeY );
    } 
  }
  treeTracks = tracks;

  tree_Tracks->Fill();

  //PrintTrackInfo(TopTracks, BotTracks);
 
  // Loop over good tracks
  for( auto track : TopTracks ) {
    double Etop = TrackerErec( track.slopeY );
    ((TH2D *)gHistList->FindObject("hTrackersTop_E"))->Fill( Einput, Etop );
    ((TH1D *)gHistList->FindObject("hTrackersSlope"))->Fill( track.slopeY );
  }
  for( auto track : BotTracks ) {
    double Ebot = TrackerErec( track.slopeY );
    ((TH2D *)gHistList->FindObject("hTrackersBot_E"))->Fill( Einput, Ebot );
    ((TH1D *)gHistList->FindObject("hTrackersSlope"))->Fill( track.slopeY );
  }

  for( auto topTrack : TopTracks ) {

    double Etop = TrackerErec( topTrack.slopeY );
    double xtop_c = XatConverter( topTrack );
    double ytop_c = YatConverter( topTrack );

    for( auto botTrack : BotTracks ) {
      
      double Ebot = TrackerErec( botTrack.slopeY );
      double xbot_c = XatConverter( botTrack );
      double ybot_c = YatConverter( botTrack );

      double pairMass = GetPairMass( topTrack, botTrack );
      
      ((TH1D *)gHistList->FindObject("hTrackers_InvMass_allPairs"))->Fill( pairMass );
      ((TH1D *)gHistList->FindObject("hTrackers_X_allPairs"))->Fill( (xtop_c + xbot_c)/2. );
      ((TH1D *)gHistList->FindObject("hTrackers_Y_allPairs"))->Fill( (ytop_c + ybot_c)/2. );
      ((TH1D *)gHistList->FindObject("hTrackers_DX_allPairs"))->Fill( xtop_c - xbot_c );
      ((TH1D *)gHistList->FindObject("hTrackers_DY_allPairs"))->Fill( ytop_c - ybot_c );

      if( fabs(xtop_c - xbot_c) > 2*Tracker_sigma || fabs(ytop_c - ybot_c) > 2*Tracker_sigma ) { continue; }
      //if( fabs(ytop_c - ybot_c) > 2*Tracker_sigma ) { continue; }
      
      ((TH1D *)gHistList->FindObject("hTrackers_InvMass"))->Fill( pairMass );
      ((TH1D *)gHistList->FindObject("hTrackers_X"))->Fill( (xtop_c + xbot_c)/2. );
      ((TH1D *)gHistList->FindObject("hTrackers_Y"))->Fill( (ytop_c + ybot_c)/2. );
      ((TH2D *)gHistList->FindObject("hTrackers_Eres"))->Fill( Einput, (Einput - (Etop + Ebot) )/Einput );
      ((TH2D *)gHistList->FindObject("hTrackers_E"))->Fill( Einput, Etop + Ebot );
      ((TH2D *)gHistList->FindObject("hTrackers_X_BotVsTop"))->Fill( xtop_c, xbot_c );
      ((TH2D *)gHistList->FindObject("hTrackers_Y_BotVsTop"))->Fill( ytop_c, ybot_c );
    }
  }

  if( TopTracks.size() > 0 && BotTracks.size() > 0 ){
    // Fill of XY-histograms.
    for( int i=0; i < maxModules; i++ ) {
      for( int j=0; j < maxSectors; j++ ) {
        if( counts_Tr[i][j] <= 0 ) { continue; }
        gpos_x[i][j] /= counts_Tr[i][j];
        gpos_y[i][j] /= counts_Tr[i][j];

        ((TH1D *)gHistList->FindObject(Form("hGlobalXY_%i_%i",i,j) ))->Fill( gpos_x[i][j], gpos_y[i][j] ); 
      } // j
    } // i
  } // Top and Bottom track check

  // Fill the energy histograms
  if( (E_CALup > 0) && (E_CALdw > 0) ){
    ((TH1D *)gHistList->FindObject("hEnergy"))->Fill( E_CALup + E_CALdw );
    ((TH2D *)gHistList->FindObject("hCAL_Eres"))->Fill( Einput, E_CALup + E_CALdw );
    ((TH1D *)gHistList->FindObject("hEup"))->Fill( E_CALup );
    ((TH1D *)gHistList->FindObject("hEdw"))->Fill( E_CALdw );
  } //Energy check

} // End of the Sequential Process Function

//-------------------------------------------------------------------------
double analyzeLumiHits::TrackerErec( double slopeY ) {

  double sinTheta = fabs( sin( atan(slopeY) ) );
 
  if( sinTheta == 0 ) { return 0.0; }

  double E = pT / sinTheta;
  
  return E;
}

//-------------------------------------------------------------------------
bool analyzeLumiHits::PixelOverlap( MyHit hit, vector<MyHit> trackSet ) {

  for( auto el : trackSet ) {
    double delta = pow( std::get<0>(hit) - std::get<0>(el), 2);
    delta += pow( std::get<1>(hit) - std::get<1>(el), 2);

    if( sqrt(delta) < Tracker_pixelSize ) { return true; }
  }

  return false;
}

//-------------------------------------------------------------------------
void analyzeLumiHits::AssembleTracks( vector<TrackClass> *tracks, vector<vector<MyHit>> hitSet ) {

  // units of hits are in cm

  for( auto hit1 : hitSet[0] ) { // tracker hit closest to IP

    for( auto hit2 : hitSet[1] ) {

      for( auto hit3 : hitSet[2] ) {

        double sum_x = std::get<0>(hit1) + std::get<0>(hit2) + std::get<0>(hit3);
        double sum_y = std::get<1>(hit1) + std::get<1>(hit2) + std::get<1>(hit3);
        double sum_z = std::get<2>(hit1) + std::get<2>(hit2) + std::get<2>(hit3);
        double sum_zx = std::get<2>(hit1)*std::get<0>(hit1) + std::get<2>(hit2)*std::get<0>(hit2) + std::get<2>(hit3)*std::get<0>(hit3);
        double sum_zy = std::get<2>(hit1)*std::get<1>(hit1) + std::get<2>(hit2)*std::get<1>(hit2) + std::get<2>(hit3)*std::get<1>(hit3);
        double sum_zz = pow(std::get<2>(hit1), 2) + pow(std::get<2>(hit2), 2) + pow(std::get<2>(hit3), 2);

        // Least squares regression algorithm: assumes equal Gaussian errors with each hit point
        // y = m_y * z + y0
        // x = m_x * z + x0
        // m_y = Sum( (z_i - <z>)*(y_i - <y>) ) / Sum( (z_i - <z>)^2 )
        //     = ( N*Sum(z_i*y_i) - Sum(z_i)*Sum(y_i) ) / ( N*Sum(z_i^2) - Sum(z_i)^2 )
        // y0  = <y> - m_y*<z>
        //     = ( Sum(y_i) - m_y*Sum(z_i) ) / N

        TrackClass track;
        if( sum_y > 0 ) { track.charge = -1; }// electrons go to top CAL (B in +x direction)
        else { track.charge = +1; }
        track.slopeX = (3. * sum_zx - sum_z * sum_x) / (3. * sum_zz - sum_z * sum_z);
        track.slopeY = (3. * sum_zy - sum_z * sum_y) / (3. * sum_zz - sum_z * sum_z);
        track.X0 = (sum_x - track.slopeX * sum_z) / 3.0;
        track.Y0 = (sum_y - track.slopeY * sum_z) / 3.0;
        // calculate polar (theta) and azimuthal angles (phi)
        double delta_x = std::get<0>(hit3) - std::get<0>(hit1);
        double delta_y = std::get<1>(hit3) - std::get<1>(hit1);
        double delta_z = std::get<2>(hit3) - std::get<2>(hit1);
        track.phi = atan2( delta_y, delta_x );
        track.theta = TMath::Pi() + atan( sqrt( pow(delta_x, 2) + pow(delta_y, 2) ) / delta_z );

        // Chi2
        double Chi2 = 0;
        auto hit_group = {hit1, hit2, hit3};
        for( auto hit : hit_group ) {
          double DeltaX = pow(std::get<0>(hit) - (track.X0 + track.slopeX * std::get<2>(hit)), 2);
          double DeltaY = pow(std::get<1>(hit) - (track.Y0 + track.slopeY * std::get<2>(hit)), 2);
          Chi2 += DeltaX + DeltaY;
        }
        track.Chi2 = Chi2;

        tracks->push_back( track );
      }
    }
  }
}

//-------------------------------------------------------------------------
// returns y deflection of ultrarel electrons in magnet region assuming primordial py = 0
double analyzeLumiHits::DeltaYmagnet( double E, double charge ) {

  //return 0;

  double R = E * RmagPreFactor; // cyclotron radius of curvature
  double dy = R - sqrt( R*R - pow(LumiSpecMag_DZ,2) );

  // electrons go to the top CAL (B in +x direction), positrons to bottom CAL
  return -charge * dy; // cm
}

//-------------------------------------------------------------------------
double analyzeLumiHits::XatConverter( TrackClass track ) {

  double x_c = (track.slopeX * LumiConverter_Z + track.X0);
  
  return x_c;
}

//-------------------------------------------------------------------------
double analyzeLumiHits::YatConverter( TrackClass track ) {
  double E = TrackerErec( track.slopeY );
  double y_c = (track.slopeY * LumiSpecMagEnd_Z + track.Y0) - DeltaYmagnet( E, track.charge );
  
  return y_c;
}

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void analyzeLumiHits::FinishWithGlobalRootLock() {

  // Do any final calculations here.
}

//-------------------------------------------------------------------------
void analyzeLumiHits::PrintTrackInfo( vector<TrackClass> topTracks, vector<TrackClass> botTracks) {
  cout<<"N good TopTracks: "<<topTracks.size()<<endl;
  for( auto el : topTracks ) {
    cout<<"theta: "<<el.theta<<"    phi: "<<el.phi<<"   slopeX: "<<el.slopeX<<"   slopeY: "<<el.slopeY<<"    Chi2: "<<el.Chi2<<endl;
  }

  cout<<"N good BotTracks: "<<botTracks.size()<<endl;
  for( auto el : botTracks ) {
    cout<<"theta: "<<el.theta<<"    phi: "<<el.phi<<"   slopeX: "<<el.slopeX<<"   slopeY: "<<el.slopeY<<"    Chi2: "<<el.Chi2<<endl;
  }
}

//-------------------------------------------------------------------------
double analyzeLumiHits::GetPairMass( TrackClass top, TrackClass bot ) {
  
  double Etop = TrackerErec( top.slopeY );
  double Ebot = TrackerErec( bot.slopeY );
  double p_top = sqrt( pow(Etop,2) - pow(constants::mass_electron,2) );
  double p_bot = sqrt( pow(Ebot,2) - pow(constants::mass_electron,2) );

  TLorentzVector electron_p;
  TLorentzVector positron_p;
  
  electron_p.SetXYZM( 
      p_top*sin(top.theta)*cos(top.phi), 
      p_top*sin(top.theta)*sin(top.phi), 
      p_top*cos(top.theta), 
      constants::mass_electron );
  // subtract py induced by B field
  double pYZ_top = sqrt( pow(electron_p.Y(), 2) + pow(electron_p.Z(), 2) ); // conserved
  electron_p.SetY( electron_p.Y() - pT );
  electron_p.SetZ( sqrt( pow(pYZ_top, 2) - pow(electron_p.Y(), 2) ) );

  positron_p.SetXYZM( 
      p_bot*sin(bot.theta)*cos(bot.phi), 
      p_bot*sin(bot.theta)*sin(bot.phi), 
      p_bot*cos(bot.theta), 
      constants::mass_electron );
  // subtract py induced by B field
  double pYZ_bot = sqrt( pow(positron_p.Y(), 2) + pow(positron_p.Z(), 2) ); // conserved
  positron_p.SetY( positron_p.Y() + pT );
  positron_p.SetZ( sqrt( pow(pYZ_bot, 2) - pow(positron_p.Y(), 2) ) );
  
  //electron_p.Print();
  //positron_p.Print();

  return (electron_p + positron_p).M();
}
