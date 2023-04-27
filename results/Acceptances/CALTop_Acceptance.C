#ifdef __CLING__
#pragma cling optimize(0)
#endif
void CALTop_Acceptance()
{
//=========Macro generated from canvas: Canvas_1/Canvas_1
//=========  (Thu Apr 27 14:42:06 2023) by ROOT version 6.26/10
   TCanvas *Canvas_1 = new TCanvas("Canvas_1", "Canvas_1",1400,633,538,326);
   Canvas_1->Range(-6.25,-467.3813,56.25,4206.431);
   Canvas_1->SetFillColor(0);
   Canvas_1->SetBorderMode(0);
   Canvas_1->SetBorderSize(2);
   Canvas_1->SetFrameBorderMode(0);
   Canvas_1->SetFrameBorderMode(0);
   
   TH1D *hCALTop_Acceptance__7 = new TH1D("hCALTop_Acceptance__7","CAL acceptance",2500,0,50);
   hCALTop_Acceptance__7->SetBinContent(151,1);
   hCALTop_Acceptance__7->SetBinContent(201,174);
   hCALTop_Acceptance__7->SetBinContent(251,1191);
   hCALTop_Acceptance__7->SetBinContent(301,1836);
   hCALTop_Acceptance__7->SetBinContent(351,2254);
   hCALTop_Acceptance__7->SetBinContent(401,2546);
   hCALTop_Acceptance__7->SetBinContent(451,2789);
   hCALTop_Acceptance__7->SetBinContent(501,2994);
   hCALTop_Acceptance__7->SetBinContent(551,3151);
   hCALTop_Acceptance__7->SetBinContent(601,3288);
   hCALTop_Acceptance__7->SetBinContent(651,3395);
   hCALTop_Acceptance__7->SetBinContent(701,3478);
   hCALTop_Acceptance__7->SetBinContent(751,3561);
   hCALTop_Acceptance__7->SetBinContent(801,3375);
   hCALTop_Acceptance__7->SetBinContent(851,3131);
   hCALTop_Acceptance__7->SetBinContent(901,2927);
   hCALTop_Acceptance__7->SetBinContent(951,2776);
   hCALTop_Acceptance__7->SetBinContent(1001,2619);
   hCALTop_Acceptance__7->SetBinContent(1051,2493);
   hCALTop_Acceptance__7->SetBinContent(1101,2402);
   hCALTop_Acceptance__7->SetBinContent(1151,2266);
   hCALTop_Acceptance__7->SetBinContent(1201,2183);
   hCALTop_Acceptance__7->SetBinContent(1251,2090);
   hCALTop_Acceptance__7->SetBinContent(1301,2030);
   hCALTop_Acceptance__7->SetBinContent(1351,1962);
   hCALTop_Acceptance__7->SetBinContent(1401,1899);
   hCALTop_Acceptance__7->SetBinContent(1451,1838);
   hCALTop_Acceptance__7->SetBinContent(1501,1804);
   hCALTop_Acceptance__7->SetBinContent(1551,1743);
   hCALTop_Acceptance__7->SetBinContent(1601,1685);
   hCALTop_Acceptance__7->SetBinContent(1651,1645);
   hCALTop_Acceptance__7->SetBinContent(1701,1628);
   hCALTop_Acceptance__7->SetBinContent(1751,1587);
   hCALTop_Acceptance__7->SetEntries(74741);
   hCALTop_Acceptance__7->Scale(1/5000.);
   
   TPaveStats *ptstats = new TPaveStats(0.78,0.775,0.98,0.935,"brNDC");
   ptstats->SetName("stats");
   ptstats->SetBorderSize(1);
   ptstats->SetFillColor(0);
   ptstats->SetTextAlign(12);
   ptstats->SetTextFont(42);
   TText *ptstats_LaTex = ptstats->AddText("hCALTop_Acceptance");
   ptstats_LaTex->SetTextSize(0.0368);
   ptstats_LaTex = ptstats->AddText("Entries = 74741  ");
   ptstats_LaTex = ptstats->AddText("Mean  =  18.67");
   ptstats_LaTex = ptstats->AddText("Std Dev   =  8.192");
   ptstats->SetOptStat(1111);
   ptstats->SetOptFit(0);
   ptstats->Draw();
   hCALTop_Acceptance__7->GetListOfFunctions()->Add(ptstats);
   ptstats->SetParent(hCALTop_Acceptance__7);

   Int_t ci;      // for color index setting
   TColor *color; // for color definition with alpha
   ci = TColor::GetColor("#000099");
   hCALTop_Acceptance__7->SetLineColor(ci);
   hCALTop_Acceptance__7->GetXaxis()->SetTitle("E_{#gamma} (GeV)");
   hCALTop_Acceptance__7->GetXaxis()->SetLabelFont(42);
   hCALTop_Acceptance__7->GetXaxis()->SetTitleOffset(1);
   hCALTop_Acceptance__7->GetXaxis()->SetTitleFont(42);
   hCALTop_Acceptance__7->GetYaxis()->SetTitle("Acceptance");
   hCALTop_Acceptance__7->GetYaxis()->SetLabelFont(42);
   hCALTop_Acceptance__7->GetYaxis()->SetTitleFont(42);
   hCALTop_Acceptance__7->GetZaxis()->SetLabelFont(42);
   hCALTop_Acceptance__7->GetZaxis()->SetTitleOffset(1);
   hCALTop_Acceptance__7->GetZaxis()->SetTitleFont(42);
   hCALTop_Acceptance__7->Draw("");
   
   TPaveText *pt = new TPaveText(0.3668657,0.932037,0.6331343,0.995,"blNDC");
   pt->SetName("title");
   pt->SetBorderSize(0);
   pt->SetFillColor(0);
   pt->SetFillStyle(0);
   pt->SetTextFont(42);
   TText *pt_LaTex = pt->AddText("CAL acceptance");
   pt->Draw();
   Canvas_1->Modified();
   Canvas_1->cd();
   Canvas_1->SetSelected(Canvas_1);
}
