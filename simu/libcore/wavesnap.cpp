#include "wavesnap.h"

wavesnap::wavesnap() {
  this->total_count = 0;
  std::vector<float> temp;
  temp.push_back(0.0);
  temp.push_back(0.0);
  temp.push_back(0.0);
  temp.push_back(0.0);
  temp.push_back(0.0);
  temp.push_back(0.0);
  temp.push_back(0.0);
  temp.push_back(0.0);
  for (uint8_t w=W_LOWER; w<W_UPPER; w++) {
    IPCs[w];
    IPCs[w] = temp;
  }
}

wavesnap::~wavesnap() {
  //nothing to do here
}

void wavesnap::update_window(DInst* dinst) {
  for (uint32_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
    if (dinst->getID()>=i) {
      uint64_t id = dinst->getID()-i;
      windows[id];
      if (!windows[id].is_set()) {
        windows[id].set_start_id(id);
      }

      windows[id].update(dinst);

      if (windows[id].get_size() == MAX_MOVING_GRAPH_NODES) {
        if (windows[id].min_fetch_time == 0) {
          windows[id].dump();
        }
        uint64_t address = windows[id].get_first_inst().getPC();
        window_info* w_a = &window_address_info[address];
        
        w_a->count++;

        pipeline_info next;
        next.count = 1;
        std::string encode = "";
        for (uint32_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
          if (windows[id].check_if_control(i)) {
            continue;
          }


          std::vector<int32_t> temp = windows[id].get_pipeline_info(i);
          next.wait_cycles.push_back(temp[0]);
          next.rename_cycles.push_back(temp[1]);
          next.issue_cycles.push_back(temp[2]);
          next.execute_cycles.push_back(temp[3]);
          next.instructions.push_back(temp[4]);
          encode += ENCODING[temp[4]];
        }
        w_a->p_i[encode];
        pipeline_info *temp = &(w_a->p_i[encode]);
        if (temp->instructions.size() == 0) {
          *temp = next;
        } else {
          temp->count++;
          for (uint32_t i=0; i<temp->instructions.size(); i++) {
            temp->instructions[i]+=next.instructions[i];
            temp->wait_cycles[i]+=next.wait_cycles[i];
            temp->rename_cycles[i]+=next.rename_cycles[i];
            temp->issue_cycles[i]+=next.issue_cycles[i];
            temp->execute_cycles[i]+=next.execute_cycles[i];
          }
        }

        windows.erase(id);
      }
    }
  }
}


void wavesnap::add_to_RAT(DInst* dinst) {
  uint64_t dst, src1, src2;
  bool skip = false;
  if (dinst->getInst()->isStore()) {
    //dst =  ((uint64_t)dinst->getAddr()%HASH_SIZE)+REGISTER_NUM;
    dst =  ((uint64_t)dinst->getAddr())+REGISTER_NUM;
    src1 = (uint32_t)dinst->getInst()->getSrc1();
    src2 = (uint32_t)dinst->getInst()->getSrc2();
  } else if (dinst->getInst()->isLoad()) {
    dst =  (uint32_t)dinst->getInst()->getDst1();
    src1 = (uint32_t)dinst->getInst()->getSrc1();
    src2 = ((uint64_t)dinst->getAddr())+REGISTER_NUM;
  } else if (dinst->getInst()->isControl()) {
    //todo
    dst =  (uint32_t)dinst->getInst()->getDst1();
    src1 = (uint32_t)dinst->getInst()->getSrc1();
    src2 = (uint32_t)dinst->getInst()->getSrc2();
    /*
    std::cout << dinst->getInst()->getOpcodeName() << " ";
    std::cout << dst << ",";
    std::cout << src1 << ",";
    std::cout << src2 << "," << std::endl;*/
    skip = true;
  } else {
    dst =  (uint32_t)dinst->getInst()->getDst1();
    src1 = (uint32_t)dinst->getInst()->getSrc1();
    src2 = (uint32_t)dinst->getInst()->getSrc2();
  }

  if (false) {
    int32_t p1_depth = 0;
    int32_t p2_depth = 0;

    RAT[dst];
    RAT[dst].valid = true;
    RAT[dst].id = (uint32_t)dinst->getID()+1;
    RAT[dst].dst = dst;
    RAT[dst].src1 = src1;
    RAT[dst].src2 = src2;
    RAT[dst].opcode_name = dinst->getInst()->getOpcodeName();
    RAT[dst].opcode = (uint32_t)dinst->getInst()->getOpcode();
    RAT[dst].time_stamp = (uint64_t)globalClock;
  }
}

void wavesnap::merge() {
/*
  std::vector<uint64_t> merge_candidates;
  uint32_t i = 0;
  for (auto& kv:window_address_info) {
    if (merge_candidates.size() == 0) {
      merge_candidates.push_back(kv.first);
    } else {
      uint32_t diff = kv.first-merge_candidates[merge_candidates.size()-1];
      if (diff==4) {
        merge_candidates.push_back(kv.first);
      } else {
        //process the candidates
  
        //finally clear the candidates
        merge_candidates.clear();
        merge_candidates.push_back(kv.first);
      }
    }
  }
*/
}

float max_average(uint8_t w, std::map<uint32_t, uint32_t>* cycles) {
  uint32_t moving_sum = 0;
  float max_ave = 0;
  std::vector<uint32_t> cycles_v;
  for (auto& kv: *cycles) {
    cycles_v.push_back(kv.second);
  }

  for (uint16_t i=0; i<w; i++) {
    moving_sum+=cycles_v[i];
  }

  max_ave = moving_sum/w;
  for (uint16_t i=w; i<cycles_v.size(); i++) {
    moving_sum += cycles_v[i];
    moving_sum -= cycles_v[i-w];
    float ave = moving_sum/w;
    if (ave>max_ave) {
      max_ave = ave;
    }
  }

  return max_ave;
}

void wavesnap::calculate_ipc() {
  float fetched_ipc = 0.0;
  float renamed_ipc = 0.0;
  float issued_ipc = 0.0;
  float executed_ipc = 0.0;
  float fetched_diff = 0.0;
  float renamed_diff = 0.0;
  float issued_diff = 0.0;
  float executed_diff = 0.0;
  for (auto& kv:window_address_info) {
    //control. this will dramatically reduce amound of calculation
    if (kv.second.count>COUNT_ALLOW) {
      this->total_count+=kv.second.count;
      if (kv.second.p_i.size()>0) {
        for (auto& a_pi:kv.second.p_i) {
          pipeline_info* current = &a_pi.second;
          std::map<uint32_t, uint32_t> cycles_fetched;
          std::map<uint32_t, uint32_t> cycles_renamed;
          std::map<uint32_t, uint32_t> cycles_issued;
          std::map<uint32_t, uint32_t> cycles_executed;
          cycles_fetched.clear();
          cycles_renamed.clear();
          cycles_issued.clear();
          cycles_executed.clear();
          for (uint16_t i=0; i<current->instructions.size(); i++) {
            uint32_t finished = 0;
            finished += std::floor(current->wait_cycles[i]/current->count);
            cycles_fetched[finished];
            cycles_fetched[finished]++;

            finished += std::floor(current->rename_cycles[i]/current->count);
            cycles_renamed[finished];
            cycles_renamed[finished]++;

            finished += std::floor(current->issue_cycles[i]/current->count);
            cycles_issued[finished];
            cycles_issued[finished]++;

            finished += std::floor(current->execute_cycles[i]/current->count);
            cycles_executed[finished];
            cycles_executed[finished]++;
          }

          //calculate ipc for particular window
          for (uint8_t w=W_LOWER; w<W_UPPER; w++) {
            //retrieve
            fetched_ipc = this->IPCs[w][0];
            fetched_diff = this->IPCs[w][1];
            renamed_ipc = this->IPCs[w][2];
            renamed_diff = this->IPCs[w][3]; 
            issued_ipc = this->IPCs[w][4];
            issued_diff = this->IPCs[w][5]; 
            executed_ipc = this->IPCs[w][6];
            executed_diff = this->IPCs[w][7]; 
            //calculate
            float temp_ave;
            uint64_t c = current->count;
            temp_ave = max_average(w, &cycles_fetched);
            fetched_ipc += std::ceil(temp_ave)*c;
            fetched_diff += (std::ceil(temp_ave)-temp_ave)*c;

            temp_ave = max_average(w, &cycles_renamed);
            renamed_ipc += std::ceil(temp_ave)*c;
            renamed_diff += (std::ceil(temp_ave)-temp_ave)*c;

            temp_ave = max_average(w, &cycles_issued);
            issued_ipc += std::ceil(temp_ave)*c;
            issued_diff += (std::ceil(temp_ave)-temp_ave)*c;

            temp_ave = max_average(w, &cycles_executed);
            executed_ipc += std::ceil(temp_ave)*c; 
            executed_diff += (std::ceil(temp_ave)-temp_ave)*c;
            //put back
            this->IPCs[w][0] = fetched_ipc;
            this->IPCs[w][1] = fetched_diff;
            this->IPCs[w][2] = renamed_ipc;
            this->IPCs[w][3] = renamed_diff;
            this->IPCs[w][4] = issued_ipc;
            this->IPCs[w][5] = issued_diff;
            this->IPCs[w][6] = executed_ipc;
            this->IPCs[w][7] = executed_diff;
          }
        }
      }
    }
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
  for (uint8_t w=W_LOWER; w<W_UPPER; w++) {
    float fetch_ipc = (IPCs[w][0]-IPCs[w][1])/this->total_count;
    float rename_ipc = (IPCs[w][2]-IPCs[w][3])/this->total_count;
    float issue_ipc = (IPCs[w][4]-IPCs[w][5])/this->total_count;
    float execute_ipc = (IPCs[w][6]-IPCs[w][7])/this->total_count;
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
  for (auto& kv:window_sign_info) {
    output << kv.second << " ";
    output << kv.first << std::endl;
  }

  output.close();
}

void wavesnap::dump_address(std::string fileName) {
  std::cout << "Dumping window addresses..." << std::endl;
  std::ofstream output;
  output.open(fileName);
  output << dump_w() << std::endl;
  for (auto& kv:window_address_info) {
    if (kv.second.count>COUNT_ALLOW) {
      output << kv.first << " ";
      output << kv.second.count << " ";
      output << kv.second.p_i.size() << std::endl;
      if (kv.second.p_i.size()>0) {
        //output << "+++" << std::endl;
        for (auto& a_pi:kv.second.p_i) {
          output << a_pi.first << " " << a_pi.second.count << " ";
          for (uint16_t i=0; i<a_pi.second.instructions.size(); i++) {
            output << std::ceil(a_pi.second.wait_cycles[i]/a_pi.second.count) << ".";
            output << std::ceil(a_pi.second.rename_cycles[i]/a_pi.second.count) << ".";
            output << std::ceil(a_pi.second.issue_cycles[i]/a_pi.second.count) << ".";
            output << std::ceil(a_pi.second.execute_cycles[i]/a_pi.second.count) << ".";
          }
          output << std::endl;
        }
        //output << "+++" << std::endl;
      }
    }
  }

  output.close();
}
