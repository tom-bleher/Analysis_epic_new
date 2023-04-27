#ifdef __CLING__
#pragma cling optimize(0)
#endif
void CALcoincidence_Acceptance()
{
//=========Macro generated from canvas: Canvas_1/Canvas_1
//=========  (Thu Apr 27 14:35:54 2023) by ROOT version 6.26/10
   TCanvas *Canvas_1 = new TCanvas("Canvas_1", "Canvas_1",1400,633,910,569);
   Canvas_1->Range(-6.224208,-323.1109,56.22421,2890.992);
   Canvas_1->SetFillColor(0);
   Canvas_1->SetBorderMode(0);
   Canvas_1->SetBorderSize(2);
   Canvas_1->SetFrameBorderMode(0);
   Canvas_1->SetFrameBorderMode(0);
   
   TH1D *hCALCoincidence_Acceptance__4 = new TH1D("hCALCoincidence_Acceptance__4","CAL acceptance",2500,0,50);
   hCALCoincidence_Acceptance__4->SetBinContent(401,130);
   hCALCoincidence_Acceptance__4->SetBinContent(451,572);
   hCALCoincidence_Acceptance__4->SetBinContent(501,984);
   hCALCoincidence_Acceptance__4->SetBinContent(551,1295);
   hCALCoincidence_Acceptance__4->SetBinContent(601,1566);
   hCALCoincidence_Acceptance__4->SetBinContent(651,1815);
   hCALCoincidence_Acceptance__4->SetBinContent(701,2002);
   hCALCoincidence_Acceptance__4->SetBinContent(751,2180);
   hCALCoincidence_Acceptance__4->SetBinContent(801,2326);
   hCALCoincidence_Acceptance__4->SetBinContent(851,2451);
   hCALCoincidence_Acceptance__4->SetBinContent(901,2446);
   hCALCoincidence_Acceptance__4->SetBinContent(951,2353);
   hCALCoincidence_Acceptance__4->SetBinContent(1001,2113);
   hCALCoincidence_Acceptance__4->SetBinContent(1051,1945);
   hCALCoincidence_Acceptance__4->SetBinContent(1101,1771);
   hCALCoincidence_Acceptance__4->SetBinContent(1151,1492);
   hCALCoincidence_Acceptance__4->SetBinContent(1201,1234);
   hCALCoincidence_Acceptance__4->SetBinContent(1251,1016);
   hCALCoincidence_Acceptance__4->SetBinContent(1301,797);
   hCALCoincidence_Acceptance__4->SetBinContent(1351,606);
   hCALCoincidence_Acceptance__4->SetBinContent(1401,437);
   hCALCoincidence_Acceptance__4->SetBinContent(1451,273);
   hCALCoincidence_Acceptance__4->SetBinContent(1501,123);
   hCALCoincidence_Acceptance__4->SetBinContent(1551,32);
   hCALCoincidence_Acceptance__4->SetBinContent(1601,26);
   hCALCoincidence_Acceptance__4->SetBinContent(1651,19);
   hCALCoincidence_Acceptance__4->SetBinContent(1701,21);
   hCALCoincidence_Acceptance__4->SetBinContent(1751,22);
   hCALCoincidence_Acceptance__4->SetEntries(32047);
   hCALCoincidence_Acceptance__4->Scale(1/5000.);
   
   TPaveStats *ptstats = new TPaveStats(0.78,0.775,0.98,0.935,"brNDC");
   ptstats->SetName("stats");
   ptstats->SetBorderSize(1);
   ptstats->SetFillColor(0);
   ptstats->SetTextAlign(12);
   ptstats->SetTextFont(42);
   TText *ptstats_LaTex = ptstats->AddText("hCALCoincidence_Acceptance");
   ptstats_LaTex->SetTextSize(0.0368);
   ptstats_LaTex = ptstats->AddText("Entries = 32047  ");
   ptstats_LaTex = ptstats->AddText("Mean  =  18.01");
   ptstats_LaTex = ptstats->AddText("Std Dev   =   4.87");
   ptstats->SetOptStat(1111);
   ptstats->SetOptFit(0);
   ptstats->Draw();
   hCALCoincidence_Acceptance__4->GetListOfFunctions()->Add(ptstats);
   ptstats->SetParent(hCALCoincidence_Acceptance__4);

   Int_t ci;      // for color index setting
   TColor *color; // for color definition with alpha
   ci = TColor::GetColor("#000099");
   hCALCoincidence_Acceptance__4->SetLineColor(ci);
   hCALCoincidence_Acceptance__4->GetXaxis()->SetTitle("E_{#gamma} (GeV)");
   hCALCoincidence_Acceptance__4->GetXaxis()->SetLabelFont(42);
   hCALCoincidence_Acceptance__4->GetXaxis()->SetTitleOffset(1);
   hCALCoincidence_Acceptance__4->GetXaxis()->SetTitleFont(42);
   hCALCoincidence_Acceptance__4->GetYaxis()->SetTitle("Acceptance");
   hCALCoincidence_Acceptance__4->GetYaxis()->SetLabelFont(42);
   hCALCoincidence_Acceptance__4->GetYaxis()->SetTitleFont(42);
   hCALCoincidence_Acceptance__4->GetZaxis()->SetLabelFont(42);
   hCALCoincidence_Acceptance__4->GetZaxis()->SetTitleOffset(1);
   hCALCoincidence_Acceptance__4->GetZaxis()->SetTitleFont(42);
   hCALCoincidence_Acceptance__4->Draw("");
   
   TPaveText *pt = new TPaveText(0.3661013,0.9336243,0.6338987,0.995,"blNDC");
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
