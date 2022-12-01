/* To Find the acceptance of the geometry.
 * Written by - Aranya Giri, University of Houston
 * Input : histograms from reconsted energy of pair spectromter and virtual plane.
 * */


#include<TCanvas.h>
#include<TGraphErrors.h>
#include<TH1.h>
#include<TH3.h>

void acceptance()
{

	const int TotalFile = 10;

	int FileNo[TotalFile] = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50};
	double NGen[TotalFile] = {0.0};

	//energy deposited in calorimeter.

	double NRec[TotalFile] = {0.0};
	double Acceptance[TotalFile] = {0.0};
	double EGen[TotalFile] = {0.0};
	TCanvas *c = new TCanvas("c", "c", 800,600);

	TGraph *gr;

	for(int jentry = 0; jentry <TotalFile; jentry++){ //for1 close

		TFile *file_in 	= new TFile( Form("Out%d.root", FileNo[jentry]), "READ");
		TH1F *hEdep_CAL = (TH1F*)file_in->Get("test_sector/hEnergy");
		TH2D *hVP_XY 	= (TH2D*)file_in->Get("test_sector/hVP_XY");

		NRec[jentry] 	= hEdep_CAL->GetEntries();
		NGen[jentry]	= hVP_XY->GetEntries();

		Acceptance[jentry] = NRec[jentry]/NGen[jentry];
		EGen[jentry] 	= (double) FileNo[jentry];

	} //for 1 close

	TFile *file_out = new TFile("acceptance_10kEv_VP1mmSilicon.root", "RECREATE");
	gr = new TGraph(TotalFile, EGen, Acceptance);
	gr->SetName("gr_acceptance");
	gr->SetTitle(" Acceptance with Bx = 0.5T ");
        gr->SetMarkerColor(4);
   	gr->SetMarkerStyle(21);
	gr->Write();
   	gr->Draw("APE");

	cout<<"End of Code"<<endl;

}// void close
