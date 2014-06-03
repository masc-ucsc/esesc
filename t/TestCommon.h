#ifndef TEST_COMMON_H
#define TEST_COMMON_H
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>

#include "GStats.h"

#include "GProcessor.h"
#include "MemObj.h"
#include "GMemorySystem.h"
#include "MemorySystem.h"


#ifndef SUCCESS
#define SUCCESS     0
#endif
#ifndef ERROR
#define ERROR       1
#endif


namespace TestCommon
{
  //static TestCommon::
  /*ifstream in("crafty.input");
    streambuf *cinbuf = cin.rdbuf(); //save old buf
    cin.rdbuf(in.rdbuf()); //redirect cin to in.txt!
   */

  /**
   * Prints information using GStats about a given branch predictor.
   * @param Processor ID number to look up stats for.
   * @return SUCCESS or ERROR
   */
  int printBranchPredictorStats(FlowID id);

  /**
   * Prints information using GStats about a given processor.
   * @param Processor ID number to look up stats for.
   * @return SUCCESS or ERROR
   */
  int printProcessorStats(FlowID id);

  /**
   * Returns a map of GStats and their names for the given processor ID.
   * @param A vector of stat names to look up.
   * @return Name -> Stats map.
   */
  std::map<std::string,GStats*> getStats(FlowID id, std::vector<std::string> names, std::string prefix = std::string(":"));

  /**
   * Prints GStats from the given map.
   * @param A map of stats and their names
   */
  void printStats(std::map<std::string,GStats*> stats);

}

#endif
// TEST_COMMON_H
