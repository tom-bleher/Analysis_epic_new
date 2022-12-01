
//
// Template for this file generated with eicmkplugin.py
//

#include "testPluginProcessor.h"
#include <services/rootfile/RootFile_service.h>

     
// The following just makes this a JANA plugin
extern "C" {
    void InitPlugin(JApplication *app) {
        InitJANAPlugin(app);
        app->Add(new testPluginProcessor);
    }
}

//-------------------------------------------
// InitWithGlobalRootLock
//-------------------------------------------
void testPluginProcessor::InitWithGlobalRootLock(){
    auto rootfile_svc = GetApplication()->GetService<RootFile_service>();
    auto rootfile = rootfile_svc->GetHistFile();
    rootfile->mkdir("testPlugin")->cd();

    // Create histograms here. e.g.
    hXY = new TH2D("hXY", "Hits X vs Y;X (mm);Y (mm)", 200,-100,100, 200,-500,500);
}

//-------------------------------------------
// ProcessSequential
//-------------------------------------------
void testPluginProcessor::ProcessSequential(const std::shared_ptr<const JEvent>& event) {

    auto app = GetApplication();
    
    for( auto hit : rawhits()  ) {

      const auto id = hit->getCellID();
      
      cout<<"cell ID :"<<id<<endl;
      
      m_geoSvc = app->template GetService<JDD4hep_service>();
      
      auto id_dec = m_geoSvc->detector()->readout( "LumiSpecCALHits" ).idSpec().decoder();
      
      int sector_idx = id_dec->index( "sector" );
      int module_idx = id_dec->index( "module" );
      
      const int sec = (int) id_dec->get( id, sector_idx );
      const int mod = (int) id_dec->get( id, module_idx );

      cout<<"module: "<<mod<<"  sector: "<<sec<<endl;

      double X = hit->getPosition().x;
      double Y = hit->getPosition().y;
      cout<<"X :"<<X<<"   Y:"<<Y<<endl;

      hXY->Fill( X, Y );
    }
}

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void testPluginProcessor::FinishWithGlobalRootLock() {

    // Do any final calculations here.
}

