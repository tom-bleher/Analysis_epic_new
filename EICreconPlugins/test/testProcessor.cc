
//
// Template for this file generated with eicmkplugin.py
//

#include "testProcessor.h"
#include <services/rootfile/RootFile_service.h>

#include "constants.h"
#include "tracker.h"

using namespace std;

// The following just makes this a JANA plugin
extern "C" {
    void InitPlugin(JApplication *app) {
        InitJANAPlugin(app);
        app->Add(new testProcessor);
    }
}

//-------------------------------------------
// InitWithGlobalRootLock
//-------------------------------------------
void testProcessor::InitWithGlobalRootLock(){
    auto rootfile_svc = GetApplication()->GetService<RootFile_service>();
    auto rootfile = rootfile_svc->GetHistFile();
    rootfile->mkdir("test")->cd();

 }

//-------------------------------------------
// ProcessSequential
//-------------------------------------------
void testProcessor::ProcessSequential(const std::shared_ptr<const JEvent>& event) {

   cout<<constants::BH_prefactor<<endl;

   TrackerAnalysis *tracker = new TrackerAnalysis();
}

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void testProcessor::FinishWithGlobalRootLock() {

  }

