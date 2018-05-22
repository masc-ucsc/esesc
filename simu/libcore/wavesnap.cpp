#include "wavesnap.h"

wavesnap::wavesnap() {
  this->window_pointer = 0;
  this->total_count = 0;
  this->update_count = 0;
  this->first_window_completed = false;
  this->try_count = 0;
  this->curr_min_time = 10000;
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

  IPCs[0];
  IPCs[0] = temp;
}

wavesnap::~wavesnap() {
  //nothing to do here
}

void wavesnap::record_pipe(pipeline_info* next) {
  this->window_sign_info[next->encode];
  pipeline_info *pipe_info = &(this->window_sign_info[next->encode]);
  if (pipe_info->execute_cycles.size() == 0) {
    *pipe_info = *next;
  } else {
    pipe_info->count++;
    for (uint32_t j=0; j<pipe_info->execute_cycles.size(); j++) {
      pipe_info->wait_cycles[j]+=next->wait_cycles[j];
      pipe_info->rename_cycles[j]+=next->rename_cycles[j];
      pipe_info->issue_cycles[j]+=next->issue_cycles[j];
      pipe_info->execute_cycles[j]+=next->execute_cycles[j];
    }
  }
}

void wavesnap::add_instruction(DInst* dinst) {
  wait_buffer.push_back(dinst->getID());
  completed.push_back(false);
}

void wavesnap::update_window(DInst* dinst) {
  //update instruction as completed
  bool found = false;
  uint64_t i = dinst->getID();
  uint64_t id = i;
  while (!found) {
    if (this->wait_buffer.size()>i) {
      if (this->wait_buffer[i]==id) {
        this->completed[i] = true;
        found = true;

        //TODO: to reduce the size, just capture required info,
        // not the whole dinst
        dinst_info[id];
        dinst_info[id] = *(dinst);
      }
    }
    i--;
  }

  //poll the very first window, wait until completed
  if (!this->first_window_completed) {
    this->first_window_completed = true;
    for (uint32_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
      if (!this->completed[i]) {
        this->first_window_completed = false;
        break;
      } else {
        DInst* d = &(dinst_info[i]);
        if (d->getFetchedTime()<this->curr_min_time) {
          this->curr_min_time = d->getFetchedTime();
          this->min_time_id = d->getID();
        }
      }
    }
  } else { 
    // after the first window completed, just check the last element
    // of the window.
    while (completed[MAX_MOVING_GRAPH_NODES-1] && completed.size()>=MAX_MOVING_GRAPH_NODES) {
      //update current min fetch time if it was removed
      //TODO: check if it is worth updating the minimum,
      //logically, very first instrcution must be minimum of the window
      if (this->last_removed_id == this->min_time_id) {
        this->update_count++;
        this->curr_min_time = dinst_info[wait_buffer[0]].getFetchedTime();
        this->min_time_id = dinst_info[wait_buffer[0]].getID();
        for (uint32_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
          DInst* d = &(dinst_info[wait_buffer[i]]);
          if (d->getFetchedTime()<=this->curr_min_time) {
            this->curr_min_time = d->getFetchedTime();
            this->min_time_id = d->getID();
          }
        }
      }

      //get pipeline info of the complete window
      pipeline_info next;

      for (uint32_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
        DInst c = dinst_info[wait_buffer[i]];
        next.wait_cycles.push_back(c.getFetchedTime()-this->curr_min_time); 
        next.rename_cycles.push_back(c.getRenamedTime()-c.getFetchedTime()); 
        next.issue_cycles.push_back(c.getIssuedTime()-c.getRenamedTime()); 
        next.execute_cycles.push_back(c.getExecutedTime()-c.getIssuedTime()); 
        next.instructions.push_back(c.getInst()->getOpcode()); 
        next.encode+=ENCODING[c.getInst()->getOpcode()];
      }
      next.count = 1;

      //record
      record_pipe(&next);

      //remove first instruction in the window and update stats
      window_pointer++;
      this->last_removed_id = dinst_info[wait_buffer[0]].getID();
      dinst_info.erase(wait_buffer[0]);
      wait_buffer.erase(wait_buffer.begin()); 
      completed.erase(completed.begin());

    }
  }
}
////////////////////////////////////////
// WINDOW BASED IPC UPDATE AND CALCULATION
/*
void wavesnap::update_window() {
  if (this->first_window_completed) {
    //if the last element in the window complete,
    //good to encode and record the window
    //else don't do anything, poll until comletes
     DInst* c = working_window[working_window.size()-1];
     //if (c->getInst()->getOpcode()==3) 
     {
       std::cout << c->getInst()->getOpcode() << " ";
       std::cout << c->getFetchedTime() << " ";
       std::cout << c->getRenamedTime() << " ";
       std::cout << c->getIssuedTime() << " ";
       std::cout << c->getExecutedTime() << " ";
       std::cout << " "  << window_pointer << " " << this->wait_buffer.size() << " " << working_window.size() << std::endl;
    }
    if (working_window[working_window.size()-1]->isPerformed()) {
      pipeline_info next;
      next.count = 1;
      std::string encode = "";

      //window size and window width must be the same
      I(working_window.size()==MAX_GRAPH_NODES);

      for (uint32_t i=0; i<working_window.size(); i++) {
        DInst* c = working_window[i]; // c for current
        next.wait_cycles.push_back(c->getFetchedTime()-this->curr_min_time); 
        next.rename_cycles.push_back(c->getRenamedTime()-this->curr_min_time); 
        next.issue_cycles.push_back(c->getIssuedTime()-this->curr_min_time); 
        next.execute_cycles.push_back(c->getExecutedTime()-this->curr_min_time); 
        next.instructions.push_back(c->getInst()->getOpcode()); 
        encode+=ENCODING[c->getInst()->getOpcode()];
      }
      //record completed window
      window_sign_info[encode];
      pipeline_info *pipe_info = &(window_sign_info[encode]);
      if (pipe_info->execute_cycles.size() == 0) {
        *pipe_info = next;
      } else {
        pipe_info->count++;
        for (uint32_t j=0; j<pipe_info->execute_cycles.size(); j++) {
          pipe_info->wait_cycles[j]+=next.wait_cycles[j];
          pipe_info->rename_cycles[j]+=next.rename_cycles[j];
          pipe_info->issue_cycles[j]+=next.issue_cycles[j];
          pipe_info->execute_cycles[j]+=next.execute_cycles[j];
        }
      }

      //update working window
      window_pointer++;
      DInst* removed_dinst = working_window[0];
      working_window.erase(working_window.begin());
      wait_buffer.erase(wait_buffer.begin());
      working_window.push_back(wait_buffer[MAX_MOVING_GRAPH_NODES]);

      //update min_time if the instruction with the same value was removed
      if (removed_dinst->getFetchedTime()==this->curr_min_time) {
        this->curr_min_time = working_window[0]->getFetchedTime();
        for (uint32_t i=0; i<working_window.size(); i++) {
          if (this->curr_min_time<working_window[i]->getFetchedTime()) {
            this->curr_min_time = working_window[i]->getFetchedTime();
          }
        }
      }
    } else {
      this->try_count++;
      //flush the window after too many tries
      if (this->try_count>TRY_COUNT_ALLOW) {
        this->try_count = 0;
        this->first_window_completed = false;
        uint32_t i = MAX_MOVING_GRAPH_NODES;
        while (i>0) {
          this->wait_buffer.erase(this->wait_buffer.begin());
          i--;
        }
      }
    }
  } else {
    std::cout << "1";
    this->first_window_completed = true;
    this->working_window.clear();
    this->curr_min_time = wait_buffer[0]->getFetchedTime();
    for (uint64_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
      if (wait_buffer[i]->getExecutedTime() == 0) {
        this->first_window_completed = false;
        break;
      } else {
        this->working_window.push_back(wait_buffer[i]);
        if (this->curr_min_time>wait_buffer[i]->getFetchedTime()) {
          this->curr_min_time = wait_buffer[i]->getFetchedTime();
        }
      }
    }
  }
}
*/

void wavesnap::test_uncompleted() {

  std::cout << "testing uncompleted instructions... wait buffer size = " << this->wait_buffer.size() << std::endl;
  uint32_t count=0;
  for (uint64_t i=0; i<completed.size(); i++) {
    if (!completed[i]) {
      std::cout << wait_buffer[i] << std::endl;
      count++;
    }
  }
    /*
  for (uint32_t i=0; i<this->wait_buffer.size(); i++) {
    DInst *c = this->wait_buffer[i];
    if (!c->isExecuted()) {
      std::cout << "d:" << c->isDispatched() << " ";
      std::cout << "op:" << c->getInst()->getOpcode() << " ";
      std::cout << "fetched:" << c->getFetchedTime() << " ";
      std::cout << "renamed:" << c->getRenamedTime() << " ";
      std::cout << "issued:" << c->getIssuedTime() << " ";
      std::cout << "executed:" << c->getExecutedTime() << " ";
      std::cout << std::endl; 
      count++;
    }
    if (c->getID() < i) {
      std::cout << c->getID() << " ";
      std::cout << i << std::endl;
    }
  }

    */
  std::cout << "uncomleted instruction = "  << count << std::endl;
}

void wavesnap::calculate_ipc() {
  uint64_t total_fetch_ipc = 0;
  uint64_t total_rename_ipc = 0;
  uint64_t total_issue_ipc = 0;
  uint64_t total_execute_ipc = 0;
  float total_fetch_diff = 0;
  float total_rename_diff = 0;
  float total_issue_diff = 0;
  float total_execute_diff = 0;
  uint64_t total_count = 0;

  for (auto& sign_kv:window_sign_info) {
    pipeline_info pipe_info = sign_kv.second;
    uint64_t c = pipe_info.count;
    if (c>COUNT_ALLOW) {
      total_count+=c;
      //average the cycles
      std::map<uint32_t, uint32_t> w_c;
      std::map<uint32_t, uint32_t> r_c;
      std::map<uint32_t, uint32_t> i_c;
      std::map<uint32_t, uint32_t> e_c;
      w_c.clear();
      r_c.clear();
      i_c.clear();
      e_c.clear();
     
      for (uint32_t j=0; j<pipe_info.execute_cycles.size(); j++) {
        //average
        uint32_t w = pipe_info.wait_cycles[j]/c;
        uint32_t r = pipe_info.rename_cycles[j]/c;
        uint32_t i = pipe_info.issue_cycles[j]/c;
        uint32_t e = pipe_info.execute_cycles[j]/c;

        //count cycles at each stage
        //wait cycles
        uint32_t f=w;
        w_c[f];
        w_c[f]++;
        //rename
        f+=r;
        r_c[f];
        r_c[f]++;
        //issue
        f+=i;
        i_c[f];
        i_c[f]++;
        //execute
        f+=e;
        e_c[f];
        e_c[f]++;
      }

      //calculate ipc at each stage
      //fetch
      uint32_t total = 0;
      for (auto& kv:w_c) {
        total+=kv.second;
      }
      float fetch_ipc = (1.0*total)/(1.0*w_c.size());
      //rename
      total = 0;
      for (auto& kv:r_c) {
        total+=kv.second;
      }
      float rename_ipc = total/(1.0*r_c.size());
      //issue
      total = 0;
      for (auto& kv:i_c) {
        total+=kv.second;
      }
      float issue_ipc = total/(1.0*i_c.size());
      //execute
      total = 0;
      for (auto& kv:e_c) {
        total+=kv.second;
        std::cout << kv.second << " ";
      }
      std::cout << std::endl;
      float execute_ipc = total/(1.0*e_c.size());
  
      //determine how much this signature contributes to the overall ipc
      //1.accumulate diffs
      total_fetch_diff += (1.0*std::ceil(fetch_ipc)-1.0*fetch_ipc)*c;


      total_rename_diff += (1.0*std::ceil(rename_ipc)-1.0*rename_ipc)*c;
      total_issue_diff += (1.0*std::ceil(issue_ipc)-1.0*issue_ipc)*c;
      total_execute_diff += (1.0*std::ceil(execute_ipc)-1.0*execute_ipc)*c;

      //2.accumulate ipcs
      total_fetch_ipc += std::ceil(fetch_ipc)*c;
      total_rename_ipc += std::ceil(rename_ipc)*c;
      total_issue_ipc += std::ceil(issue_ipc)*c;
      total_execute_ipc += std::ceil(execute_ipc)*c;
    }
  }

  std::cout << "fetch: " << (1.0*total_fetch_ipc-total_fetch_diff)/total_count << std::endl;
  std::cout << "rename: " << (1.0*total_rename_ipc-total_rename_diff)/total_count << std::endl;
  std::cout << "issue: " << (1.0*total_issue_ipc-total_issue_diff)/total_count << std::endl;
  std::cout << "execute: " << (1.0*total_execute_ipc-total_execute_diff)/total_count << std::endl;
}
//WINDOW BASED IPC END
////////////////////////////////////////


/////////////////////////////////
//FULL IPC UPDATE and CALCULATION
void wavesnap::full_ipc_update(DInst* dinst) {
  uint64_t fetched = dinst->getFetchedTime();
  uint64_t renamed = dinst->getRenamedTime();
  uint64_t issued = dinst->getIssuedTime();
  uint64_t executed = dinst->getExecutedTime();

  this->full_fetch_ipc[fetched];
  this->full_fetch_ipc[fetched]++;

  this->full_rename_ipc[renamed];
  this->full_rename_ipc[renamed]++;

  this->full_issue_ipc[issued];
  this->full_issue_ipc[issued]++;

  this->full_execute_ipc[executed];
  this->full_execute_ipc[executed]++;
}

void wavesnap::calculate_full_ipc() {
  //calculate fetch ipc
  uint64_t total_fetch = 0;
  for (auto& kv:full_fetch_ipc) {
    total_fetch += kv.second;
  }
  std::cout << "fetch ipc: " << 1.0*total_fetch/full_fetch_ipc.size() << std::endl;

  //calculate rename ipc
  uint64_t total_rename = 0;
  for (auto& kv:full_rename_ipc) {
    total_rename += kv.second;
  }
  std::cout << "rename ipc: " << 1.0*total_rename/full_rename_ipc.size() << std::endl;

  //calculate issue ipc
  uint64_t total_issue = 0;
  for (auto& kv:full_issue_ipc) {
    total_issue+=kv.second;
  }
  std::cout << "issue ipc: " << 1.0*total_issue/full_issue_ipc.size() << std::endl;

  //calculate execute ipc
  uint64_t total_execute = 0;
  for (auto& kv:full_execute_ipc) {
    total_execute += kv.second;
  }
  std::cout << "execute ipc: " << 1.0*total_execute/full_execute_ipc.size() << std::endl;

}
//FULL IPC END
/////////////////////////////////

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

float average(std::map<uint32_t, uint32_t>* cycles) {
  uint64_t sum = 0;
  uint64_t count = 0;
  for (auto& kv: *cycles) {
    sum+=kv.second;
    count++;
  }

  return sum/(1.0*count);
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

void wavesnap::dump_address(std::string fileName) {
  std::cout << "Dumping window addresses..." << std::endl;
  std::ofstream output;
  output.open(fileName);
  output << dump_w() << std::endl;
  /*
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
  */

  output.close();
}
