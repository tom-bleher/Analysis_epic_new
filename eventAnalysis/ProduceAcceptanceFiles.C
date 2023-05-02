{

  TFile *f = new TFile("output/MergedOutput.root","READ");
  TH1D *CALTop_Acceptance = (TH1D*)f->Get("LumiSpecCAL/hCALTop_Acceptance");
  TH1D *CALCoincidence_Acceptance = (TH1D*)f->Get("LumiSpecCAL/hCALCoincidence_Acceptance");
  TH1D *TrackerTop_Acceptance = (TH1D*)f->Get("LumiTracker/hTrackerTop_Acceptance");
  TH1D *TrackerCoincidence_Acceptance = (TH1D*)f->Get("LumiTracker/hTrackerCoincidence_Acceptance");

  TH1D *GenEventCount = (TH1D*)f->Get("hGenEventCount");

  CALTop_Acceptance->Divide(GenEventCount);
  CALCoincidence_Acceptance->Divide(GenEventCount);
  TrackerTop_Acceptance->Divide(GenEventCount);
  TrackerCoincidence_Acceptance->Divide(GenEventCount);

  TFile *fout = new TFile("Acceptances.root","RECREATE");

  CALTop_Acceptance->Write();
  CALCoincidence_Acceptance->Write();
  TrackerTop_Acceptance->Write();
  TrackerCoincidence_Acceptance->Write();

}
