#include "lightson.h"

Lightson::Lightson() {
  count  = 0;
  schema = "";
  str    = "";
}

void Lightson::register_member(string name) {
  if(schema != "")
    schema += ",";
  schema += name;
  count++;
}

void Lightson::clear() {
  str = "";
}

void Lightson::push(int val) {
  if(str != "")
    str += ",";
  ostringstream s;
  s << val;
  str += s.str();
}

void Lightson::push(double val) {
  if(str != "")
    str += ",";
  ostringstream s;
  s << val;
  str += s.str();
}

void Lightson::push(string val) {
  if(str != "")
    str += ",";
  str += "\"" + val + "\"";
}

string Lightson::get_schema() {
  return schema;
}

string Lightson::get_data() {
  return str;
}

void Lightson::set_data(string s) {
  str = s;
}

int Lightson::pull_int() {
  int          out;
  stringstream ss(pull_element());
  ss >> out;
  return out;
}

double Lightson::pull_double() {
  double       out;
  stringstream ss(pull_element());
  ss >> out;
  return out;
}

string Lightson::pull_string() {
  return pull_element();
}

string Lightson::pull_element() {
  int  i;
  bool f = true;
  for(i = 0; i < str.length() && f; i++) {
    if(str.at(i) == ',') {
      f = false;
      i--;
    }
  }
  string out = str.substr(1, i - 2);
  if(f)
    str = "";
  else
    str = str.substr(i + 1);
  return out;
}
