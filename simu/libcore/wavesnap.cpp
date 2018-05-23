#include "wavesnap.h"

wavesnap::wavesnap() {
  this->window_pointer         = 0;
  this->update_count           = 0;
  this->first_window_completed = false;
  this->curr_min_time = 10000;
  this->working_window.count = 1;
}

wavesnap::~wavesnap() {
  // nothing to do here
}

void wavesnap::record_pipe(pipeline_info *next) {
  this->window_sign_info[next->encode];
  pipeline_info *pipe_info = &(this->window_sign_info[next->encode]);
  if(pipe_info->execute_cycles.size() == 0) {
    *pipe_info = *next;
  } else {
    pipe_info->count++;
    for(uint32_t j = 0; j < pipe_info->execute_cycles.size(); j++) {
      pipe_info->wait_cycles[j] += next->wait_cycles[j];
      pipe_info->rename_cycles[j] += next->rename_cycles[j];
      pipe_info->issue_cycles[j] += next->issue_cycles[j];
      pipe_info->execute_cycles[j] += next->execute_cycles[j];
    }
  }
}

void wavesnap::add_pipeline_info(pipeline_info* pipe_info, DInst* d) {
  pipe_info->wait_cycles.push_back(d->getFetchedTime()-this->curr_min_time); 
  pipe_info->rename_cycles.push_back(d->getRenamedTime()-d->getFetchedTime()); 
  pipe_info->issue_cycles.push_back(d->getIssuedTime()-d->getRenamedTime()); 
  pipe_info->execute_cycles.push_back(d->getExecutedTime()-d->getIssuedTime()); 
  pipe_info->instructions.push_back(d->getInst()->getOpcode()); 
  pipe_info->encode+=ENCODING[d->getInst()->getOpcode()];
}

void wavesnap::add_instruction(DInst *dinst) {
  wait_buffer.push_back(dinst->getID());
  completed.push_back(false);
}

void wavesnap::update_window(DInst *dinst) {
  // update instruction as completed
  bool     found = false;
  uint64_t i     = dinst->getID();
  uint64_t id    = i;
  while(!found) {
    if(this->wait_buffer.size() > i) {
      if(this->wait_buffer[i] == id) {
        this->completed[i] = true;
        found              = true;

        // TODO: to reduce the size, just capture required info,
        // not the whole dinst
        dinst_info[id];
        dinst_info[id] = *(dinst);
      }
    }
    i--;
  }

  // poll the very first window, wait until completed
  if(!this->first_window_completed) {
    this->first_window_completed = true;
    for(uint32_t i = 0; i < MAX_MOVING_GRAPH_NODES; i++) {
      if(!this->completed[i]) {
        this->first_window_completed = false;
        this->working_window.clear();
        break;
      } else {
        DInst *d = &(dinst_info[i]);
        if(d->getFetchedTime() < this->curr_min_time) {
          this->curr_min_time = d->getFetchedTime();
          this->min_time_id   = d->getID();
        }
      }
    }
  } else {
    // after the first window completed, just check the last element
    // of the window.
    while(completed[MAX_MOVING_GRAPH_NODES - 1] && completed.size() >= MAX_MOVING_GRAPH_NODES) {
      // update current min fetch time if it was removed
      // TODO: check if it is worth updating the minimum,
      // logically, very first instrcution must be minimum of the window
      if(this->last_removed_id == this->min_time_id) {
        this->update_count++;
        this->curr_min_time = dinst_info[wait_buffer[0]].getFetchedTime();
        this->min_time_id   = dinst_info[wait_buffer[0]].getID();
        for(uint32_t i = 0; i < MAX_MOVING_GRAPH_NODES; i++) {
          DInst *d = &(dinst_info[wait_buffer[i]]);
          if(d->getFetchedTime() <= this->curr_min_time) {
            this->curr_min_time = d->getFetchedTime();
            this->min_time_id   = d->getID();
          }
        }
      }
      //add last instruction of the window
      pipeline_info next;
      next.count = 1;
      for (uint32_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
        add_pipeline_info(&next, &(dinst_info[wait_buffer[i]]));
      }

      // record
      record_pipe(&next);

      // remove first instruction in the window and update stats
      window_pointer++;
      this->last_removed_id = dinst_info[wait_buffer[0]].getID();
      dinst_info.erase(wait_buffer[0]);
      wait_buffer.erase(wait_buffer.begin());
      completed.erase(completed.begin());
    }
  }
}

void wavesnap::test_uncompleted() {

  std::cout << "testing uncompleted instructions... wait buffer size = " << this->wait_buffer.size() << std::endl;
  uint32_t count = 0;
  for(uint64_t i = 0; i < completed.size(); i++) {
    if(!completed[i]) {
      std::cout << wait_buffer[i] << std::endl;
      count++;
    }
  }
  std::cout << "uncomleted instruction = " << count << std::endl;
}

void wavesnap::calculate_ipc() {
  uint64_t total_fetch_ipc    = 0;
  uint64_t total_rename_ipc   = 0;
  uint64_t total_issue_ipc    = 0;
  uint64_t total_execute_ipc  = 0;
  float    total_fetch_diff   = 0;
  float    total_rename_diff  = 0;
  float    total_issue_diff   = 0;
  float    total_execute_diff = 0;
  uint64_t total_count        = 0;

  for(auto &sign_kv : window_sign_info) {
    pipeline_info pipe_info = sign_kv.second;
    uint64_t      c         = pipe_info.count;
    if(c > COUNT_ALLOW) {
      total_count += c;
      // average the cycles
      std::map<uint32_t, uint32_t> w_c;
      std::map<uint32_t, uint32_t> r_c;
      std::map<uint32_t, uint32_t> i_c;
      std::map<uint32_t, uint32_t> e_c;
      w_c.clear();
      r_c.clear();
      i_c.clear();
      e_c.clear();

      for(uint32_t j = 0; j < pipe_info.execute_cycles.size(); j++) {
        // average
        uint32_t w = pipe_info.wait_cycles[j] / c;
        uint32_t r = pipe_info.rename_cycles[j] / c;
        uint32_t i = pipe_info.issue_cycles[j] / c;
        uint32_t e = pipe_info.execute_cycles[j] / c;

        // count cycles at each stage
        // wait cycles
        uint32_t f = w;
        w_c[f];
        w_c[f]++;
        // rename
        f += r;
        r_c[f];
        r_c[f]++;
        // issue
        f += i;
        i_c[f];
        i_c[f]++;
        // execute
        f += e;
        e_c[f];
        e_c[f]++;
      }

      // calculate ipc at each stage
      // fetch
      uint32_t total = 0;
      for(auto &kv : w_c) {
        total += kv.second;
      }
      float fetch_ipc = (1.0 * total) / (1.0 * w_c.size());
      // rename
      total = 0;
      for(auto &kv : r_c) {
        total += kv.second;
      }
      float rename_ipc = total / (1.0 * r_c.size());
      // issue
      total = 0;
      for(auto &kv : i_c) {
        total += kv.second;
      }
      float issue_ipc = total / (1.0 * i_c.size());
      // execute
      total = 0;
      for(auto &kv : e_c) {
        total += kv.second;
      }
      float execute_ipc = total / (1.0 * e_c.size());

      // determine how much this signature contributes to the overall ipc
      // 1.accumulate diffs
      total_fetch_diff += (1.0 * std::ceil(fetch_ipc) - 1.0 * fetch_ipc) * c;

      total_rename_diff += (1.0 * std::ceil(rename_ipc) - 1.0 * rename_ipc) * c;
      total_issue_diff += (1.0 * std::ceil(issue_ipc) - 1.0 * issue_ipc) * c;
      total_execute_diff += (1.0 * std::ceil(execute_ipc) - 1.0 * execute_ipc) * c;

      // 2.accumulate ipcs
      total_fetch_ipc += std::ceil(fetch_ipc) * c;
      total_rename_ipc += std::ceil(rename_ipc) * c;
      total_issue_ipc += std::ceil(issue_ipc) * c;
      total_execute_ipc += std::ceil(execute_ipc) * c;
    }
  }

  std::cout << "fetch: " << (1.0 * total_fetch_ipc - total_fetch_diff) / total_count << std::endl;
  std::cout << "rename: " << (1.0 * total_rename_ipc - total_rename_diff) / total_count << std::endl;
  std::cout << "issue: " << (1.0 * total_issue_ipc - total_issue_diff) / total_count << std::endl;
  std::cout << "execute: " << (1.0 * total_execute_ipc - total_execute_diff) / total_count << std::endl;
}
// WINDOW BASED IPC END
////////////////////////////////////////

/////////////////////////////////
//FULL IPC UPDATE and CALCULATION
void wavesnap::full_ipc_update(DInst* dinst, uint64_t commited) {  
  uint64_t fetched = dinst->getFetchedTime();
  uint64_t renamed = dinst->getRenamedTime();
  uint64_t issued = dinst->getIssuedTime();
  uint64_t executed = dinst->getExecutedTime();

  if (dinst->getInst()->doesJump2Label()) {
    update_count++;/*
    std::cout << dinst->getInst()->getOpcodeName() << " ";
    std::cout << fetched << " ";
    std::cout << renamed << " ";
    std::cout << issued << " ";
    std::cout << executed << " ";
    std::cout << commited << " ";
    std::cout << std::endl;
    */
  }

  this->full_fetch_ipc[fetched];
  this->full_fetch_ipc[fetched]++;

  this->full_rename_ipc[renamed];
  this->full_rename_ipc[renamed]++;

  this->full_issue_ipc[issued];
  this->full_issue_ipc[issued]++;

  this->full_execute_ipc[executed];
  this->full_execute_ipc[executed]++;

  this->full_commit_ipc[commited];
  this->full_commit_ipc[commited]++;

}

void wavesnap::calculate_full_ipc() {
  // calculate fetch ipc
  uint64_t total_fetch = 0;
  for(auto &kv : full_fetch_ipc) {
    total_fetch += kv.second;
  }
  std::cout << "fetch ipc:  " << 1.0 * total_fetch / full_fetch_ipc.size() << std::endl;

  // calculate rename ipc
  uint64_t total_rename = 0;
  for(auto &kv : full_rename_ipc) {
    total_rename += kv.second;
  }
  std::cout << "rename ipc: " << 1.0 * total_rename / full_rename_ipc.size() << std::endl;

  // calculate issue ipc
  uint64_t total_issue = 0;
  for(auto &kv : full_issue_ipc) {
    total_issue += kv.second;
  }
  std::cout << "issue ipc: " << 1.0 * total_issue / full_issue_ipc.size() << std::endl;

  // calculate execute ipc
  uint64_t total_execute = 0;
  uint64_t f, s, execute_zeros = 0;
  bool first = true;
  for(auto &kv : full_execute_ipc) {
    s = kv.first;
    if (!first) {
      if (s-f>2) {
          execute_zeros+=s-f;
        //std::cout << s-f << std::endl;
      }
    }
    f = s;
    first = false;
    total_execute += kv.second;
  }
  std::cout << "execute ipc: " << (1.0*total_execute)/(full_execute_ipc.size()+execute_zeros) << std::endl;

  //calculate commit ipc
  first = true;
  uint64_t commit_zeros = 0;
  uint64_t total_commit = 0;
  for (auto& kv:full_commit_ipc) {
    s = kv.first;
    if (!first) {
      if (s-f>1) {
        if (s-f<11) {
          commit_zeros++;
        }
      }
    }
    f = s;
    first = false;
    total_commit += kv.second;
  }
  std::cout << "commit ipc: " << 1.0*total_commit/(full_commit_ipc.size()+commit_zeros) << " | commit zeros = " << commit_zeros << std::endl;
  std::cout << "total_commit: " << total_commit << std::endl;
  std::cout << "uncommited: " << update_count << std::endl;
}
// FULL IPC END
/////////////////////////////////

void wavesnap::add_to_RAT(DInst *dinst) {
  uint64_t dst, src1, src2;
  bool     skip = false;
  if(dinst->getInst()->isStore()) {
    // dst =  ((uint64_t)dinst->getAddr()%HASH_SIZE)+REGISTER_NUM;
    dst  = ((uint64_t)dinst->getAddr()) + REGISTER_NUM;
    src1 = (uint32_t)dinst->getInst()->getSrc1();
    src2 = (uint32_t)dinst->getInst()->getSrc2();
  } else if(dinst->getInst()->isLoad()) {
    dst  = (uint32_t)dinst->getInst()->getDst1();
    src1 = (uint32_t)dinst->getInst()->getSrc1();
    src2 = ((uint64_t)dinst->getAddr()) + REGISTER_NUM;
  } else if(dinst->getInst()->isControl()) {
    // todo
    dst  = (uint32_t)dinst->getInst()->getDst1();
    src1 = (uint32_t)dinst->getInst()->getSrc1();
    src2 = (uint32_t)dinst->getInst()->getSrc2();
    /*
    std::cout << dinst->getInst()->getOpcodeName() << " ";
    std::cout << dst << ",";
    std::cout << src1 << ",";
    std::cout << src2 << "," << std::endl;*/
    skip = true;
  } else {
    dst  = (uint32_t)dinst->getInst()->getDst1();
    src1 = (uint32_t)dinst->getInst()->getSrc1();
    src2 = (uint32_t)dinst->getInst()->getSrc2();
  }

  if(false) {
    int32_t p1_depth = 0;
    int32_t p2_depth = 0;

    RAT[dst];
    RAT[dst].valid       = true;
    RAT[dst].id          = (uint32_t)dinst->getID() + 1;
    RAT[dst].dst         = dst;
    RAT[dst].src1        = src1;
    RAT[dst].src2        = src2;
    RAT[dst].opcode_name = dinst->getInst()->getOpcodeName();
    RAT[dst].opcode      = (uint32_t)dinst->getInst()->getOpcode();
    RAT[dst].time_stamp  = (uint64_t)globalClock;
  }
}

void wavesnap::merge() {
  // TODO
}
