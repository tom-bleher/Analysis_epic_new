/* Crstall Ball fit function to fit the reconstructed energy of calorimeter.
 * Written By - Aranya Giri, University of Houston
 * Analysis for the EPIC far-backward collaboration
 * Choose the Driver code input according to you output file, Make a Histogrtam and then fit directly with this code.
 * */


#include <TFile.h>
#include <TTree.h>
#include <TMath.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TF1.h>

//CB fit function
double CB_Fit_Function(double *x, double *par){

	double fitval = 0.0;
	double left_term = 0.0;
	double right_term = 0.0;

	double A = 0.0;
	double B = 0.0;

	double gauss_arg = 0.0;
	double mod_alpha = 0.0;

	// par[4] - Norm 
	// par[2] - alpha 
	// par[3] - n
	// par[1] - sigma 
	// par[0] - mu
	
	gauss_arg = ( x[0] - par[0] )/par[1] ;
	mod_alpha = TMath::Abs(par[2]);

	A = TMath::Power( (par[3]/mod_alpha), par[3] )*TMath::Exp(-1*0.5*mod_alpha*mod_alpha);
	B = (par[3]/mod_alpha) - mod_alpha ;

	right_term = TMath::Exp(-0.5*gauss_arg*gauss_arg);
	left_term = A*( TMath::Power( (B - gauss_arg) , -par[3]) );

	if( gauss_arg > (-par[2]) ){
		fitval = right_term;	
	}
	else if(gauss_arg <= (-par[2]) ){
		fitval = left_term;
	}

	fitval = par[4]*fitval;

	return fitval;

}

//Gauss Fit Function


//Driver Code
void CrystalBall_FitFunction(){

	const double GenEnergy = 5; //GeV

	//Read the file and store the data
	TFile *file = new TFile("eicrecon.root", "READ");
	TH1D *hist = (TH1D*)file->Get("/test_sector/hEnergy"); //The histogram to be fitted.
	
	//The fit function
	TF1 *func = new TF1("fit", CB_Fit_Function, 0.0, 5.0, 5);
	
	// par[4] - Norm 
	// par[2] - alpha 
	// par[3] - n
	// par[1] - sigma 
	// par[0] - mu

	func->SetParNames("#mu", "#sigma", "#alpha", "n", "Norm");
	func->SetParameters(hist->GetMean(),hist->GetStdDev(),0.5, 8, 10);
	
	//plot the fitted graph
	TCanvas *c1 = new TCanvas("c1", "c1",72,64,1275,917);
   	gStyle->SetOptFit(111);
   	gStyle->SetOptStat(0);
   	c1->Range(1.094951,-186.2599,8.797976,1220.16);
   	c1->SetFillColor(0);
   	c1->SetBorderMode(0);
   	c1->SetBorderSize(2);
  	c1->SetTopMargin(0.06846239);
   	c1->SetBottomMargin(0.1324355);
   	c1->SetFrameBorderMode(0);
   	c1->SetFrameLineStyle(2);
   	c1->SetFrameBorderMode(0);

	hist->Fit("fit","R");
	hist->Draw();

	cout<<"End of Code"<<endl;
}
