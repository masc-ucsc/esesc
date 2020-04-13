
#include <cmath>

#include "wavesnap.h"

wavesnap::wavesnap() {
  this->window_pointer         = 0;
  this->update_count           = 0;
  this->first_window_completed = false;
  this->signature_count        = 0;
  this->current_encoding       = "";
  #ifdef RECORD_ONCE
    for (uint64_t i=0; i<HASH_SIZE; i++) {
      this->signature_hit.push_back(false);
    }
  #endif
}

wavesnap::~wavesnap() {
  // nothing to do here
}

uint64_t wavesnap::hash(std::string signature, uint64_t more) {
  uint64_t hash = HASH_SEED + more;
  for (uint32_t i = 0; i < signature.length(); i++) {
    hash = (hash * 101 + (uint32_t)signature[i])%HASH_SIZE;
  }

  return hash;
}


void wavesnap::record_pipe(pipeline_info *next) {
  #ifdef HASHED_RECORD
    uint64_t index = this->current_hash;
  #else
    std::string index = this->current_encoding;
  #endif

  this->signature_count++;

  this->window_sign_info[index];
  pipeline_info *pipe_info = &(this->window_sign_info[index]);
  if(pipe_info->execute_cycles.size() == 0) {
    #ifdef RECORD_ONCE
      if(pipe_info->count == 0) {
        pipe_info->count = 2;
      } else {
        pipe_info->count++;
      }

      if(pipe_info->count > COUNT_ALLOW) {
        *pipe_info = *next;
      }
    #else
      *pipe_info = *next;
    #endif
  } else {
    pipe_info->count++;
    #ifndef RECORD_ONCE
      for(uint32_t j = 0; j < pipe_info->execute_cycles.size(); j++) {
        pipe_info->wait_cycles[j] += next->wait_cycles[j];
        pipe_info->rename_cycles[j] += next->rename_cycles[j];
        pipe_info->issue_cycles[j] += next->issue_cycles[j];
        pipe_info->execute_cycles[j] += next->execute_cycles[j];
        pipe_info->commit_cycles[j] += next->commit_cycles[j];
      }
    #endif
  }
}

void wavesnap::add_pipeline_info(pipeline_info* pipe_info, instruction_info* d) {
  uint64_t min_time = this->dinst_info[wait_buffer[0]].fetched_time;

  pipe_info->wait_cycles.push_back(d->fetched_time - min_time);
  pipe_info->rename_cycles.push_back(d->renamed_time - d->fetched_time);
  pipe_info->issue_cycles.push_back(d->issued_time - d->renamed_time);
  pipe_info->execute_cycles.push_back(d->executed_time - d->issued_time);
  pipe_info->commit_cycles.push_back(d->committed_time - d->executed_time);
}

void wavesnap::add_instruction(DInst *dinst) {
  wait_buffer.push_back(dinst->getID());
  completed.push_back(false);
}

instruction_info wavesnap::extract_inst_info(DInst* dinst, uint64_t committed) {
  instruction_info result;

  result.fetched_time   = dinst->getFetchedTime();
  result.renamed_time   = dinst->getRenamedTime();
  result.issued_time    = dinst->getIssuedTime();
  result.executed_time  = dinst->getExecutedTime();
  result.committed_time = committed;
  result.opcode         = dinst->getInst()->getOpcode();
  result.pc             = dinst->getPC();
  return result;
}

void wavesnap::update_window(DInst *dinst, uint64_t committed) {
  // update instruction as completed
  bool     found = false;
  uint64_t i     = dinst->getID();
  uint64_t id    = i;
  if(i > this->wait_buffer.size()) {
    i = this->wait_buffer.size() - 1;
  }
  while(!found) {
    if(this->wait_buffer[i] == id) {
      this->completed[i] = true;
      found              = true;

      instruction_info inst_info = extract_inst_info(dinst, committed);
      dinst_info[id];
      dinst_info[id] = inst_info;
    }
    i--;
  }

  // poll the very first window, wait until completed
  if(!this->first_window_completed) {
    this->first_window_completed = true;
    for(uint32_t i = 0; i < MAX_MOVING_GRAPH_NODES; i++) {
      if(!this->completed[i]) {
        this->first_window_completed = false;
        this->current_encoding = "";
        break;
      } else {
        instruction_info *d = &(dinst_info[wait_buffer[i]]);
        if(i < MAX_MOVING_GRAPH_NODES-1) {
          this->current_encoding += ENCODING[d->opcode];
        }
      }
    }
  } else {
    // after the first window completed, just check the last element
    // of the window.
    while(completed[MAX_MOVING_GRAPH_NODES - 1] && completed.size() >= MAX_MOVING_GRAPH_NODES) {

      //add last instruction of the window
      instruction_info *d = &(dinst_info[wait_buffer[MAX_MOVING_GRAPH_NODES - 1]]);
      this->current_encoding += ENCODING[d->opcode];
      this->current_hash = this->hash(this->current_encoding, d->pc);
      #ifndef RECORD_ONCE
        pipeline_info next;
        next.count = 1;
        for (uint32_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
          add_pipeline_info(&next, &(dinst_info[wait_buffer[i]]));
        }

        // record
        record_pipe(&next);
      #else
        #ifdef HASHED_RECORD
          if(!signature_hit[this->current_hash]) {
            signature_hit[this->current_hash] = true;
          } else {
            pipeline_info next;
            for (uint32_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
              add_pipeline_info(&next, &(dinst_info[wait_buffer[i]]));
            }

            //record
            record_pipe(&next);
          }
        #else
          std::cout << "RECORD_ONCE must be used with HASHED_RECORD defined" << std::endl;
        #endif
      #endif

      // remove first instruction in the window and update stats
      window_pointer++;
      this->dinst_info.erase(this->wait_buffer[0]);
      this->wait_buffer.erase(this->wait_buffer.begin());
      this->completed.erase(this->completed.begin());
      this->current_encoding.erase(this->current_encoding.begin());
    }
  }
}

void wavesnap::calculate_ipc() {
  uint64_t total_fetch_ipc    = 0;
  uint64_t total_rename_ipc   = 0;
  uint64_t total_issue_ipc    = 0;
  uint64_t total_execute_ipc  = 0;
  uint64_t total_commit_ipc   = 0;
  float    total_fetch_diff   = 0;
  float    total_rename_diff  = 0;
  float    total_issue_diff   = 0;
  float    total_execute_diff = 0;
  float    total_commit_diff  = 0;
  uint64_t total_count        = 0;

  for(auto &sign_kv : window_sign_info) {
    pipeline_info pipe_info = sign_kv.second;
    uint64_t      count     = pipe_info.count;
    if(count<=COUNT_ALLOW) continue;

    total_count += count;
    // average the cycles
    std::map<uint32_t, uint32_t> w_c;
    std::map<uint32_t, uint32_t> r_c;
    std::map<uint32_t, uint32_t> i_c;
    std::map<uint32_t, uint32_t> e_c;
    std::map<uint32_t, uint32_t> c_c;
    w_c.clear();
    r_c.clear();
    i_c.clear();
    e_c.clear();
    c_c.clear();

    for(uint32_t j = 0; j < pipe_info.execute_cycles.size(); j++) {
      // average
      uint32_t w, r, i, e, c;
      #ifdef RECORD_ONCE
        w = pipe_info.wait_cycles[j];
        r = pipe_info.rename_cycles[j];
        i = pipe_info.issue_cycles[j];
        e = pipe_info.execute_cycles[j];
        c = pipe_info.commit_cycles[j];
      #else
        w = pipe_info.wait_cycles[j] / count;
        r = pipe_info.rename_cycles[j] / count;
        i = pipe_info.issue_cycles[j] / count;
        e = pipe_info.execute_cycles[j] / count;
        c = pipe_info.commit_cycles[j] / count;
      #endif


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
      //commit
      f += c;
      c_c[f];
      c_c[f]++;
    }

    // calculate ipc at each stage
    bool first_iter;
    uint32_t f, s, total, zeros;
    // fetch:
    total = 0;
    zeros = 0;
    first_iter = true;
    for(auto &kv : w_c) {
      s = kv.first;
      if(!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
        zeros += s - f - 1;
      }
      f = s;
      first_iter = false;

      total += kv.second;
    }
    float fetch_ipc = 1.0 * total / (w_c.size() + zeros);

    // rename:
    total = 0;
    zeros = 0;
    first_iter = true;
    for(auto &kv : r_c) {
      s = kv.first;
      if(!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
        zeros += s - f - 1;
      }
      f = s;
      first_iter = false;

      total += kv.second;
    }
    float rename_ipc = 1.0 * total / (r_c.size() + zeros);

    // issue
    total = 0;
    zeros = 0;
    first_iter = true;
    for(auto &kv : i_c) {
      s = kv.first;
      if(!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
        zeros += s - f - 1;
      }
      f = s;
      first_iter = false;

      total += kv.second;
    }
    float issue_ipc = 1.0 * total / (i_c.size() + zeros);

    // execute
    total = 0;
    zeros = 0;
    first_iter = true;
    for(auto &kv : e_c) {
      s = kv.first;
      if(!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
        zeros += s - f - 1;
      }
      f = s;
      first_iter = false;

      total += kv.second;
    }
    float execute_ipc = 1.0 * total / (e_c.size() + zeros);

    // commit
    total = 0;
    zeros = 0;
    first_iter = true;
    for(auto &kv : c_c) {
      s = kv.first;
      if(!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
        zeros += s - f - 1;
      }
      f = s;
      first_iter = false;

      total += kv.second;
    }
    float commit_ipc = 1.0 * total / (c_c.size() + zeros);

    // determine how much this signature contributes to the overall ipc
    // 1.accumulate diffs
    total_fetch_diff += (1.0 * std::ceil(fetch_ipc) - fetch_ipc) * count;
    total_rename_diff += (1.0 * std::ceil(rename_ipc) - rename_ipc) * count;
    total_issue_diff += (1.0 * std::ceil(issue_ipc) -  issue_ipc) * count;
    total_execute_diff += (1.0 * std::ceil(execute_ipc) - execute_ipc) * count;
    total_commit_diff += (1.0 * std::ceil(commit_ipc) - commit_ipc) * count;

    // 2.accumulate ipcs
    total_fetch_ipc += std::ceil(fetch_ipc) * count;
    total_rename_ipc += std::ceil(rename_ipc) * count;
    total_issue_ipc += std::ceil(issue_ipc) * count;
    total_execute_ipc += std::ceil(execute_ipc) * count;
    total_commit_ipc += std::ceil(commit_ipc) * count;
  }

  //report
  std::cout << "-------windowed ipc calculation-----------" << std::endl;
  std::cout << "fetch:   " << (1.0 * total_fetch_ipc - total_fetch_diff) / total_count << std::endl;
  std::cout << "rename:  " << (1.0 * total_rename_ipc - total_rename_diff) / total_count << std::endl;
  std::cout << "issue:   " << (1.0 * total_issue_ipc - total_issue_diff) / total_count << std::endl;
  std::cout << "execute: " << (1.0 * total_execute_ipc - total_execute_diff) / total_count << std::endl;
  std::cout << "commit:  " << (1.0 * total_commit_ipc - total_commit_diff) / total_count << std::endl;
  std::cout << "------------------------------------------" << std::endl;
}
// WINDOW BASED IPC END
////////////////////////////////////////

/////////////////////////////////
//FULL IPC UPDATE and CALCULATION
void wavesnap::update_single_window(DInst* dinst, uint64_t committed) {
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

  this->full_commit_ipc[committed];
  this->full_commit_ipc[committed]++;

}

void wavesnap::calculate_single_window_ipc() {
  bool first_iter;
  uint64_t f, s;

  // calculate fetch ipc
  uint64_t total_fetch = 0;
  uint64_t fetch_zeros = 0;
  first_iter = true;
  for(auto &kv : full_fetch_ipc) {
    s = kv.first;
    if(!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
      fetch_zeros += s - f - 1;
    }
    f = s;
    first_iter = false;

    total_fetch += kv.second;
  }

  // calculate rename ipc
  uint64_t total_rename = 0;
  uint64_t rename_zeros = 0;
  first_iter = true;
  for(auto &kv : full_rename_ipc) {
    s = kv.first;
    if(!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
      rename_zeros += s - f - 1;
    }
    f = s;
    first_iter = false;

    total_rename += kv.second;
  }

  // calculate issue ipc
  uint64_t total_issue = 0;
  uint64_t issue_zeros = 0;
  for(auto &kv : full_issue_ipc) {
    s = kv.first;
    if(!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
      rename_zeros += s - f - 1;
    }
    f = s;
    first_iter = false;

    total_issue += kv.second;
  }

  // calculate execute ipc
  uint64_t total_execute = 0;
  uint64_t execute_zeros = 0;
  for(auto &kv : full_execute_ipc) {
    s = kv.first;
    if (!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
      execute_zeros += s - f - 1;
    }

    f = s;
    first_iter = false;
    total_execute += kv.second;
  }

  //calculate commit ipc
  first_iter = true;
  uint64_t total_commit = 0;
  uint64_t commit_zeros = 0;
  for (auto& kv : full_commit_ipc) {
    s = kv.first;
    if (!first_iter && (s - f - 1) < INSTRUCTION_GAP) {
      commit_zeros += s - f - 1;
    }
    f = s;
    first_iter = false;
    total_commit += kv.second;
  }

  //report
  std::cout << "--------------------" << std::endl;
  std::cout << "fetch ipc:   " << 1.0 * total_fetch / (full_fetch_ipc.size() + fetch_zeros) << std::endl;
  std::cout << "rename ipc:  " << 1.0 * total_rename / (full_rename_ipc.size() + rename_zeros) << std::endl;
  std::cout << "issue ipc:   " << 1.0 * total_issue / (full_issue_ipc.size() + issue_zeros) << std::endl;
  std::cout << "execute ipc: " << 1.0 * total_execute / (full_execute_ipc.size() + execute_zeros) << std::endl;
  std::cout << "commit ipc:  " << 1.0 * total_commit / (full_commit_ipc.size() + commit_zeros) << std::endl;
  std::cout << "--------------------" << std::endl;
}
// FULL IPC END
/////////////////////////////////

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

void wavesnap::window_frequency() {
  uint8_t threshold = 80;

  std::vector<uint64_t> counts;
  for(auto &sign_kv : window_sign_info) {
    pipeline_info pipe_info = sign_kv.second;
    uint64_t      count     = pipe_info.count;
    counts.push_back(count);
  }

  std::sort(counts.rbegin(), counts.rend());

  std::ofstream outfile;
  outfile.open(DUMP_PATH);
  for (uint64_t i=0; i<counts.size(); i++) {
    outfile <<  counts[i] << std::endl;
  }
  outfile.close();

  float total_percent = 0;
  uint32_t i;
  for(i=0; i<counts.size(); i++) {
    float curr_percent = (100.0 * counts[i]) / this->signature_count;
    total_percent += curr_percent;
    if(total_percent>threshold) {
      break;
    }
  }

  std::cout << "********************" << std::endl;
  std::cout << (uint32_t)threshold << " perc of the instructions are covered by " << (100.0 * i) / counts.size() << " of windows" << std::endl;
  std::cout << "counts size = " << counts.size() << " | " << i << std::endl;
  std::cout << "********************" << std::endl;
}

std::string wavesnap::break_into_bytes(uint64_t n, uint8_t byte_num) {
  std::string result = "";
  while(byte_num>0) {
    result = (char)(n&0x00FF) + result;
    n = n >> 8;
    byte_num--;
  }

  return result;
}

void wavesnap::save() {
  std::ofstream outfile;
  outfile.open(DUMP_PATH);
  for(auto &sign_kv : window_sign_info) {
    outfile << sign_kv.first;
    outfile << break_into_bytes(sign_kv.second.count, 4);
    for (uint32_t i=0; i<MAX_MOVING_GRAPH_NODES; i++) {
      outfile << break_into_bytes(sign_kv.second.wait_cycles[i], 4);
      outfile << break_into_bytes(sign_kv.second.rename_cycles[i], 4);
      outfile << break_into_bytes(sign_kv.second.issue_cycles[i], 4);
      outfile << break_into_bytes(sign_kv.second.execute_cycles[i], 4);
      outfile << break_into_bytes(sign_kv.second.commit_cycles[i], 4);
    }
  }
  outfile.close();
}

void wavesnap::load() {

}
