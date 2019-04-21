//BinMap.hxx
#include <map>

void loadBinMap(std::map<std::string, std::vector<float> >& BinMap){
  std::vector<float> bin_vec = {0., 5000., 50.};
  BinMap["AK4HT"] = (std::vector<float>) {0., 5000., 50.};
  BinMap["topMass"] = (std::vector<float>) {0,1500,50};
}
