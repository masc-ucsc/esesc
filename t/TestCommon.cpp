#include <iterator>
#include "TestCommon.h"
#include "GProcessor.h"
#include "TaskHandler.h"


int TestCommon::printBranchPredictorStats(FlowID id) {
  std::vector<std::string> statNames;
  statNames.push_back(std::string("nBranches"));
  statNames.push_back(std::string("nTaken"));
  statNames.push_back(std::string("nMiss"));

  FlowID maxProcessorId_ = TaskHandler::getNumCPUS();
  if (id >= maxProcessorId_ || !dynamic_cast<GProcessor*>(TaskHandler::getSimu(id))) {
    printf("ERROR: Processor %u not found\n", id);
    return ERROR;
  }
  //GProcessor *proc = dynamic_cast<GProcessor *>(TaskHandler::getSimu(i));

  std::map<std::string,GStats*> stats = TestCommon::getStats(id, statNames, "_BPred:");
  if (stats.empty()) {
    printf("ERROR: Failed to get GStats for branch predictor %u\n", id);
    return ERROR;
  }

  printf("------------- Branch Predictor %u Stats -------------\n", id);
  TestCommon::printStats(stats);

  return SUCCESS;
}


int TestCommon::printProcessorStats(FlowID id) {
  std::vector<std::string> statNames;
  statNames.push_back(std::string("clockTicks"));
  statNames.push_back(std::string("nCommitted"));
  statNames.push_back(std::string("noFetch"));
  statNames.push_back(std::string("noFetch2"));
  statNames.push_back(std::string("nFreeze"));

  FlowID maxProcessorId_ = TaskHandler::getNumCPUS();
  if (id >= maxProcessorId_ || !dynamic_cast<GProcessor*>(TaskHandler::getSimu(id))) {
    printf("ERROR: Processor %u not found\n", id);
    return ERROR;
  }
  //GProcessor *proc = dynamic_cast<GProcessor *>(TaskHandler::getSimu(i));

  std::map<std::string,GStats*> stats = TestCommon::getStats(id, statNames);
  if (stats.empty()) {
    printf("ERROR: Failed to get GStats for processor %u\n", id);
    return ERROR;
  }

  printf("------------- Processor %u Stats -------------\n", id);
  TestCommon::printStats(stats);

  return SUCCESS;
}
std::map<std::string,GStats*> TestCommon::getStats(FlowID id, std::vector<std::string> names, std::string prefix) {
  std::map<std::string,GStats*> stats;

  // Get each GStat
  char str[255];
  std::vector<std::string>::iterator it;
  //for (std::string name : names) {
  for(it = names.begin(); it != names.end(); it++) {
    std::string name = (*it);
    const char* nameStr = name.c_str();
    const char* prefixStr = prefix.c_str();
    sprintf(str, "P(%d)%s%s", id, prefixStr, nameStr);
    GStats *gref = GStats::getRef(str);
    if (!gref) {
      printf("ERROR: Failed to get GStats %s\n", str);
      stats.clear();
      return stats;
    }
    stats[name] = gref;
  }

  return stats;
}

void TestCommon::printStats(std::map<std::string,GStats*> stats) {
  // Print GStats
  char str[255]  = {'\t', '\0'};
  std::map<std::string,GStats*>::iterator it;
  for (it = stats.begin(); it != stats.end(); it++) {
    const char* nameStr = (it->first).c_str();
    uint64_t value= (it->second)->getSamples();

    sprintf(str,"%s%s=%ld ",str, nameStr, value);
  }
  printf("%s", str);
}


