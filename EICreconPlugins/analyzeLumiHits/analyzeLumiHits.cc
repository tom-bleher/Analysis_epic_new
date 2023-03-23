
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
  hTrackers_Eres = new TH2D("hTrackers_Eres","Tracker E resolution;E_{#gamma} (GeV);(E_{gen}-E_{rec})/E_{gen}",200,0,50, 200,-1,1);
  
  hTrackers_E = new TH2D("hTrackers_E","Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50);
  hTrackersTop_E = new TH2D("hTrackersTop_E","Top Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50);
  hTrackersBot_E = new TH2D("hTrackersBot_E","Bottom Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50);

  hTrackers_X = new TH1D("hTrackers_X","X at converter;(x_{electron} + x_{positron})/2 (mm);counts",1000,-50,50);
  hTrackers_Y = new TH1D("hTrackers_Y","Y at converter;(y_{electron} + y_{positron})/2 (mm);counts",1000,-50,50);
  hTrackers_X_BotVsTop = new TH2D("hTrackers_X_BotVsTop","X at converter;X positron (mm);X electron (mm)", 1000,-50,50, 1000,-50,50);
  hTrackers_Y_BotVsTop = new TH2D("hTrackers_Y_BotVsTop","Y at converter;Y positron (mm);Y electron (mm)", 1000,-50,50, 1000,-50,50);


  for( int i=0; i < maxModules; i++ ) {
    for( int j=0; j < maxSectors; j++ ) {
      hGlobalXY[i][j] = new TH2D( 
          Form("hGlobalXY_%d%d", i,j), "Global Tracker Hits X vs Y;X (cm);Y (cm)", 120,-30,30, 120,-30,30);
    }
  }

  hEraw  = new TH1D("Eraw",  "hit energy (raw)", 2500, 0, 50);
  hErawTotal  = new TH1D("ErawTotal",  "summed hit energy (raw)", 2500, 0, 50);
  hEup  = new TH1D("hEup", "Upper CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50);
  hEdw  = new TH1D("hEdw", "Lower CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50);
  hEnergy  = new TH1D("hEnergy", "CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50);
  hCAL_Eres = new TH2D("hCAL_Eres","CAL E resolution;E_{#gamma} (GeV);E_{rec}", 200,0,50, 2500,0,50);

  hProtoClusterCount = new TH1D("hProtoClusterCount", "Number of proto island clusters / event", 20,-0.5,19.5);
  hClusterCount = new TH1D("hClusterCount", "Number of clusters / event", 20,-0.5,19.5);

  hTrackChi2 = new TH1D("hTrackChi2", "Track #chi^{2} / Nlayers;#chi^{2};counts", 1000,0,1);
  hTrackersSlope = new TH1D("hTrackersSlope", "", 1000,0,1);

  // spectrometer dimensions/placements in cm
  double SpecMag_to_SpecCAL_DZ = (LumiSpecMag_Z - LumiSpecMag_DZ/2.0) - (LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0);
  LumiSpecTracker_Z1 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 21;
  LumiSpecTracker_Z2 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 11;
  LumiSpecTracker_Z3 = LumiSpecCAL_Z + LumiSpecCALTower_DZ/2.0 + 1;

  Tracker_Zs = {LumiSpecTracker_Z1, LumiSpecTracker_Z2, LumiSpecTracker_Z3};
  for( auto el : Tracker_Zs ) { Tracker_meanZ += el; }
  Tracker_meanZ /= double(Tracker_Zs.size()); 

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

  tree_Tracks = new TTree("tree_Tracks","Tracks");
  tree_Tracks->Branch("X", &X_mean);
  tree_Tracks->Branch("Y", &Y_mean);
  tree_Tracks->Branch("Xe", &X_electron);
  tree_Tracks->Branch("Ye", &Y_electron);
  tree_Tracks->Branch("Xp", &X_positron);
  tree_Tracks->Branch("Yp", &Y_positron);
  tree_Tracks->Branch("Chi2e", &Chi2_electron);
  tree_Tracks->Branch("Chi2p", &Chi2_positron);
  
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
  app->SetDefaultParameter("analyzeLumiHits:Egen", Einput);
  app->SetDefaultParameter("analyzeLumiHits:Ntrackers", Ntrackers);
  //cout<<"E gen = "<<Einput<<"   Ntrackers = "<<Ntrackers<<endl;

  ///////////////////////////////////////////////////////////////////////
  // Digitized ADC raw hits
  for( auto adc : CAL_adc() ) hADCsignal->Fill( adc->getAmplitude() );

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

  hProtoClusterCount->Fill( CAL_protoClusters().size() );


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

  hClusterCount->Fill( CAL_clusters().size() );

  if( E_CALup > 0 && E_CALdw > 0 ) { 
    hCALCluster_Eres->Fill( Einput, E_CALup + E_CALdw );
    hCAL_Acceptance->Fill( Einput );
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

    hEraw->Fill( hit->getEnergy() );

    E_CALhits_total += hit->getEnergy();

  } //Calorimeter hits close

  if( E_CALhits_total > 0 ) { hErawTotal->Fill( E_CALhits_total ); }

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
      //cout<<setprecision(6)<<get<0>(myhit)<<"  "<<get<1>(myhit)<<"  "<<get<2>(myhit)<<endl;

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
  
  for( auto track : AllTopTracks ) { hTrackChi2->Fill( track.Chi2/3. ); }
  for( auto track : AllBotTracks ) { hTrackChi2->Fill( track.Chi2/3. ); }

  // Remove high Chi2 tracks (likely due to secondary hits)
  vector<TrackClass> TopTracks;
  vector<TrackClass> BotTracks;
  for( auto track : AllTopTracks ) { if( track.Chi2/3. < max_chi2ndf ) { TopTracks.push_back( track ); } }
  for( auto track : AllBotTracks ) { if( track.Chi2/3. < max_chi2ndf ) { BotTracks.push_back( track ); } }

  //cout<<"N good TopTracks: "<<TopTracks.size()<<endl;
  //cout<<"N good BotTracks: "<<BotTracks.size()<<endl;
  //if( TopTracks.size() > 1 ) {

  //  for( auto el : TopTracks ) {
  //    cout<<el.slopeX<<"  "<<el.slopeY<<"    "<<el.Chi2<<endl;
  //  }

  //  for( auto hit : TopTracker1 ) { cout<<"1  x: "<<std::get<0>(hit)<<"   y: "<<std::get<1>(hit)<<endl; }
  //  for( auto hit : TopTracker2 ) { cout<<"2  x: "<<std::get<0>(hit)<<"   y: "<<std::get<1>(hit)<<endl; }
  //  for( auto hit : TopTracker3 ) { cout<<"3  x: "<<std::get<0>(hit)<<"   y: "<<std::get<1>(hit)<<endl; }

  //}


  for( auto track : TopTracks ) {
    double Etop = TrackerErec( track );
    hTrackersTop_E->Fill( Einput, Etop );
    hTrackersSlope->Fill( track.slopeY );
  }
  for( auto track : BotTracks ) {
    double Ebot = TrackerErec( track );
    hTrackersBot_E->Fill( Einput, Ebot );
    hTrackersSlope->Fill( track.slopeY );
  }

  for( auto topTrack : TopTracks ) {
    
    double Etop = TrackerErec( topTrack );
    double xtop_c = (topTrack.slopeX * LumiConverter_Z + topTrack.X0);
    double ytop_c = (topTrack.slopeY * LumiSpecMagEnd_Z + topTrack.Y0) - DeltaYmagnet( Etop, topTrack.charge );

    for( auto botTrack : BotTracks ) {

      double Ebot = TrackerErec( botTrack );
      double xbot_c = (botTrack.slopeX * LumiConverter_Z + botTrack.X0);
      double ybot_c = (botTrack.slopeY * LumiSpecMagEnd_Z + botTrack.Y0) - DeltaYmagnet( Ebot, botTrack.charge );

      //cout<<"Egen = "<<Einput<<"  E_trackers = "<<E_trackers<<endl;
      //cout<<xtop_c<<"  "<<xbot_c<<"    "<<ytop_c<<"  "<<ybot_c<<endl;
      hTrackers_X->Fill( (xtop_c + xbot_c)/2. );
      hTrackers_Y->Fill( (ytop_c + ybot_c)/2. );
      hTrackers_Eres->Fill( Einput, (Einput - (Etop + Ebot) )/Einput );
      hTrackers_E->Fill( Einput, Etop + Ebot );
      hTrackers_X_BotVsTop->Fill( xtop_c, xbot_c );
      hTrackers_Y_BotVsTop->Fill( ytop_c, ybot_c );

      X_mean = (xtop_c + xbot_c) / 2.;
      Y_mean = (ytop_c + ybot_c) / 2.;
      X_electron = xtop_c;
      Y_electron = ytop_c;
      Chi2_electron = topTrack.Chi2;
      X_positron = xbot_c;
      Y_positron = ybot_c;
      Chi2_positron = botTrack.Chi2;

      tree_Tracks->Fill();
    }
  }

  if( TopTracks.size() > 0 && BotTracks.size() > 0 ){
    // Fill of XY-histograms.
    for( int i=0; i < maxModules; i++ ) {
      for( int j=0; j < maxSectors; j++ ) {
        if( counts_Tr[i][j] <= 0 ) { continue; }
        gpos_x[i][j] /= counts_Tr[i][j];
        gpos_y[i][j] /= counts_Tr[i][j];

        hGlobalXY[i][j]->Fill( gpos_x[i][j], gpos_y[i][j] ); 
      } // j
    } // i
  } // Top and Bottom track check

  // Fill the energy histograms
  if( (E_CALup > 0) && (E_CALdw > 0) ){
    hEnergy->Fill( E_CALup + E_CALdw );
    hCAL_Eres->Fill( Einput, E_CALup + E_CALdw );
    hEup->Fill( E_CALup );
    hEdw->Fill( E_CALdw );
  } //Energy check

} // End of the Sequential Process Function

//-------------------------------------------------------------------------
double analyzeLumiHits::TrackerErec( TrackClass track ) {

  double sinTheta = fabs( sin( atan(track.slopeY) ) );
 
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

 for( auto hit1 : hitSet[0] ) {

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

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void analyzeLumiHits::FinishWithGlobalRootLock() {

  // Do any final calculations here.
}


