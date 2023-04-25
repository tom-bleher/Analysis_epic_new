{

gStyle->SetOptStat(0);

 vector<string> files = {"eicrecon_ideal_YesBcorr.root","eicrecon_Air_5um.root","eicrecon_Air_50um.root","eicrecon_Air_500um.root", "eicrecon_Air_500um_CuLayer.root", "eicrecon_Air_500um_CuLayer_20cmTrackerSpacing.root", "eicrecon_Air_ACLGAD_FrontConverter.root", "eicrecon_Air_ACLGAD_PVphotons.root"};

  vector<tuple<double,double>> sigmas;

  for(auto fname : files){
     string path = "/home/dhevan/eic/Analysis_epic/EICreconPlugins/" + fname;
     cout<<"File: "<<path<<endl;
     TFile *f = new TFile(path.data(),"READ");
     TH1D *hist = (TH1D*)f->Get("hTrackers_Y");
     hist->GetXaxis()->SetRangeUser(-10,10);
     sigmas.push_back( {hist->GetRMS(), hist->GetRMSError()} );
     cout<<get<0>(sigmas.back())<<endl;
     f->Close();
  }


  TCanvas *c1 = new TCanvas("c1","c1",10,10,700,500);
   c1->SetGrid();
   c1->SetTopMargin(0.15);
 
  gPad->DrawFrame(0.,0.,35,0.55);

  const Int_t nx = 6;
  const char *configs[nx] = {"1 Tracker", "2 Trackers", "3 Trackers", "Air Chamber", "He Chamber", "Beam effects"};

   TH1F *h = new TH1F("h","Tracker resolutions",3,0,3);
   h->SetStats(0);
   h->SetFillColor(38);
   h->SetCanExtend(TH1::kAllAxes);
   for (Int_t bin=1; bin < nx+1; bin++) {
      h->SetBinContent(bin, y[bin-1]); 
      h->SetBinError(bin, y_e[bin-1]); 
      h->GetXaxis()->SetBinLabel(bin, configs[bin-1]);
   }
   h->GetYaxis()->SetTitle("CAL #sigma");
   //h->GetYaxis()->SetTitle("E_{rec} rms");
   //h->GetYaxis()->SetTitle("<E_{rec}>");
   h->SetTitle("CAL");
   //h->SetTitle("Trackers");
   h->GetXaxis()->SetLabelSize(0.06);
   h->GetYaxis()->SetTitleSize(0.06);
   h->GetYaxis()->SetTitleOffset(0.8);
   //h->LabelsDeflate();
   h->Draw();
   //TPaveText *pt = new TPaveText(0.7,0.85,0.98,0.98,"brNDC");
   //pt->SetFillColor(18);
   //pt->SetTextAlign(12);
   //pt->AddText("Use the axis Context Menu LabelsOption");
   //pt->AddText(" \"a\"   to sort by alphabetic order");
   //pt->AddText(" \">\"   to sort by decreasing values");
   //pt->AddText(" \"<\"   to sort by increasing values");
   //pt->Draw();
   //return c1;
 

  gPad->SetGridx();
  gPad->SetGridy();


 
}
