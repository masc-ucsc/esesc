#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
using namespace std;

class Lightson {
private:
  string str;
  int    count;
  string schema;

  string pull_element();

public:
  Lightson();
  void   register_member(string name);
  void   clear();
  void   push(int val);
  void   push(double val);
  void   push(string val);
  string get_schema();
  string get_data();
  void   set_data(string s);
  int    pull_int();
  double pull_double();
  string pull_string();
};