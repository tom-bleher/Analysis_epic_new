#include <cmath>
#include <iostream>
#include <math.h>
#include <random>
#include <cctype>

#include "TFile.h"
#include "TMath.h"
#include "TRandom.h"
#include "TF1.h"
#include "TF2.h"
#include "TH2D.h"
#include "TLorentzVector.h"

using namespace std;

double Z = 1;
double electronPz = -18;
double hadronPz = 275;

double prefactor = 2.3179; // 4 alpha r_e^2 (mb)
double protonMass = 0.938272;
double electronMass = 0.51099895e-3;
double photonMass = 0;
double muonMass = 0.1056583745;
double pionZeroMass = 0.1349768;
double pionMass = 0.13957039;

double IntegratedCrossSection( double Elow, double Ehigh, TF1 *BH, TH1D *Acc = nullptr );

void BH_plotter(bool plotFromFile = false) {

  gStyle->SetOptStat(0);

  TFile *fin = nullptr;
  TH1D *acc = nullptr;

  TCanvas *c2 = new TCanvas("c2","c2",10,10,700,500);
 
  gPad->DrawFrame(0.,0.,35,0.55);

  if(plotFromFile) {
    fin = new TFile("../recoPlugins/eicrecon.root","READ");
    acc = (TH1D*)fin->Get("hCAL_Acceptance");
    acc->Scale(1/1000.);
    acc->SetMarkerStyle(20);
    acc->GetXaxis()->SetRangeUser(0,35);
    acc->GetYaxis()->SetRangeUser(0,0.55);
    acc->GetYaxis()->SetTitle("CAL acceptance");
    acc->SetTitle("");
    acc->Draw("p");
  }

    TF1 *BH_E = new TF1("BH_E", "[0] * ([1] - x)/(x*[1])*([1]/([1] - x) + ([1] - x)/[1] - 2/3.)*(log(4*[2]*[1]*([1] - x)/([3]*[4]*x)) - 0.5)", 0.1,20);
    TF1 *BH_E_scaled = new TF1("BH_E_scaled", "1/10.*log([0] * ([1] - x)/(x*[1])*([1]/([1] - x) + ([1] - x)/[1] - 2/3.)*(log(4*[2]*[1]*([1] - x)/([3]*[4]*x)) - 0.5))", 0.1,20);
    vector<TF1*> funcs = {BH_E, BH_E_scaled};
    for( auto el : funcs ) {
      el->SetParameter( 0, Z*Z*prefactor );
      el->SetParameter( 1, fabs( electronPz ) );
      el->SetParameter( 2, fabs( hadronPz ) );
      el->SetParameter( 3, protonMass );
      el->SetParameter( 4, electronMass );
      el->SetNpx( 10000 );
      el->GetXaxis()->SetTitle("E (GeV)");
      el->GetYaxis()->SetTitle("d#sigma/dE (mb/GeV)");
      el->SetTitle("Bethe-Heitler");
      el->GetXaxis()->SetRangeUser(0.1,18);
    }

  if(plotFromFile) {
    BH_E_scaled->Draw("same"); 
    
    double Emin_eff = BH_E_scaled->GetX(0.55);
    double Emax_eff = BH_E_scaled->GetX(0.04);
    TGaxis *A3 = new TGaxis(35,0.04,34.999,0.55,BH_E->Eval(Emax_eff),BH_E->Eval(Emin_eff),50510,"G");
    A3->SetTitle("Bethe-Heitler d#sigma/dE (mb/GeV)");
    A3->SetLabelSize(0.04);
    A3->SetTitleSize(0.04);
    A3->SetTitleOffset(0.5);
    A3->SetLabelOffset(0.025);
    A3->Draw("same");
  
    TLegend *leg = new TLegend(0.6,0.7,0.83,0.9);
    leg->AddEntry(acc,"CAL acceptance","p");
    leg->AddEntry(BH_E,"Bethe-Heitler d#sigma/dE","l");
    leg->Draw("same");
  }
  else { 
    BH_E->Draw(); 
    gPad->SetLogy();
  }

  double Elow = 1, Ehigh = 18; // GeV
  TFile *fAcc = new TFile("../results/Acceptances/Acceptances_2023_Apr28.root","READ");
  TH1D *hAcc = nullptr;
  if( fAcc ) {
     cout<<"Acceptance file found"<<endl;
     //hAcc = (TH1D*)fAcc->Get("hCALCoincidence_Acceptance");
     hAcc = (TH1D*)fAcc->Get("hCALTop_Acceptance");
  }
  
  double crossSection = IntegratedCrossSection( Elow, Ehigh, BH_E, hAcc );

  double LumiInst = 1.54e33; // instantenous lumi (cm^-2*sec^-1)
  double mbTocm2 = 1e-27;
  double Tbunch = 44e-9; // spacing between bunch centers in sec
  double LumiPerBunch = LumiInst * mbTocm2 * Tbunch;
  double photonsPerSec = LumiInst * mbTocm2 * crossSection;
  double photonsPerBunchCrossing = LumiPerBunch * crossSection;

  cout<<"Integrated Bremsstrahlung cross section = "<<crossSection<<" mb"<<endl;
  cout<<"Number of photons per second = "<<photonsPerSec<<endl;
  cout<<"Number of photons per bunch crossing = "<<photonsPerBunchCrossing<<endl;

  gPad->SetGridx();
  gPad->SetGridy();

}

double IntegratedCrossSection( double Elow, double Ehigh, TF1 *BH, TH1D *Acc = nullptr ) {

  double crossSection = 0;

  if( ! Acc ) { // No acceptance histogram
    crossSection = BH->Integral(Elow, Ehigh);
  }
  else {
    int FirstGoodBin = 0;
    double FirstBinCenter = 0;
    double SecondBinCenter = 0;

    for( int bin = 1; bin <= Acc->GetNbinsX(); bin++ ) {
      
      double E = Acc->GetXaxis()->GetBinCenter(bin);
      if( E < 0.001 || E > 18 ) continue;
      
      if( Acc->GetBinContent(bin) > 0 ) {
        if( FirstBinCenter == 0 ) {
          FirstGoodBin = bin;
          FirstBinCenter = Acc->GetXaxis()->GetBinCenter( bin );
        }
        if( SecondBinCenter == 0 && bin != FirstGoodBin ) {
          SecondBinCenter = Acc->GetXaxis()->GetBinCenter( bin );
        }
      }
      crossSection += Acc->GetBinContent(bin) * BH->Eval( E );
    }
  
    // Bin Width correction
    crossSection *= (SecondBinCenter - FirstBinCenter); 
  }

  return crossSection;
}

