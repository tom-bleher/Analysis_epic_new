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

    gHistList->Add( new TH1D("hTrackerTop_Acceptance","Top tracker electron acceptance;E_{#gamma} (GeV);Acceptance", 200, 0, 20) );

    gHistList->Add( new TH1D("hTrackerBot_Acceptance","Bottom tracker positron acceptance;E_{#gamma} (GeV);Acceptance", 200, 0, 20) );
    gHistList->Add( new TH1D("hTrackerCoincidence_Acceptance","Tracker coincidence acceptance;E_{#gamma} (GeV);Acceptance", 200, 0, 20) );

    gHistList->Add( new TH1D("hTrackerTop_Acceptance_v2_Numer", "Tracker Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));
    gHistList->Add( new TH1D("hTrackerTop_Acceptance_v2_Denom", "Tracker Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));
    gHistList->Add( new TH1D("hTrackerBot_Acceptance_v2_Numer", "Tracker Acceptance;E_{e+MC} (GeV); Counts", 200, 0, 20));
    gHistList->Add( new TH1D("hTrackerBot_Acceptance_v2_Denom", "Tracker Acceptance;E_{e+MC} (GeV); Counts", 200, 0, 20));
    gHistList->Add( new TH1D("hTrackerCoincidence_Acceptance_v2_Numer", "Tracker Acceptance; E_{#gamma MC} (GeV); Counts", 200, 0, 20));
    gHistList->Add( new TH1D("hTrackerCoincidence_Acceptance_v2_Denom", "Tracker Acceptance; E_{#gamma MC} (GeV); Counts", 200, 0, 20));


    gHistList->Add( new TH2D("hTrackers_Eres","Tracker E resolution;E_{#gamma} (GeV);(E_{gen}-E_{rec})/E_{gen}",200,0,20, 200,-1,1) );

    gHistList->Add( new TH2D("hTrackers_E","Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,20, 500,0,50) );
    gHistList->Add( new TH2D("hTrackersTop_E","Top Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,20, 500,0,50) );
    gHistList->Add( new TH2D("hTrackersBot_E","Bottom Tracker E;E_{#gamma} (GeV);E_{rec} (GeV)", 200,0,20, 500,0,50) );

    gHistList->Add( new TH1D("hTrackers_X_allPairs","X at converter;(x_{electron} + x_{positron})/2 (mm);counts",100,-50,50) );
    gHistList->Add( new TH1D("hTrackers_Y_allPairs","Y at converter;(y_{electron} + y_{positron})/2 (mm);counts",100,-50,50) );
    gHistList->Add( new TH1D("hTrackers_DX_allPairs","#DeltaX at converter;x_{electron} - x_{positron} (mm);counts",100,-50,50) );
    gHistList->Add( new TH1D("hTrackers_DY_allPairs","#DeltaY at converter;y_{electron} - y_{positron} (mm);counts",100,-50,50) );

    gHistList->Add( new TH1D("hTrackers_X","X at converter;(x_{electron} + x_{positron})/2 mm);counts",100,-50,50) );
    gHistList->Add( new TH1D("hTrackers_Y","Y at converter;(y_{electron} + y_{positron})/2 (mm);counts",100,-50,50) );
    gHistList->Add( new TH2D("hTrackers_X_BotVsTop","X at converter;X positron (mm);X electron (mm)", 100,-50,50, 100,-50,50) );
    gHistList->Add( new TH2D("hTrackers_Y_BotVsTop","Y at converter;Y positron (mm);Y electron (mm)", 100,-50,50, 100,-50,50) );

    gHistList->Add( new TH1D("hTrackers_InvMass_allPairs","tracker pair mass",100,0,0.1) );
    gHistList->Add( new TH1D("hTrackers_InvMass","tracker pair mass",100,0,0.1) );

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

    gHistList->Add( new TH1D("hCALTop_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 200, 0, 20) );
    gHistList->Add( new TH1D("hCALBot_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 200, 0, 20) );
    gHistList->Add( new TH1D("hCALCoincidence_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 200, 0, 20) ); 
    gHistList->Add( new TH2D("hCALCoincidence_Acceptance_v2", "CAL acceptance; (E_{e-MC} + E_{e+MC}) (GeV); (E_{TopDet} + E_{BotDet}) (GeV)", 200, 0, 20, 200, 0, 20) ); 
    gHistList->Add( new TH1D("hCALCoincidence_Acceptance_v3_Numer", "CAL Acceptance; E_{#gamma MC} (GeV); Counts", 200, 0, 20));
    gHistList->Add( new TH1D("hCALCoincidence_Acceptance_v3_Denom", "CAL Acceptance; E_{#gamma MC} (GeV); Counts", 200, 0, 20));

    gHistList->Add( new TH1D("hCALTop_Acceptance_v2", "CAL acceptance;E_{e-MC} (GeV);Acceptance", 200, 0, 20) );
    gHistList->Add( new TH2D("hCALTop_Acceptance_v3", "CAL acceptance;E_{e-MC} (GeV);E_{TopDet} (GeV)", 200, 0, 20, 200, 0, 20) );
    gHistList->Add( new TH1D("hCALTop_Acceptance_v4_Numer", "CAL Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));
    gHistList->Add( new TH1D("hCALTop_Acceptance_v4_Denom", "CAL Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));

    gHistList->Add( new TH1D("hCALBot_Acceptance_v2", "CAL acceptance;E_{e+MC} (GeV);Acceptance", 200, 0, 20) );
    gHistList->Add( new TH2D("hCALBot_Acceptance_v3", "CAL acceptance;E_{e+MC} (GeV);E_{BotDet} (GeV)", 200, 0, 20, 200, 0, 20) );
    gHistList->Add( new TH1D("hCALBot_Acceptance_v4_Numer", "CAL Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));
    gHistList->Add( new TH1D("hCALBot_Acceptance_v4_Denom", "CAL Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));

    gHistList->Add( new TH2D("hCAL_Eres","CAL E resolution;E_{#gamma} (GeV);(E_{#gamma}-E_{rec})/E_{#gamma}",80,0,20, 200,-1,1) );
    gHistList->Add( new TH2D("hCALTop_Eres","Top CAL E resolution;E_{e-MC} (GeV);(E_{e-MC}-E_{det})/E_{e-MC}",80,0,20, 200,-1,1) );
    gHistList->Add( new TH2D("hCALBot_Eres","Bot CAL E resolution;E_{e+MC} (GeV);(E_{e+MC}-E_{det})/E_{e+MC}",80,0,20, 200,-1,1) );

    gHistList->Add( new TH1D("hEraw",  "hit energy (raw)", 200, 0, 20) );
    gHistList->Add( new TH1D("hErawTotal",  "summed hit energy (raw)", 200, 0, 20) );
    gHistList->Add( new TH1D("hErawTotalTop",  "Top Det summed hit energy (raw)", 200, 0, 20) );
    gHistList->Add( new TH1D("hErawTotalBot",  "Bot Det summed hit energy (raw)", 200, 0, 20) );
    gHistList->Add( new TH1D("hEup", "Upper CAL. Energy; Rec. Energy (GeV); Events",  200, 0,20) );
    gHistList->Add( new TH1D("hEdw", "Lower CAL. Energy; Rec. Energy (GeV); Events",  200, 0,20) );
    gHistList->Add( new TH1D("hEnergy", "CAL. Energy; Rec. Energy (GeV); Events",  200, 0,20) );
    gHistList->Add( new TH2D("hCALCluster_Eres", "E_{#gamma gen} vs (E_{top}+E_{bot})_{rec}; E_{#gamma gen} (GeV); (E_{topRec}+E_{BotRec}) (GeV)", 80,0,20, 200,0,20) );

    gHistList->Add( new TH2D("hSampFracTop", " Top Det Sampling Fraction; E_{e-MC} (GeV); Sampling Fraction", 200, 0, 20, 1000, 0, 1) );
    gHistList->Add( new TH2D("hSampFracTopRaw", " Top Det Sampling Fraction (raw hits); E_{e-MC} (GeV); Sampling Fraction", 200, 0, 20, 1000, 0, 1) );
    gHistList->Add( new TH2D("hSampFracBot", " Bot Det Sampling Fraction; E_{e+MC} (GeV); Sampling Fraction", 200, 0, 20, 1000, 0, 1) );
    gHistList->Add( new TH2D("hSampFracBotRaw", " Bot Det Sampling Fraction (raw hits); E_{e+MC} (GeV); Sampling Fraction", 200, 0, 20, 1000, 0, 1) );

    gHistList->Add( new TH2D("hErawTotal_EMC_Top", "Top Det Summed Energy vs E_{e-MC}; E_{e-MC} (GeV); E_{#Sigma hits} (GeV)", 200, 0, 20, 200, 0, 20) );
    gHistList->Add( new TH2D("hErawTotal_EMC_Bot", "Bot Det Summed Energy vs E_{e+MC}; E_{e+MC} (GeV); E_{#Sigma hits} (GeV)", 200, 0, 20, 200, 0, 20) );
    gHistList->Add( new TH2D("hErawTotal_EMC_Coin", "Coin Det Summed Energy vs E_{#gamma MC}; E_{#gamma MC} (GeV); E_{#Sigma hits} (GeV)", 200, 0, 20, 200, 0, 20) );

    gHistList->Add( new TH2D("hCAL_ClusterFracE_Top", "Cal Cluster E Frac E_{MC}; E_{e-MC}; E_{Clus}/E_{#Sigma hits}", 200, 0, 20, 120, 0, 1.2));
    gHistList->Add( new TH2D("hCAL_ClusterFracE_Bot", "Cal Cluster E Frac E_{MC}; E_{e+MC}; E_{Clus}/E_{#Sigma hits}", 200, 0, 20, 120, 0, 1.2));

    gHistList->Add( new TH2D("hCALYE", "CAL EDep Y; E_{Det} (GeV); y (mm)", 200, 0, 20, 600, -300, 300));
    gHistList->Add( new TH3D("hCALYZE", "CAL YZ Energy Dep; y (mm); z (mm); E_{Det} (GeV)", 600, -300, 300, 500, -70000, -6000, 200, 0, 20) );

    ////////////////////////////////////////////////////////
    // root dir histograms 
    dir->cd();

    gHistList->Add( new TH1D("hGenPhoton_E", "Generated photon energy;E_{#gamma} (GeV);Nevents", 200,0,20));
    gHistList->Add( new TH2D("hGenPhoton_xy", "Generated photon vertex;x (mm);y (mm)", 600,-300,300, 600,-300,300));
    gHistList->Add( new TH1D("hGenElectron_E", "Generated electron energy;E_{#gamma} (GeV);Nevents", 200,0,20));
    gHistList->Add( new TH2D("hGenElectron_xy", "Generated electron vertex;x (mm);y (mm)", 600,-300,300, 600,-300,300));
    gHistList->Add( new TH1D("hGenPositron_E", "Generated positron energy;E_{#gamma} (GeV);Nevents", 200,0,20));
    gHistList->Add( new TH2D("hGenPositron_xy", "Generated positron vertex;x (mm);y (mm)", 600,-300,300, 600,-300,300));
    gHistList->Add( new TH1D("hGenEventCount", "Number of generated events per Egen;E_{#gamma} (GeV);Nevents", 200, 0, 20) );

    gHistList->Add( new TH1D("hProtoClusterCount", "Number of proto island clusters / event", 20,-0.5,19.5) );
    gHistList->Add( new TH1D("hClusterCount", "Number of clusters / event;# clusters / event", 20,-0.5,19.5) );
    gHistList->Add( new TH1D("hADCsignal", "ADC signal", 16385,-0.5,16384.5) );

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
