#include <llvm/ADT/DenseMap.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <vector>

using Key = unsigned;
using Val = uintptr_t;
using Keys = std::vector<Key>;
using SysTimePt = std::chrono::time_point<std::chrono::high_resolution_clock>;

struct TimeSpan {
  SysTimePt startTime, endTime;
  std::string title;
  TimeSpan(SysTimePt a, SysTimePt b, std::string title)
      : startTime(a), endTime(b), title(title) {}
};

static SysTimePt getSysClockStamp() {
  return std::chrono::high_resolution_clock::now();
}

struct Owner {
  std::map<Key, Val> stdMap;
  llvm::DenseMap<Key, Val> llvmMap;
  std::vector<TimeSpan> times;

  Owner(const Keys &keys) {
    // Populate the stdc++ map.
    auto a = getSysClockStamp();
    for (const auto &k : keys) stdMap[k] = -k;
    auto b = getSysClockStamp();
    times.push_back({a, b, "Populate std::map"});

    // Populate the llvm map.
    a = getSysClockStamp();
    for (const auto &k : keys) llvmMap[k] = -k;
    b = getSysClockStamp();
    times.push_back({a, b, "Populate llvm::DenseMap"});
  }
};

std::ostream &operator<<(std::ostream &os, const TimeSpan &ts) {
  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(ts.endTime -
                                                                 ts.startTime)
                .count();
  return os << ts.title << ": " << ns;
}

std::ostream &operator<<(std::ostream &os, const Owner &o) {
  os << "----- Results -----" << std::endl;
  for (const auto &ts : o.times) os << ts << std::endl;
  return os << "----- End -----";
}

static Keys initKeys(size_t nKeys) {
  auto vals = Keys(nKeys);
  auto seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 rng(seed);
  std::generate_n(vals.begin(), nKeys, [&rng]() { return rng(); });
  return vals;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <# of keys to measure>" << std::endl;
    return 0;
  }

  // Initialize the seed map.  This provides the random keys for the maps, and
  // later on is used to visit each key.
  const unsigned nKeys = std::atoi(argv[1]);
  Keys keys = initKeys(nKeys);
  Owner foo(keys);

  std::cout << foo << std::endl;

  return 0;
}

