#pragma once

namespace dirt {

class Predicate {
public:
  i32 dx;
  i32 dy;
  i32 dz;
  i32 r;

  static std::optional<Predicate> FromJSON(std::string const &json) {
    using namespace std;
    if (!json.starts_with("{") || !json.ends_with("}")) {
      return nullopt;
    }
    string str = json.substr(1, json.size() - 2);
    optional<i32> dx;
    optional<i32> dy;
    optional<i32> dz;
    optional<i32> r;
    while (!str.empty()) {
      auto found = str.find(':');
      if (found == string::npos) {
        break;
      }
      string key = str.substr(0, found);
      str = str.substr(found + 1);
      found = str.find(',');
      string value;
      if (found == string::npos) {
        value = str;
        str.clear();
      } else {
        value = str.substr(0, found);
        str = str.substr(found + 1);
      }
      i32 t;
      if (sscanf(value.c_str(), "%d", &t) != 1) {
        return nullopt;
      }
      if (key == "dx") {
        if (dx) {
          return nullopt;
        }
        dx = t;
      } else if (key == "dy") {
        if (dy) {
          return nullopt;
        }
        dy = t;
      } else if (key == "dz") {
        if (dz) {
          return nullopt;
        }
        dz = t;
      } else if (key == "r") {
        if (r) {
          return nullopt;
        }
        r = t;
      } else {
        return nullopt;
      }
    }
    if (dx && dy && dz && r) {
      Predicate p;
      p.dx = *dx;
      p.dy = *dy;
      p.dz = *dz;
      p.r = *r;
      return p;
    } else {
      return nullopt;
    }
  }
};

} // namespace dirt
