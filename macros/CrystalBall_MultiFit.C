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

  // par[0] - mu
  // par[1] - sigma 
  // par[2] - alpha 
  // par[3] - n
  // par[4] - Norm 

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
void CrystalBall_MultiFit(TString fname){

  //Read the file and store the data
  TFile *file = new TFile(fname, "READ");
  TH2D *hist2D = (TH2D*)file->Get("hCAL_Eres"); //The histogram to be fitted.
  std::vector<TCanvas*> cans;

  //The fit function
  TF1 *cbfunc = new TF1("cbfunc", CB_Fit_Function, 0, 50, 5);
  cbfunc->SetParNames("#mu", "#sigma", "#alpha", "n", "Norm");
  //cbfunc->SetParLimits(0, 5,30);
  //cbfunc->SetParLimits(1, 0.05,0.2);
  //cbfunc->SetParLimits(2, 0.2,3);
  //cbfunc->SetParLimits(3, 0,1);
  //cbfunc->SetParLimits(4, 1,100);
  //
  for( int bin=0; bin<hist2D->GetNbinsX(); bin++) {

    double Egen = hist2D->GetXaxis()->GetBinLowEdge(bin);
    //if(Egen != 19.5 ) continue;
    if(Egen > 20 ) continue;
    cout<<"Egen="<<Egen<<endl;
    
    TH1D *pro = hist2D->ProjectionY(Form("pro_%i",bin),bin, bin);
    pro->SetTitle(Form("E_{gen }= %.1f",Egen));
    pro->GetXaxis()->SetRangeUser(0.5*Egen, 1.2*Egen);

    if( pro->Integral() < 150 ) { continue; }

    // "#mu"     "#sigma"     "#alpha"    "n"   "Norm"
    double mu_i = pro->GetMean();
    double sigma_i = pro->GetStdDev()/20.;
    double alpha_i = 1;
    double n_i = 1;
    double norm_i = pro->GetMaximum();
    cbfunc->SetParameters( mu_i, sigma_i, alpha_i, n_i, norm_i );
       
    cbfunc->SetParLimits(0, 0.75*mu_i, 1.25*mu_i);
    cbfunc->SetParLimits(1, 0.5*sigma_i, sigma_i + 0.1);
    cbfunc->SetParLimits(2, 0.1*alpha_i, 2*alpha_i);
    cbfunc->SetParLimits(3, 0.0*n_i, 10*n_i);
    cbfunc->SetParLimits(4, 0.1*norm_i, 2*norm_i);
    cout<<pro->GetMean()<<"  "<<pro->GetStdDev()<<"  "<<pro->GetMaximum()<<endl;

    //plot the fitted graph
    cans.push_back( new TCanvas(Form("canvas_%i",bin)) );
    gStyle->SetOptFit(111);
    gStyle->SetOptStat(0);
   
    pro->Fit("cbfunc", "e", "", 0.1*Egen, 1.5*Egen);
    //pro->Draw();
    //cbfunc->Draw("same");
  }
}
