
//
// Template for this file generated with eicmkplugin.py
//

#include "SimpleAcceptanceProcessor.h"
#include <services/rootfile/RootFile_service.h>


// The following just makes this a JANA plugin
extern "C" {
    void InitPlugin(JApplication *app) {
        InitJANAPlugin(app);
        app->Add(new SimpleAcceptanceProcessor);
    }
}

//-------------------------------------------
// InitWithGlobalRootLock
//-------------------------------------------
void SimpleAcceptanceProcessor::InitWithGlobalRootLock(){
    auto rootfile_svc = GetApplication()->GetService<RootFile_service>();
    auto rootfile = rootfile_svc->GetHistFile();
    rootfile->mkdir("SimpleAcceptance")->cd();

    // Create histograms here. e.g.
    hAcceptance  = new TH1D("hAcceptance",  "Lumi Spectrometer Acceptance;E (GeV);A", 50,-0.5,49.5);
}

//-------------------------------------------
// ProcessSequential
//-------------------------------------------
void SimpleAcceptanceProcessor::ProcessSequential(const std::shared_ptr<const JEvent>& event) {

  auto app = GetApplication();
  m_geoSvc = app->template GetService<JDD4hep_service>();

  Egen = 0;
  app->SetDefaultParameter("SimpleAcceptance:Egen", Egen);
  
  double Etotal_top = 0;
  double Etotal_bot = 0;

  for( auto hit : CAL_hits() ) {
    double E_hit = hit->getEnergy();
    edm4hep::Vector3f vec = hit->getPosition();// mm
    double x_hit = vec.x;
    double y_hit = vec.y;

    if( y_hit > 0 ) {
      Etotal_top += E_hit;
    }
    if( y_hit < 0 ) {
      Etotal_bot += E_hit;
    }
  }

  if( Etotal_top > 1 && Etotal_bot > 1 ) {
    hAcceptance->Fill( Egen );
  }

}

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void SimpleAcceptanceProcessor::FinishWithGlobalRootLock() {

    // Do any final calculations here.
}

