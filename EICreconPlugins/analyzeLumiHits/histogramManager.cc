#ifndef HISTOGRAMMANAGER_CC
#define HISTOGRAMMANAGER_CC

#include "histogramManager.h"

namespace histogramManager {

  THashList *gHistList = new THashList();
  
  // TTree *treeCAL_Hits = new TTree("treeCAL_Hits","Hits");
  // TTree *treeCAL_RecHits = new TTree("treeCAL_RecHits","RecHits");
  // TTree *treeCAL_Clusters = new TTree("treeCAL_Clusters","Clusters");
  TTree *treeTracker_Hits = new TTree("treeTracker_Hits","Hits");
  TTree *treeTracksTop = new TTree("treeTracksTop","Tracks");
  TTree *treeTracksBot = new TTree("treeTracksBot","Tracks");
  TTree *treePhotons = new TTree("treePhotons","Tracks");
  TTree *treeGenPhotons = new TTree("treeGenPhotons","Particles");

  TF1 *posres_pol1 = new TF1("posres_pol1","[0]+[1]*x",-64200,-64000);

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
    dir->mkdir("LumiSpecCAL_General")->cd();

    gHistList->Add( new TH1D("h1Eraw",  "Cal hit energy (raw)", 100, 0, 0.01) );
    gHistList->Add( new TH1D("h1ErawTotal",  "Summed hit energy (raw)", 1000, 0, 1) );
    gHistList->Add( new TH1D("h1ErawTotalTop",  "Top Det summed hit energy (raw)", 1000, 0, 1) );
    gHistList->Add( new TH1D("h1ErawTotalBot",  "Bot Det summed hit energy (raw)", 1000, 0, 1) );
    gHistList->Add( new TH1D("h1ErawTotalTop_Coin",  "Top Det summed hit energy (raw, Coin events only)", 1000, 0, 1) );
    gHistList->Add( new TH1D("h1ErawTotalBot_Coin",  "Bot Det summed hit energy (raw, Coin events only)", 1000, 0, 1) );
    gHistList->Add( new TH1D("h1ErawTotalCoin",  "Coin Det summed hit energy (raw)", 1000, 0, 1) );
    gHistList->Add( new TH1D("h1nHitsTop", "Top Det nHits", 100, 0, 100) );
    gHistList->Add( new TH1D("h1nHitsBot", "Bot Det nHits", 100, 0, 100) );

    // Hit position info histograms
    dir->mkdir("CAL_Position_Info")->cd();
    gHistList->Add( new TH1D("h1XTop",  "X Top Det;x(mm)", 200, -100, 100) );
    gHistList->Add( new TH1D("h1XBot",  "X Bot Det;x(mm)", 200, -100, 100) );
    gHistList->Add( new TH1D("h1YTop",  "Y Top Det;y(mm)", 250, 50, 300) );
    gHistList->Add( new TH1D("h1YBot",  "Y Bot Det;y(mm)", 250, -300, -50) );
    gHistList->Add( new TH2D("h2Zr",  "Zr Det;z(mm);r(mm)", 180, -64180, -64000, 250, 0, 250) );
    gHistList->Add( new TH2D("h2ZE",  "ZE Det;z(mm);E", 180, -64180, -64000, 100, 0, 0.01) );    
    gHistList->Add( new TH2D("h2ZETop",  "ZE Top Det;z(mm);E", 180, -64180, -64000, 100, 0, 0.01) );
    gHistList->Add( new TH2D("h2ZEBot",  "ZE Bot Det;z(mm);E", 180, -64180, -64000, 100, 0, 0.01) );
    gHistList->Add( new TH2D("h2XYETop",  "XYE Top Det;x(mm);y(mm)", 200, -100, 100, 250, 50, 300) );
    gHistList->Add( new TH2D("h2XYEBot",  "XYE Bot Det;x(mm);y(mm)", 200, -100, 100, 250, -300, -50) );    

    gHistList->Add( new TH2D("h2ZXETop_1mm",  "ZXE Top Det (1mm);z(mm);x(mm)", 180, -64180, -64000, 270, -135, 135 ));
    gHistList->Add( new TH2D("h2ZYETop_1mm",  "ZYE Top Det (1mm);z(mm);Y(mm)", 180, -64180, -64000, 360, 0, 360 ));
    gHistList->Add( new TH2D("h2ZXETop_3mm",  "ZXE Top Det (3mm);z(mm);x(mm)", 60, -64180, -64000, 90, -135, 135 ));
    gHistList->Add( new TH2D("h2ZYETop_3mm",  "ZYE Top Det (3mm);z(mm);Y(mm)", 60, -64180, -64000, 120, 0, 360 ));
    gHistList->Add( new TH2D("h2ZXETop_9mm",  "ZXE Top Det (9mm);z(mm);x(mm)", 20, -64180, -64000, 30, -135, 135 ));
    gHistList->Add( new TH2D("h2ZYETop_9mm",  "ZYE Top Det (9mm);z(mm);Y(mm)", 20, -64180, -64000, 40, 0, 360 ));

    gHistList->Add( new TH2D("h2ZXEBot_1mm",  "ZXE Bot Det (1mm);z(mm);x(mm)", 180, -64180, -64000, 270, -135, 135 ));
    gHistList->Add( new TH2D("h2ZYEBot_1mm",  "ZYE Bot Det (1mm);z(mm);Y(mm)", 180, -64180, -64000, 360, -360, 0 ));
    gHistList->Add( new TH2D("h2ZXEBot_3mm",  "ZXE Bot Det (3mm);z(mm);x(mm)", 60, -64180, -64000, 90, -135, 135 ));
    gHistList->Add( new TH2D("h2ZYEBot_3mm",  "ZYE Bot Det (3mm);z(mm);Y(mm)", 60, -64180, -64000, 120, -360, 0 ));
    gHistList->Add( new TH2D("h2ZXEBot_9mm",  "ZXE Bot Det (9mm);z(mm);x(mm)", 20, -64180, -64000, 30, -135, 135 ));
    gHistList->Add( new TH2D("h2ZYEBot_9mm",  "ZYE Bot Det (9mm);z(mm);Y(mm)", 20, -64180, -64000, 40, -360, 0 ));

    // Plots for Moliere radius
    gHistList->Add( new TH2D("h2ZdXTop",  "Z#DeltaX (X_{hit} - X_{mc}) Top Det;Z(mm);#DeltaX(mm)", 180, -64180, -64000, 800, -100, 100 ));
    gHistList->Add( new TH2D("h2ZdXBot",  "Z#DeltaX (X_{hit} - X_{mc}) Bot Det;Z(mm);#DeltaX(mm)", 180, -64180, -64000, 800, -100, 100 ));
    gHistList->Add( new TH2D("h2ZdYTop",  "Z#DeltaY (Y_{hit} - Y_{mc}) Top Det;Z(mm);#DeltaY(mm)", 180, -64180, -64000, 1200, 0, 300 ));
    gHistList->Add( new TH2D("h2ZdYBot",  "Z#DeltaY (Y_{hit} - Y_{mc}) Bot Det;Z(mm);#DeltaY(mm)", 180, -64180, -64000, 1200, -300, 0 ));

    gHistList->Add( new TH2D("h2EdXTop_1mm",  "E#DeltaX (X_{hit} - X_{mc}) Top Det (1mm pixel);E_{e^{+}Rec};#DeltaX(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdXBot_1mm",  "E#DeltaX (X_{hit} - X_{mc}) Bot Det (1mm pixel);E_{e^{-}Rec};#DeltaX(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdXTop_3mm",  "E#DeltaX (X_{hit} - X_{mc}) Top Det (3mm pixel);E_{e^{+}Rec};#DeltaX(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdXBot_3mm",  "E#DeltaX (X_{hit} - X_{mc}) Bot Det (3mm pixel);E_{e^{-}Rec};#DeltaX(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdXTop_9mm",  "E#DeltaX (X_{hit} - X_{mc}) Top Det (9mm pixel);E_{e^{+}Rec};#DeltaX(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdXBot_9mm",  "E#DeltaX (X_{hit} - X_{mc}) Bot Det (9mm pixel);E_{e^{-}Rec};#DeltaX(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdXTop_1mm",  "E_{MC}#DeltaX (X_{hit} - X_{mc}) Top Det (1mm pixel);E_{e^{+}MC};#DeltaX(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdXBot_1mm",  "E_{MC}#DeltaX (X_{hit} - X_{mc}) Bot Det (1mm pixel);E_{e^{-}MC};#DeltaX(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdXTop_3mm",  "E_{MC}#DeltaX (X_{hit} - X_{mc}) Top Det (3mm pixel);E_{e^{+}MC} ;#DeltaX(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdXBot_3mm",  "E_{MC}#DeltaX (X_{hit} - X_{mc}) Bot Det (3mm pixel);E_{e^{-}MC} ;#DeltaX(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdXTop_9mm",  "E_{MC}#DeltaX (X_{hit} - X_{mc}) Top Det (9mm pixel);E_{e^{+}MC};#DeltaX(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdXBot_9mm",  "E_{MC}#DeltaX (X_{hit} - X_{mc}) Bot Det (9mm pixel);E_{e^{-}MC} ;#DeltaX(mm)", 200, 0, 20, 320, -40, 40 ));
    
    gHistList->Add( new TH2D("h2EdYTop_1mm",  "E#DeltaY (Y_{hit} - Y_{mc}) Top Det (1mm pixel);E_{e^{+}Rec};#DeltaY(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdYBot_1mm",  "E#DeltaY (Y_{hit} - Y_{mc}) Bot Det (1mm pixel);E_{e^{-}Rec};#DeltaY(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdYTop_3mm",  "E#DeltaY (Y_{hit} - Y_{mc}) Top Det (3mm pixel);E_{e^{+}Rec};#DeltaY(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdYBot_3mm",  "E#DeltaY (Y_{hit} - Y_{mc}) Bot Det (3mm pixel);E_{e^{-}Rec};#DeltaY(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdYTop_9mm",  "E#DeltaY (Y_{hit} - Y_{mc}) Top Det (9mm pixel);E_{e^{+}Rec};#DeltaY(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EdYBot_9mm",  "E#DeltaY (Y_{hit} - Y_{mc}) Bot Det (9mm pixel);E_{e^{-}Rec} ;#DeltaY(mm)", 100, 0, 1, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdYTop_1mm",  "E_{MC}#DeltaY (Y_{hit} - Y_{mc}) Top Det (1mm pixel);E_{e^{+}MC};#DeltaY(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdYBot_1mm",  "E_{MC}#DeltaY (Y_{hit} - Y_{mc}) Bot Det (1mm pixel);E_{e^{-}MC};#DeltaY(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdYTop_3mm",  "E_{MC}#DeltaY (Y_{hit} - Y_{mc}) Top Det (3mm pixel);E_{e^{+}MC};#DeltaY(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdYBot_3mm",  "E_{MC}#DeltaY (Y_{hit} - Y_{mc}) Bot Det (3mm pixel);E_{e^{-}MC};#DeltaY(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdYTop_9mm",  "E_{MC}#DeltaY (Y_{hit} - Y_{mc}) Top Det (9mm pixel);E_{e^{+}MC};#DeltaY(mm)", 200, 0, 20, 320, -40, 40 ));
    gHistList->Add( new TH2D("h2EMCdYBot_9mm",  "E_{MC}#DeltaY (Y_{hit} - Y_{mc}) Bot Det (9mm pixel);E_{e^{-}MC};#DeltaY(mm)", 200, 0, 20, 320, -40, 40 ));

    // Event level versions, reset at end of each event filling loop - Test incorporating here
    dir->mkdir("CAL_Position_Info_Event_Level")->cd();
    gHistList->Add( new TH2D("h2ZXETop_1mm_Event",  "ZXE Top Det (1mm);z(mm);X(mm)", 180, -64180, -64000, 270, -135, 135 ));
    gHistList->Add( new TH2D("h2ZXETop_3mm_Event",  "ZXE Top Det (3mm);z(mm);X(mm)", 60, -64180, -64000, 90, -135, 135 ));
    gHistList->Add( new TH2D("h2ZXETop_9mm_Event",  "ZXE Top Det (9mm);z(mm);X(mm)", 20, -64180, -64000, 30, -135, 135 ));
    gHistList->Add( new TH2D("h2ZXEBot_1mm_Event",  "ZXE Bot Det (1mm);z(mm);X(mm)", 180, -64180, -64000, 270, -135, 135 ));
    gHistList->Add( new TH2D("h2ZXEBot_3mm_Event",  "ZXE Bot Det (3mm);z(mm);X(mm)", 60, -64180, -64000, 90, -135, 135 ));
    gHistList->Add( new TH2D("h2ZXEBot_9mm_Event",  "ZXE Bot Det (9mm);z(mm);X(mm)", 20, -64180, -64000, 30, -135, 135 ));

    gHistList->Add( new TH2D("h2ZYETop_1mm_Event",  "ZYE Top Det (1mm);z(mm);Y(mm)", 180, -64180, -64000, 360, 0, 360 ));
    gHistList->Add( new TH2D("h2ZYETop_3mm_Event",  "ZYE Top Det (3mm);z(mm);Y(mm)", 60, -64180, -64000, 120, 0, 360 ));
    gHistList->Add( new TH2D("h2ZYETop_9mm_Event",  "ZYE Top Det (9mm);z(mm);Y(mm)", 20, -64180, -64000, 40, 0, 360 ));
    gHistList->Add( new TH2D("h2ZYEBot_1mm_Event",  "ZYE Bot Det (1mm);z(mm);Y(mm)", 180, -64180, -64000, 360, -360, 0 ));
    gHistList->Add( new TH2D("h2ZYEBot_3mm_Event",  "ZYE Bot Det (3mm);z(mm);Y(mm)", 60, -64180, -64000, 120, -360, 0 ));
    gHistList->Add( new TH2D("h2ZYEBot_9mm_Event",  "ZYE Bot Det (9mm);z(mm);Y(mm)", 20, -64180, -64000, 40, -360, 0 ));

    dir->mkdir("CAL_Acceptance_Info")->cd();
    gHistList->Add( new TH2D("h2SampFracTop", "Top Det Sampling Fraction; E_{e^{+}MC} (GeV); Sampling Fraction", 200, 0, 20, 100, 0, 0.1) );
    gHistList->Add( new TH2D("h2SampFracBot", "Bot Det Sampling Fraction; E_{e^{-}MC} (GeV); Sampling Fraction", 200, 0, 20, 100, 0, 0.1) );
    gHistList->Add( new TH2D("h2SampFracTop_Coin", "Top Det Sampling Fraction (Coin events only); E_{e^{+}MC} (GeV); Sampling Fraction", 200, 0, 20, 100, 0, 0.1) );
    gHistList->Add( new TH2D("h2SampFracBot_Coin", "Bot Det Sampling Fraction (Coin events only); E_{e^{-}MC} (GeV); Sampling Fraction", 200, 0, 20, 100, 0, 0.1) );
    gHistList->Add( new TH2D("h2SampFracCoin", "Coin Sampling Fraction; E_{#gammaMC} (GeV); Sampling Fraction", 200, 0, 20, 100, 0, 0.1) );

    gHistList->Add( new TH1D("h1CALTopAccept", "Top Det acceptance;E_{e^{+}MC} (GeV);Counts", 2000, 0, 20) );  
    gHistList->Add( new TH1D("h1CALBotAccept", "Bot Det acceptance;E_{e^{-}MC} (GeV);Counts", 2000, 0, 20) );
    gHistList->Add( new TH1D("h1CALCoinAccept", "Coin acceptance;E_{#gammaMC} (GeV);Counts", 2000, 0, 20) );
    gHistList->Add( new TH2D("h2CALTopAccept", "Top Det acceptance;E_{e^{+}MC} (GeV);E_{TopDet} (GeV)", 200, 0, 20, 100, 0, 1) );
    gHistList->Add( new TH2D("h2CALBotAccept", "Bot Det acceptance;E_{e^{-}MC} (GeV);E_{BotDet} (GeV)", 200, 0, 20, 100, 0, 1) );
    gHistList->Add( new TH2D("h2CALCoinAccept", "Coin acceptance;E_{#gammaMC} (GeV);E_{Coin} (GeV)", 200, 0, 20, 100, 0, 1) );

    dir->mkdir("CAL_Acceptance_Info_Position_Cuts")->cd();
    for(int i = 0; i < 5; i++){
      gHistList->Add( new TH1D(Form("h1CALTopAccept_%i", (i+1)), Form("Top Det acceptance %i cm < y < %i cm;E_{e^{+}MC} (GeV);Counts",(6+(i+1)), (24-(i+1))), 2000, 0, 20) );  
      gHistList->Add( new TH1D(Form("h1CALBotAccept_%i", (i+1)), Form("Bot Det acceptance %i cm < y < %i cm;E_{e^{-}MC} (GeV);Counts",(-24+(i+1)),(-6-(i+1))), 2000, 0, 20) );
      gHistList->Add( new TH1D(Form("h1CALCoinAccept_%i", (i+1)), Form("Coin acceptance %i cm < y_{Bot} < %i cm & %i cm < y_{Top} < %i cm;E_{#gammaMC} (GeV);Counts", (-24+(i+1)), (-6-(i+1)), (6+(i+1)), (24-(i+1))), 2000, 0, 20) );
      gHistList->Add( new TH2D(Form("h2SampFracTop_%i", (i+1)), Form("Top Det Sampling Fraction %i cm < y < %i cm; E_{e^{+}MC} (GeV); Sampling Fraction",(6+(i+1)), (24-(i+1))), 200, 0, 20, 100, 0, 0.1) );
      gHistList->Add( new TH2D(Form("h2SampFracBot_%i", (i+1)), Form("Bot Det Sampling Fraction  %i cm < y < %i cm; E_{e^{-}MC} (GeV); Sampling Fraction",(-24+(i+1)),(-6-(i+1))), 200, 0, 20, 100, 0, 0.1) );
      gHistList->Add( new TH2D(Form("h2SampFracCoin_%i", (i+1)), Form("Coin Sampling Fraction  %i cm < y_{Bot} < %i cm & %i cm < y_{Top} < %i cm; E_{#gammaMC} (GeV); Sampling Fraction", (-24+(i+1)), (-6-(i+1)), (6+(i+1)), (24-(i+1))), 200, 0, 20, 100, 0, 0.1) );  
    }

    ////////////////////////////////////////////////////////
    // root dir histograms 
    dir->cd();

    gHistList->Add( new TH1D("hGenPhoton_E", "Generated #gamma energy;E_{#gamma} (GeV);Nevents", 2000,0,20));
    gHistList->Add( new TH2D("hGenPhoton_xy", "Generated #gamma vertex;x (mm);y (mm)", 6000,-300,300, 6000,-300,300));
    gHistList->Add( new TH1D("hGenElectron_E", "Generated e^{-} energy;E_{e^{-}} (GeV);Nevents", 2000,0,20));
    gHistList->Add( new TH2D("hGenElectron_xy", "Generated e^{-} vertex;x (mm);y (mm)", 6000,-300,300, 6000,-300,300));
    gHistList->Add( new TH1D("hGenPositron_E", "Generated e^{+} energy;E_{e^{+}} (GeV);Nevents", 2000,0,20));
    gHistList->Add( new TH2D("hGenPositron_xy", "Generated photon vertex;x (mm);y (mm)", 6000,-300,300, 6000,-300,300));
    gHistList->Add( new TH1D("hGenEventCount", "Number of generated events per Egen;E_{#gamma} (GeV);Nevents", 2500, 0, 50) );

    gHistList->Add( new TH1D("hProtoClusterCount", "Number of proto island clusters / event", 20,-0.5,19.5) );
    gHistList->Add( new TH1D("hClusterCount", "Number of clusters / event;# clusters / event", 20,-0.5,19.5) );
    gHistList->Add( new TH1D("hADCsignal", "ADC signal", 16385,-0.5,16384.5) );

    gHistList->Add( new TH2D("hBxdotdL", "Bx*dz versus x and y;x (cm);y (cm)", 33,-8.25,8.25, 81,-20.25,20.25) );
    gHistList->Add( new TH2D("hBydotdL", "By*dz versus x and y;x (cm);y (cm)", 33,-8.25,8.25, 81,-20.25,20.25) );
    gHistList->Add( new TH2D("hBzdotdL", "Bz*dz versus x and y;x (cm);y (cm)", 33,-8.25,8.25, 81,-20.25,20.25) );

    // TTrees
    // treeCAL_Hits->SetDirectory( dir );
    // treeCAL_Hits->Branch("E", &variables::E_hit);
    // treeCAL_Hits->Branch("x", &variables::x_hit);
    // treeCAL_Hits->Branch("y", &variables::y_hit);
    // treeCAL_Hits->Branch("r", &variables::r_hit);
    // treeCAL_Hits->Branch("sec_id", &variables::sec_id);
    // treeCAL_Hits->Branch("mod_id", &variables::mod_id);
    // treeCAL_Hits->Branch("fiber_x_id", &variables::fiber_x_id);
    // treeCAL_Hits->Branch("fiber_y_id", &variables::fiber_y_id);

    // treeCAL_RecHits->SetDirectory( dir );
    // treeCAL_RecHits->Branch("E", &variables::E_hit);
    // treeCAL_RecHits->Branch("x", &variables::x_hit);
    // treeCAL_RecHits->Branch("y", &variables::y_hit);
    // treeCAL_RecHits->Branch("r", &variables::r_hit);
    // treeCAL_RecHits->Branch("t", &variables::t_hit);

    // treeCAL_Clusters->SetDirectory( dir );
    // treeCAL_Clusters->Branch("Nhits", &variables::Nhits_cluster);
    // treeCAL_Clusters->Branch("E", &variables::E_cluster);
    // treeCAL_Clusters->Branch("x", &variables::x_cluster);
    // treeCAL_Clusters->Branch("y", &variables::y_cluster);
    // treeCAL_Clusters->Branch("r", &variables::r_cluster);
    // treeCAL_Clusters->Branch("t", &variables::t_cluster);
    // treeCAL_Clusters->Branch("Radius", &variables::Radius_cluster);
    // treeCAL_Clusters->Branch("Dispersion", &variables::Dispersion_cluster);
    // treeCAL_Clusters->Branch("SigmaThetaPhi_short", &variables::SigmaThetaPhi2_cluster);
    // treeCAL_Clusters->Branch("SigmaThetaPhi_long", &variables::SigmaThetaPhi1_cluster);

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

// 16/01/24 - Archived

    // gHistList->Add( new TH1D("hCALTop_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 200, 0, 20) );
    // gHistList->Add( new TH1D("hCALBot_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 200, 0, 20) );
    // gHistList->Add( new TH1D("hCALCoincidence_Acceptance", "CAL acceptance;E_{#gamma} (GeV);Acceptance", 200, 0, 20) ); 
    // gHistList->Add( new TH2D("hCALCoincidence_Acceptance_v2", "CAL acceptance; (E_{e^{-}MC} + E_{e+MC}) (GeV); (E_{TopDet} + E_{BotDet}) (GeV)", 200, 0, 20, 200, 0, 20) ); 
    // gHistList->Add( new TH1D("hCALCoincidence_Acceptance_v3_Numer", "CAL Acceptance; E_{#gamma MC} (GeV); Counts", 200, 0, 20));
    // gHistList->Add( new TH1D("hCALCoincidence_Acceptance_v3_Denom", "CAL Acceptance; E_{#gamma MC} (GeV); Counts", 200, 0, 20));

    // gHistList->Add( new TH1D("hCALTop_Acceptance_v2", "CAL acceptance;E_{e-MC} (GeV);Acceptance", 200, 0, 20) );
    // gHistList->Add( new TH2D("hCALTop_Acceptance_v3", "CAL acceptance;E_{e-MC} (GeV);E_{TopDet} (GeV)", 200, 0, 20, 200, 0, 20) );
    // gHistList->Add( new TH1D("hCALTop_Acceptance_v4_Numer", "CAL Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));
    // gHistList->Add( new TH1D("hCALTop_Acceptance_v4_Denom", "CAL Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));
    // gHistList->Add( new TH1D("hCALBot_Acceptance_v2", "CAL acceptance;E_{e+MC} (GeV);Acceptance", 200, 0, 20) );
    // gHistList->Add( new TH2D("hCALBot_Acceptance_v3", "CAL acceptance;E_{e+MC} (GeV);E_{BotDet} (GeV)", 200, 0, 20, 200, 0, 20) );
    // gHistList->Add( new TH1D("hCALBot_Acceptance_v4_Numer", "CAL Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));
    // gHistList->Add( new TH1D("hCALBot_Acceptance_v4_Denom", "CAL Acceptance;E_{e-MC} (GeV); Counts", 200, 0, 20));

    // gHistList->Add( new TH2D("hCAL_Eres","CAL E resolution;E_{#gamma} (GeV);(E_{#gamma}-E_{rec})/E_{#gamma}",80,0,20, 200,-1,1) );
    // gHistList->Add( new TH2D("hCALTop_Eres","Top CAL E resolution;E_{e-MC} (GeV);(E_{e-MC}-E_{det})/E_{e-MC}",80,0,20, 200,-1,1) );
    // gHistList->Add( new TH2D("hCALBot_Eres","Bot CAL E resolution;E_{e+MC} (GeV);(E_{e+MC}-E_{det})/E_{e+MC}",80,0,20, 200,-1,1) );

    // gHistList->Add( new TH1D("hEraw",  "hit energy (raw)", 100, 0, 0.01) );
    // gHistList->Add( new TH1D("hErawTotal",  "summed hit energy (raw)", 1000, 0, 1) );
    // gHistList->Add( new TH1D("hErawTotalTop",  "Top Det summed hit energy (raw)", 1000, 0, 1) );
    // gHistList->Add( new TH1D("hErawTotalBot",  "Bot Det summed hit energy (raw)", 1000, 0, 1) );
    // gHistList->Add( new TH1D("hEup", "Upper CAL. Energy; Rec. Energy (GeV); Events",  1000, 0,1) );
    // gHistList->Add( new TH1D("hEdw", "Lower CAL. Energy; Rec. Energy (GeV); Events",  1000, 0,1) );
    // gHistList->Add( new TH1D("hEnergy", "CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50) );
    // gHistList->Add( new TH2D("hCALCluster_Eres", "Egen vs Cluster based photon Erec", 200,0,50, 2500,0,50) );

    // gHistList->Add( new TH1D("hnHitsTop", "Top Det nHits", 100, 0, 100) );
    // gHistList->Add( new TH1D("hnHitsBot", "Bot Det nHits", 100, 0, 100) );

    // gHistList->Add( new TH2D("hSampFracTop", " Top Det Sampling Fraction; E_{e-MC} (GeV); Sampling Fraction", 200, 0, 20, 1000, 0, 1) );
    // gHistList->Add( new TH2D("hSampFracTopRaw", " Top Det Sampling Fraction (raw hits); E_{e-MC} (GeV); Sampling Fraction", 200, 0, 20, 1000, 0, 1) );
    // gHistList->Add( new TH2D("hSampFracBot", " Bot Det Sampling Fraction; E_{e+MC} (GeV); Sampling Fraction", 200, 0, 20, 1000, 0, 1) );
    // gHistList->Add( new TH2D("hSampFracBotRaw", " Bot Det Sampling Fraction (raw hits); E_{e+MC} (GeV); Sampling Fraction", 200, 0, 20, 1000, 0, 1) );

    // gHistList->Add( new TH2D("hErawTotal_EMC_Top", "Top Det Summed Energy vs E_{e-MC}; E_{e-MC} (GeV); E_{#Sigma hits} (GeV)", 200, 0, 20, 100, 0, 1) );
    // gHistList->Add( new TH2D("hErawTotal_EMC_Bot", "Bot Det Summed Energy vs E_{e+MC}; E_{e+MC} (GeV); E_{#Sigma hits} (GeV)", 200, 0, 20, 100, 0, 1) );
    // gHistList->Add( new TH2D("hErawTotal_EMC_Coin", "Coin Det Summed Energy vs E_{#gamma MC}; E_{#gamma MC} (GeV); E_{#Sigma hits} (GeV)", 200, 0, 20, 100, 0, 1) );

    // gHistList->Add( new TH2D("hCALYE", "CAL EDep Y; E_{Det} (GeV); y (mm)", 200, 0, 20, 600, -300, 300));
    // gHistList->Add( new TH3D("hCALYZE", "CAL YZ Energy Dep; y (mm); z (mm); E_{Det} (GeV)", 600, -300, 300, 200, -70000, -50000, 200, 0, 20) );
