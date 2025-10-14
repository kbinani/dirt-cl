#pragma once

namespace dirt {

class Predicate {
public:
  int32_t dx;
  int32_t dy;
  int32_t dz;
  int32_t r;

  static std::optional<Predicate> FromJSON(std::string const &json) {
    using namespace std;
    if (!json.starts_with("{") || !json.ends_with("}")) {
      return nullopt;
    }
    string str = json.substr(1, json.size() - 2);
    optional<int32_t> dx;
    optional<int32_t> dy;
    optional<int32_t> dz;
    optional<int32_t> r;
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
      int32_t t;
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
