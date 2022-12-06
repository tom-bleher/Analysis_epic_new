
//
// Template for this file generated with eicmkplugin.py
//

#include <algorithm> //+
#include <bitset> //+
#include <spdlog/spdlog.h> //+

#include <JANA/JEventProcessorSequentialRoot.h>
#include <TH2D.h>
#include <TFile.h>

// Include appropirate class headers. e.g.
// #include <edm4hep/SimCalorimeterHit.h>
// #include <detectors/BEMC/BEMCRawCalorimeterHit.h>

#include <edm4hep/SimTrackerHit.h>
#include <edm4hep/SimCalorimeterHit.h>

#include <services/geometry/dd4hep/JDD4hep_service.h> //+

using namespace std; //+

class test_sectorProcessor: public JEventProcessorSequentialRoot {
	private:

		// Declare histogram and tree pointers here. e.g.
		TH1D* hEraw  	= nullptr;

		TH2D* hGlobalXY[4]	= { nullptr };
		TH2D* hLocalXY[4]	= { nullptr };
		
		TH1D* hEup 	= nullptr;
		TH1D* hEdw 	= nullptr;
		TH1D* hEnergy 	= nullptr;
	        
		TH2D* hVP_XY    = nullptr;	

		// Data objects we will need from JANA e.g.
		PrefetchT<edm4hep::SimCalorimeterHit> CALhits   = {this, "LumiSpecCALHits"};
		PrefetchT<edm4hep::SimTrackerHit> Trackerhits   = {this, "LumiSpecTrackerHits"};
		PrefetchT<edm4hep::SimTrackerHit> VirtPlanehits   = {this, "LumiSpecVirtPlaneHits"};

	public:
		test_sectorProcessor() { SetTypeName(NAME_OF_THIS); }

		void InitWithGlobalRootLock() override;
		void ProcessSequential(const std::shared_ptr<const JEvent>& event) override;
		void FinishWithGlobalRootLock() override;

	protected:

		std::shared_ptr<JDD4hep_service> m_geoSvc;
};
