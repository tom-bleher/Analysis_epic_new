
//
// Template for this file generated with eicmkplugin.py
//
#include <algorithm>
#include <bitset>

#include <services/geometry/dd4hep/JDD4hep_service.h>
#include <spdlog/spdlog.h>
#include <JANA/JEventProcessorSequentialRoot.h>
#include <TH2D.h>
#include <TFile.h>

// Include appropirate class headers. e.g.
#include <edm4hep/SimTrackerHit.h>
#include <edm4hep/SimCalorimeterHit.h>

using namespace std;

class testPluginProcessor: public JEventProcessorSequentialRoot {
private:

    // Data objects we will need from JANA e.g.
    //PrefetchT<edm4hep::SimTrackerHit> rawhits   = {this, "LumiSpecTrackerHits"};
    PrefetchT<edm4hep::SimCalorimeterHit> rawhits   = {this, "LumiSpecCALHits"};

    // Declare histogram and tree pointers here. e.g.
    TH2D* hXY = nullptr ;

public:
    testPluginProcessor() { SetTypeName(NAME_OF_THIS); }
    
    void InitWithGlobalRootLock() override;
    void ProcessSequential(const std::shared_ptr<const JEvent>& event) override;
    void FinishWithGlobalRootLock() override;

protected:
    // Pointer to the geometry service
    std::shared_ptr<JDD4hep_service> m_geoSvc;

};
