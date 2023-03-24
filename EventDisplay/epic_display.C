//==============================================================================
// Global variables.
//------------------------------------------------------------------------------
// run the macro: 
//root [0] .L epic_display.C++
//root [1] epic_display()
// Macro for  event display; 
// Shyam Kumar; INFN,Bari, Italy. shyam055119@gmail.com; shyam.kumar@cern.ch

#include "TEveTrack.h"
#include "TEveTrackPropagator.h"
#include "TEveElement.h"
#include "TEveGeoShape.h" 
#include "TGeoTube.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TROOT.h" 
#include "TSystem.h"
#include "TFile.h"
#include "TEveManager.h"
#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TEveGeoNode.h"
#include "TEveText.h"
#ifndef __RUN_EPIC_DISPLAY__


TString geoFile = std::string(std::getenv("HOME")) + "/eic/epic/detector_geometry.root";
TString simFile = std::string(std::getenv("HOME")) + "/eic/Analysis_epic/simulations/Output.edm4hep.root";

//==============================================================================
// Forward decalarations of CINT functions.
//------------------------------------------------------------------------------	
void epic_load_event();
void epic_make_gui();
void SetViewers(TEveViewer *RPhi, TEveViewer *RhoZ, TEveScene *fRPhiScene, TEveScene *fRhoZScene);

template <typename T>
TEvePointSet* AddHits( TTreeReaderArray<T> *xhit, TTreeReaderArray<T> *yhit, TTreeReaderArray<T> *zhit, TEvePointSet *hits, TString name = "")
{
  // Add Hits to vertex
  Int_t nhits = xhit->GetSize();
  hits = new TEvePointSet(nhits);
  
  for (Int_t hitno=0; hitno<nhits; ++hitno) { // Reading hits
    hits->SetPoint( hitno, double(xhit->At(hitno)*0.1), double(yhit->At(hitno)*0.1), double(zhit->At(hitno)*0.1) );
  }

  hits->SetName(Form("Hits_%s",name.Data()));
  hits->SetMarkerColor(kGreen);
  hits->SetMarkerStyle(47);
  hits->SetMarkerSize(1.4);	
  return hits;
}


void run_epic_display();

// Implemented in MultiView.C
class MultiView;
MultiView* gMultiView = 0;
TEveTrack  *track; TEveRecTrackD *rc;  TEvePointSetArray *hits=0; 
TEveTrackList *gTrackList = 0; 
Int_t iEvent = 0;

//==============================================================================
// Constants.
//------------------------------------------------------------------------------
// Reading Tree
TFile* file = new TFile(simFile.Data()); // Tree with tracks and hits
TTreeReader myReader("events", file); // name of tree and file

TTreeReaderArray<Float_t> charge(myReader, "MCParticles.charge"); 
TTreeReaderArray<Double_t> vx_mc(myReader, "MCParticles.vertex.x"); 
TTreeReaderArray<Double_t> vy_mc(myReader, "MCParticles.vertex.y"); 
TTreeReaderArray<Double_t> vz_mc(myReader, "MCParticles.vertex.z"); 
TTreeReaderArray<Float_t> px_mc(myReader, "MCParticles.momentum.x"); 
TTreeReaderArray<Float_t> py_mc(myReader, "MCParticles.momentum.y"); 
TTreeReaderArray<Float_t> pz_mc(myReader, "MCParticles.momentum.z"); 
TTreeReaderArray<Int_t> status(myReader, "MCParticles.generatorStatus"); 
TTreeReaderArray<Int_t> pdg(myReader, "MCParticles.PDG"); 

TTreeReaderArray<Double_t> *lumiTracker_x, *lumiTracker_y, *lumiTracker_z;
TTreeReaderArray<Float_t> *lumiCAL_x, *lumiCAL_y, *lumiCAL_z;

void epic_display()
{

  // Hits on detectors
  // Lumi Tracker Hits  
  lumiTracker_x = new TTreeReaderArray<Double_t>(myReader, "LumiSpecTrackerHits.position.x"); 
  lumiTracker_y = new TTreeReaderArray<Double_t>(myReader, "LumiSpecTrackerHits.position.y"); 
  lumiTracker_z = new TTreeReaderArray<Double_t>(myReader, "LumiSpecTrackerHits.position.z"); 

  // Lumi spectrometer CAL Hits  
  lumiCAL_x = new TTreeReaderArray<Float_t>(myReader, "LumiSpecCALHits.position.x"); 
  lumiCAL_y = new TTreeReaderArray<Float_t>(myReader, "LumiSpecCALHits.position.y"); 
  lumiCAL_z = new TTreeReaderArray<Float_t>(myReader, "LumiSpecCALHits.position.z"); 

  gROOT->LoadMacro("MultiView.C+");
  run_epic_display();

}		

//==============================================================================
// Main - pythia_display()
//------------------------------------------------------------------------------

void run_epic_display()
{
  //========================================================================
  // Create views and containers.
  //========================================================================
  Bool_t enable_rectrack_draw = kFALSE;
  Bool_t enable_hit_draw = kTRUE;
  Bool_t drawname = kFALSE;

  TEveManager::Create();
  gGeoManager = TGeoManager::Import( geoFile ); // or use TGeoManager::Import(rootfile)
  if (gGeoManager == nullptr) return;


  // Set Color and Transparency of Detectors
  TObjArray* allvolumes = gGeoManager->GetListOfVolumes();
  TGeoVolume *vol;
  TIter next(allvolumes);
  while ((vol=(TGeoVolume*)next())) {
    
    TString volname = vol->GetIconName();

    vol->SetTransparency(80);
  }
    

  // TopNode has many nodes access this way
  TObjArray* allnodes = gGeoManager->GetTopNode()->GetNodes(); 
  const int nNodes = allnodes->GetEntries();
  TEveGeoTopNode *EPIC_Tracker = new TEveGeoTopNode(gGeoManager,gGeoManager->GetTopNode());

  // Add specific Node to event display
  /* int startnode = 3; int endnode = 10;

     for(Int_t i=startnode; i<endnode;i++){
     TGeoNode* node= (TGeoNode*)allnodes->At(i);
     TString volname = node->GetVolume()->GetName();
     TEveGeoTopNode *evenode = new TEveGeoTopNode(gGeoManager,node);
     gEve->AddGlobalElement(evenode);  
     }*/
  gEve->AddGlobalElement(EPIC_Tracker);  
  gEve->FullRedraw3D(kTRUE);

  gEve->GetBrowser()->GetTabRight()->SetTab(1);

  TEveProjectionManager *fRPhiProjManager = new TEveProjectionManager(TEveProjection::kPT_RPhi);
  TEveProjectionManager *fRhoZProjManager = new TEveProjectionManager(TEveProjection::kPT_RhoZ);
  gEve->AddToListTree(fRPhiProjManager, kFALSE);
  gEve->AddToListTree(fRhoZProjManager, kFALSE);
  TEveProjectionAxes *fAxesPhi = new TEveProjectionAxes(fRPhiProjManager);
  TEveProjectionAxes *fAxesRho = new TEveProjectionAxes(fRhoZProjManager);

  TEveWindowSlot *RPhiSlot = TEveWindow::CreateWindowInTab(gEve->GetBrowser()->GetTabRight());
  TEveWindowPack *RPhiPack = RPhiSlot->MakePack();
  RPhiPack->SetElementName("RPhi View");
  RPhiPack->SetShowTitleBar(kFALSE);
  RPhiPack->NewSlot()->MakeCurrent();
  TEveViewer *fRPhiView = gEve->SpawnNewViewer("RPhi View", "");
  TEveScene *fRPhiScene = gEve->SpawnNewScene("RPhi", "Scene holding axis.");
  fRPhiScene->AddElement(fAxesPhi);

  TEveWindowSlot *RhoZSlot = TEveWindow::CreateWindowInTab(gEve->GetBrowser()->GetTabRight());
  TEveWindowPack *RhoZPack = RhoZSlot->MakePack();
  RhoZPack->SetElementName("RhoZ View");
  RhoZPack->SetShowTitleBar(kFALSE);
  RhoZPack->NewSlot()->MakeCurrent();
  TEveViewer *fRhoZView = gEve->SpawnNewViewer("RhoZ View", "");
  TEveScene *fRhoZScene = gEve->SpawnNewScene("RhoZ", "Scene holding axis.");
  fRhoZScene->AddElement(fAxesRho);

  SetViewers(fRPhiView, fRhoZView,fRPhiScene,fRhoZScene);


  gTrackList = new TEveTrackList("MCTracks");
  gTrackList->SetMainColor(kMagenta);
  gTrackList->SetLineColor(kMagenta);
  gTrackList->SetMarkerColor(kMagenta);
  gTrackList->SetMarkerStyle(4);
  gTrackList->SetMarkerSize(0.5);
  gEve->AddElement(gTrackList);
  //-----Array of Hits----------
  for (int ilayer=0; ilayer<7; ilayer++){
    hits = new TEvePointSetArray(Form("EPICHits%d",ilayer)); 
    hits->SetMarkerColor(kRed);
    hits->SetMarkerStyle(47);
    hits->SetMarkerSize(1.4);	
    gEve->AddElement(hits); 
  }

  TEveTrackPropagator* trkProp = gTrackList->GetPropagator();
  trkProp->SetFitDaughters(kFALSE);
  trkProp->SetMaxZ(300);
  trkProp->SetMaxR(300);
  trkProp->SetMagField(0.,0.,1.7); // 3.0 Tesla in Z-direction
  trkProp->SetMaxStep(25.0);

  //========================================================================
  //========================================================================

  epic_make_gui();
  epic_load_event();

}


//==============================================================================
// Next event
//------------------------------------------------------------------------------

void epic_load_event()
{
  cout<<"Event No. "<<iEvent<<endl;
  myReader.SetEntry(iEvent);
  gTrackList->DestroyElements();
  hits->DestroyElements();

  Int_t propagator = 1; // 0 default, 1 RungeKutta, 2 kHelix
  TEveTrackPropagator *trkProp = gTrackList->GetPropagator(); //default propogator
  if (propagator==1)
  {
    trkProp->SetStepper(TEveTrackPropagator::kRungeKutta);
    gTrackList->SetName("RK Propagator");
  }
  else if (propagator==2)
  {
    trkProp->SetStepper(TEveTrackPropagator::kHelix);
    gTrackList->SetName("Heix Propagator");
  }

  bool flag = true;
  for (int iParticle = 0; iParticle < charge.GetSize(); ++iParticle) 
  {
    if ((status[iParticle]!=1)) continue;
    rc = new TEveRecTrackD(); // Reconstructed Track
    rc->fV.Set(vx_mc[iParticle],vy_mc[iParticle],vz_mc[iParticle]);
    rc->fP.Set(px_mc[iParticle],py_mc[iParticle],pz_mc[iParticle]);
    rc->fSign = charge[iParticle];

    Double_t prec = sqrt(px_mc[iParticle]*px_mc[iParticle]+py_mc[iParticle]*py_mc[iParticle]+pz_mc[iParticle]*pz_mc[iParticle]);
    Double_t etarec = -1.0*TMath::Log(TMath::Tan((TMath::ACos(pz_mc[iParticle]/prec))/2));
    // if (etarec<3.0 || etarec>3.5) continue; 
    // if (prec>1.0) flag = true;
    track = new TEveTrack(rc, trkProp);
    track->SetMarkerStyle(4);
    track->SetLineWidth(2);
    track->SetLineColor(kMagenta);
    track->SetName(Form("Track_No%d",iParticle));
    track->SetElementTitle(Form("Event =%d \n" "TrackNo.=%d,"
          "P  = (%.3f, %.3f, %.3f)\n" "P  = %.3f GeV/c \n" "Eta =%.3f \n" "Pt =%.3f \n"
          ,iEvent,iParticle, px_mc[iParticle],py_mc[iParticle],pz_mc[iParticle],prec,etarec,sqrt(px_mc[iParticle]*px_mc[iParticle]+py_mc[iParticle]*py_mc[iParticle])));
    gTrackList->AddElement(track);
  }
  gTrackList->MakeTracks();
  gTrackList->SetLineColor(kMagenta);
  TEvePointSet *hits_EPIC[2];
  if (flag){
    hits->AddElement( AddHits(lumiTracker_x, lumiTracker_y, lumiTracker_z, hits_EPIC[0],"Lumi Tracker Hits") );
    hits->AddElement( AddHits(lumiCAL_x, lumiCAL_y, lumiCAL_z, hits_EPIC[1],"Lumi CAL Hits") );
  }

  /////////////////////////////////////////////////////
  // Viewer settings

  TGLViewer  *viewer = gEve->GetDefaultGLViewer();
  viewer->ResetCameras();
  viewer->GetClipSet()->SetClipType(TGLClip::kClipNone); // Other options are kClipNone = 0, kClipPlane, kClipBox  

  double zoom = 0.2; // smaller values for more zoom
  double truck_x = -4770.0; // translation distance
  double hRotate = -0.4; // rad
  double vRotate = -0.6; // rad
  Double_t camera_center[3] ={0., 0., -6500.};

  viewer->SetPerspectiveCamera( TGLViewer::kCameraPerspXOY, 1, 0, nullptr, 0, 0);
  viewer->CurrentCamera().Configure( zoom, 0, camera_center, hRotate, vRotate );
  viewer->CurrentCamera().Truck( truck_x, 0);
  
  // Draw axes for reference
  //viewer->SetGuideState( TGLUtil::kAxesOrigin, kTRUE, kFALSE, 0 );
  
  // Draw camera center
  //viewer->SetDrawCameraCenter( true );
  
  viewer->DoDraw();

  TEveElement* top = static_cast<TEveElement*>(gEve->GetEventScene());
  gEve->Redraw3D();
}


//==============================================================================
// GUI stuff
//------------------------------------------------------------------------------
class EventHandler
{
  public:
    void Fwd()
    {
      ++iEvent;
      epic_load_event();
    }
    void Bck()
    {
      if(iEvent>0) {
        --iEvent;
        epic_load_event();
      }
    }
};

//______________________________________________________________________________
void epic_make_gui()
{
  // Create minimal GUI for event navigation.

  TEveBrowser* browser = gEve->GetBrowser();
  browser->StartEmbedding(TRootBrowser::kLeft);

  TGMainFrame* frmMain = new TGMainFrame(gClient->GetRoot(), 1000, 600);
  frmMain->SetWindowName("XX GUI");
  frmMain->SetCleanup(kDeepCleanup);

  TGHorizontalFrame* hf = new TGHorizontalFrame(frmMain);
  {

    TString icondir = "./";
    TGPictureButton* b = 0; 


    EventHandler *fh = new EventHandler;

    b = new TGPictureButton(hf, gClient->GetPicture(icondir+"GoBack.gif"));
    b->SetToolTipText("Go to previous event - not supported.");
    hf->AddFrame(b);
    b->Connect("Clicked()", "EventHandler", fh, "Bck()");

    b = new TGPictureButton(hf, gClient->GetPicture(icondir+"GoForward.gif"));
    b->SetToolTipText("Generate new event.");
    hf->AddFrame(b);
    b->Connect("Clicked()", "EventHandler", fh, "Fwd()");                                

  }
  frmMain->AddFrame(hf);

  frmMain->MapSubwindows();
  frmMain->Resize();
  frmMain->MapWindow();

  browser->StopEmbedding();
  browser->SetTabTitle("Event Control", 0);
}


void SetViewers(TEveViewer *RPhi, TEveViewer *RhoZ, TEveScene *fRPhiScene, TEveScene *fRhoZScene)
{

  RPhi->GetGLViewer()->SetCurrentCamera(TGLViewer::kCameraOrthoXOY);
  // set clip plane and camera parameters
  // RPhi->GetGLViewer()->GetClipSet()->SetClipType(TGLClip::kClipPlane);
  // RPhi->GetGLViewer()->GetClipSet()->SetClipState(TGLClip::kClipPlane, fRPhiPlane);
  RPhi->GetGLViewer()->GetCameraOverlay()->SetOrthographicMode(TGLCameraOverlay::kAxis);
  RPhi->GetGLViewer()->GetCameraOverlay()->SetShowOrthographic(kTRUE);
  // switch off left, right, top and bottom light sources
  RPhi->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightLeft, false);
  RPhi->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightRight, false);
  RPhi->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightTop, false);
  RPhi->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightBottom, false);

  RhoZ->GetGLViewer()->SetCurrentCamera(TGLViewer::kCameraOrthoZOY);
  // set clip plane and camera parameters
  // RhoZ->GetGLViewer()->GetClipSet()->SetClipType(TGLClip::kClipPlane);
  // RhoZ->GetGLViewer()->GetClipSet()->SetClipState(TGLClip::kClipPlane, fRhoZPlane);
  RhoZ->GetGLViewer()->GetCameraOverlay()->SetOrthographicMode(TGLCameraOverlay::kAxis);
  RhoZ->GetGLViewer()->GetCameraOverlay()->SetShowOrthographic(kTRUE);
  // switch off left, right and front light sources
  RhoZ->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightLeft, false);
  RhoZ->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightRight, false);
  RhoZ->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightFront, false);

  RPhi->AddScene(fRPhiScene);
  RPhi->AddScene(gEve->GetGlobalScene());   
  RPhi->AddScene(gEve->GetEventScene());
  RhoZ->AddScene(fRhoZScene);
  RhoZ->AddScene(gEve->GetGlobalScene());
  RhoZ->AddScene(gEve->GetEventScene());

}
#endif

