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

double IntegratedCrossSection( double Elow, double Ehigh, TF1 *BH, double SF = 1.0, TH1D *Acc = nullptr );

void BH_plotter(bool plotFromFile = false) {

  gStyle->SetOptStat(0);

  TFile *fin = nullptr;
  TH1D *accTop = nullptr;
  TH1D *accCoinc = nullptr;

  TCanvas *c2 = new TCanvas("c2","c2",10,10,700,500);
 
  gPad->DrawFrame(0.,0.,20,0.55);

  if(plotFromFile) {
    fin = new TFile("../results/Acceptances/Acceptances_2023_Nov14.root","READ");
    accTop = (TH1D*)fin->Get("hTrackerCoincidence_Acceptance");
    //TH1D *genPhoton_E = (TH1D*)fin->Get("hGenPhoton_E");
    //accTop->Divide(genPhoton_E);
    accTop->SetMarkerStyle(20);
    accTop->GetXaxis()->SetRangeUser(0,20);
    accTop->GetYaxis()->SetRangeUser(0,0.55);
    accTop->GetYaxis()->SetTitle("acceptance");
    accTop->SetTitle("");
    accTop->Draw("p");
  }

    TF1 *BH_E = new TF1("BH_E", "[0] * ([1] - x)/(x*[1])*([1]/([1] - x) + ([1] - x)/[1] - 2/3.)*(log(4*[2]*[1]*([1] - x)/([3]*[4]*x)) - 0.5)", 0.1,20);
    TF1 *BH_E_scaled = new TF1("BH_E_scaled", "1/10.*log([0] * ([1] - x)/(x*[1])*([1]/([1] - x) + ([1] - x)/[1] - 2/3.)*(log(4*[2]*[1]*([1] - x)/([3]*[4]*x)) - 0.5))", 0.1,20);
    vector<TF1*> funcs = {BH_E, BH_E_scaled};
    for( auto el : funcs ) {
      el->SetParameter( 0, prefactor * Z*Z );
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
    TGaxis *A3 = new TGaxis(20,0.04,19.999,0.55,BH_E->Eval(Emax_eff),BH_E->Eval(Emin_eff),50510,"G");
    A3->SetTitle("Bethe-Heitler d#sigma/dE (mb/GeV)");
    A3->SetLabelSize(0.04);
    A3->SetTitleSize(0.04);
    A3->SetTitleOffset(0.5);
    A3->SetLabelOffset(0.025);
    A3->Draw("same");
  
    TLegend *leg = new TLegend(0.35,0.7,0.73,0.9);
    leg->AddEntry(accTop,"Tracker acceptance","p");
    leg->AddEntry(BH_E,"Bethe-Heitler d#sigma/dE","l");
    leg->Draw("same");
  }
  else { 
    BH_E->Draw(); 
    gPad->SetLogy();
  }

  double Elow = 1, Ehigh = 18; // GeV
  //TFile *fAcc = new TFile("../results/Acceptances/Acceptances_2023_Nov14.root","READ");
  //if( fAcc ) {
  //   cout<<"Acceptance file found"<<endl;
  //   accTop = (TH1D*)fAcc->Get("hTrackerTop_Acceptance");
  //   accCoinc = (TH1D*)fAcc->Get("hTrackerCoincidence_Acceptance");
  //}

  return;

  // Inst Lumi (cm^-2*sec^-1)
  // Tbunch is spacing between bunch centers in sec for 18 GeV electron beam (~10 ns for lower E)
  const int Nconfigs = 9;
  string configs[Nconfigs] = {"ep 275x18", "ep 275x10", "ep 100x10", "ep 100x5", "ep 41x5", "eAu 110x18", "eAu 110x10", "eAu 110x5", "eAu 41x5"};
  double eEnergies[Nconfigs] = {18, 10, 10, 5, 5, 18, 10, 5, 5};
  double hEnergies[Nconfigs] = {275, 275, 100, 100, 41, 110, 110, 110, 41};
  double Z[Nconfigs] = {1, 1, 1, 1, 1, 79, 79, 79, 79};
  double A[Nconfigs] = {1, 1, 1, 1, 1, 197, 197, 197, 197};
  double LumiInst[Nconfigs] = {1.54e33, 10.0e33, 4.48e33, 3.68e33, 0.44e33, 0.52e33, 4.76e33, 4.77e33, 1.67e33};
  double Tbunch[Nconfigs] = {44e-9, 11e-9, 11e-9, 11e-9, 11e-9, 44e-9, 11e-9, 11e-9, 11e-9};
  double BfieldSF[Nconfigs] = {1.0, 10/18., 10/18., 5/18., 5/18., 1.0, 10/18., 5/18., 5/18.};

  double mbTocm2 = 1e-27;

  // conversion: 1cm Al exit-window, 37m air, 1cm Al entrance cap, 1mm Al conversion foil
  // For photons, mean free path = lambda = 9/7 * X0 
  // radiation lengths found in PDF: ATOMIC AND NUCLEAR PROPERTIES OF MATERIALS
  // X0_air = 304.2 m, X0_Al = 0.089 m
  // Pconv = 1 - exp( - DeltaZ / lambda )
  double convProb = 0.92 * 0.91 * 0.92 * 0.01;
  
  double Ncoinc[Nconfigs] = {100};
  double Nsingle[Nconfigs] = {100};

  TF1 *BH_clone = (TF1*)BH_E->Clone("BH_clone");
  TH1D *RatesTop = new TH1D("Rates","single",Nconfigs,-0.5,Nconfigs-0.5);
  TH1D *RatesCoinc = new TH1D("Rates","coincidence",Nconfigs,-0.5,Nconfigs-0.5);

  for( int i = 0; i < Nconfigs; i++ ) {
    cout<<"-------------  "<<configs[i]<<"  -------------"<<endl;
    BH_clone->SetParameter( 0, prefactor * Z[ i ]*Z[ i ] );
    BH_clone->SetParameter( 1, eEnergies[ i ] );
    BH_clone->SetParameter( 2, hEnergies[ i ] );
    double IntCrossSectionTop = IntegratedCrossSection( Elow, Ehigh, BH_clone, BfieldSF[ i ], accTop );
    double IntCrossSectionCoinc = IntegratedCrossSection( Elow, Ehigh, BH_clone, BfieldSF[ i ], accCoinc );
    double LumiPerBunch = LumiInst[ i ] / A[ i ] * mbTocm2 * Tbunch[ i ];
    //
    Nsingle[ i ] = convProb * LumiPerBunch * IntCrossSectionTop;
    Ncoinc[ i ]  = convProb * LumiPerBunch * IntCrossSectionCoinc;
    //
    RatesTop->Fill( i, Nsingle[ i ] );
    RatesCoinc->Fill( i, Ncoinc[ i ] );
    RatesTop->GetXaxis()->SetBinLabel(i+1, configs[i].data());
    RatesCoinc->GetXaxis()->SetBinLabel(i+1, configs[i].data());
    cout<<"Integrated Bremsstrahlung cross section in Top = "<<IntCrossSectionTop<<" mb"<<endl;
    cout<<"Number of singles per bunch crossing in Top = "<<Nsingle[ i ]<<endl;
    cout<<"---"<<endl;
    cout<<"Integrated Bremsstrahlung cross section with coincidence = "<<IntCrossSectionCoinc<<" mb"<<endl;
    cout<<"Number of photons per bunch crossing with coincidence = "<<Ncoinc[ i ]<<endl;
  }


  TCanvas *c3 = new TCanvas("c3","c3",10,10,700,500);
  
  RatesTop->SetMarkerStyle(20);
  RatesTop->SetMarkerColor(4);
  RatesCoinc->SetMarkerStyle(20);
  RatesCoinc->SetMarkerColor(2);
  RatesTop->GetYaxis()->SetTitle("N / bunch xing");
  RatesTop->GetXaxis()->SetLabelSize( 0.05 );
  RatesTop->SetTitle("");

  TLegend *legRates = new TLegend(0.15,0.7,0.45,0.9);

  RatesTop->Draw("hist p");
  RatesCoinc->Draw("hist p same");
  legRates->AddEntry(RatesTop,"single", "p");
  legRates->AddEntry(RatesCoinc,"coincidence", "p");
  legRates->Draw("same");
 
  gPad->SetGridx();
  gPad->SetGridy();

}

double IntegratedCrossSection( double Elow, double Ehigh, TF1 *BH, double SF = 1.0, TH1D *Acc = nullptr ) {

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
     
      // SF is to scale up/down to simulate scaled magnet current
      crossSection += Acc->GetBinContent(bin) * BH->Eval( SF * E );
    }
  
    // Bin Width correction
    crossSection *= SF * (SecondBinCenter - FirstBinCenter); 
  }

  return crossSection;
}

