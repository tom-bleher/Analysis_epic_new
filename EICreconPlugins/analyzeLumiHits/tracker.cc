#ifndef TRACKER_CC
#define TRACKER_CC

#include "tracker.h"

// Constructor
//-------------------------------------------------------------------------
TrackerAnalysis::TrackerAnalysis() { 
  /*dd4hep::Detector* description = m_geoSvc->detector();
  //description->fromCompact( "/home/dhevan/eic/epic/epic_ip6_extended.xml" );
  double posV[3] = { 0, 0, -6250 }  ;
  double bfieldV[3] ;
  description->field().magneticField( posV  , bfieldV  ) ;

  printf(" LUMI B FIELD: %+15.8e  %+15.8e  %+15.8e  %+15.8e  %+15.8e  %+15.8e  \n",
      posV[0]/dd4hep::cm, posV[1]/dd4hep::cm,  posV[2]/dd4hep::cm,
      bfieldV[0]/dd4hep::tesla , bfieldV[1]/dd4hep::tesla, bfieldV[2]/dd4hep::tesla ) ;
*/
}

// The data structure from the simulation has to be passed for each event
//-------------------------------------------------------------------------
void TrackerAnalysis::Prepare( std::vector<const edm4hep::SimTrackerHit*> &hits, std::shared_ptr<DD4hep_service> geoSvc ) {
  
  // hit structure
  m_edm4hepTrackerHits = hits;

  // geometry service
  m_geoSvc = geoSvc;

  // clear all vectors
  m_AllTopTracks.clear();
  m_AllBotTracks.clear();
  m_TopTracks.clear();
  m_BotTracks.clear();

  m_TrackerHits.clear();

  m_TrackerHits.resize( variables::maxSectors );
  for( auto &sec : m_TrackerHits ) {
    sec.resize( variables::maxModules );
  }

}

// Fill Hit Structures
//-------------------------------------------------------------------------
void TrackerAnalysis::FillTrackerHits() {

  map<string, int> Trackerfield_idx_Map{ {"sector", 0}, {"module", 0} };

  for( auto hit : m_edm4hepTrackerHits ){

    variables::x_hit = hit->x();
    variables::y_hit = hit->y();
    variables::z_hit = hit->z();

    treeTracker_Hits->Fill();

    const auto id = hit->getCellID();

    auto id_dec   = m_geoSvc->detector()->readout( "LumiSpecTrackerHits" ).idSpec().decoder();
    bool secondary = (hit->getQuality() == 0) ? false : true; // 1 << 30 if produced by secondary (edm4hep docs)

    // field of readout fields
    vector<dd4hep::BitFieldElement> hitFields = id_dec->fields();

    // try to find the expected fields and store field index
    for( auto field : hitFields ) {
      if( Trackerfield_idx_Map.find( field.name() ) != Trackerfield_idx_Map.end() ) {
        Trackerfield_idx_Map[ field.name() ] = id_dec->index( field.name() );
      }
    }

    // look up sector,module,fiber... id of this hit 
    const int sec_id 	= (int) id_dec->get( id, Trackerfield_idx_Map["sector"] ); // top or bottom
    const int mod_id 	= (int) id_dec->get( id, Trackerfield_idx_Map["module"] ); // layer, closet to furthest from IP

    //for global positions.  CAUTION: Have to convert it to mm
    const auto gpos 	= m_geoSvc->cellIDPositionConverter()->position(id); // cm

    //for local positions
    const auto volman 	= m_geoSvc->detector()->volumeManager();
    const auto alignment = volman.lookupDetElement(id).nominal();
    const auto lpos 	= alignment.worldToLocal( dd4hep::Position( gpos.x(), gpos.y(), gpos.z() ) ); // cm

    if( sec_id < variables::maxSectors && mod_id < variables::maxModules ) {

      // mm units
      TrackHit myhit = {10*gpos.x(), 10*gpos.y(), 10*gpos.z()};
      //cout<<setprecision(6)<<"hit X: "<<get<0>(myhit)<<"   hit Y: "<<get<1>(myhit)<<"   hit Z: "<<get<2>(myhit)<<endl;

      if( ! PixelOverlap( myhit, m_TrackerHits[ sec_id ][ mod_id ] ) ) {
        m_TrackerHits[ sec_id ][ mod_id ].push_back( myhit );
      }
           
      ((TH1D *)gHistList->FindObject(Form("hGlobalXY_%i_%i",mod_id,sec_id) ))->Fill( std::get<0>(myhit), std::get<1>(myhit) );

    }
  } //Tracker hits close

}

//-------------------------------------------------------------------------
void TrackerAnalysis::AssembleAllTracks(){

  AssembleTracks( &m_AllTopTracks, 0 );
  AssembleTracks( &m_AllBotTracks, 1 );

  // Filter out the bad tracks
  for( auto track : m_AllTopTracks ) { 
    if( track.Chi2/track.Nhits < variables::max_chi2ndf ) { 
      m_TopTracks.push_back( track );
    }
  }

  for( auto track : m_AllBotTracks ) { 
    if( track.Chi2/track.Nhits < variables::max_chi2ndf ) { 
      m_BotTracks.push_back( track ); 
    }
  }

}

//-------------------------------------------------------------------------
void TrackerAnalysis::AssembleTracks( vector<TrackClass> *tracks, int sec_id ) {

  // units of hits are in mm
  double Nmodules = 0;
  for( int mod = 0; mod < variables::maxModules; mod++ ) {

    if( m_TrackerHits[ sec_id ][ mod ].size() > 0 ) {
      Nmodules++;
    }
  }

  int modN = 0;
  double sum_x = 0;
  double sum_y = 0;
  double sum_z = 0;
  double sum_xz = 0;
  double sum_yz = 0;
  double sum_zz = 0;
  vector<TrackHit> hits;

  if( Nmodules >= 2 ) {
  ComputeLinearRegressionComponents( tracks, &hits, Nmodules, sec_id, 0, &sum_x, &sum_y, &sum_z, &sum_xz,  &sum_yz, &sum_zz );
  }

}

//-------------------------------------------------------------------------
void TrackerAnalysis::ComputeLinearRegressionComponents( vector<TrackClass> *tracks, vector<TrackHit> *hits, double Nmods, int sec, int mod, double *x, double *y, double *z, double *xz, double *yz, double *zz ) {
  
  if( m_TrackerHits[ sec ][ mod ].size() == 0 && mod < (variables::maxModules-1) ) {
    ComputeLinearRegressionComponents( tracks, hits, Nmods, sec, mod + 1, x, y, z, xz, yz, zz );
  }

  for( auto hit : m_TrackerHits[ sec ][ mod ] ) {

    double newCoords[3] = {
      std::get<0>(hit), 
      std::get<1>(hit), 
      std::get<2>(hit)}; 

    *x  += newCoords[0];
    *y  += newCoords[1];
    *z  += newCoords[2];
    *xz += newCoords[0] * newCoords[2];
    *yz += newCoords[1] * newCoords[2];
    *zz += newCoords[2] * newCoords[2];

    hits->push_back( hit );

    if( mod < (variables::maxModules-1) ) {
      ComputeLinearRegressionComponents( tracks, hits, Nmods, sec, mod + 1, x, y, z, xz, yz, zz );
    }
  }

  if( mod == (variables::maxModules-1) ) { // reached the last tracking layer

    TrackClass track;

    if( (*y * variables::B) > 0 ) { track.charge = -1; }// electrons go to top CAL (B in +x direction)
    else                          { track.charge = +1; }

    track.slopeX = ( Nmods * (*xz) - (*x) * (*z) ) / ( Nmods * (*zz) - (*z) * (*z) );
    track.slopeY = ( Nmods * (*yz) - (*y) * (*z) ) / ( Nmods * (*zz) - (*z) * (*z) );
    track.X0     = ( (*x) - track.slopeX * (*z) ) / Nmods;
    track.Y0     = ( (*y) - track.slopeY * (*z) ) / Nmods;
    //track.X0    += track.slopeX * fabs(variables::LumiSpecCAL_Z); // account for Z shift 
    //track.Y0    += track.slopeY * fabs(variables::LumiSpecCAL_Z); // account for Z shift
    // calculate polar (theta) and azimuthal angles (phi)
    track.phi    = atan( track.slopeY / track.slopeX );
    track.theta  = TMath::Pi() - atan( sqrt( pow(track.slopeX, 2) + pow(track.slopeY, 2) ) );

    // Chi2
    double Chi2 = 0;
    for( auto hit : *hits ) {
      double DeltaX = pow(std::get<0>(hit) - (track.X0 + track.slopeX * std::get<2>(hit)), 2);
      double DeltaY = pow(std::get<1>(hit) - (track.Y0 + track.slopeY * std::get<2>(hit)), 2);
      Chi2 += DeltaX + DeltaY;
    }

    track.Chi2 = Chi2;
    track.Nhits = Nmods;

    tracks->push_back( track );

    *x = 0; *y = 0; *z = 0; *xz = 0; *yz = 0; *zz = 0;
    hits->clear();
  }
  return;
}

//-------------------------------------------------------------------------
double TrackerAnalysis::TrackerErec( double slopeY ) {

  double sinTheta = fabs( sin( atan(slopeY) ) );
 
  if( sinTheta == 0 ) { return 0.0; }

  double E = variables::pT / sinTheta;
  
  return E;
}

//-------------------------------------------------------------------------
bool TrackerAnalysis::PixelOverlap( TrackHit hit, vector<TrackHit> trackSet ) {

  for( auto el : trackSet ) {
    double delta = pow( std::get<0>(hit) - std::get<0>(el), 2);
    delta += pow( std::get<1>(hit) - std::get<1>(el), 2);

    if( sqrt(delta) < variables::Tracker_pixelSize ) { return true; }
  }

  return false;
}

//-------------------------------------------------------------------------
// returns y deflection of ultrarel of electrons traveling in -Z in magnet region
// assuming primordial py = 0
double TrackerAnalysis::DeltaYmagnet( double E, double charge ) {

  //return 0;

  double R = E * variables::RmagPreFactor; // cyclotron radius of curvature
  double dy = R - sqrt( R*R - pow(variables::LumiAnalyzerMag_DZ,2) );

  // electrons go to the top CAL (B in +x direction), positrons to bottom CAL
  return -charge * variables::B/fabs(variables::B) * dy; // mm
}

//-------------------------------------------------------------------------
// returns y deflection of ultrarel electrons traveling in -Z in magnet region 
// assuming primordial py = 0
double TrackerAnalysis::DeltaYmagnet( TrackClass track, double z ) {

  double E = TrackerErec( track.slopeY );
  double R = E * variables::RmagPreFactor; // cyclotron radius of curvature
  double dy = -R*cos(TMath::Pi() - track.theta) + sqrt( R*R - pow(R*sin(TMath::Pi() - track.theta) - z, 2) );

  // electrons go to the top CAL (B in +x direction), positrons to bottom CAL
  return -track.charge * variables::B/fabs(variables::B) * dy; // mm
}

//-------------------------------------------------------------------------
// returns y deflection of ultrarel electrons in magnet region assuming primordial py = 0
std::pair<double,double> TrackerAnalysis::DCA( TrackClass track1, TrackClass track2 ) {

  bool pastMin = false;
  double dca = 10000;
  double Zstart = variables::LumiAnalyzerMag_Z + variables::LumiAnalyzerMag_DZ/2.;
  double Zstep = 0;
  
  while( ! pastMin ) {
    Zstep++;
    double Z = Zstart - Zstep;
    double deltaX = (track1.slopeX * Z + track1.X0) - (track2.slopeX * Z + track2.X0);
    double deltaY = (track1.slopeY * variables::LumiAnalyzerMagEnd_Z + track1.Y0) - DeltaYmagnet( track1, variables::LumiAnalyzerMag_DZ - Zstep);
    deltaY -= (track2.slopeY * variables::LumiAnalyzerMagEnd_Z + track2.Y0) - DeltaYmagnet( track2, variables::LumiAnalyzerMag_DZ - Zstep);
    double dca_new = sqrt( pow(deltaX,2) + pow(deltaY,2) );
    if( dca_new < dca ) {
      dca = dca_new;
    }else {
      pastMin = true;
    }
    if( Zstep > variables::LumiAnalyzerMag_DZ ) { pastMin = true; }
  }
  
  return std::pair<double,double> {dca, Zstart - Zstep};
}

//-------------------------------------------------------------------------
double TrackerAnalysis::XatAnaMagStart( TrackClass track ) {

  double x_c = (track.slopeX * variables::LumiAnalyzerMagStart_Z + track.X0);
  
  return x_c;
}

//-------------------------------------------------------------------------
double TrackerAnalysis::YatAnaMagStart( TrackClass track ) {
  double E = TrackerErec( track.slopeY );
  double y_c = (track.slopeY * variables::LumiAnalyzerMagEnd_Z + track.Y0) - DeltaYmagnet( E, track.charge );
  //cout<<"y from line: "<< (track.slopeY * variables::LumiAnalyzerMagEnd_Z + track.Y0)<<"   deltaYMag: "<<DeltaYmagnet( E, track.charge )<<endl;
  return y_c;
}

//-------------------------------------------------------------------------
void TrackerAnalysis::PrintTrackInfo( vector<TrackClass> topTracks, vector<TrackClass> botTracks) {
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
double TrackerAnalysis::GetPairMass( TrackClass top, TrackClass bot ) {
  
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
  electron_p.SetY( electron_p.Y() - variables::pT );
  electron_p.SetZ( sqrt( pow(pYZ_top, 2) - pow(electron_p.Y(), 2) ) );

  positron_p.SetXYZM( 
      p_bot*sin(bot.theta)*cos(bot.phi), 
      p_bot*sin(bot.theta)*sin(bot.phi), 
      p_bot*cos(bot.theta), 
      constants::mass_electron );
  // subtract py induced by B field
  double pYZ_bot = sqrt( pow(positron_p.Y(), 2) + pow(positron_p.Z(), 2) ); // conserved
  positron_p.SetY( positron_p.Y() + variables::pT );
  positron_p.SetZ( sqrt( pow(pYZ_bot, 2) - pow(positron_p.Y(), 2) ) );
  
  //electron_p.Print();
  //positron_p.Print();

  return (electron_p + positron_p).M();
}

//-------------------------------------------------------------------------
void TrackerAnalysis::FillTrackerHistograms() {

  for( auto track : m_AllTopTracks ) { ((TH1D *)gHistList->FindObject("hTrackChi2"))->Fill( track.Chi2/3. ); }
  for( auto track : m_AllBotTracks ) { ((TH1D *)gHistList->FindObject("hTrackChi2"))->Fill( track.Chi2/3. ); }

  // Loop over good tracks
  bool EventWithTopTrackNearConverterCenter = false;
  bool EventWithBotTrackNearConverterCenter = false;
  for( auto track : m_TopTracks ) {
    
    double Etop = TrackerErec( track.slopeY );
    ((TH2D *)gHistList->FindObject("hTrackersTop_E"))->Fill( variables::Ephoton, Etop );
    ((TH1D *)gHistList->FindObject("hTrackersSlope"))->Fill( track.slopeY );

    cout<<XatAnaMagStart( track )<<"  "<<YatAnaMagStart( track )<<endl;
    if( fabs( XatAnaMagStart( track ) ) < variables::LumiConverterCut_DXY / 2. 
        && fabs( YatAnaMagStart( track ) ) < variables::LumiConverterCut_DXY / 2. ) {
      EventWithTopTrackNearConverterCenter = true;
    }
  }
  for( auto track : m_BotTracks ) {
    
    double Ebot = TrackerErec( track.slopeY );
    ((TH2D *)gHistList->FindObject("hTrackersBot_E"))->Fill( variables::Ephoton, Ebot );
    ((TH1D *)gHistList->FindObject("hTrackersSlope"))->Fill( track.slopeY );

    if( fabs( XatAnaMagStart( track ) ) < variables::LumiConverterCut_DXY / 2. 
        && fabs( YatAnaMagStart( track ) ) < variables::LumiConverterCut_DXY / 2. ) {
      EventWithBotTrackNearConverterCenter = true;
    }
  }

  if( EventWithTopTrackNearConverterCenter ) { 
    ((TH1D*)gHistList->FindObject("hTrackerTop_Acceptance"))->Fill( variables::Ephoton );
  }
  if( EventWithBotTrackNearConverterCenter ) { 
    ((TH1D*)gHistList->FindObject("hTrackerBot_Acceptance"))->Fill( variables::Ephoton );
  }
  if( EventWithTopTrackNearConverterCenter && EventWithBotTrackNearConverterCenter ) { 
    ((TH1D*)gHistList->FindObject("hTrackerCoincidence_Acceptance"))->Fill( variables::Ephoton );
  }

  // Loop over good pairs of tracks
  for( auto topTrack : m_TopTracks ) {

    double Etop = TrackerErec( topTrack.slopeY );
    double xtop_c = XatAnaMagStart( topTrack );
    double ytop_c = YatAnaMagStart( topTrack );

    for( auto botTrack : m_BotTracks ) {

      double Ebot = TrackerErec( botTrack.slopeY );
      double xbot_c = XatAnaMagStart( botTrack );
      double ybot_c = YatAnaMagStart( botTrack );
      double xPhoton = (xtop_c + xbot_c)/2.;
      double yPhoton = (ytop_c + ybot_c)/2.;

      double pairMass = GetPairMass( topTrack, botTrack );

      std::pair<double, double> DCAandZ = DCA( topTrack, botTrack );

      ((TH2D *)gHistList->FindObject("hTrackers_DCAvsZ"))->Fill( DCAandZ.first, DCAandZ.second );

      
      ((TH1D *)gHistList->FindObject("hTrackers_InvMass_allPairs"))->Fill( pairMass );
      ((TH1D *)gHistList->FindObject("hTrackers_X_allPairs"))->Fill( xPhoton );
      ((TH1D *)gHistList->FindObject("hTrackers_Y_allPairs"))->Fill( yPhoton );
      ((TH1D *)gHistList->FindObject("hTrackers_X-Xphoton_allPairs"))->Fill( xPhoton - variables::Xphoton );
      ((TH1D *)gHistList->FindObject("hTrackers_Y-Yphoton_allPairs"))->Fill( yPhoton - variables::Yphoton );

      ((TH1D *)gHistList->FindObject("hTrackers_DX_allPairs"))->Fill( xtop_c - xbot_c );
      ((TH1D *)gHistList->FindObject("hTrackers_DY_allPairs"))->Fill( ytop_c - ybot_c );
      ((TH2D *)gHistList->FindObject("hTrackers_E_allPairs"))->Fill( variables::Ephoton, Etop + Ebot );
      ((TH2D *)gHistList->FindObject("hTrackers_E_allPairs"))->Fill( variables::Ephoton, Etop + Ebot );
      ((TH2D *)gHistList->FindObject("hTrackers_Eres_allPairs"))->Fill( variables::Ephoton, (variables::Ephoton - (Etop + Ebot) )/variables::Ephoton );

      //////////////////////
      // Track proximity cut
      if( fabs(xtop_c - xbot_c) > 2*variables::Tracker_sigma 
          || fabs(ytop_c - ybot_c) > 2*variables::Tracker_sigma ) { continue; }
      
      ((TH1D *)gHistList->FindObject("hTrackers_InvMass"))->Fill( pairMass );
      ((TH1D *)gHistList->FindObject("hTrackers_X"))->Fill( xPhoton );
      ((TH1D *)gHistList->FindObject("hTrackers_Y"))->Fill( yPhoton );
      ((TH2D *)gHistList->FindObject("hTrackers_X_BotVsTop"))->Fill( xtop_c, xbot_c );
      ((TH2D *)gHistList->FindObject("hTrackers_Y_BotVsTop"))->Fill( ytop_c, ybot_c );
      ((TH2D *)gHistList->FindObject("hTrackers_E"))->Fill( variables::Ephoton, Etop + Ebot );
      ((TH2D *)gHistList->FindObject("hTrackers_Eres"))->Fill( variables::Ephoton, (variables::Ephoton - (Etop + Ebot) )/variables::Ephoton );
    }
  }

  // Trees with all track pairings
  for( auto topTrack : m_AllTopTracks ) {

    double Etop = TrackerErec( topTrack.slopeY );
    double xtop_c = XatAnaMagStart( topTrack );
    double ytop_c = YatAnaMagStart( topTrack );

    for( auto botTrack : m_AllBotTracks ) {

      double Ebot = TrackerErec( botTrack.slopeY );
      double xbot_c = XatAnaMagStart( botTrack );
      double ybot_c = YatAnaMagStart( botTrack );
      double xPhoton = (xtop_c + xbot_c)/2.;
      double yPhoton = (ytop_c + ybot_c)/2.;

      double pairMass = GetPairMass( topTrack, botTrack );

      std::pair<double, double> DCAandZ = DCA( topTrack, botTrack );

      recPhotons.E = Etop + Ebot;
      recPhotons.Etop = Etop;
      recPhotons.Ebot = Ebot;
      recPhotons.Egen = variables::Ephoton;
      recPhotons.Mass = pairMass;
      recPhotons.X = xPhoton;
      recPhotons.Y = yPhoton;
      recPhotons.Xtop = xtop_c;
      recPhotons.Xbot = xbot_c;
      recPhotons.Ytop = ytop_c;
      recPhotons.Ybot = ybot_c;
      recPhotons.Xgen = variables::Xphoton;
      recPhotons.Ygen = variables::Yphoton;
      recPhotons.DCA = std::get<0>(DCAandZ);
      recPhotons.Chi2top = topTrack.Chi2;
      recPhotons.Chi2bot = botTrack.Chi2;
      recPhotons.Nhitstop = topTrack.Nhits;
      recPhotons.Nhitsbot = botTrack.Nhits;

      treePhotons->Fill();
    }
  }

}

//-------------------------------------------------------------------------
void TrackerAnalysis::FillTrackerTrees() {

  TreeTrackClass tracksBuffer;
  
  for( auto track : m_TopTracks ) { 
      tracksBuffer.X0_e.push_back( XatAnaMagStart( track ) );
      tracksBuffer.Y0_e.push_back( YatAnaMagStart( track ) );
      tracksBuffer.slopeX_e.push_back( track.slopeX );
      tracksBuffer.slopeY_e.push_back( track.slopeY );
      tracksBuffer.theta_e.push_back( track.theta );
      tracksBuffer.Nhits_e.push_back( track.Nhits );
  }
  for( auto track : m_BotTracks ) { 
      tracksBuffer.X0_p.push_back( XatAnaMagStart( track ) );
      tracksBuffer.Y0_p.push_back( YatAnaMagStart( track ) );
      tracksBuffer.slopeX_p.push_back( track.slopeX );
      tracksBuffer.slopeY_p.push_back( track.slopeY );
      tracksBuffer.theta_p.push_back( track.theta );
      tracksBuffer.Nhits_p.push_back( track.Nhits );
  }

  histogramManager::tracks = tracksBuffer;

  treeTracks->Fill();
}

#endif
