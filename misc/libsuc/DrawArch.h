#include <fstream>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "nanassert.h"

#define GOOGLE_GRAPH_API 0
using namespace std;

class DrawArch {
protected:
  std::vector<std::string> rows;

public:
  DrawArch(){
      // Constructor
  };
  ~DrawArch(){
  }

  void addObj(const std::string &mystr) {
    rows.push_back(mystr);
  }

#if GOOGLE_GRAPH_API
  void drawArchHtml(const char *filename) {
    if(filename == NULL) {
      MSG("Invalid (NULL) file name. Cannot generate the architecture file");
      return;
    }

    std::fstream fs(filename, std::fstream::out);
    if(!fs.good()) {
      std::cerr << "WriteFile() : Opening file failed." << std::endl;
      return;
    }
    fs << "<html>";
    fs << "\n<head>";
    fs << "\n<script type='text/javascript' src='https://www.google.com/jsapi'></script>";
    fs << "\n<script type='text/javascript'> google.load('visualization', '1', {packages:['orgchart']});";
    fs << "\ngoogle.setOnLoadCallback(drawChart);";
    fs << "\nfunction drawChart() {";
    fs << "\nvar data = new google.visualization.DataTable();";
    fs << "\ndata.addColumn('string', 'Name');";
    fs << "\ndata.addColumn('string', 'Type');";
    fs << "\ndata.addColumn('string', 'ToolTip');";
    fs << "\ndata.addRows([";

    std::string str;
    for(size_t i = 0; i < rows.size(); i++) {
      str = rows.at(i);
      fs << "\n";
      fs << str.c_str();
      if(i < (rows.size() - 1))
        fs << ",";
    }

    fs << "\n]);";
    fs << "\nvar chart = new google.visualization.OrgChart(document.getElementById('chart_div')); ";
    fs << "\nchart.draw(data, {allowHtml:true, size:'medium'});";
    fs << "\n}";
    fs << "\n</script>";
    fs << "\n</head>";
    fs << "\n<body> ";
    fs << "\n<div id='chart_div'></div> ";
    fs << "\n</body> ";
    fs << "\n</html>";
  }
#else
  void drawArchDot(const char *filename) {
    if(filename == NULL) {
      MSG("Invalid (NULL) file name. Cannot generate the architecture file");
      return;
    }

    std::fstream fs(filename, std::fstream::out);
    if(!fs.good()) {
      std::cerr << "WriteFile() : Opening file failed." << std::endl;
      return;
    }

    fs << "\ndigraph simple_hierarchy {";
    fs << "\n\nnode [color=Green,fontcolor=Blue,font=Courier,shape=record]\n";

    std::string str;
    for(size_t i = 0; i < rows.size(); i++) {
      str = rows.at(i);
      fs << "\n";
      fs << str.c_str();
      // if (i < (rows.size() - 1))
      //  fs << ",";
    }

    fs << "\n}\n";
  }
#endif
};
