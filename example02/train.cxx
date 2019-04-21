//////////////////////////////////////////////////////////////////////////
//                            BasicTrainAndTest.cxx                     //
// =====================================================================//
//                                                                      //
//   Train the forest, save the trees, and test the results.            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// _______________________Includes_______________________________________//
///////////////////////////////////////////////////////////////////////////

#include "CategoryReader.h"
#include "Tree.h"
#include "SignificanceMetrics.hxx"
#include "TRandom3.h"
#include "TLorentzVector.h"
#include "TNtuple.h"
#include "TFile.h"
#include <TROOT.h>
#include <TChain.h>
#include <sstream>
#include <fstream>
#include <map>

#include "LoadEvents.hxx"
#include "weights.hxx"
#include "BinMap.hxx"
#include "samples_test.hxx"
//////////////////////////////////////////////////////////////////////////
// ______________________Categorization_Settings _______________________//
/////////////////////////////////////////////////////////////////////////

// Set the settings for the regression here. You may choose to overwrite these values in
// main if you want input from the terminal to determine the settings.

// Fundamental settings for the regression.
Int_t nodes = 16;
int nbins = 50;
int unctype = 0;                   // type of uncertainty to use (defined in SignificanceMetrics.hxx)
double nparams = 1;                // input needed when considering a special uncertainty in the significance metrics
bool scale_data = false;           // used ratio of data/mc in control region to scale bkg mc in signal region 
bool scale_fluctuations = false;   // analytic estimate of bkg expected in signal region based upon bkg in control region
bool smooth = false;                // estimate bkg in a bin using avg of bkg in that bin + bkg in neighboring bins
int nbkgmin = 100;
SignificanceMetric* sm = new PoissonSignificance(unctype, nparams, nbkgmin, scale_fluctuations, scale_data, smooth);
// Whether to save the trees from the regression into a directory specified later.
bool saveTree = true;

// Where to save the trees.
TString treeDirectory("./");

// Where to get samples. Note that sample list is loaded in BuildTree from samples.hxx
TString inputdir("/eos/uscms/store/user/lpcljm/2016/LJMet80X_1lepTT_062617_step2slimmed/nominal/");

// Define which variables (features) and binning to use in this training
std::vector<std::string> useWhichVars{"NJetsCSV_JetSubCalc", "NJetsH1btagged", "NJetsWtagged_0p6"};
std::string useWhichBins = "topMass";

// Define the list of samples to be used
std::vector<std::string> signal_col{"TprimeTprime_M-"};
std::vector<std::string> bkg_col{"TT_", "ST_"}; //"DYJetsToLL", "WJetsToLNu_HT-","WW", "WZ", "ZZ", "QCD_HT-"};
std::vector<std::string> SelSample(signal_col);
auto it = SelSample.insert(SelSample.end(), bkg_col.begin(), bkg_col.end());

//////////////////////////////////////////////////////////////////////////
// ______________________Regression_________ ___________________________//
/////////////////////////////////////////////////////////////////////////

void buildCategorizationTree()
{
  std::map< std::string, TString> samples;
  loadSamples(samples);
  std::map<std::string, std::vector<float> > BinMap;
  loadBinMap(BinMap);
  std::map<std::string, std::map<std::string, double> > WeightsDict;
  std::map<std::string, double> WeightsDouble;
  loadWeights(WeightsDict, WeightsDouble);
  // Build a tree and save it in xml

  ///////////////////////////////////
  // Train 
  ///////////////////////////////////

  // The training and testing events.
  std::vector<Event*> trainingEvents = std::vector<Event*>();
  std::string sampleName;
  double weight;
  bool wanted;
  for(auto& sample: samples) {
    sampleName = sample.first;

    weight = 1.0;
    if (sampleName.find("Data") != std::string::npos) {}
    else weight = WeightsDict["weight"][sampleName];
    std::cout<< weight<<std::endl;

    wanted = false;
    for (auto& Process: SelSample){
      if((sample.second).Contains(TString(Process))) {wanted = true; break;}
    }
    if(wanted) loadEvents(trainingEvents, useWhichVars, inputdir+sample.second, sample.first, useWhichBins, (int) BinMap[useWhichBins][2], BinMap[useWhichBins][0], BinMap[useWhichBins][1], weight);
  }
  
  std::cout << std::endl << "Number of training events: " << trainingEvents.size() << std::endl << std::endl;
  // Initialize new forest.
  Tree* tree = new Tree(trainingEvents, (int) BinMap[useWhichBins][2]);
  useWhichVars.push_back("isEM");
  tree->setFeatureNames(useWhichVars);
  // Output the save directory to the screen
  TString savename = "tree.xml";
  // Output the parameters of the current run. 
  std::cout << "=======================================" << std::endl;
  std::cout << "Nodes              : " << nodes << std::endl;
  std::cout << "Significance Metric: " << sm->name << std::endl;
  std::cout << "Features considered: "; for(auto& iVar: useWhichVars) std::cout<< iVar<<", "; std::cout << std::endl;
  std::cout << "tree save name     : " << treeDirectory+savename << std::endl;
  std::cout << "=======================================" << std::endl;    
  // Do the regression and save the trees.
  tree->buildTree(nodes, sm);

  if(saveTree){
      std::cout << "save tree to: " << treeDirectory+savename << std::endl;
      tree->saveToXML(treeDirectory+savename);
      std::cout << "tree saved" <<std::endl;
  }
  // Rank the variable importance and output it to the screen.
  std::vector<std::string> rank;
  tree->outputVariableRanking(rank);
  delete tree;
  // ----------------------------------------------------
  ///////////////////////////////////////////////////////
  XMLCategorizer xmlc(treeDirectory+savename);
  xmlc.outputCategories();
}


//////////////////////////////////////////////////////////////////////////
// ______________________Main___________________________________________//
//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
// Run a regression with the appropriate settings.

    // Gather regression settings from the command line if you want.
    // Then you can run as ./TrainAndEvaluate setting1 setting2 ...

    // Simply overwrite the settings at the beginning
    // with those from the command line like so.

    for(int i=1; i<argc; i++)
    {
        std::stringstream ss;
        ss << argv[i];
        //if(i==1) ss >> mode;
    }

    buildCategorizationTree();
    return 0;
}
