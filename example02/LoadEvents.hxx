//////////////////////////////////////////////////////////////////////////
//                            LoadEvents.hxx                     //
// =====================================================================//
//                                                                      //
//   LoadEvents from ROOT files or from CSV.                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// _______________________Includes_______________________________________//
///////////////////////////////////////////////////////////////////////////

#include "TLorentzVector.h"
#include "TNtuple.h"
#include "TFile.h"
#include "TROOT.h"
#include "TTree.h"
#include <sstream>
#include <fstream>
#include <map>
#include "TRandom3.h"

//////////////////////////////////////////////////////////////////////////
// ______________________Load Info From_ROOT___________________________//
/////////////////////////////////////////////////////////////////////////

void loadEvents(std::vector<Event*>& events, std::vector<std::string>& useWhichVars, TString infilename, std::string sample, std::string binvar, float Nbins, float binvarmin, float binvarmax, double weight)// std::map<std::string, std::map<std::string, float> >& WeightsDict, std::map<std::string, float>& WeightsFloat)
{
    // Load information from an input NTuple
    TFile* infile = new TFile(infilename);
    TTree* tree = (TTree*) infile->Get("ljmet");
    std::cout << "  /// Loading training events from " << infilename << std::endl;

    // nonfeature variables that we always need
    Int_t is_signal;
    Float_t binWidth, binvarval;//, weight;

    binWidth = (binvarmax-binvarmin)/Nbins;
    
    is_signal = 0;
    if (sample.find("TTM")!=std::string::npos || sample.find("BBM")!=std::string::npos ) {is_signal = 1; std::cout<<"signal"<<std::endl;}
    else if (sample.find("Data")!=std::string::npos) {is_signal = -1; std::cout<<"data"<<std::endl;}
    else std::cout<<"bkg"<<std::endl;
    std::cout << weight << std::endl; 
    std::cout << "declare variables" <<std::endl;

    std::map<std::string, Int_t> intMap;
    std::map<std::string, Float_t> floatMap;
    std::map<std::string, Double_t> doubleMap;

    std::vector<std::string> intVar{"NJets_JetSubCalc", "NJetsAK8_JetSubCalc", "NJetsCSV_JetSubCalc", "NJetsH1btagged", "NJetsWtagged_0p6", "isElectron", "isMuon"};
    std::vector<std::string> floatVar{"AK4HT", "leptonPt_singleLepCalc", "leptonEta_singleLepCalc","minDR_lepJet", "ptRel_lepJet", "minDR_leadAK8otherAK8"};
    std::vector<std::string> doubleVar{"corr_met_singleLepCalc"};
    std::vector<double> *theJetPt_JetSubCalc_PtOrdered=0;
    std::vector<double> *theJetAK8Pt_JetSubCalc_PtOrdered=0;
    TBranch *b_theJetPt_JetSubCalc_PtOrdered=0;
    TBranch *b_theJetAK8Pt_JetSubCalc_PtOrdered=0;

    // tell the ttree where to load the variables
    tree->SetBranchAddress("theJetPt_JetSubCalc_PtOrdered", &theJetPt_JetSubCalc_PtOrdered, &b_theJetPt_JetSubCalc_PtOrdered);
    tree->SetBranchAddress("theJetAK8Pt_JetSubCalc_PtOrdered", &theJetAK8Pt_JetSubCalc_PtOrdered, &b_theJetAK8Pt_JetSubCalc_PtOrdered);
    for(unsigned int i = 0; i< intVar.size(); i++) tree->SetBranchAddress(intVar.at(i).c_str(), &intMap[intVar.at(i)]);
    for(unsigned int i = 0; i< floatVar.size(); i++) tree->SetBranchAddress(floatVar.at(i).c_str(), &floatMap[floatVar.at(i)]);
    for(unsigned int i = 0; i< doubleVar.size(); i++) tree->SetBranchAddress(doubleVar.at(i).c_str(), &doubleMap[doubleVar.at(i)]);
    std::cout<<"load odd number events"<<std::endl;
    int countN = 0;
    Long64_t NEvt = tree->GetEntries();
    for(Long64_t i=0; i< NEvt; i++){
      if( i%2 == 0) continue;

      tree->GetEntry(i);
      //std::cout<<"Get Entry "<< i <<std::endl;

      //cuts on 1l events
      if (floatMap["leptonPt_singleLepCalc"] > 60. && ((intMap["isElectron"] && abs(floatMap["leptonEta_singleLepCalc"]) <2.5) || (intMap["isMuon"] && abs(floatMap["leptonEta_singleLepCalc"]) <2.4)) && doubleMap["corr_met_singleLepCalc"] > 75. && (floatMap["minDR_lepJet"] > 0.4 || floatMap["ptRel_lepJet"] > 40)) {}
      else continue;
      //cuts on Jets
      if (intMap["NJets_JetSubCalc"] >= 3 && theJetPt_JetSubCalc_PtOrdered->at(0) > 300. && theJetPt_JetSubCalc_PtOrdered->at(1) > 150. && theJetPt_JetSubCalc_PtOrdered->at(2) > 100.) {}
      else continue;
      //cuts on JetAK8
      if (intMap["NJetsAK8_JetSubCalc"] >=2 && theJetAK8Pt_JetSubCalc_PtOrdered->at(0)>200. && theJetAK8Pt_JetSubCalc_PtOrdered->at(1) >200.) {} 
      else continue;
      //cuts on SR
      if (floatMap["minDR_leadAK8otherAK8"] > 0.8 && floatMap["minDR_leadAK8otherAK8"] < 3.0) {} 
      else continue;

      Event* e = new Event();
      e->bin = (binvarval - binvarmin)/binWidth;
      if(e->bin >= Nbins) e->bin = Nbins-1;
      e->bin = 0;
      //std::cout<< e->bin <<std::endl;
      e->data = std::vector<double>();
      e->data.push_back(0); //dummy variable (not used)
      e->trueValue = is_signal;
      e->weight = weight;
      e->id = i;
      for(auto& ifeature : useWhichVars){
        if(intMap.count(ifeature)>0) {e->data.push_back(intMap[ifeature]); }//std::cout<<"loading feature "<<ifeature<<" with value "<< intMap[ifeature]<<std::endl;}
        else if (floatMap.count(ifeature)>0) e->data.push_back(floatMap[ifeature]);
      }
      int isEM = -1;
      if (intMap["isElectron"]) isEM = 1;
      else if (intMap["isMuon"]) isEM = 0;
      e->data.push_back(isEM);
      events.push_back(e);
      //delete e;
    }
    delete infile;
}

