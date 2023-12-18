
#include<TCanvas.h>
#include<TGraphErrors.h>
#include<TH1.h>
#include<TH3.h>

void ProduceAcceptanceFile( TString fname = "../eventAnalysis/ConvMiddle/MergedOutput.root" )
{

  TFile *f = new TFile(fname, "READ");

  TH1D *trackerTop = (TH1D*)f->Get("LumiTracker/hTrackerTop_Acceptance");
  TH1D *trackerBot = (TH1D*)f->Get("LumiTracker/hTrackerBot_Acceptance");
  TH1D *trackerCoinc = (TH1D*)f->Get("LumiTracker/hTrackerCoincidence_Acceptance");
  TH1D *calTop = (TH1D*)f->Get("LumiSpecCAL/hCALTop_Acceptance");
  TH1D *calBot = (TH1D*)f->Get("LumiSpecCAL/hCALBot_Acceptance");
  TH1D *calCoinc = (TH1D*)f->Get("LumiSpecCAL/hCALCoincidence_Acceptance");

  vector<TH1D*> histoSet = {trackerTop, trackerBot, trackerCoinc, calTop, calBot, calCoinc};

  TH1D *genPhotons = (TH1D*)f->Get("hGenPhoton_E");
 
  TFile *fout = new TFile("acceptanceFile.root","RECREATE");

  for( auto h : histoSet ) {
    h->Divide( genPhotons );
    h->SetDirectory(0);
    h->GetYaxis()->SetTitle("Acceptance");

    h->Write();
  }


}// void close
