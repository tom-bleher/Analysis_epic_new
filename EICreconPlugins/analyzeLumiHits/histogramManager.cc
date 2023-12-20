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
  TTree *treeGenPhotons = new TTree("treeGenPhotons","Particles");

  //TreeTrackClass tracks;
  TrackClass g_track;
  PhotonRecClass g_recPhoton;
  PhotonGenClass g_genPhoton;
 
  void bookHistograms( TDirectory *dir ) {
    // Tracker histograms
    dir->mkdir("LumiTracker")->cd();

    gHistList->Add( new TH1D("hTrackerTop_Acceptance","Top tracker electron acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );

    gHistList->Add( new TH1D("hTrackerBot_Acceptance","Bottom tracker positron acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );
    gHistList->Add( new TH1D("hTrackerCoincidence_Acceptance","Tracker coincidence acceptance;E_{#gamma} (GeV);Acceptance", 2500, 0, 50) );

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
    treeTracksTop->Branch("xGamma", &g_track.xGamma);
    treeTracksTop->Branch("yGamma", &g_track.yGamma);
    treeTracksTop->Branch("slopeX", &g_track.slopeX);
    treeTracksTop->Branch("slopeY", &g_track.slopeY);
    treeTracksTop->Branch("theta", &g_track.theta);
    treeTracksTop->Branch("phi", &g_track.phi);
    treeTracksTop->Branch("chi2", &g_track.chi2);
    treeTracksTop->Branch("nHits", &g_track.nHits);
    treeTracksTop->Branch("eDeps", &g_track.eDeps); 
    treeTracksTop->Branch("time", &g_track.time); 
    treeTracksTop->Branch("primary", &g_track.primary); 
    
    treeTracksBot->SetDirectory( dir );
    treeTracksBot->Branch("e", &g_track.e);
    treeTracksBot->Branch("x0", &g_track.x0);
    treeTracksBot->Branch("y0", &g_track.y0);
    treeTracksBot->Branch("xGamma", &g_track.xGamma);
    treeTracksBot->Branch("yGamma", &g_track.yGamma);
    treeTracksBot->Branch("slopeX", &g_track.slopeX);
    treeTracksBot->Branch("slopeY", &g_track.slopeY);
    treeTracksBot->Branch("theta", &g_track.theta);
    treeTracksBot->Branch("phi", &g_track.phi);
    treeTracksBot->Branch("chi2", &g_track.chi2);
    treeTracksBot->Branch("nHits", &g_track.nHits);
    treeTracksBot->Branch("eDeps", &g_track.eDeps); 
    treeTracksBot->Branch("time", &g_track.time); 
    treeTracksBot->Branch("primary", &g_track.primary); 
   
    treeGenPhotons->SetDirectory( dir );
    treeGenPhotons->Branch("e", &g_genPhoton.e);
    treeGenPhotons->Branch("eElec", &g_genPhoton.eElec);
    treeGenPhotons->Branch("ePos", &g_genPhoton.ePos);
    treeGenPhotons->Branch("theta", &g_genPhoton.theta);
    treeGenPhotons->Branch("phi", &g_genPhoton.phi);
    treeGenPhotons->Branch("x", &g_genPhoton.x);
    treeGenPhotons->Branch("y", &g_genPhoton.y);

    treePhotons->SetDirectory( dir );
    treePhotons->Branch("e", &g_recPhoton.e);
    treePhotons->Branch("eTop", &g_recPhoton.eTop);
    treePhotons->Branch("eBot", &g_recPhoton.eBot);
    treePhotons->Branch("mass", &g_recPhoton.mass);
    treePhotons->Branch("x", &g_recPhoton.x);
    treePhotons->Branch("y", &g_recPhoton.y);
    treePhotons->Branch("dca", &g_recPhoton.dca);
    treePhotons->Branch("eGen", &g_recPhoton.eGen);
    treePhotons->Branch("xGen", &g_recPhoton.xGen);
    treePhotons->Branch("yGen", &g_recPhoton.yGen);
    treePhotons->Branch("thetaGen", &g_recPhoton.thetaGen);
    treePhotons->Branch("phiGen", &g_recPhoton.phiGen);
    treePhotons->Branch("chi2Top", &g_recPhoton.chi2Top);
    treePhotons->Branch("nHitsTop", &g_recPhoton.nHitsTop);
    //treePhotons->Branch("timeTop", &g_recPhoton.timeTop);
    treePhotons->Branch("primaryTop", &g_recPhoton.primaryTop);
    treePhotons->Branch("chi2Bot", &g_recPhoton.chi2Bot);
    treePhotons->Branch("nHitsBot", &g_recPhoton.nHitsBot);
    //treePhotons->Branch("timeBot", &g_recPhoton.timeBot);
    treePhotons->Branch("primaryBot", &g_recPhoton.primaryBot);
  }

  }
#endif
