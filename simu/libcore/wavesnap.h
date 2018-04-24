/*
  USAGE:
    1. In GProcessor.h uncomment: #define WAVESNAP_EN

    2. In GProcessor.cpp, in the constructor change the range of instruction you want to capture.
       Example: this->capture = new wavesnap(0, 100);

    3. In TaskHandler.cpp, in TaskHandler::unplug() specify the format and the file where you
       want to dump the traces. In case wavedrom format is picked, copy and paste generated
       file into Wavedrom edittor to visualize. 
*/

#ifndef _WAVESNAP_
#define _WAVESNAP_

//signature defines
#define REGISTER_NUM 32
#define HASH_SIZE 65536

//node list defines
#define MAX_NODE_NUM 1000
#define MAX_EDGE_NUM 100000

//dump path
#define EDGE_DUMP_PATH "./edge_list/"

#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include "InstOpcode.h"
#include "EmuSampler.h"
#include "DInst.h"

class wavesnap {
  private:
    class instructionInfo {
      public:
        uint32_t id;
        uint32_t dst;
        uint32_t src1;
        uint32_t src2;
        uint32_t parent1_id;
        uint32_t parent2_id;
        uint32_t depth;
        bool valid;
        std::string opcode_name;
        uint32_t opcode;

        instructionInfo() {
          this->id = 0;
          this->dst = 0;
          this->src1 = 0;
          this->src2 = 0;
          this->parent1_id = 0;
          this->parent2_id = 0;
          this->depth = 0;
          this->valid = false;
        }

        std::string dump() {
          std::stringstream result;

          result << "(";
          result << id << ", ";
          result << dst << ", ";
          result << src1 << ", ";
          result << src2 << ", ";
          result << parent1_id << ", ";
          result << parent2_id << ", ";
          result << depth << ")";
    
          return result.str();
        }
    };

    class instruction_node {
      public:
        instructionInfo info;
        std::vector<uint32_t> dependencies;
        bool valid;
 
        instruction_node() {
          this->info.id = 0;
          this->dependencies.push_back(0);
          this->valid = false;
        }
    };

    class edge {
      public:
        uint32_t v1;
        uint32_t v2;
        uint32_t v1_opcode;
        uint32_t v1_depth;

        edge() {
          //nothing to do here
        }
  
        edge (uint32_t v1, uint32_t v2, uint32_t v1_opcode, uint32_t v1_depth) {
          this->v1 = v1;
          this->v2 = v2;
          this->v1_opcode = v1_opcode;
          this->v1_depth = v1_depth;
        }

        ~edge() {
          //nothing to do here
        }

        std::string to_string() {
          std::stringstream result;
          result << v1 << " " << v1_opcode << " " << v1_depth  << " " << v2;
          return result.str();
        }
    };

    instructionInfo RAT[REGISTER_NUM+HASH_SIZE];
    std::map<uint32_t, instruction_node> nodeGraph;
    std::map<uint32_t, std::vector<uint32_t>> connGraph;
    std::vector<edge> edgeList;
    void sign(uint32_t depth, uint32_t w);
    void addNode(instructionInfo node);
    void addNode(uint32_t v1, uint32_t);
    void addEdge(uint32_t v1, uint32_t v2, uint32_t v1_opcode, uint32_t v1_depth);
    bool connected(uint32_t v1, uint32_t v2, uint32_t distance);

  public:
    wavesnap();
    ~wavesnap();
    void add(DInst* dinst);
    void dumpGraph(std::string fileName);
    void dumpConnGraph();
    void dumpEdgeList(std::string fileName);

    //statistical variables
    uint32_t dump_num;
    uint32_t second_edges_added;
    uint32_t second_edges_skipped;
};
#endif
