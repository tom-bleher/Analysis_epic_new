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
    bool primary = (hit->getQuality() == 0) ? true : false; // 1 << 30 if produced by secondary (edm4hep docs)

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

      // units: mm, GeV, nsec
      TrackHit myhit = {10*gpos.x(), 10*gpos.y(), 10*gpos.z(), hit->getEDep(), hit->getTime(), primary};
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
    if( track.chi2/track.nHits < variables::max_chi2ndf ) { 
      m_TopTracks.push_back( track );
    }
  }

  for( auto track : m_AllBotTracks ) { 
    if( track.chi2/track.nHits < variables::max_chi2ndf ) { 
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
    track.x0     = ( (*x) - track.slopeX * (*z) ) / Nmods;
    track.y0     = ( (*y) - track.slopeY * (*z) ) / Nmods;
    //track.x0    += track.slopeX * fabs(variables::LumiSpecCAL_Z); // account for Z shift 
    //track.y0    += track.slopeY * fabs(variables::LumiSpecCAL_Z); // account for Z shift
    // calculate polar (theta) and azimuthal angles (phi)
    track.theta  = TMath::Pi() - atan( sqrt( pow(track.slopeX, 2) + pow(track.slopeY, 2) ) );
    track.phi    = TMath::Pi() + atan2( track.slopeY, track.slopeX );

    // hit quantities
    double chi2 = 0;
    for( int i = 0; i < hits->size(); i++ ) {
      TrackHit hit = hits->at(i);
      double DeltaX = pow(std::get<0>(hit) - (track.x0 + track.slopeX * std::get<2>(hit)), 2);
      double DeltaY = pow(std::get<1>(hit) - (track.y0 + track.slopeY * std::get<2>(hit)), 2);
      chi2 += DeltaX + DeltaY;
      track.eDeps.push_back( std::get<3>(hit) );
      track.time.push_back( std::get<4>(hit) );
      track.primary.push_back( std::get<5>(hit) );
    }

    track.chi2 = chi2;
    track.nHits = Nmods;
    track.e = TrackerErec( track.slopeY );

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
std::pair<double,double> TrackerAnalysis::DCA( TrackClass top, TrackClass bot ) {

  bool pastMin = false;
  double dca = 10000;
  double Zstart = variables::LumiAnalyzerMagStart_Z;
  double Zstep = 0;
  
  while( ! pastMin ) {
    Zstep++;
    double Z = Zstart - Zstep;
    double deltaX = (top.slopeX * Z + top.x0) - (bot.slopeX * Z + bot.x0);
    double deltaY = (top.slopeY * variables::LumiAnalyzerMagEnd_Z + top.y0) - DeltaYmagnet( top, variables::LumiAnalyzerMag_DZ - Zstep);
    deltaY -= (bot.slopeY * variables::LumiAnalyzerMagEnd_Z + bot.y0) - DeltaYmagnet( bot, variables::LumiAnalyzerMag_DZ - Zstep);
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

  double x_c = (track.slopeX * variables::LumiAnalyzerMagStart_Z + track.x0);
  
  return x_c;
}

//-------------------------------------------------------------------------
double TrackerAnalysis::YatAnaMagStart( TrackClass track ) {
  
  double E = TrackerErec( track.slopeY );
  double y_c = (track.slopeY * variables::LumiAnalyzerMagEnd_Z + track.y0) - DeltaYmagnet( E, track.charge );
  
  return y_c;
}

//-------------------------------------------------------------------------
void TrackerAnalysis::PrintTrackInfo( vector<TrackClass> topTracks, vector<TrackClass> botTracks) {
  cout<<"N good TopTracks: "<<topTracks.size()<<endl;
  for( auto el : topTracks ) {
    cout<<"theta: "<<el.theta<<"    phi: "<<el.phi<<"   slopeX: "<<el.slopeX<<"   slopeY: "<<el.slopeY<<"   .chi2: "<<el.chi2<<endl;
  }

  cout<<"N good BotTracks: "<<botTracks.size()<<endl;
  for( auto el : botTracks ) {
    cout<<"theta: "<<el.theta<<"    phi: "<<el.phi<<"   slopeX: "<<el.slopeX<<"   slopeY: "<<el.slopeY<<"   .chi2: "<<el.chi2<<endl;
  }
}

//-------------------------------------------------------------------------
double TrackerAnalysis::GetPairMass( TrackClass top, TrackClass bot ) {
  
  TLorentzVector top4Vector, bot4Vector;
  vector<TLorentzVector> fourVectors = {top4Vector, bot4Vector};
  vector<TrackClass> tracks = {top, bot};
  
  for(int i = 0; i < 2; i++) {
    TrackClass track = tracks[i];
    double E = TrackerErec( track.slopeY );
    double p = sqrt( pow(E, 2) - pow(constants::mass_electron, 2) );

    double px = p * sin(track.theta) * cos(track.phi);
    double py = p * sin(track.theta) * sin(track.phi);
    double pz = p * cos(track.theta);
    double pYZ = sqrt( pow(py, 2) + pow(pz, 2) ); // conserved
    double new_py = py + pow(-1, i+1) * variables::pT; // subtract py induced by B field
    double new_pz = sqrt( pow(pYZ, 2) - pow(new_py, 2) );
    fourVectors[i].SetXYZM( px, new_py, new_pz, constants::mass_electron ); 
  }
  //fourVectors[0].Print();
  //fourVectors[1].Print();
  //cout<<(fourVectors[0] + fourVectors[1]).M()<<endl;
  return (fourVectors[0] + fourVectors[1]).M();
}

//-------------------------------------------------------------------------
void TrackerAnalysis::FillTrackerHistograms() {

  bool EventWithTopTrackNearConverterCenter = false;
  bool EventWithBotTrackNearConverterCenter = false;
  for( auto track : m_TopTracks ) {
    
    double Etop = TrackerErec( track.slopeY );
    ((TH2D *)gHistList->FindObject("hTrackersTop_E"))->Fill( variables::Ephoton, Etop );
    ((TH1D *)gHistList->FindObject("hTrackersSlope"))->Fill( track.slopeY );

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

      g_recPhoton.e = Etop + Ebot;
      g_recPhoton.eTop = Etop;
      g_recPhoton.eBot = Ebot;
      g_recPhoton.mass = pairMass;
      g_recPhoton.x = xPhoton;
      g_recPhoton.y = yPhoton;
      g_recPhoton.dca = std::get<0>(DCAandZ);
      g_recPhoton.eGen = variables::Ephoton;
      g_recPhoton.xGen = variables::Xphoton;
      g_recPhoton.yGen = variables::Yphoton;
      g_recPhoton.thetaGen = variables::ThetaPhoton;
      g_recPhoton.phiGen = variables::PhiPhoton;
      g_recPhoton.chi2Top = topTrack.chi2;
      g_recPhoton.nHitsTop = topTrack.nHits;
      //g_recPhoton.eDepsTop = topTrack.eDeps;
      //g_recPhoton.timeTop = topTrack.time;
      //g_recPhoton.primaryTop = topTrack.primary;
      g_recPhoton.chi2Bot = topTrack.chi2;
      g_recPhoton.nHitsBot = topTrack.nHits;
      //g_recPhoton.eDepsBot = topTrack.eDeps;
      //g_recPhoton.timeBot = topTrack.time;
      //g_recPhoton.primaryBot = topTrack.primary;

      treePhotons->Fill();
    }
  }

}

//-------------------------------------------------------------------------
void TrackerAnalysis::FillTrackerTrees() {
  
  for( auto track : m_TopTracks ) { 
    g_track = track;
    treeTracksTop->Fill();
  }
  for( auto track : m_BotTracks ) { 
    g_track = track;
    treeTracksBot->Fill();
  }

}

#endif
