// Macro for visualzing geometry using Eve
// Sham Kumar; shyam055119@gmail.com; shyam.kumar@ba.infn.it

void draw_geom()
{
	
  TString rootfile="detector_geometry.root"; // put geometry name
  TEveManager::Create();
  gGeoManager = TGeoManager::Import(rootfile); // or use TGeoManager::Import(rootfile)
  if (gGeoManager == nullptr) return;
  TEveGeoTopNode *EPIC = new TEveGeoTopNode(gGeoManager,gGeoManager->GetTopNode()); // pass node here instead of topnode
  gEve->AddGlobalElement(EPIC);
  gEve->FullRedraw3D(kTRUE);

}


