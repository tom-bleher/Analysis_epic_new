// 24/01/24 - Stephen JD Kay, University of York

// A script to make some quick plots of quantities from the lumi pair spec reconstruction
// Execute via root -l 'PairSpecPlots.C("Input_File.root")'
#define PairSpecPlots_cxx
          
// Include relevant stuff
#include <TStyle.h>
#include <TCanvas.h>
#include <TLine.h>
#include <TMath.h>
#include <TPaveText.h>
#include <TGaxis.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <TSystem.h>
#include <TTree.h>
#include <TArc.h>
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TLegend.h>
#include "ECCEStyle.C"

void PairSpecPlots(string InFilename = ""){

  gROOT->SetBatch(kTRUE); // Force script to always run without flashing up graphics
  //gROOT->ProcessLine("SetECCEStyle()");
  gStyle->SetOptStat(0);
  TString LumiCal_rootFile;

  if(InFilename == ""){
    cout << "Enter a calorimeter filename to analyse: ";
    cin >> InFilename;
  }
  
  TString TInFilename = InFilename;
  LumiCal_rootFile = TInFilename;

  if(gSystem->AccessPathName(LumiCal_rootFile) == kTRUE){
    cerr << "!!!!! ERROR !!!!!" << endl << LumiCal_rootFile << " not found" << endl << "!!!!! ERROR !!!!!" << endl;
    exit(0);
  }
  else{
    cout << "Opening " << LumiCal_rootFile << endl;
  }
  
  TFile *InFile = new TFile(LumiCal_rootFile);

  TString DetNames[3]={"Top", "Bot", "Coin"};
  double AcceptMaxY[3];
  
  // Somewhat tedious, but manually grab each
  // Convention for hists will be top(positron)/bot(electron)/coin(photon/electron+positron)
  TH1F* Truth_EDist_Hists[3];
  Truth_EDist_Hists[0] = (TH1F*)((TH1F*)InFile->Get("hGenPositron_E")); Truth_EDist_Hists[0]->Sumw2();
  Truth_EDist_Hists[1] = (TH1F*)((TH1F*)InFile->Get("hGenElectron_E")); Truth_EDist_Hists[1]->Sumw2();
  Truth_EDist_Hists[2] = (TH1F*)((TH1F*)InFile->Get("hGenPhoton_E")); Truth_EDist_Hists[2]->Sumw2();
  TH1F* Accept_Hists[3];
  Accept_Hists[0] = (TH1F*)((TH1F*)InFile->Get("CAL_Acceptance_Info/h1CALTopAccept")); Accept_Hists[0]->Sumw2();
  AcceptMaxY[0] = (Accept_Hists[0]->GetBinContent(Accept_Hists[0]->GetMaximumBin()))*1.05; Accept_Hists[0]->GetYaxis()->SetRangeUser(0, AcceptMaxY[0]);
  Accept_Hists[1] = (TH1F*)((TH1F*)InFile->Get("CAL_Acceptance_Info/h1CALBotAccept")); Accept_Hists[1]->Sumw2();
  AcceptMaxY[1] = (Accept_Hists[1]->GetBinContent(Accept_Hists[1]->GetMaximumBin()))*1.05; Accept_Hists[1]->GetYaxis()->SetRangeUser(0, AcceptMaxY[1]);
  Accept_Hists[2] = (TH1F*)((TH1F*)InFile->Get("CAL_Acceptance_Info/h1CALCoinAccept")); Accept_Hists[2]->Sumw2();
  AcceptMaxY[2] = (Accept_Hists[2]->GetBinContent(Accept_Hists[2]->GetMaximumBin()))*1.05; Accept_Hists[2]->GetYaxis()->SetRangeUser(0, AcceptMaxY[2]);
  TH2F* SampFrac_Hists[3];
  SampFrac_Hists[0] = (TH2F*)((TH2F*)InFile->Get("CAL_Acceptance_Info/h2SampFracTop")); SampFrac_Hists[0]->Sumw2();
  SampFrac_Hists[1] = (TH2F*)((TH2F*)InFile->Get("CAL_Acceptance_Info/h2SampFracBot")); SampFrac_Hists[1]->Sumw2();
  SampFrac_Hists[2] = (TH2F*)((TH2F*)InFile->Get("CAL_Acceptance_Info/h2SampFracCoin")); SampFrac_Hists[2]->Sumw2(); 
  
  TH1F* Accept_Hists_PosCut[3][5];
  TH2F* SampFrac_Hists_PosCut[3][5];

  for(int i = 0; i < 5; i++){
    Accept_Hists_PosCut[0][i] = (TH1F*)((TH1F*)InFile->Get(Form("CAL_Acceptance_Info_Position_Cuts/h1CALTopAccept_%i", (i+1))));
    Accept_Hists_PosCut[1][i] = (TH1F*)((TH1F*)InFile->Get(Form("CAL_Acceptance_Info_Position_Cuts/h1CALBotAccept_%i", (i+1))));
    Accept_Hists_PosCut[2][i] = (TH1F*)((TH1F*)InFile->Get(Form("CAL_Acceptance_Info_Position_Cuts/h1CALCoinAccept_%i", (i+1))));
    for(int j = 0; j < 3; j++){
      Accept_Hists_PosCut[j][i] ->GetYaxis()->SetRangeUser(0, AcceptMaxY[j]);
    }
    SampFrac_Hists_PosCut[0][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Acceptance_Info_Position_Cuts/h2SampFracTop_%i", (i+1))));
    SampFrac_Hists_PosCut[1][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Acceptance_Info_Position_Cuts/h2SampFracBot_%i", (i+1))));
    SampFrac_Hists_PosCut[2][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Acceptance_Info_Position_Cuts/h2SampFracCoin_%i", (i+1)))); 
  }

  // Energy resolution - Fit slices in Y of sampling fraction plots. Get mean/sigma for fits. Plot mean/sigma as a new histogram
  TObjArray SampFracSlices[3][6];
  TH1F *SampFracMeanVals[3][6];
  TH1F *SampFracSigVals[3][6];
  TH1F *ERes[3][6];
  TString CutVals[3][6];
  TF1* EResFits[3][6];
  for(int i = 0; i < 6; i++){
    CutVals[0][i] = Form("%i cm < y_{Top} < %i cm", (7+i), (23-i));
    CutVals[1][i] = Form("%i cm < y_{Bot} < %i cm", (-23+i), (-7-i));
    CutVals[2][i] = Form("%i cm < y_{Top} < %i cm & %i cm < y_{Bot} < %i cm", (7+(i)), (23-i), (-23+i), (-7-i));
    if (i == 0){ // Assuming 200 bins in x from 0 to 20, this is 4 to 18.5 GeV
      SampFrac_Hists[0]->FitSlicesY(0, 40, 185, 0, "QNR", &SampFracSlices[0][i]);
      SampFrac_Hists[1]->FitSlicesY(0, 40, 185, 0, "QNR", &SampFracSlices[1][i]);
      SampFrac_Hists[2]->FitSlicesY(0, 40, 185, 0, "QNR", &SampFracSlices[2][i]);
    }
    else{
      SampFrac_Hists_PosCut[0][i-1]->FitSlicesY(0, 40, 185, 0, "QNR", &SampFracSlices[0][i]);
      SampFrac_Hists_PosCut[1][i-1]->FitSlicesY(0, 40, 185, 0, "QNR", &SampFracSlices[1][i]);
      SampFrac_Hists_PosCut[2][i-1]->FitSlicesY(0, 40, 185, 0, "QNR", &SampFracSlices[2][i]);
    }

    for(int j = 0; j <3; j++){
      SampFracMeanVals[j][i] = (TH1F*)SampFracSlices[j][i][1]->Clone();
      SampFracMeanVals[j][i]->Sumw2();
      SampFracMeanVals[j][i]->SetName(Form("h1_%i_%s_Det_SampFrac_Mean", i+1, DetNames[j].Data()));
      SampFracSigVals[j][i] = (TH1F*)SampFracSlices[j][i][2]->Clone();
      SampFracSigVals[j][i]->Sumw2();
      SampFracSigVals[j][i]->SetName(Form("h1_%i_%s_Det_SampFrac_Sigma", i+1, DetNames[j].Data()));
      ERes[j][i] = (TH1F*)SampFracSigVals[j][i]->Clone();
      ERes[j][i]->Divide(SampFracMeanVals[j][i]);
      ERes[j][i]->SetName(Form("h1_%i_%s_Det_ERes", i+1, DetNames[j].Data()));
      ERes[j][i]->SetTitle(Form("ERes of %s det, %s", DetNames[j].Data(), CutVals[j][i].Data()));
      ERes[j][i]->GetYaxis()->SetTitle("#sigma/#mu of Sampling Frac Slice Fit");
      EResFits[j][i] = new TF1(Form("EResFit_%s_Det_%i", DetNames[j].Data(), i+1), "pol1", 4, 18.5);
      ERes[j][i]->Fit(EResFits[j][i], "QR");
      SampFracSlices[j][i].Clear(); // Explicitly clear object to prevent memory leak
    }
  }
  
  TH2F* ZX_Hists[2][3];
  TH2F* ZY_Hists[2][3];
  TH2F* EdX_Hists[2][3];
  TH2F* EdY_Hists[2][3];
  TH2F* EMCdX_Hists[2][3];
  TH2F* EMCdY_Hists[2][3];
  
  for(int i = 0; i < 3; i++){
    ZX_Hists[0][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2ZXETop_%imm", (int)pow(3,i))));
    ZX_Hists[1][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2ZXEBot_%imm", (int)pow(3,i))));
    ZY_Hists[0][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2ZYETop_%imm", (int)pow(3,i))));
    ZY_Hists[1][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2ZYEBot_%imm", (int)pow(3,i))));
    EdX_Hists[0][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2EdXTop_%imm", (int)pow(3,i))));
    EdX_Hists[1][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2EdXBot_%imm", (int)pow(3,i))));
    EdY_Hists[0][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2EdYTop_%imm", (int)pow(3,i))));
    EdY_Hists[1][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2EdYBot_%imm", (int)pow(3,i))));
    EMCdX_Hists[0][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2EMCdXTop_%imm", (int)pow(3,i))));
    EMCdX_Hists[1][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2EMCdXBot_%imm", (int)pow(3,i))));
    EMCdY_Hists[0][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2EMCdYTop_%imm", (int)pow(3,i))));
    EMCdY_Hists[1][i] = (TH2F*)((TH2F*)InFile->Get(Form("CAL_Position_Info/h2EMCdYBot_%imm", (int)pow(3,i))));
  }

  // Position resolution plots, take slices of EdX and EdY histograms, extract mean and sigma values for plot. Fit mean with Pol1. Plot mean/sigma too?
  // Very bloated loop, could be trimmed down a lot
  TObjArray XResSlices[2][3];
  TH1F *XResOffsVals[2][3];
  TH1F *XResMeanVals[2][3];
  TH1F *XResSigVals[2][3];
  TObjArray YResSlices[2][3];
  TH1F *YResOffsVals[2][3];
  TH1F *YResMeanVals[2][3];
  TH1F *YResSigVals[2][3];
  TF1* PResFits[2][2][2][3]; // Fit/X or Y/Top or Bottom/Pixel size
  for(int i = 0; i < 3; i++){
    EdX_Hists[0][i]->FitSlicesY(0, 10, 45, 0, "QNR", &XResSlices[0][i]);
    EdX_Hists[1][i]->FitSlicesY(0, 10, 45, 0, "QNR", &XResSlices[1][i]);
    EdY_Hists[0][i]->FitSlicesY(0, 10, 45, 0, "QNR", &YResSlices[0][i]);
    EdY_Hists[1][i]->FitSlicesY(0, 10, 45, 0, "QNR", &YResSlices[1][i]);
    
    XResOffsVals[0][i] = (TH1F*)XResSlices[0][i][0]->Clone();
    XResOffsVals[0][i]->SetName(Form("h1_%s_Det_EdX_Offs_%imm_Pixel", DetNames[0].Data(), (int)pow(3,i)));
    XResOffsVals[0][i]->GetXaxis()->SetRangeUser(0,0.5);
    
    XResOffsVals[1][i] = (TH1F*)XResSlices[0][i][0]->Clone();
    XResOffsVals[1][i]->SetName(Form("h1_%s_Det_EdX_Offs_%imm_Pixel", DetNames[1].Data(), (int)pow(3,i)));
    XResOffsVals[1][i]->GetXaxis()->SetRangeUser(0,0.5);
   
    YResOffsVals[0][i] = (TH1F*)YResSlices[0][i][0]->Clone();
    YResOffsVals[0][i]->SetName(Form("h1_%s_Det_EdY_Offs_%imm_Pixel", DetNames[0].Data(), (int)pow(3,i)));
    YResOffsVals[0][i]->GetXaxis()->SetRangeUser(0,0.5);
    
    YResOffsVals[1][i] = (TH1F*)YResSlices[0][i][0]->Clone();
    YResOffsVals[1][i]->SetName(Form("h1_%s_Det_EdY_Offs_%imm_Pixel", DetNames[1].Data(), (int)pow(3,i)));
    YResOffsVals[1][i]->GetXaxis()->SetRangeUser(0,0.5);

    XResMeanVals[0][i] = (TH1F*)XResSlices[0][i][1]->Clone();
    XResMeanVals[0][i]->SetName(Form("h1_%s_Det_EdX_Mean_%imm_Pixel", DetNames[0].Data(), (int)pow(3,i)));
    XResMeanVals[0][i]->GetXaxis()->SetRangeUser(0,0.5);
    PResFits[0][0][0][i] = new TF1(Form("XPResMeanFit_%s_%imm_Pixel", DetNames[0].Data(), (int)pow(3,1)), "pol1", 0.15, 0.4);
    XResMeanVals[0][i]->Fit(PResFits[0][0][0][i], "QR");
    
    XResMeanVals[1][i] = (TH1F*)XResSlices[0][i][1]->Clone();
    XResMeanVals[1][i]->SetName(Form("h1_%s_Det_EdX_Mean_%imm_Pixel", DetNames[1].Data(), (int)pow(3,i)));
    XResMeanVals[1][i]->GetXaxis()->SetRangeUser(0,0.5);
    PResFits[0][0][1][i] = new TF1(Form("XPResMeanFit_%s_%imm_Pixel", DetNames[1].Data(), (int)pow(3,1)), "pol1", 0.15, 0.4);
    XResMeanVals[1][i]->Fit(PResFits[0][0][1][i], "QR");
    
    YResMeanVals[0][i] = (TH1F*)YResSlices[0][i][1]->Clone();
    YResMeanVals[0][i]->SetName(Form("h1_%s_Det_EdY_Mean_%imm_Pixel", DetNames[0].Data(), (int)pow(3,i)));
    YResMeanVals[0][i]->GetXaxis()->SetRangeUser(0,0.5);
    PResFits[0][1][0][i] = new TF1(Form("YPResMeanFit_%s_%imm_Pixel", DetNames[0].Data(), (int)pow(3,1)), "pol1", 0.15, 0.4);
    YResMeanVals[0][i]->Fit(PResFits[0][1][0][i], "QR");

    YResMeanVals[1][i] = (TH1F*)YResSlices[0][i][1]->Clone();
    YResMeanVals[1][i]->SetName(Form("h1_%s_Det_EdY_Mean_%imm_Pixel", DetNames[1].Data(), (int)pow(3,i)));
    YResMeanVals[1][i]->GetXaxis()->SetRangeUser(0,0.5);
    PResFits[0][1][1][i] = new TF1(Form("XPResMeanFit_%s_%imm_Pixel", DetNames[1].Data(), (int)pow(3,1)), "pol1", 0.15, 0.4);
    YResMeanVals[1][i]->Fit(PResFits[0][1][1][i], "QR");
    
    XResSigVals[0][i] = (TH1F*)XResSlices[0][i][2]->Clone();
    XResSigVals[0][i]->SetName(Form("h1_%s_Det_EdX_Sig_%imm_Pixel", DetNames[0].Data(), (int)pow(3,i)));
    XResSigVals[0][i]->GetXaxis()->SetRangeUser(0,0.5);
    PResFits[1][0][0][i] = new TF1(Form("XPResSigFit_%s_%imm_Pixel", DetNames[0].Data(), (int)pow(3,1)), "pol2", 0.15, 0.4);
    XResSigVals[0][i]->Fit(PResFits[1][0][0][i], "QR");
    
    XResSigVals[1][i] = (TH1F*)XResSlices[0][i][2]->Clone();
    XResSigVals[1][i]->SetName(Form("h1_%s_Det_EdX_Sig_%imm_Pixel", DetNames[1].Data(), (int)pow(3,i)));
    XResSigVals[1][i]->GetXaxis()->SetRangeUser(0,0.5);
    PResFits[1][0][1][i] = new TF1(Form("XPResSigFit_%s_%imm_Pixel", DetNames[1].Data(), (int)pow(3,1)), "pol2", 0.15, 0.4);
    XResSigVals[1][i]->Fit(PResFits[1][0][1][i], "QR");

    YResSigVals[0][i] = (TH1F*)YResSlices[0][i][2]->Clone();
    YResSigVals[0][i]->SetName(Form("h1_%s_Det_EdY_Sig_%imm_Pixel", DetNames[0].Data(), (int)pow(3,i)));
    YResSigVals[0][i]->GetXaxis()->SetRangeUser(0,0.5);
    PResFits[1][1][0][i] = new TF1(Form("YPResSigFit_%s_%imm_Pixel", DetNames[0].Data(), (int)pow(3,1)), "pol2", 0.15, 0.4);
    YResSigVals[0][i]->Fit(PResFits[1][1][0][i], "QR");

    YResSigVals[1][i] = (TH1F*)YResSlices[0][i][2]->Clone();
    YResSigVals[1][i]->SetName(Form("h1_%s_Det_EdY_Sig_%imm_Pixel", DetNames[1].Data(), (int)pow(3,i)));
    YResSigVals[1][i]->GetXaxis()->SetRangeUser(0,0.5);
    PResFits[1][1][1][i] = new TF1(Form("YPResSigFit_%s_%imm_Pixel", DetNames[1].Data(), (int)pow(3,1)), "pol2", 0.15, 0.4);
    YResSigVals[1][i]->Fit(PResFits[1][1][1][i], "QR");

    for(int j = 0; j < 2; j++){ // Explicitly clear object at end of loop to prevent memory leak
      XResSlices[j][i].Clear();
      YResSlices[j][i].Clear();
    }
  }
  
   
  TH2F* ZdXE_Hists[2];
  ZdXE_Hists[0] = (TH2F*)((TH2F*)InFile->Get("CAL_Position_Info/h2ZdXTop"));
  ZdXE_Hists[1] = (TH2F*)((TH2F*)InFile->Get("CAL_Position_Info/h2ZdXBot"));
  TH2F* ZdYE_Hists[2];
  ZdYE_Hists[0] = (TH2F*)((TH2F*)InFile->Get("CAL_Position_Info/h2ZdYTop"));
  ZdYE_Hists[1] = (TH2F*)((TH2F*)InFile->Get("CAL_Position_Info/h2ZdYBot"));  
  
  TString Output_Name;
  TObjArray *tmp_Name_arr;
  // If input file name is provided as a path with / included - chop
  if(TInFilename.Contains("/")){
    tmp_Name_arr = TInFilename.Tokenize("/");
    Output_Name = (((TObjString *)(tmp_Name_arr->At(tmp_Name_arr->GetLast())))->String()).ReplaceAll(".root","");
  }
  else{
    Output_Name = TInFilename.ReplaceAll(".root","");;
  }
  
  TH1F *Acceptance_Result_Hists[3][6];
  TCanvas *c_Acceptance[6];
  c_Acceptance[0] = new TCanvas("c_Acceptance_1", "Acceptance Plots", 100, 0, 2560, 1920);
  c_Acceptance[0]->Divide(3,3);
  for(int i = 0; i <3; i++){
    
    Acceptance_Result_Hists[i][0] = (TH1F*)Accept_Hists[i]->Clone();
    Acceptance_Result_Hists[i][0]->Divide(Truth_EDist_Hists[i]);
    
    c_Acceptance[0]->cd(1+(i*3));
    Truth_EDist_Hists[i]->Draw();
    c_Acceptance[0]->cd(2+(i*3));
    Accept_Hists[i]->Draw();
    c_Acceptance[0]->cd(3+(i*3));
    if( i < 2) Acceptance_Result_Hists[i][0]->GetYaxis()->SetRangeUser(0,1.2);
    else Acceptance_Result_Hists[i][0]->GetYaxis()->SetRangeUser(0, 0.4);
    Acceptance_Result_Hists[i][0]->GetYaxis()->SetTitle("Acceptance");
    Acceptance_Result_Hists[i][0]->Draw("histerr");
  }

  c_Acceptance[0]->Print(Output_Name+".pdf"+"(");

  for(int i = 0; i < 5; i++){
    c_Acceptance[i+1] = new TCanvas(Form("c_Acceptance_%i", i+2), "Acceptance Plots", 100, 0, 2560, 1920);
    c_Acceptance[i+1]->Divide(3,3);
    for(int j = 0; j <3; j++){
      Acceptance_Result_Hists[j][i+1] = (TH1F*)Accept_Hists_PosCut[j][i]->Clone();
      Acceptance_Result_Hists[j][i+1]->Divide(Truth_EDist_Hists[j]);
    
      c_Acceptance[i+1]->cd(1+(j*3));
      Truth_EDist_Hists[j]->Draw();
      c_Acceptance[i+1]->cd(2+(j*3));
      Accept_Hists_PosCut[j][i]->Draw();
      c_Acceptance[i+1]->cd(3+(j*3));
      Acceptance_Result_Hists[j][i+1]->GetYaxis()->SetTitle("Acceptance");
      if( j < 2) Acceptance_Result_Hists[j][i+1]->GetYaxis()->SetRangeUser(0,1.2);
      else Acceptance_Result_Hists[j][i+1]->GetYaxis()->SetRangeUser(0, 0.4);
      Acceptance_Result_Hists[j][i+1]->Draw("histerr");
    }
    c_Acceptance[i+1]->Print(Output_Name+".pdf");
  }

  TCanvas *c_SamplingFracTop = new TCanvas("c_SamplingFracTop", "Sampling Fraction (Top Detector)", 100, 0, 2560, 1920);
  c_SamplingFracTop->Divide(3,2);
  c_SamplingFracTop->cd(1);
  SampFrac_Hists[0]->Draw("COLZ");
  for(int i = 0; i <5; i++){
    c_SamplingFracTop->cd(i+2);
    SampFrac_Hists_PosCut[0][i]->Draw("COLZ");
  }
  c_SamplingFracTop->Print(Output_Name+".pdf");

  TCanvas *c_EResTop = new TCanvas("c_EResTop", "ERes (Top Detector)", 100, 0, 2560, 1920);
  c_EResTop->Divide(3,2);
  for(int i = 0; i <6; i++){
    c_EResTop->cd(i+1);
    ERes[0][i]->GetYaxis()->SetRangeUser(0, 0.2);
    ERes[0][i]->Draw("histerr");
    EResFits[0][i]->SetRange(0, 20);
    EResFits[0][i]->Draw("same");
  }
  c_EResTop->Print(Output_Name+".pdf");
  
  TCanvas *c_SamplingFracBot = new TCanvas("c_SamplingFracBot", "Sampling Fraction (Bot Detector)", 100, 0, 2560, 1920);
  c_SamplingFracBot->Divide(3,2);
  c_SamplingFracBot->cd(1);
  SampFrac_Hists[1]->Draw("COLZ");
  for(int i = 0; i <5; i++){
    c_SamplingFracBot->cd(i+2);
    SampFrac_Hists_PosCut[1][i]->Draw("COLZ");
  }
  c_SamplingFracBot->Print(Output_Name+".pdf");

  TCanvas *c_EResBot = new TCanvas("c_EResBot", "ERes (Bot Detector)", 100, 0, 2560, 1920);
  c_EResBot->Divide(3,2);
  for(int i = 0; i <6; i++){
    c_EResBot->cd(i+1);
    ERes[1][i]->GetYaxis()->SetRangeUser(0, 0.2);
    ERes[1][i]->Draw("histerr");
    EResFits[1][i]->SetRange(0, 20);
    EResFits[1][i]->Draw("same");
  }
  c_EResBot->Print(Output_Name+".pdf");
  
  TCanvas *c_SamplingFracCoin = new TCanvas("c_SamplingFracCoin", "Sampling Fraction (Coincidence)", 100, 0, 2560, 1920);
  c_SamplingFracCoin->Divide(3,2);
  c_SamplingFracCoin->cd(1);
  SampFrac_Hists[2]->Draw("COLZ");
  for(int i = 0; i <5; i++){
    c_SamplingFracCoin->cd(i+2);
    SampFrac_Hists_PosCut[2][i]->Draw("COLZ");
  }
  c_SamplingFracCoin->Print(Output_Name+".pdf");

  TCanvas *c_EResCoin = new TCanvas("c_EResCoin", "ERes (Coin Detector)", 100, 0, 2560, 1920);
  c_EResCoin->Divide(3,2);
  for(int i = 0; i <6; i++){
    c_EResCoin->cd(i+1);
    ERes[2][i]->GetYaxis()->SetRangeUser(0, 0.2);
    ERes[2][i]->Draw("histerr");
    EResFits[2][i]->SetRange(0, 20);
    EResFits[2][i]->Draw("same");
  }
  c_EResCoin->Print(Output_Name+".pdf");

  TF1 *MolFit[2];
  MolFit[0] = new TF1("MolFit_X", "gaus");
  MolFit[1] = new TF1("MolFit_Y", "gaus");
  
  TCanvas *c_XEnergyDist = new TCanvas("c_XEnergyDist","",100,0,2560,1920);
  c_XEnergyDist->Divide(2,2);
  c_XEnergyDist->cd(1);
  ZdXE_Hists[0]->Draw("COLZ");  
  c_XEnergyDist->cd(2);
  ZdXE_Hists[0]->ProjectionY("ZdXE_Top_ProjY");
  TH1D* ZdXE_Top_ProjY = (TH1D*)gDirectory->Get("ZdXE_Top_ProjY");
  ZdXE_Top_ProjY->SetMarkerColor(2); ZdXE_Top_ProjY->Draw();  
  ZdXE_Top_ProjY->Fit(MolFit[0],"Q");
  c_XEnergyDist->cd(3);
  ZdXE_Hists[1]->Draw("COLZ");  
  c_XEnergyDist->cd(4);
  ZdXE_Hists[1]->ProjectionY("ZdXE_Bot_ProjY");
  TH1D* ZdXE_Bot_ProjY = (TH1D*)gDirectory->Get("ZdXE_Bot_ProjY");
  ZdXE_Bot_ProjY->SetMarkerColor(2); ZdXE_Bot_ProjY->Draw();
  ZdXE_Bot_ProjY->Fit(MolFit[1], "Q");
  c_XEnergyDist->Print(Output_Name+".pdf");

  TCanvas *c_YEnergyDist = new TCanvas("c_YEnergyDist","",100,0,2560,1920);
  c_YEnergyDist->Divide(2,2);
  c_YEnergyDist->cd(1);
  ZdYE_Hists[0]->Draw("COLZ");  
  c_YEnergyDist->cd(2);
  ZdYE_Hists[0]->ProjectionY("ZdYE_Top_ProjY");
  TH1D* ZdYE_Top_ProjY = (TH1D*)gDirectory->Get("ZdYE_Top_ProjY");
  ZdYE_Top_ProjY->SetMarkerColor(2); ZdYE_Top_ProjY->Draw();  
  ZdYE_Top_ProjY->Fit(MolFit[0],"Q");
  c_YEnergyDist->cd(3);
  ZdYE_Hists[1]->Draw("COLZ");  
  c_YEnergyDist->cd(4);
  ZdYE_Hists[1]->ProjectionY("ZdYE_Bot_ProjY");
  TH1D* ZdYE_Bot_ProjY = (TH1D*)gDirectory->Get("ZdYE_Bot_ProjY");
  ZdYE_Bot_ProjY->SetMarkerColor(2); ZdYE_Bot_ProjY->Draw();
  ZdYE_Bot_ProjY->Fit(MolFit[1], "Q");
  c_YEnergyDist->Print(Output_Name+".pdf");
  
  // Moliere radius contains 90% of the energy deposition of the shower
  // Assuming normally distributed, this is 1.645 sigma
  cout << "Moliere radius from Top Det = " << 1.645*MolFit[0]->GetParameter(2)*0.1 << "cm" <<endl;
  cout << "Moliere radius from Bot Det = " << 1.645*MolFit[1]->GetParameter(2)*0.1 << "cm" <<endl;
  // Alternative method, base it upon integral of histogram. When 90& of integral reached, this is the moliere radius
  cout << ZdXE_Top_ProjY->Integral()/2 << endl;
  double TopEDep = ZdXE_Top_ProjY->Integral()/2;
  int Bin = ZdXE_Top_ProjY->GetMaximumBin();
  double TopIntSum = 0;
  while(TopIntSum < 0.9*TopEDep & Bin < ZdXE_Top_ProjY->GetNbinsX()){
    TopIntSum+=ZdXE_Top_ProjY->GetBinContent(Bin);
    Bin++;
  }
  cout << ZdXE_Top_ProjY->GetBinCenter(Bin) << endl;

  // Drawing of these is complicated and longwinded. Should be simplified. Make a function which is just fed X/Y and the pixel size?
  TCanvas *c_PositionResX_1mm = new TCanvas("c_PositionResX_1mm", "", 100, 0, 2560, 1920);
  c_PositionResX_1mm->Divide(5,2);
  c_PositionResX_1mm->cd(1);
  ZX_Hists[0][0]->Draw("COLZ");
  c_PositionResX_1mm->cd(2);
  EdX_Hists[0][0]->Draw("COlZ");
  c_PositionResX_1mm->cd(3);
  XResOffsVals[0][0]->Draw("histerr");
  c_PositionResX_1mm->cd(4);
  XResMeanVals[0][0]->Draw("histerr");
  PResFits[0][0][0][0]->SetRange(0,0.5);
  PResFits[0][0][0][0]->Draw("SAME");
  c_PositionResX_1mm->cd(5);
  XResSigVals[0][0]->Draw("histerr");
  PResFits[1][0][0][0]->SetRange(0,0.5);
  PResFits[1][0][0][0]->Draw("SAME");
  c_PositionResX_1mm->cd(6);
  ZX_Hists[1][0]->Draw("COLZ");
  c_PositionResX_1mm->cd(7);
  EdX_Hists[1][0]->Draw("COlZ");
  c_PositionResX_1mm->cd(8);  
  XResOffsVals[1][0]->Draw("histerr");
  c_PositionResX_1mm->cd(9);
  XResMeanVals[1][0]->Draw("histerr");
  PResFits[0][0][1][0]->SetRange(0,0.5);
  PResFits[0][0][1][0]->Draw("SAME");
  c_PositionResX_1mm->cd(10);
  XResSigVals[1][0]->Draw("histerr");
  PResFits[1][0][1][0]->SetRange(0,0.5);
  PResFits[1][0][1][0]->Draw("SAME");
  c_PositionResX_1mm->Print(Output_Name+".pdf");
  
  TCanvas *c_PositionResY_1mm = new TCanvas("c_PositionResY_1mm", "", 100, 0, 2560, 1920);
  c_PositionResY_1mm->Divide(5,2);
  c_PositionResY_1mm->cd(1);
  ZY_Hists[0][0]->Draw("COLZ");
  c_PositionResY_1mm->cd(2);
  EdY_Hists[0][0]->Draw("COlZ");
  c_PositionResY_1mm->cd(3);
  YResOffsVals[0][0]->Draw("histerr");
  c_PositionResY_1mm->cd(4);
  YResMeanVals[0][0]->Draw("histerr");
  PResFits[0][1][0][0]->SetRange(0,0.5); // Fit/X Y /Top Bottom/Pixel
  PResFits[0][1][0][0]->Draw("SAME");
  c_PositionResY_1mm->cd(5);
  YResSigVals[0][0]->Draw("histerr");
  PResFits[1][1][0][0]->SetRange(0,0.5);
  PResFits[1][1][0][0]->Draw("SAME");
  c_PositionResY_1mm->cd(6);
  ZY_Hists[1][0]->Draw("COLZ");
  c_PositionResY_1mm->cd(7);
  EdY_Hists[1][0]->Draw("COlZ");
  c_PositionResY_1mm->cd(8);  
  YResOffsVals[1][0]->Draw("histerr");
  c_PositionResY_1mm->cd(9);
  YResMeanVals[1][0]->Draw("histerr");
  PResFits[0][1][1][0]->SetRange(0,0.5);
  PResFits[0][1][1][0]->Draw("SAME");
  c_PositionResY_1mm->cd(10);
  YResSigVals[1][0]->Draw("histerr");
  PResFits[1][1][1][0]->SetRange(0,0.5);
  PResFits[1][1][1][0]->Draw("SAME");
  c_PositionResY_1mm->Print(Output_Name+".pdf");

  TCanvas *c_PositionResX_3mm = new TCanvas("c_PositionResX_3mm", "", 100, 0, 2560, 1920);
  c_PositionResX_3mm->Divide(5,2);
  c_PositionResX_3mm->cd(1);
  ZX_Hists[0][1]->Draw("COLZ");
  c_PositionResX_3mm->cd(2);
  EdX_Hists[0][1]->Draw("COlZ");
  c_PositionResX_3mm->cd(3);
  XResOffsVals[0][1]->Draw("histerr");
  c_PositionResX_3mm->cd(4);
  XResMeanVals[0][1]->Draw("histerr");
  PResFits[0][0][0][1]->SetRange(0,0.5); // Fit/X Y /Top Bottom/Pixel
  PResFits[0][0][0][1]->Draw("SAME");
  c_PositionResX_3mm->cd(5);
  XResSigVals[0][1]->Draw("histerr");
  PResFits[1][0][0][1]->SetRange(0,0.5);
  PResFits[1][0][0][1]->Draw("SAME");
  c_PositionResX_3mm->cd(6);
  ZX_Hists[1][1]->Draw("COLZ");
  c_PositionResX_3mm->cd(7);
  EdX_Hists[1][1]->Draw("COlZ");
  c_PositionResX_3mm->cd(8);  
  XResOffsVals[1][1]->Draw("histerr");
  c_PositionResX_3mm->cd(9);
  XResMeanVals[1][1]->Draw("histerr");
  PResFits[0][0][1][1]->SetRange(0,0.5); // Fit/X Y /Top Bottom/Pixel
  PResFits[0][0][1][1]->Draw("SAME");
  c_PositionResX_3mm->cd(10);
  XResSigVals[1][1]->Draw("histerr");
  PResFits[1][0][1][1]->SetRange(0,0.5);
  PResFits[1][0][1][1]->Draw("SAME");
  c_PositionResX_3mm->Print(Output_Name+".pdf");
  
  TCanvas *c_PositionResY_3mm = new TCanvas("c_PositionResY_3mm", "", 100, 0, 2560, 1920);
  c_PositionResY_3mm->Divide(5,2);
  c_PositionResY_3mm->cd(1);
  ZY_Hists[0][1]->Draw("COLZ");
  c_PositionResY_3mm->cd(2);
  EdY_Hists[0][1]->Draw("COlZ");
  c_PositionResY_3mm->cd(3);
  YResOffsVals[0][1]->Draw("histerr");
  c_PositionResY_3mm->cd(4);
  YResMeanVals[0][1]->Draw("histerr");
  PResFits[0][1][0][1]->SetRange(0,0.5); // Fit/X Y /Top Bottom/Pixel
  PResFits[0][1][0][1]->Draw("SAME");
  c_PositionResY_3mm->cd(5);
  YResSigVals[0][1]->Draw("histerr");
  PResFits[1][1][0][1]->SetRange(0,0.5);
  PResFits[1][1][0][1]->Draw("SAME");
  c_PositionResY_3mm->cd(6);
  ZY_Hists[1][1]->Draw("COLZ");
  c_PositionResY_3mm->cd(7);
  EdY_Hists[1][1]->Draw("COlZ");
  c_PositionResY_3mm->cd(8);  
  YResOffsVals[1][1]->Draw("histerr");
  c_PositionResY_3mm->cd(9);
  YResMeanVals[1][1]->Draw("histerr");
  PResFits[0][1][1][1]->SetRange(0,0.5); // Fit/X Y /Top Bottom/Pixel
  PResFits[0][1][1][1]->Draw("SAME");
  c_PositionResY_3mm->cd(10);
  YResSigVals[1][1]->Draw("histerr");
  PResFits[1][1][1][1]->SetRange(0,0.5);
  PResFits[1][1][1][1]->Draw("SAME");
  c_PositionResY_3mm->Print(Output_Name+".pdf");

  TCanvas *c_PositionResX_9mm = new TCanvas("c_PositionResX_9mm", "", 100, 0, 2560, 1920);
  c_PositionResX_9mm->Divide(5,2);
  c_PositionResX_9mm->cd(1);
  ZX_Hists[0][2]->Draw("COLZ");
  c_PositionResX_9mm->cd(2);
  EdX_Hists[0][2]->Draw("COlZ");
  c_PositionResX_9mm->cd(3);
  XResOffsVals[0][2]->Draw("histerr");
  c_PositionResX_9mm->cd(4);
  XResMeanVals[0][2]->Draw("histerr");
  PResFits[0][0][0][2]->SetRange(0,0.5); // Fit/X Y /Top Bottom/Pixel
  PResFits[0][0][0][2]->Draw("SAME");
  c_PositionResX_9mm->cd(5);
  XResSigVals[0][2]->Draw("histerr");
  PResFits[1][0][0][2]->SetRange(0,0.5);
  PResFits[1][0][0][2]->Draw("SAME");
  c_PositionResX_9mm->cd(6);
  ZX_Hists[1][2]->Draw("COLZ");
  c_PositionResX_9mm->cd(7);
  EdX_Hists[1][2]->Draw("COlZ");
  c_PositionResX_9mm->cd(8);  
  XResOffsVals[1][2]->Draw("histerr");
  c_PositionResX_9mm->cd(9);
  XResMeanVals[1][2]->Draw("histerr");
  PResFits[0][0][1][2]->SetRange(0,0.5); // Fit/X Y /Top Bottom/Pixel
  PResFits[0][0][1][2]->Draw("SAME");
  c_PositionResX_9mm->cd(10);
  XResSigVals[1][2]->Draw("histerr");
  PResFits[1][0][1][2]->SetRange(0,0.5);
  PResFits[1][0][1][2]->Draw("SAME");
  c_PositionResX_9mm->Print(Output_Name+".pdf");
  
  TCanvas *c_PositionResY_9mm = new TCanvas("c_PositionResY_9mm", "", 100, 0, 2560, 1920);
  c_PositionResY_9mm->Divide(5,2);
  c_PositionResY_9mm->cd(1);
  ZY_Hists[0][2]->Draw("COLZ");
  c_PositionResY_9mm->cd(2);
  EdY_Hists[0][2]->Draw("COlZ");
  c_PositionResY_9mm->cd(3);
  YResOffsVals[0][2]->Draw("histerr");
  c_PositionResY_9mm->cd(4);
  YResMeanVals[0][2]->Draw("histerr");
  PResFits[0][1][0][2]->SetRange(0,0.5); // Fit/X Y /Top Bottom/Pixel
  PResFits[0][1][0][2]->Draw("SAME");
  c_PositionResY_9mm->cd(5);
  YResSigVals[0][2]->Draw("histerr");
  PResFits[1][1][0][2]->SetRange(0,0.5);
  PResFits[1][1][0][2]->Draw("SAME");
  c_PositionResY_9mm->cd(6);
  ZY_Hists[1][2]->Draw("COLZ");
  c_PositionResY_9mm->cd(7);
  EdY_Hists[1][2]->Draw("COlZ");
  c_PositionResY_9mm->cd(8);  
  YResOffsVals[1][2]->Draw("histerr");
  c_PositionResY_9mm->cd(9);
  YResMeanVals[1][2]->Draw("histerr");
  PResFits[0][1][1][2]->SetRange(0,0.5); // Fit/X Y /Top Bottom/Pixel
  PResFits[0][1][1][2]->Draw("SAME");
  c_PositionResY_9mm->cd(10);
  YResSigVals[1][2]->Draw("histerr");
  PResFits[1][1][1][2]->SetRange(0,0.5);
  PResFits[1][1][1][2]->Draw("SAME");
  c_PositionResY_9mm->Print(Output_Name+".pdf"+")");
  
  // Position resolution plots
  // Moliere radius plots
  
  InFile->Close();

}
