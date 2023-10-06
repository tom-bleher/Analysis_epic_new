#ifndef HISTOGRAMMANAGER_CC
#define HISTOGRAMMANAGER_CC

#include "histogramManager.h"

namespace histogramManager {

  THashList *gHistList = new THashList();
  
  TTree *treeCAL_Hits = new TTree("treeCAL_Hits","Hits");
  TTree *treeCAL_RecHits = new TTree("treeCAL_RecHits","RecHits");
  TTree *treeCAL_Clusters = new TTree("treeCAL_Clusters","Clusters");
  TTree *treeTracker_Hits = new TTree("treeTracker_Hits","Hits");
  TTree *treeTracksTop = new TTree("treeTracksTop","Tracks");
  TTree *treeTracksBot = new TTree("treeTracksBot","Tracks");
  TTree *treePhotons = new TTree("treePhotons","Tracks");

  //TreeTrackClass tracks;
  TrackClass g_track;
  PhotonRecClass g_recPhotons;
 
  void bookHistograms( TDirectory *dir ) {
    // Tracker histograms
    dir->mkdir("LumiTracker")->cd();

    gHistList->Add( new TH1D("hTrackerTop_Acceptance","Top tracker electron acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );

    gHistList->Add( new TH1D("hTrackerBot_Acceptance","Bottom tracker positron acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );
    gHistList->Add( new TH1D("hTrackerCoincidence_Acceptance","Tracker coincidence acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );
    gHistList->Add( new TH2D("hTrackers_Eres_allPairs","Tracker E resolution;E_{#gamma} (GeV);(E_{gen}-E_{rec})/E_{gen}",200,0,50, 200,-1,1) );
    gHistList->Add( new TH2D("hTrackers_Eres","Tracker E resolution;E_{#gamma} (GeV);(E_{gen}-E_{rec})/E_{gen}",200,0,50, 200,-1,1) );

    gHistList->Add( new TH2D("hTrackers_E_allPairs","Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50) );
    gHistList->Add( new TH2D("hTrackers_E","Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50) );
    gHistList->Add( new TH2D("hTrackersTop_E","Top Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50) );
    gHistList->Add( new TH2D("hTrackersBot_E","Bottom Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,50, 500,0,50) );

    gHistList->Add( new TH1D("hTrackers_X_allPairs","X at converter;(x_{electron} + x_{positron})/2 (mm);counts",1000,-50,50) );
    gHistList->Add( new TH1D("hTrackers_Y_allPairs","Y at converter;(y_{electron} + y_{positron})/2 (mm);counts",1000,-50,50) );
    gHistList->Add( new TH1D("hTrackers_DX_allPairs","#DeltaX at converter;x_{electron} - x_{positron} (mm);counts",1000,-50,50) );
    gHistList->Add( new TH1D("hTrackers_DY_allPairs","#DeltaY at converter;y_{electron} - y_{positron} (mm);counts",1000,-50,50) );

    gHistList->Add( new TH1D("hTrackers_X","X at converter;(x_{electron} + x_{positron})/2 mm);counts",1000,-50,50) );
    gHistList->Add( new TH1D("hTrackers_Y","Y at converter;(y_{electron} + y_{positron})/2 (mm);counts",1000,-50,50) );
    gHistList->Add( new TH2D("hTrackers_X_BotVsTop","X at converter;X positron (mm);X electron (mm)", 1000,-50,50, 1000,-50,50) );
    gHistList->Add( new TH2D("hTrackers_Y_BotVsTop","Y at converter;Y positron (mm);Y electron (mm)", 1000,-50,50, 1000,-50,50) );

    gHistList->Add( new TH2D("hTrackers_DCAvsZ","pair DCA vs Z;DCA (mm); Z (mm)",1000,0,100, 1000,variables::LumiAnalyzerMag_Z - variables::LumiAnalyzerMag_DZ, variables::LumiAnalyzerMag_Z + variables::LumiAnalyzerMag_DZ ) );
    gHistList->Add( new TH1D("hTrackers_InvMass_allPairs","tracker pair mass",1000,0,1) );
    gHistList->Add( new TH1D("hTrackers_InvMass","tracker pair mass",1000,0,1) );

    gHistList->Add( new TH1D("hTrackChi2", "Track #chi^{2} / Nlayers;#chi^{2};counts", 1000,0,1) );
    gHistList->Add( new TH1D("hTrackersSlope", "", 1000,0,1) );

    for( int i=0; i < variables::maxModules; i++ ) {
      for( int j=0; j < variables::maxSectors; j++ ) {
        gHistList->Add( new TH2D( 
              Form("hGlobalXY_%d_%d", i,j), "Global Tracker Hits X vs Y;X (mm);Y (mm)", 600,-300,300, 600,-300,300) );
      }
    }

    ////////////////////////////////////////////////////////
    // CAL histograms
    dir->mkdir("LumiSpecCAL")->cd();

    gHistList->Add( new TH1D("hCALTop_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );
    gHistList->Add( new TH1D("hCALBot_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );
    gHistList->Add( new TH1D("hCALCoincidence_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );

    gHistList->Add( new TH1D("hEraw",  "hit energy (raw)", 2500, 0, 50) );
    gHistList->Add( new TH1D("hErawTotal",  "summed hit energy (raw)", 2500, 0, 50) );
    gHistList->Add( new TH1D("hEup", "Upper CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50) );
    gHistList->Add( new TH1D("hEdw", "Lower CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50) );
    gHistList->Add( new TH1D("hEnergy", "CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50) );
    gHistList->Add( new TH2D("hCALCluster_Eres", "Egen vs Cluster based photon Erec", 200,0,50, 2500,0,50) );

    ////////////////////////////////////////////////////////
    // root dir histograms 
    dir->cd();

    gHistList->Add( new TH1D("hGenPhoton_E", "Generated photon energy;E_{#gamma} (GeV);Nevents", 2500,0,50));
    gHistList->Add( new TH2D("hGenPhoton_xy", "Generated photon vertex;x (mm);y (mm)", 6000,-300,300, 6000,-300,300));
    gHistList->Add( new TH1D("hGenElectron_E", "Generated photon energy;E_{#gamma} (GeV);Nevents", 2500,0,50));
    gHistList->Add( new TH2D("hGenElectron_xy", "Generated photon vertex;x (mm);y (mm)", 6000,-300,300, 6000,-300,300));
    gHistList->Add( new TH1D("hGenPositron_E", "Generated photon energy;E_{#gamma} (GeV);Nevents", 2500,0,50));
    gHistList->Add( new TH2D("hGenPositron_xy", "Generated photon vertex;x (mm);y (mm)", 6000,-300,300, 6000,-300,300));
    gHistList->Add( new TH1D("hGenEventCount", "Number of generated events per Egen;E_{#gamma} (GeV);Nevents", 2500, 0, 50) );

    gHistList->Add( new TH1D("hProtoClusterCount", "Number of proto island clusters / event", 20,-0.5,19.5) );
    gHistList->Add( new TH1D("hClusterCount", "Number of clusters / event;# clusters / event", 20,-0.5,19.5) );
    gHistList->Add( new TH1D("hADCsignal", "ADC signal", 16385,-0.5,16384.5) );

    gHistList->Add( new TH2D("hBxdotdL", "Bx*dz versus x and y;x (cm);y (cm)", 33,-8.25,8.25, 81,-20.25,20.25) );
    gHistList->Add( new TH2D("hBydotdL", "By*dz versus x and y;x (cm);y (cm)", 33,-8.25,8.25, 81,-20.25,20.25) );
    gHistList->Add( new TH2D("hBzdotdL", "Bz*dz versus x and y;x (cm);y (cm)", 33,-8.25,8.25, 81,-20.25,20.25) );

    // TTrees
    treeCAL_Hits->SetDirectory( dir );
    treeCAL_Hits->Branch("E", &variables::E_hit);
    treeCAL_Hits->Branch("x", &variables::x_hit);
    treeCAL_Hits->Branch("y", &variables::y_hit);
    treeCAL_Hits->Branch("r", &variables::r_hit);
    treeCAL_Hits->Branch("sec_id", &variables::sec_id);
    treeCAL_Hits->Branch("mod_id", &variables::mod_id);
    treeCAL_Hits->Branch("fiber_x_id", &variables::fiber_x_id);
    treeCAL_Hits->Branch("fiber_y_id", &variables::fiber_y_id);

    treeCAL_RecHits->SetDirectory( dir );
    treeCAL_RecHits->Branch("E", &variables::E_hit);
    treeCAL_RecHits->Branch("x", &variables::x_hit);
    treeCAL_RecHits->Branch("y", &variables::y_hit);
    treeCAL_RecHits->Branch("r", &variables::r_hit);
    treeCAL_RecHits->Branch("t", &variables::t_hit);

    treeCAL_Clusters->SetDirectory( dir );
    treeCAL_Clusters->Branch("Nhits", &variables::Nhits_cluster);
    treeCAL_Clusters->Branch("E", &variables::E_cluster);
    treeCAL_Clusters->Branch("x", &variables::x_cluster);
    treeCAL_Clusters->Branch("y", &variables::y_cluster);
    treeCAL_Clusters->Branch("r", &variables::r_cluster);
    treeCAL_Clusters->Branch("t", &variables::t_cluster);
    treeCAL_Clusters->Branch("Radius", &variables::Radius_cluster);
    treeCAL_Clusters->Branch("Dispersion", &variables::Dispersion_cluster);
    treeCAL_Clusters->Branch("SigmaThetaPhi_short", &variables::SigmaThetaPhi2_cluster);
    treeCAL_Clusters->Branch("SigmaThetaPhi_long", &variables::SigmaThetaPhi1_cluster);

    treeTracker_Hits->SetDirectory( dir );
    treeTracker_Hits->Branch("x", &variables::x_hit);
    treeTracker_Hits->Branch("y", &variables::y_hit);
    treeTracker_Hits->Branch("z", &variables::z_hit);

    treeTracksTop->SetDirectory( dir );
    treeTracksTop->Branch("e", &g_track.e);
    treeTracksTop->Branch("x0", &g_track.x0);
    treeTracksTop->Branch("y0", &g_track.y0);
    treeTracksTop->Branch("slopeX", &g_track.slopeX);
    treeTracksTop->Branch("slopeY", &g_track.slopeY);
    treeTracksTop->Branch("theta", &g_track.theta);
    treeTracksTop->Branch("chi2", &g_track.chi2);
    treeTracksTop->Branch("nHits", &g_track.nHits);
    treeTracksTop->Branch("eDeps", &g_track.eDeps); 
    treeTracksTop->Branch("time", &g_track.time); 
    treeTracksTop->Branch("primary", &g_track.primary); 
    
    treeTracksBot->SetDirectory( dir );
    treeTracksBot->Branch("e", &g_track.e);
    treeTracksBot->Branch("x0", &g_track.x0);
    treeTracksBot->Branch("y0", &g_track.y0);
    treeTracksBot->Branch("slopeX", &g_track.slopeX);
    treeTracksBot->Branch("slopeY", &g_track.slopeY);
    treeTracksBot->Branch("theta", &g_track.theta);
    treeTracksBot->Branch("chi2", &g_track.chi2);
    treeTracksBot->Branch("nHits", &g_track.nHits);
    treeTracksBot->Branch("eDeps", &g_track.eDeps); 
    treeTracksBot->Branch("time", &g_track.time); 
    treeTracksBot->Branch("primary", &g_track.primary); 
    
    treePhotons->SetDirectory( dir );
    treePhotons->Branch("e", &g_recPhotons.e);
    treePhotons->Branch("mass", &g_recPhotons.mass);
    treePhotons->Branch("x", &g_recPhotons.x);
    treePhotons->Branch("y", &g_recPhotons.y);
    treePhotons->Branch("dca", &g_recPhotons.dca);
    treePhotons->Branch("eGen", &g_recPhotons.eGen);
    treePhotons->Branch("xGen", &g_recPhotons.xGen);
    treePhotons->Branch("yGen", &g_recPhotons.yGen);
    treePhotons->Branch("chi2Top", &g_recPhotons.chi2Top);
    treePhotons->Branch("nHitsTop", &g_recPhotons.nHitsTop);
    treePhotons->Branch("timeTop", &g_recPhotons.timeTop);
    treePhotons->Branch("primaryTop", &g_recPhotons.primaryTop);
    treePhotons->Branch("chi2Bot", &g_recPhotons.chi2Bot);
    treePhotons->Branch("nHitsBot", &g_recPhotons.nHitsBot);
    treePhotons->Branch("timeBot", &g_recPhotons.timeBot);
    treePhotons->Branch("primaryBot", &g_recPhotons.primaryBot);
  }

}
#endif
