
#include "test_sectorProcessor.h"
#include <services/rootfile/RootFile_service.h>

// The following just makes this a JANA plugin
extern "C" {
	void InitPlugin(JApplication *app) {
		InitJANAPlugin(app);
		app->Add(new test_sectorProcessor);
	}
}

//-------------------------------------------
// InitWithGlobalRootLock
//-------------------------------------------
void test_sectorProcessor::InitWithGlobalRootLock(){

	auto rootfile_svc = GetApplication()->GetService<RootFile_service>();
	auto rootfile = rootfile_svc->GetHistFile();
	rootfile->mkdir("test_sector")->cd();

	//Virtual Plane
	hVP_XY = new TH2D("hVP_XY", "Virtual Plane Hits X vs Y;X (cm);Y (cm)", 120, -30, 30, 120, -30, 30);

	// Create histograms here. e.g.
	hEraw  = new TH1D("Eraw",  "hit energy (raw)", 2500, 0, 50);

	for(int i=0; i<4; i++){
		hGlobalXY[i] = new TH2D( Form("hGlobalXY%d", i), "Global Tracker Hits X vs Y;X (cm);Y (cm)", 120,-30,30, 120,-30,30);
		hLocalXY[i] = new TH2D( Form("hLobalXY%d", i), "Local Tracker Hits X vs Y;X (cm);Y (cm)", 120,-30,30, 120,-30,30);

	}

	hEup  = new TH1D("hEup",  "Upper CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50);
	hEdw  = new TH1D("hEdw",  "Lower CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50);
	hEnergy  = new TH1D("hEnergy",  "CAL. Energy; Rec. Energy (GeV); Events",  2500, 0,50);

	int counter = 0; //+
}

//-------------------------------------------
// ProcessSequential
//-------------------------------------------
void test_sectorProcessor::ProcessSequential(const std::shared_ptr<const JEvent>& event) {


	//virtual variables
	int count_vp = 0;

	double pos_x_vp = 0.0;
	double pos_y_vp = 0.0;
	
	int hitcount = 0; //+

	double Eup  	= 0.0;
	double Edw  	= 0.0;
	
	int counts_up	= 0;
	int counts_dw	= 0;

	int counts[4]	= {0};

	//global position
	double gpos_x[4] = {0.0};
	double gpos_y[4] = {0.0};

	//local position
	double lpos_x[4] = {0.0};
	double lpos_y[4] = {0.0};

	auto app = GetApplication();

	//Virtual Plane Input__________________________________________
	for( auto hit : VirtPlanehits() ){

		const auto id = hit->getCellID();
/*
		m_geoSvc 		= app->template GetService<JDD4hep_service>();
		const auto gpos 	= m_geoSvc->cellIDPositionConverter()->position(id);
	
		//if( (gpos.x()!=0.0) && (gpos.y()!=0.0) ){
			pos_x_vp += gpos.x();
			pos_y_vp += gpos.y();

			count_vp ++;
			//}
			//
			*/
			pos_x_vp += hit->getPosition().x;
			pos_y_vp += hit->getPosition().y;
			count_vp++;
	}
	
	//Calorimeter Input___________________________________________
	for( auto hit : CALhits()  ) {

		hitcount++; //+

		const auto id 		= hit->getCellID();

		m_geoSvc 		= app->template GetService<JDD4hep_service>();
		auto id_dec 		= m_geoSvc->detector()->readout( "LumiSpecCALHits" ).idSpec().decoder();

		int sector_idx 		= id_dec->index( "sector" ); //Top (0) and Bottom (1)
		int module_idx 		= id_dec->index( "module" ); //8x8 Matrix of bars (0-127)

		const int sec_id 	= (int) id_dec->get( id, sector_idx );
		const int mod_id 	= (int) id_dec->get( id, module_idx );

		hEraw->Fill( hit->getEnergy() );

		//cout<<"sec_id : "<<sec_id<<" : mod_id : "<<mod_id<<" : Hit No. : "<<hitcount<<" : hit energy : "<<hit->getEnergy()<<endl;//+
		
		if(sec_id == 0) 	{ Eup += hit->getEnergy(); counts_up++; }
		else if (sec_id == 1)	{ Edw += hit->getEnergy(); counts_dw++; }

	} //Calorimeter hits close

	//Tracker Input Section______________________________________
	for( auto hit : Trackerhits() ){

		const auto id 		= hit->getCellID();

		m_geoSvc 		= app->template GetService<JDD4hep_service>();
		auto id_dec 		= m_geoSvc->detector()->readout( "LumiSpecTrackerHits" ).idSpec().decoder();

		int module_idx 		= id_dec->index( "module" ); //Front(0) and Back (1)
		int sector_idx 		= id_dec->index( "sector" ); //Top (0) and Bottom Layer (1)

		const int mod_id 	= (int) id_dec->get( id, module_idx );
		const int sec_id 	= (int) id_dec->get( id, sector_idx );

		//for global positions
		const auto gpos 	= m_geoSvc->cellIDPositionConverter()->position(id);

		//for local positions
		const auto volman 	= m_geoSvc->detector()->volumeManager();
		const auto alignment 	= volman.lookupDetElement(id).nominal();
		const auto lpos 	= alignment.worldToLocal( dd4hep::Position( gpos.x(), gpos.y(), gpos.z() ) );

		int choice = -99; //selecting array index A/tracker plane

		if((mod_id == 0) && (sec_id == 0)){ choice = 0; }
		if((mod_id == 0) && (sec_id == 1)){ choice = 1; }

		if((mod_id == 1) && (sec_id == 0)){ choice = 2; }
		if((mod_id == 1) && (sec_id == 1)){ choice = 3; }

		if( (choice >=0) && (choice <4) ){
			gpos_x[choice] += gpos.x();
			gpos_y[choice] += gpos.y();

			lpos_x[choice] += lpos.x();
			lpos_y[choice] += lpos.y();

			counts[choice]++;
		}
	} //Tracker hits close

	//Plotting Section________________________________________
	if( (counts[0]>0) && (counts[1]>0) && (counts[2]>0) && (counts[3]>0) ){

		for(int i=0; i<4; i++){

			gpos_x[i] /= counts[i];
			gpos_y[i] /= counts[i];

			lpos_x[i] /= counts[i];
			lpos_y[i] /= counts[i];
		}


		if( (gpos_y[0] >0) && (gpos_y[2]>0) && (gpos_y[1] < 0) && (gpos_y[3] <3 ) ){

			//Fill the energy histograms
			if( (Eup>0) && (Edw>0) ){

				hEnergy->Fill( (Eup + Edw) );
				hEup->Fill( Eup );
				hEdw->Fill( Edw );

				//Final Fill of XY-histograms.
				for(int i =0; i<4; i++){
					hGlobalXY[i]->Fill(	gpos_x[i], gpos_y[i] ); 
					hLocalXY[i]->Fill(	lpos_x[i], lpos_y[i] );
				}//for-loop close

			}//Energy check

		}//gpos-y check
	
	}//if-counts check

	//Virtplane Counts
	if(count_vp >0){
		hVP_XY->Fill( (pos_x_vp/count_vp), (pos_y_vp/count_vp));
	}//vit coordinates


	//End of the Sequential Process Function
} //sequence close

//-------------------------------------------
// FinishWithGlobalRootLock
//-------------------------------------------
void test_sectorProcessor::FinishWithGlobalRootLock() {

	// Do any final calculations here.
}

