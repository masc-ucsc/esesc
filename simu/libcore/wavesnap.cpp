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

void wavesnap::addNode(uint32_t v1, uint32_t v2) {
  connGraph[v1];
  connGraph[v1].push_back(v2);
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
      } else {
        result = true;
        break;
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

void wavesnap::add(DInst *dinst) {
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
        second_edges_skipped++;
      }
    }

    // addNode(RAT[dst]);
  }
}

void wavesnap::dumpGraph(std::string fileName) {
  uint32_t i = 0;
  uint32_t j = 0;
  std::cout << "Dumping graph..." << std::endl;
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

void wavesnap::dumpEdgeList(std::string fileName) {
  std::cout << "Dumping edge list..." << std::endl;
  std::ofstream output;
  output.open(fileName + "list" + std::to_string(dump_num));
  for(uint32_t i = 0; i < edgeList.size(); i++) {
    output << edgeList[i].to_string() << std::endl;
  }
  output.close();
  edgeList.clear();
}

void wavesnap::dumpConnGraph() {
  std::cout << connGraph.size() << std::endl;
}
