#ifdef __CLING__
#pragma cling optimize(0)
#endif
void Trackercoincidence_Acceptance()
{
//=========Macro generated from canvas: Canvas_1/Canvas_1
//=========  (Thu Apr 27 14:36:22 2023) by ROOT version 6.26/10
   TCanvas *Canvas_1 = new TCanvas("Canvas_1", "Canvas_1",1400,633,910,569);
   Canvas_1->Range(-6.224208,-339.8531,56.22421,3040.791);
   Canvas_1->SetFillColor(0);
   Canvas_1->SetBorderMode(0);
   Canvas_1->SetBorderSize(2);
   Canvas_1->SetFrameBorderMode(0);
   Canvas_1->SetFrameBorderMode(0);
   
   TH1D *hTrackerCoincidence_Acceptance__6 = new TH1D("hTrackerCoincidence_Acceptance__6","Tracker coincidence acceptance",2500,0,50);
   hTrackerCoincidence_Acceptance__6->SetBinContent(401,149);
   hTrackerCoincidence_Acceptance__6->SetBinContent(451,586);
   hTrackerCoincidence_Acceptance__6->SetBinContent(501,1005);
   hTrackerCoincidence_Acceptance__6->SetBinContent(551,1307);
   hTrackerCoincidence_Acceptance__6->SetBinContent(601,1569);
   hTrackerCoincidence_Acceptance__6->SetBinContent(651,1821);
   hTrackerCoincidence_Acceptance__6->SetBinContent(701,2006);
   hTrackerCoincidence_Acceptance__6->SetBinContent(751,2180);
   hTrackerCoincidence_Acceptance__6->SetBinContent(801,2328);
   hTrackerCoincidence_Acceptance__6->SetBinContent(851,2468);
   hTrackerCoincidence_Acceptance__6->SetBinContent(901,2578);
   hTrackerCoincidence_Acceptance__6->SetBinContent(951,2448);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1001,2127);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1051,1807);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1101,1505);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1151,1243);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1201,1007);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1251,787);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1301,563);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1351,401);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1401,260);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1451,88);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1501,26);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1551,30);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1601,25);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1651,14);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1701,17);
   hTrackerCoincidence_Acceptance__6->SetBinContent(1751,20);
   hTrackerCoincidence_Acceptance__6->SetEntries(30365);
   hTrackerCoincidence_Acceptance__6->Scale(1/5000.);
   
   TPaveStats *ptstats = new TPaveStats(0.78,0.775,0.98,0.935,"brNDC");
   ptstats->SetName("stats");
   ptstats->SetBorderSize(1);
   ptstats->SetFillColor(0);
   ptstats->SetTextAlign(12);
   ptstats->SetTextFont(42);
   TText *ptstats_LaTex = ptstats->AddText("hTrackerCoincidence_Acceptance");
   ptstats_LaTex->SetTextSize(0.0368);
   ptstats_LaTex = ptstats->AddText("Entries = 30365  ");
   ptstats_LaTex = ptstats->AddText("Mean  =  17.52");
   ptstats_LaTex = ptstats->AddText("Std Dev   =  4.585");
   ptstats->SetOptStat(1111);
   ptstats->SetOptFit(0);
   ptstats->Draw();
   hTrackerCoincidence_Acceptance__6->GetListOfFunctions()->Add(ptstats);
   ptstats->SetParent(hTrackerCoincidence_Acceptance__6);

   Int_t ci;      // for color index setting
   TColor *color; // for color definition with alpha
   ci = TColor::GetColor("#000099");
   hTrackerCoincidence_Acceptance__6->SetLineColor(ci);
   hTrackerCoincidence_Acceptance__6->GetXaxis()->SetTitle("E_{#gamma} (GeV)");
   hTrackerCoincidence_Acceptance__6->GetXaxis()->SetLabelFont(42);
   hTrackerCoincidence_Acceptance__6->GetXaxis()->SetTitleOffset(1);
   hTrackerCoincidence_Acceptance__6->GetXaxis()->SetTitleFont(42);
   hTrackerCoincidence_Acceptance__6->GetYaxis()->SetTitle("Acceptance");
   hTrackerCoincidence_Acceptance__6->GetYaxis()->SetLabelFont(42);
   hTrackerCoincidence_Acceptance__6->GetYaxis()->SetTitleFont(42);
   hTrackerCoincidence_Acceptance__6->GetZaxis()->SetLabelFont(42);
   hTrackerCoincidence_Acceptance__6->GetZaxis()->SetTitleOffset(1);
   hTrackerCoincidence_Acceptance__6->GetZaxis()->SetTitleFont(42);
   hTrackerCoincidence_Acceptance__6->Draw("");
   
   TPaveText *pt = new TPaveText(0.2444053,0.9336243,0.7555947,0.995,"blNDC");
   pt->SetName("title");
   pt->SetBorderSize(0);
   pt->SetFillColor(0);
   pt->SetFillStyle(0);
   pt->SetTextFont(42);
   TText *pt_LaTex = pt->AddText("Tracker coincidence acceptance");
   pt->Draw();
   Canvas_1->Modified();
   Canvas_1->cd();
   Canvas_1->SetSelected(Canvas_1);
}
