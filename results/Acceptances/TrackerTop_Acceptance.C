#ifdef __CLING__
#pragma cling optimize(0)
#endif
void TrackerTop_Acceptance()
{
//=========Macro generated from canvas: Canvas_1/Canvas_1
//=========  (Thu Apr 27 14:36:11 2023) by ROOT version 6.26/10
   TCanvas *Canvas_1 = new TCanvas("Canvas_1", "Canvas_1",1400,633,910,569);
   Canvas_1->Range(-6.224208,-458.2348,56.22421,4099.996);
   Canvas_1->SetFillColor(0);
   Canvas_1->SetBorderMode(0);
   Canvas_1->SetBorderSize(2);
   Canvas_1->SetFrameBorderMode(0);
   Canvas_1->SetFrameBorderMode(0);
   
   TH1D *hTrackerTop_Acceptance__5 = new TH1D("hTrackerTop_Acceptance__5","Top tracker electron acceptance",2500,0,50);
   hTrackerTop_Acceptance__5->SetBinContent(201,194);
   hTrackerTop_Acceptance__5->SetBinContent(251,1208);
   hTrackerTop_Acceptance__5->SetBinContent(301,1846);
   hTrackerTop_Acceptance__5->SetBinContent(351,2258);
   hTrackerTop_Acceptance__5->SetBinContent(401,2549);
   hTrackerTop_Acceptance__5->SetBinContent(451,2786);
   hTrackerTop_Acceptance__5->SetBinContent(501,2994);
   hTrackerTop_Acceptance__5->SetBinContent(551,3151);
   hTrackerTop_Acceptance__5->SetBinContent(601,3288);
   hTrackerTop_Acceptance__5->SetBinContent(651,3393);
   hTrackerTop_Acceptance__5->SetBinContent(701,3476);
   hTrackerTop_Acceptance__5->SetBinContent(751,3433);
   hTrackerTop_Acceptance__5->SetBinContent(801,3176);
   hTrackerTop_Acceptance__5->SetBinContent(851,2932);
   hTrackerTop_Acceptance__5->SetBinContent(901,2770);
   hTrackerTop_Acceptance__5->SetBinContent(951,2615);
   hTrackerTop_Acceptance__5->SetBinContent(1001,2486);
   hTrackerTop_Acceptance__5->SetBinContent(1051,2359);
   hTrackerTop_Acceptance__5->SetBinContent(1101,2232);
   hTrackerTop_Acceptance__5->SetBinContent(1151,2123);
   hTrackerTop_Acceptance__5->SetBinContent(1201,2071);
   hTrackerTop_Acceptance__5->SetBinContent(1251,1972);
   hTrackerTop_Acceptance__5->SetBinContent(1301,1895);
   hTrackerTop_Acceptance__5->SetBinContent(1351,1848);
   hTrackerTop_Acceptance__5->SetBinContent(1401,1818);
   hTrackerTop_Acceptance__5->SetBinContent(1451,1748);
   hTrackerTop_Acceptance__5->SetBinContent(1501,1690);
   hTrackerTop_Acceptance__5->SetBinContent(1551,1649);
   hTrackerTop_Acceptance__5->SetBinContent(1601,1612);
   hTrackerTop_Acceptance__5->SetBinContent(1651,1572);
   hTrackerTop_Acceptance__5->SetBinContent(1701,1542);
   hTrackerTop_Acceptance__5->SetBinContent(1751,1505);
   hTrackerTop_Acceptance__5->SetEntries(72191);
   hTrackerTop_Acceptance__5->Scale(1/5000.);
   
   TPaveStats *ptstats = new TPaveStats(0.78,0.775,0.98,0.935,"brNDC");
   ptstats->SetName("stats");
   ptstats->SetBorderSize(1);
   ptstats->SetFillColor(0);
   ptstats->SetTextAlign(12);
   ptstats->SetTextFont(42);
   TText *ptstats_LaTex = ptstats->AddText("hTrackerTop_Acceptance");
   ptstats_LaTex->SetTextSize(0.0368);
   ptstats_LaTex = ptstats->AddText("Entries = 72191  ");
   ptstats_LaTex = ptstats->AddText("Mean  =  18.49");
   ptstats_LaTex = ptstats->AddText("Std Dev   =  8.218");
   ptstats->SetOptStat(1111);
   ptstats->SetOptFit(0);
   ptstats->Draw();
   hTrackerTop_Acceptance__5->GetListOfFunctions()->Add(ptstats);
   ptstats->SetParent(hTrackerTop_Acceptance__5);

   Int_t ci;      // for color index setting
   TColor *color; // for color definition with alpha
   ci = TColor::GetColor("#000099");
   hTrackerTop_Acceptance__5->SetLineColor(ci);
   hTrackerTop_Acceptance__5->GetXaxis()->SetTitle("E_{#gamma} (GeV)");
   hTrackerTop_Acceptance__5->GetXaxis()->SetLabelFont(42);
   hTrackerTop_Acceptance__5->GetXaxis()->SetTitleOffset(1);
   hTrackerTop_Acceptance__5->GetXaxis()->SetTitleFont(42);
   hTrackerTop_Acceptance__5->GetYaxis()->SetTitle("Acceptance");
   hTrackerTop_Acceptance__5->GetYaxis()->SetLabelFont(42);
   hTrackerTop_Acceptance__5->GetYaxis()->SetTitleFont(42);
   hTrackerTop_Acceptance__5->GetZaxis()->SetLabelFont(42);
   hTrackerTop_Acceptance__5->GetZaxis()->SetTitleOffset(1);
   hTrackerTop_Acceptance__5->GetZaxis()->SetTitleFont(42);
   hTrackerTop_Acceptance__5->Draw("");
   
   TPaveText *pt = new TPaveText(0.2449559,0.9336243,0.7550441,0.995,"blNDC");
   pt->SetName("title");
   pt->SetBorderSize(0);
   pt->SetFillColor(0);
   pt->SetFillStyle(0);
   pt->SetTextFont(42);
   TText *pt_LaTex = pt->AddText("Top tracker electron acceptance");
   pt->Draw();
   Canvas_1->Modified();
   Canvas_1->cd();
   Canvas_1->SetSelected(Canvas_1);
}
