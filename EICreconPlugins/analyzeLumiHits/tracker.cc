#ifndef TRACKER_CC
#define TRACKER_CC

#include "tracker.h"

// Constructor
//-------------------------------------------------------------------------
TrackerAnalysis::TrackerAnalysis( std::shared_ptr<DD4hep_service> geoSvc ) { 
  
  m_geoSvc = geoSvc;

  ROOT::Math::Translation3D trans(0.0, 0.0, variables::LumiAnalyzerMag_Z);//( rot, tr2 );
  coordTranslate_inv = trans.Inverse();

  CalcBdotDl();
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

//-------------------------------------------------------------------------
//void TrackerAnalysis::get_sensitive_volumeIDs(const dd4hep::IDDescriptor &id_desc, const dd4hep::PlacedVolume &pv, std::vector<dd4hep::VolumeID> *result, dd4hep::VolumeID id = 0) {
//  
//  id |= id_desc.encode(pv.volIDs());
//  
//  if (pv.volume().isSensitive()) {
//    result->push_back(id);
//    //std::cerr << pv->GetName() << " - " << std::hex << id << std::dec << std::endl;
//  }
//  for (TObject *node : *pv.volume()->GetNodes()) {
//    dd4hep::PlacedVolume pv { dynamic_cast<TGeoNode*>(node) };
//    get_sensitive_volumeIDs(id_desc, pv, result, id);
//  }
//}

//-------------------------------------------------------------------------
void TrackerAnalysis::InsertSensorNoise() {

  // Doesn't work at the moment
  // Not clear how to get list of cellIDs from a detector element
  //

  //gsl::not_null<const dd4hep::Detector*> det = m_geoSvc->detector();
  //const string type = "LumiSpecTracker";
  //
  //for(dd4hep::DetElement d : det->detectors(0) ) {
  //  
  //  if( d.type().compare( type ) != 0 ) { continue; }
  //  
  //  cout<<d.volumeID()<<endl;
  //  
  //  //cout<< d->name << "\t" << d->id << "\t" << d->volumeID << std::endl;
  //  //    d.volume()->Print();
  //  //    d.volume()->PrintNodes();
  //  //    std::cerr << d.volume()->CountNodes() << std::endl;
  //  for (const std::pair<std::string, dd4hep::DetElement> &p : d.children()) {
  //    std::cout << p.first << " <-" << std::endl;
  //  }
  //  for (const std::pair<std::string, int> &p : d.placement().volIDs()) {
  //    cout << p.first << "\t" << p.second << std::endl;
  //  }
  //  std::vector<dd4hep::VolumeID> ids;
  //  auto children = d.children();
  //  cout<<"N children: "<<children.size()<<endl;
  //  auto pl = d.placement();
  //  cout<<"N vols: "<<pl.volIDs().size()<<endl;
  //  //get_sensitive_volumeIDs(id_desc, d.placement(), &ids);
  //  //cout << ids.size() << std::endl;
  //}

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
    const auto gpos 	= m_geoSvc->converter()->position(id); // cm

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

    if( (*y * m_Bx_sign) > 0 ) { track.charge = -1; }// electrons go to bot CAL (Bx < 0)
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
    bool primaryTrack = true;
    for( int i = 0; i < hits->size(); i++ ) {
      TrackHit hit = hits->at(i);
      double DeltaX = pow(std::get<0>(hit) - (track.x0 + track.slopeX * std::get<2>(hit)), 2);
      double DeltaY = pow(std::get<1>(hit) - (track.y0 + track.slopeY * std::get<2>(hit)), 2);
      chi2 += DeltaX + DeltaY;
      track.eDeps.push_back( std::get<3>(hit) );
      track.time.push_back( std::get<4>(hit) );
      if( ! std::get<5>(hit) ) { primaryTrack = false; }
    }

    track.primary = primaryTrack;
    track.chi2 = chi2;
    track.nHits = Nmods;
    track.e = TrackerErec( track.slopeY );
    
    BackPropagate( &track ); // fill xGamma and yGamma in track structure

    //cout<<"x: "<<track.x0<<"  "<<track.xGamma<<endl;
    //cout<<"Tracker y at CAL front: "<<track.slopeY*variables::LumiSpecTracker_Z1 + track.y0<<"  Converter y, without B: "<<track.slopeY*variables::LumiConverter_Z + track.y0<<"   with B: "<<track.yGamma<<endl;
    
    tracks->push_back( track );

    *x = 0; *y = 0; *z = 0; *xz = 0; *yz = 0; *zz = 0;
    hits->clear();
  }

}

//-------------------------------------------------------------------------
void TrackerAnalysis::BackPropagate( TrackClass *track ) {

  // Exact Method:
  // for electrons traversing a non-uniform B field, a non-linear PDE emerges
  // v_z'' - v_z' *(Grad(B) dot v)/Bx + (q*Bx/m)^2 * v_z = 0
  // middle term contains v_z' * v_z, which makes it non-linear

  // Approximate Method: 
  // assume a constant integral Bx*dz --> known electron energy
  // back propagate to converter in steps
  double z = variables::LumiAnalyzerMagEnd_Z - 1000;
  double dz = 1; // mm
  double P_yz = TrackerErec( track->slopeY );
  double slopeXZ = track->slopeX;
  double slopeYZ = track->slopeY;
  double x = slopeXZ * z + track->x0;
  double y = slopeYZ * z + track->y0;
  double px = P_yz * sin(track->theta) * cos(track->phi);
  double py = P_yz * sin(track->theta) * sin(track->phi);
  double pz = P_yz * cos(track->theta);

  gsl::not_null<const dd4hep::Detector*> description = m_geoSvc->detector();
  
  //cout<<"Initial y: "<<y<<"   dy: "<<dz * slopeYZ<<"   Nsteps: "<<fabs(variables::LumiConverter_Z - z)/dz<<endl;
  //cout<<"P_yz: "<<P_yz<<"  Initial theta: "<<track->theta<<"  Initial phi: "<<track->phi<<endl;

  while( z < variables::LumiConverter_Z && P_yz > 0 ) {

    // convert mm to cm (dd4hep description in cm)
    double posV[3] = { x/10., y/10., z/10. };
    double B[3];
    description->field().magneticField( posV , B );
    // divide by dd4hep::tesla to get the field in tesla
    B[0] /= dd4hep::tesla;
    B[1] /= dd4hep::tesla;
    B[2] /= dd4hep::tesla;

    double dx = 0;
    double dy = 0;

    // Bx dominates
    if( fabs(B[0]) < 0.0001 ) { 
      dy = dz * slopeYZ;
    }
    else {
      double R_yz = P_yz * variables::RmagPreFactor / fabs(B[0]); // cyclotron radius of curvature
      double theta = atan(slopeYZ);
      dy = R_yz * cos(TMath::Pi() - theta) + sqrt( R_yz*R_yz - pow(dz - R_yz*sin(TMath::Pi() - theta), 2) );
    }

    // By is very small
    dx = dz * slopeXZ; // ignores By
    //if( fabs(B[1]) < 0.0001 ) { 
    //  dx = dz * slopeXZ;
    //}
    //else {
    //  double R_xz = P_yz * variables::RmagPreFactor / fabs(B[1]); // cyclotron radius of curvature
    //  double theta = atan(slopeXZ);
    //  dx = R_xz * cos(TMath::Pi() - theta) + sqrt( R_xz*R_xz - pow(dz - R_xz*sin(TMath::Pi() - theta), 2) );
    //}

    double dpx = 0;//0.29979 * (dz/1000.) * B[1] * track->charge; // GeV, dz(m) and B(T)
    double dpy = 0.29979 * (dz/1000.) * B[0] * track->charge; // GeV, dz(m) and B(T)
    double dpz = -fabs(0.29979 * (dy/1000.) * B[0]);

    //cout<<"x: "<<x<<"  y: "<<y<<"  z: "<<z<<"  px: "<<px<<"  py: "<<py<<"  pz: "<<pz<<"   Bx: "<<B[1]<<"  dx: "<<dx<<"  dpx: "<<dpx<<"  dpz: "<<dpz<<"  slopeXZ: "<<slopeXZ<<endl;
    
    px += dpx;
    py += dpy;
    pz += dpz;
    x += dx;
    y += dy;
    z += dz;
    slopeXZ = px / pz;
    slopeYZ = py / pz;
  }
  
  track->xGamma = x;
  track->yGamma = y;
}

//-------------------------------------------------------------------------
double TrackerAnalysis::TrackerErec( double slopeY ) {

  double sinTheta = fabs( sin( atan(slopeY) ) );
 
  if( sinTheta == 0 ) { return 0.0; }

  // ( c * BxDotDz ) = pT generated in B field in GeV: (c*GeV/eV)*B(T)*dZ(m)
  double E = fabs(constants::speedLight * m_BxDotDz_central) / sinTheta;
  
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
    double new_py = py + pow(-1, i+1) * fabs(constants::speedLight * m_BxDotDz_central); // subtract py induced by B field
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

  for( auto track : m_TopTracks ) {
    
    if( fabs( track.xGamma ) < variables::LumiConverterCut_DXY 
        && fabs( track.yGamma ) < variables::LumiConverterCut_DXY ) {
      ((TH1D*)gHistList->FindObject("hTrackerTop_Acceptance"))->Fill( variables::Ephoton );
    }
  }
  for( auto track : m_BotTracks ) {
    
    if( fabs( track.xGamma ) < variables::LumiConverterCut_DXY 
        && fabs( track.yGamma ) < variables::LumiConverterCut_DXY ) {
      ((TH1D*)gHistList->FindObject("hTrackerBot_Acceptance"))->Fill( variables::Ephoton );
    }
  }

 
  // Trees with all track pairings
  for( auto topTrack : m_AllTopTracks ) {

    double Etop = TrackerErec( topTrack.slopeY );

    for( auto botTrack : m_AllBotTracks ) {

      double Ebot = TrackerErec( botTrack.slopeY );

      double pairMass = GetPairMass( topTrack, botTrack );

      double dca = sqrt( pow(topTrack.xGamma - botTrack.xGamma,2) + pow(topTrack.yGamma - botTrack.yGamma,2) );
      if( dca < variables::LumiPhotonDCAcut ) {
        ((TH1D*)gHistList->FindObject("hTrackerCoincidence_Acceptance"))->Fill( variables::Ephoton );
      }
      //std::pair<double, double> DCAandZ = DCA( topTrack, botTrack );

      g_recPhoton.e = Etop + Ebot;
      g_recPhoton.eTop = Etop;
      g_recPhoton.eBot = Ebot;
      g_recPhoton.mass = pairMass;
      g_recPhoton.x = (topTrack.xGamma + botTrack.xGamma)/2.;
      g_recPhoton.y = (topTrack.yGamma + botTrack.yGamma)/2.;
      //g_recPhoton.dca = std::get<0>(DCAandZ);
      g_recPhoton.dca = dca;
      g_recPhoton.eGen = variables::Ephoton;
      g_recPhoton.xGen = variables::Xphoton;
      g_recPhoton.yGen = variables::Yphoton;
      g_recPhoton.thetaGen = variables::ThetaPhoton;
      g_recPhoton.phiGen = variables::PhiPhoton;
      g_recPhoton.chi2Top = topTrack.chi2;
      g_recPhoton.nHitsTop = topTrack.nHits;
      g_recPhoton.primaryTop = topTrack.primary;
      g_recPhoton.chi2Bot = botTrack.chi2;
      g_recPhoton.nHitsBot = botTrack.nHits;
      g_recPhoton.primaryBot = botTrack.primary;

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

//-------------------------------------------------------------------------
void TrackerAnalysis::CalcBdotDl() {

   // calculate B*dL
  std::cout<<"Calculating B*dL in lumi analyzer magnet"<<std::endl;

  gsl::not_null<const dd4hep::Detector*> description = m_geoSvc->detector();

  double FiveSigmaLimits = 7.5 / 5.; // cm
  
  double BxdL_mean = 0;
  double BydL_mean = 0;
  double BzdL_mean = 0;
  double BxdL_mean2 = 0;
  double BydL_mean2 = 0;
  double BzdL_mean2 = 0;
  double entries_mean = 0;

  for( double x = -8; x < 8.1; x += 0.5 ) {
    for( double y = -20; y < 20.1; y += 0.5 ) {

      double BxdL = 0;
      double BydL = 0;
      double BzdL = 0;

      for( double z = -5850; z > -6150; z -= 1.0 ) {

        double posV[3] = { x, y, z };
        double bfieldV[3];
        description->field().magneticField( posV , bfieldV );

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

      // central value used for back propagation
      if( fabs(x) < 0.25 && fabs(y) < 0.25 ) {
        m_BxDotDz_central = BxdL;
        m_Bx_sign = int( BxdL / fabs(BxdL) );
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

  cout<<"Central Bx*dL: "<<m_BxDotDz_central<<endl;
  cout<<"Mean Bx*dL in 5 sigma region: "<<BxdL_mean<<"   Std: "<<sqrt( BxdL_mean2 - pow(BxdL_mean,2) )<<endl;
  cout<<"Mean By*dL in 5 sigma region: "<<BydL_mean<<"   Std: "<<sqrt( BydL_mean2 - pow(BydL_mean,2) )<<endl;
  cout<<"Mean Bz*dL in 5 sigma region: "<<BzdL_mean<<"   Std: "<<sqrt( BzdL_mean2 - pow(BzdL_mean,2) )<<endl;
   
  //printf(" LUMI B FIELD: %+15.8e  %+15.8e  %+15.8e  %+15.8e  %+15.8e  %+15.8e  \n",
  //    posV[0]/dd4hep::cm, posV[1]/dd4hep::cm,  posV[2]/dd4hep::cm,
  //    bfieldV[0]/dd4hep::tesla , bfieldV[1]/dd4hep::tesla, bfieldV[2]/dd4hep::tesla ) ;

}


#endif
