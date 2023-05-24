#ifndef TRACKER_CC
#define TRACKER_CC

#include "constants.h"
#include "variables.h"
#include "histogramManager.h"

#include "tracker.h"

// Constructor
//-------------------------------------------------------------------------
TrackerAnalysis::TrackerAnalysis( JEventProcessorSequentialRoot::PrefetchT<edm4hep::SimTrackerHit>& hits ) {
  // Constructor
  Tracker_hits = hits;
}

// Fill Hit Structures
//-------------------------------------------------------------------------
void TrackerAnalysis::FillTrackerHits() {

//global position
  map<string, int> Trackerfield_idx_Map{ {"sector", 0}, {"module", 0}};

  cout<<Tracker_hits().size()<<endl;

  for( auto hit : Tracker_hits() ){

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
cout<<gpos.y()<<endl;
    if( sec_id < variables::maxSectors && mod_id < variables::maxModules ) {

      MyHit myhit = {10*gpos.x(), 10*gpos.y(), variables::Tracker_Zs[mod_id]}; // mm
      //cout<<setprecision(6)<<"hit X: "<<get<0>(myhit)<<"   hit Y: "<<get<1>(myhit)<<"   hit Z: "<<get<2>(myhit)<<endl;

      if( sec_id == 0 ) { // top
        if( mod_id == 0 ) { // closest to IP
          if( ! PixelOverlap( myhit, TopTracker1Hits ) ) { TopTracker1Hits.push_back( myhit ); }
        }
        if( mod_id == 1 ) {
          if( ! PixelOverlap( myhit, TopTracker2Hits ) ) { TopTracker2Hits.push_back( myhit ); }
        }
        if( mod_id == 2 ) {
          if( ! PixelOverlap( myhit, TopTracker3Hits ) ) { TopTracker3Hits.push_back( myhit ); }
        }
      }
      if( sec_id == 1 ) { // bottom
        if( mod_id == 0 ) { // closest to IP
          if( ! PixelOverlap( myhit, BotTracker1Hits ) ) { BotTracker1Hits.push_back( myhit ); }
        }
        if( mod_id == 1 ) {
          if( ! PixelOverlap( myhit, BotTracker2Hits ) ) { BotTracker2Hits.push_back( myhit ); }
        }
        if( mod_id == 2 ) {
          if( ! PixelOverlap( myhit, BotTracker3Hits ) ) { BotTracker3Hits.push_back( myhit ); }
        }
      }
      
      ((TH1D *)gHistList->FindObject(Form("hGlobalXY_%i_%i",mod_id,sec_id) ))->Fill( std::get<0>(myhit), std::get<1>(myhit) );

    }
  } //Tracker hits close

  // Tracker Tree
  for( auto hit : Tracker_hits() ){
    variables::x_hit = hit->x();
    variables::y_hit = hit->y();
    variables::z_hit = hit->z();

    treeTracker_Hits->Fill();
  }

}

//-------------------------------------------------------------------------
void TrackerAnalysis::AssembleAllTracks(){

  AssembleTracks( &AllTopTracks, TopTracker1Hits, TopTracker2Hits, TopTracker3Hits );
  AssembleTracks( &AllBotTracks, BotTracker1Hits, BotTracker2Hits, BotTracker3Hits );

  // Filter out the bad tracks
  for( auto track : AllTopTracks ) { 
    if( track.Chi2/3. < variables::max_chi2ndf ) { 
      TopTracks.push_back( track );
    }
  }

  for( auto track : AllBotTracks ) { 
    if( track.Chi2/3. < variables::max_chi2ndf ) { 
      BotTracks.push_back( track ); 
    }
  }

}

//-------------------------------------------------------------------------
void TrackerAnalysis::AssembleTracks( vector<TrackClass> *tracks, vector<MyHit> tracker1Hits, vector<MyHit> tracker2Hits, vector<MyHit> tracker3Hits) {

  vector<vector<MyHit>> hitSet = {tracker1Hits, tracker2Hits, tracker3Hits};
  // units of hits are in mm

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
double TrackerAnalysis::TrackerErec( double slopeY ) {

  double sinTheta = fabs( sin( atan(slopeY) ) );
 
  if( sinTheta == 0 ) { return 0.0; }

  double E = variables::pT / sinTheta;
  
  return E;
}

//-------------------------------------------------------------------------
bool TrackerAnalysis::PixelOverlap( MyHit hit, vector<MyHit> trackSet ) {

  for( auto el : trackSet ) {
    double delta = pow( std::get<0>(hit) - std::get<0>(el), 2);
    delta += pow( std::get<1>(hit) - std::get<1>(el), 2);

    if( sqrt(delta) < variables::Tracker_pixelSize ) { return true; }
  }

  return false;
}

//-------------------------------------------------------------------------
// returns y deflection of ultrarel electrons in magnet region assuming primordial py = 0
double TrackerAnalysis::DeltaYmagnet( double E, double charge ) {

  //return 0;

  double R = E * variables::RmagPreFactor; // cyclotron radius of curvature
  double dy = R - sqrt( R*R - pow(variables::LumiSpecMag_DZ,2) );

  // electrons go to the top CAL (B in +x direction), positrons to bottom CAL
  return -charge * dy; // mm
}

//-------------------------------------------------------------------------
double TrackerAnalysis::XatConverter( TrackClass track ) {

  double x_c = (track.slopeX * variables::LumiConverter_Z + track.X0);
  
  return x_c;
}

//-------------------------------------------------------------------------
double TrackerAnalysis::YatConverter( TrackClass track ) {
  double E = TrackerErec( track.slopeY );
  double y_c = (track.slopeY * variables::LumiSpecMagEnd_Z + track.Y0) - DeltaYmagnet( E, track.charge );
  
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

  for( auto track : AllTopTracks ) { ((TH1D *)gHistList->FindObject("hTrackChi2"))->Fill( track.Chi2/3. ); }
  for( auto track : AllBotTracks ) { ((TH1D *)gHistList->FindObject("hTrackChi2"))->Fill( track.Chi2/3. ); }

// Loop over good tracks
  bool EventWithTopTrackNearConverterCenter = false;
  bool EventWithBotTrackNearConverterCenter = false;
  for( auto track : TopTracks ) {
    
    double Etop = TrackerErec( track.slopeY );
    ((TH2D *)gHistList->FindObject("hTrackersTop_E"))->Fill( variables::Einput, Etop );
    ((TH1D *)gHistList->FindObject("hTrackersSlope"))->Fill( track.slopeY );
 
    if( fabs( XatConverter( track ) ) < variables::LumiConverterCut_DXY / 2. 
        && fabs( YatConverter( track ) ) < variables::LumiConverterCut_DXY / 2. ) {
      EventWithTopTrackNearConverterCenter = true;
    }
  }
  for( auto track : BotTracks ) {
    
    double Ebot = TrackerErec( track.slopeY );
    ((TH2D *)gHistList->FindObject("hTrackersBot_E"))->Fill( variables::Einput, Ebot );
    ((TH1D *)gHistList->FindObject("hTrackersSlope"))->Fill( track.slopeY );

    if( fabs( XatConverter( track ) ) < variables::LumiConverterCut_DXY / 2. 
        && fabs( YatConverter( track ) ) < variables::LumiConverterCut_DXY / 2. ) {
      EventWithBotTrackNearConverterCenter = true;
    }
  }

  if( EventWithTopTrackNearConverterCenter ) { 
    ((TH1D*)gHistList->FindObject("hTrackerTop_Acceptance"))->Fill( variables::Einput );
  }
  if( EventWithBotTrackNearConverterCenter ) { 
    ((TH1D*)gHistList->FindObject("hTrackerBot_Acceptance"))->Fill( variables::Einput );
  }
  if( EventWithTopTrackNearConverterCenter && EventWithBotTrackNearConverterCenter ) { 
    ((TH1D*)gHistList->FindObject("hTrackerCoincidence_Acceptance"))->Fill( variables::Einput );
  }

  // Loop over good pairs of tracks
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

      if( fabs(xtop_c - xbot_c) > 2*variables::Tracker_sigma 
          || fabs(ytop_c - ybot_c) > 2*variables::Tracker_sigma ) { continue; }
      //if( fabs(ytop_c - ybot_c) > 2*Tracker_sigma ) { continue; }
      
      ((TH1D *)gHistList->FindObject("hTrackers_InvMass"))->Fill( pairMass );
      ((TH1D *)gHistList->FindObject("hTrackers_X"))->Fill( (xtop_c + xbot_c)/2. );
      ((TH1D *)gHistList->FindObject("hTrackers_Y"))->Fill( (ytop_c + ybot_c)/2. );
      ((TH2D *)gHistList->FindObject("hTrackers_Eres"))->Fill( variables::Einput, (variables::Einput - (Etop + Ebot) )/variables::Einput );
      ((TH2D *)gHistList->FindObject("hTrackers_E"))->Fill( variables::Einput, Etop + Ebot );
      ((TH2D *)gHistList->FindObject("hTrackers_X_BotVsTop"))->Fill( xtop_c, xbot_c );
      ((TH2D *)gHistList->FindObject("hTrackers_Y_BotVsTop"))->Fill( ytop_c, ybot_c );
    }
  }

}

//-------------------------------------------------------------------------
void TrackerAnalysis::FillTrackerTrees() {

  TreeTrackClass tracksBuffer;
  
  for( auto track : TopTracks ) { 
      tracksBuffer.X0_e.push_back( XatConverter( track ) );
      tracksBuffer.Y0_e.push_back( YatConverter( track ) );
      tracksBuffer.slopeX_e.push_back( track.slopeX );
      tracksBuffer.slopeY_e.push_back( track.slopeY );
  }
  for( auto track : BotTracks ) { 
      tracksBuffer.X0_p.push_back( XatConverter( track ) );
      tracksBuffer.Y0_p.push_back( YatConverter( track ) );
      tracksBuffer.slopeX_p.push_back( track.slopeX );
      tracksBuffer.slopeY_p.push_back( track.slopeY );
  }

  histogramManager::tracks = tracksBuffer;

  treeTracks->Fill();
}

#endif
