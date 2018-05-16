#include "wavesnap.h"

wavesnap::wavesnap() {
  dump_num             = 0;
  second_edges_added   = 0;
  second_edges_skipped = 0;
}

wavesnap::~wavesnap() {
  // nothing to do here
}

void wavesnap::addNode(instructionInfo node) {
  nodeGraph[node.id];

  if(!nodeGraph[node.id].valid) {
    nodeGraph[node.id].valid = true;
    nodeGraph[node.id].info  = node;
  }

  if(nodeGraph[node.parent1_id].valid) {
    nodeGraph[node.parent1_id].dependencies.push_back(node.id);
  }

  if(nodeGraph[node.parent2_id].valid) {
    nodeGraph[node.parent2_id].dependencies.push_back(node.id);
  }
}

pipeline_info next;
next.count         = 1;
std::string encode = "";
for(uint32_t i = 0; i < MAX_MOVING_GRAPH_NODES; i++) {
  if(windows[id].check_if_control(i)) {
    continue;
  }

  void wavesnap::addEdge(uint32_t v1, uint32_t v2, uint32_t v1_opcode, uint32_t v1_depth) {
    edge e;
    e.v1        = v1;
    e.v2        = v2;
    e.v1_opcode = v1_opcode;
    e.v1_depth  = v1_depth;
    addNode(v1, v2);
    this->edgeList.push_back(e);
  }

  bool wavesnap::connected(uint32_t v1, uint32_t v2, uint32_t distance) {
    bool                  result = false;
    std::vector<uint32_t> q;
    std::set<uint32_t>    visited;
    q.push_back(v1);

    for(uint32_t i = 0; i < q.size() && q.size() < distance; i++) {
      uint32_t process = q[i];
      for(uint32_t j = 0; j < connGraph[process].size(); j++) {
        if(connGraph[process][j] != v2) {
          if(visited.find(connGraph[process][j]) == visited.end()) {
            q.push_back(connGraph[process][j]);
            visited.insert(connGraph[process][j]);
          }
          w_a->p_i[encode];
          pipeline_info *temp = &(w_a->p_i[encode]);
          if(temp->instructions.size() == 0) {
            *temp = next;
          } else {
            temp->count++;
            for(uint32_t i = 0; i < temp->instructions.size(); i++) {
              temp->instructions[i] += next.instructions[i];
              temp->wait_cycles[i] += next.wait_cycles[i];
              temp->rename_cycles[i] += next.rename_cycles[i];
              temp->issue_cycles[i] += next.issue_cycles[i];
              temp->execute_cycles[i] += next.execute_cycles[i];
            }
          }

          windows.erase(id);
        }
      }
      if(result) {
        break;
      }
    }
    q.clear();
    visited.clear();
    return result;
  }

  void wavesnap::add(DInst * dinst) {
    uint32_t dst, src1, src2;
    bool     skip = false;
    if(dinst->getInst()->isStore()) {
      dst  = ((uint32_t)dinst->getAddr() % HASH_SIZE) + REGISTER_NUM;
      src1 = (uint32_t)dinst->getInst()->getSrc1();
      src2 = (uint32_t)dinst->getInst()->getSrc2();
    } else if(dinst->getInst()->isLoad()) {
      dst  = (uint32_t)dinst->getInst()->getDst1();
      src1 = (uint32_t)dinst->getInst()->getSrc1();
      src2 = ((uint32_t)dinst->getAddr() % HASH_SIZE) + REGISTER_NUM;
    } else if(dinst->getInst()->isControl()) {
      // todo
      skip = true;
    } else {
      dst  = (uint32_t)dinst->getInst()->getDst1();
      src1 = (uint32_t)dinst->getInst()->getSrc1();
      src2 = (uint32_t)dinst->getInst()->getSrc2();
    }

    if(!skip) {
      int32_t p1_depth = 0;
      int32_t p2_depth = 0;

      RAT[dst].valid       = true;
      RAT[dst].id          = (uint32_t)dinst->getID();
      RAT[dst].dst         = dst;
      RAT[dst].src1        = src1;
      RAT[dst].src2        = src2;
      RAT[dst].opcode_name = dinst->getInst()->getOpcodeName();
      RAT[dst].opcode      = (uint32_t)dinst->getInst()->getOpcode();
      if(RAT[src1].valid) {
        RAT[dst].parent1_id = RAT[src1].id;
        p1_depth            = RAT[src1].depth + 1;
      }
      if(RAT[src2].valid) {
        RAT[dst].parent2_id = RAT[src2].id;
        p2_depth            = RAT[src2].depth + 1;
      }

      if(p1_depth > p2_depth) {
        RAT[dst].depth = p1_depth;
        addEdge(RAT[dst].id, RAT[dst].parent1_id, RAT[dst].opcode, RAT[dst].depth);
        if(!connected(RAT[dst].parent1_id, RAT[dst].parent2_id, RAT[dst].depth - p2_depth)) {
          addEdge(RAT[dst].id, RAT[dst].parent2_id, RAT[dst].opcode, RAT[dst].depth);
          second_edges_added++;
        } else {
          second_edges_skipped++;
        }
      } else {
        RAT[dst].depth = p2_depth;
        addEdge(RAT[dst].id, RAT[dst].parent2_id, RAT[dst].opcode, RAT[dst].depth);
        if(!connected(RAT[dst].parent2_id, RAT[dst].parent1_id, RAT[dst].depth - p1_depth)) {
          addEdge(RAT[dst].id, RAT[dst].parent1_id, RAT[dst].opcode, RAT[dst].depth);
          second_edges_added++;
        } else {
          // process the candidates

          // finally clear the candidates
          merge_candidates.clear();
          merge_candidates.push_back(kv.first);
        }
      }
    }
    * /
  }

  float max_average(uint8_t w, std::map<uint32_t, uint32_t> * cycles) {
    uint32_t              moving_sum = 0;
    float                 max_ave    = 0;
    std::vector<uint32_t> cycles_v;
    for(auto &kv : *cycles) {
      cycles_v.push_back(kv.second);
    }

    for(uint16_t i = 0; i < w; i++) {
      moving_sum += cycles_v[i];
    }

    max_ave = moving_sum / w;
    for(uint16_t i = w; i < cycles_v.size(); i++) {
      moving_sum += cycles_v[i];
      moving_sum -= cycles_v[i - w];
      float ave = moving_sum / w;
      if(ave > max_ave) {
        max_ave = ave;
      }
    }

    return max_ave;
  }

  void wavesnap::calculate_ipc() {
    float fetched_ipc   = 0.0;
    float renamed_ipc   = 0.0;
    float issued_ipc    = 0.0;
    float executed_ipc  = 0.0;
    float fetched_diff  = 0.0;
    float renamed_diff  = 0.0;
    float issued_diff   = 0.0;
    float executed_diff = 0.0;
    for(auto &kv : window_address_info) {
      // control. this will dramatically reduce amound of calculation
      if(kv.second.count > COUNT_ALLOW) {
        this->total_count += kv.second.count;
        if(kv.second.p_i.size() > 0) {
          for(auto &a_pi : kv.second.p_i) {
            pipeline_info *              current = &a_pi.second;
            std::map<uint32_t, uint32_t> cycles_fetched;
            std::map<uint32_t, uint32_t> cycles_renamed;
            std::map<uint32_t, uint32_t> cycles_issued;
            std::map<uint32_t, uint32_t> cycles_executed;
            cycles_fetched.clear();
            cycles_renamed.clear();
            cycles_issued.clear();
            cycles_executed.clear();
            for(uint16_t i = 0; i < current->instructions.size(); i++) {
              uint32_t finished = 0;
              finished += std::floor(current->wait_cycles[i] / current->count);
              cycles_fetched[finished];
              cycles_fetched[finished]++;

              // addNode(RAT[dst]);
            }
          }

          std::string wavesnap::dump_w() {
            std::stringstream output;
            output << std::setw(10) << "width";
            output << std::setw(10) << "fetch";
            output << std::setw(10) << "rename";
            output << std::setw(10) << "issue";
            output << std::setw(10) << "execute";
            output << std::endl;
            for(uint8_t w = W_LOWER; w < W_UPPER; w++) {
              float fetch_ipc   = (IPCs[w][0] - IPCs[w][1]) / this->total_count;
              float rename_ipc  = (IPCs[w][2] - IPCs[w][3]) / this->total_count;
              float issue_ipc   = (IPCs[w][4] - IPCs[w][5]) / this->total_count;
              float execute_ipc = (IPCs[w][6] - IPCs[w][7]) / this->total_count;
              output << std::setw(10) << (int)w;
              output << std::setw(10) << fetch_ipc;
              output << std::setw(10) << rename_ipc;
              output << std::setw(10) << issue_ipc;
              output << std::setw(10) << execute_ipc;
              output << std::endl;
            }

            return output.str();
          }

          void wavesnap::dump_signatures(std::string fileName) {
            std::cout << "Dumping window signatures..." << std::endl;
            std::ofstream output;
            output.open(fileName);
            for(const auto &p : nodeGraph) {
              output << p.second.info.id << " ";
              output << p.second.info.opcode << " ";
              for(int i = 1; i < p.second.dependencies.size(); i++) {
                output << p.second.dependencies[i] << " ";
              }
              output << std::endl;
              i++;
              if(i % MAX_NODE_NUM == 0) {
                j++;
                output.close();
                output.open(fileName + std::to_string(j));
              }
            }

            output.close();
          }

          void wavesnap::dump_address(std::string fileName) {
            std::cout << "Dumping window addresses..." << std::endl;
            std::ofstream output;
            output.open(fileName + "list" + std::to_string(dump_num));
            for(uint32_t i = 0; i < edgeList.size(); i++) {
              output << edgeList[i].to_string() << std::endl;
            }

            output.close();
          }
