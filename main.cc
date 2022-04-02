#include <llvm/ADT/DenseMap.h>
#include <sys/user.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <vector>

// Modify these to test other sizes and map containers.
using Key = uint64_t;
using Val = uint64_t;
using MapA = std::map<Key, Val>;
using MapB = llvm::DenseMap<Key, Val>;

using Keys = std::vector<Key>;
using SysTimePt = std::chrono::time_point<std::chrono::high_resolution_clock>;

// A place to write scratch data, to hopefully avoid the compiler eliminating
// desired functionality.
uintptr_t dummy;

// Each test we collect a start and end time.  That pair make up a TimeSpan.
struct TimeSpan final {
  SysTimePt startTime, endTime;
  std::string prefix, title;
  TimeSpan(SysTimePt a, SysTimePt b, std::string prefix, std::string title)
      : startTime(a), endTime(b), prefix(prefix), title(title) {}
};

static SysTimePt getSysClockStamp() {
  return std::chrono::high_resolution_clock::now();
}

// This class holds the two map structures being compared, and also perfoms
// the test/measurements.
template <typename MapTypeA, typename MapTypeB>
struct Owner final {
  MapTypeA mapA;
  MapTypeB mapB;
  std::string mapAName = typeid(MapTypeA).name();
  std::string mapBName = typeid(MapTypeB).name();
  size_t nKeys;
  std::vector<TimeSpan> times;

  // Measure the time performance of filling up the maps.
  void populate(const Keys &keys) {
    nKeys = keys.size();

    // Populate the mapA.
    auto a = getSysClockStamp();
    for (const auto &k : keys) mapA[k] = (Val)&k;
    auto b = getSysClockStamp();
    times.push_back({a, b, "MapA", "Populate"});

    // Populate the mapB.
    a = getSysClockStamp();
    for (const auto &k : keys) mapB[k] = (Val)&k;
    b = getSysClockStamp();
    times.push_back({a, b, "MapB", "Populate"});
  }

  // Measure the time performance of accessing items from the maps.
  void randomAccess(const Keys &keys) {
    assert(nKeys && !keys.empty() && !mapA.empty() && !mapB.empty());

    // Scan mapA looking/adding keys via operator[].
    auto a = getSysClockStamp();
    for (const auto &k : keys) dummy |= (uintptr_t)mapA[k];
    auto b = getSysClockStamp();
    times.push_back({a, b, "MapA", "Random access via operator[]"});

    // Scan mapA looking/adding keys via operator[].
    a = getSysClockStamp();
    for (const auto &k : keys) dummy |= (uintptr_t)mapA[k];
    b = getSysClockStamp();
    times.push_back({a, b, "MapB", "Random access via operator[]"});

    // Scan mapA looking/adding keys via find().
    a = getSysClockStamp();
    for (const auto &k : keys)
      if (auto v = mapA.find(k); v != mapA.end()) dummy |= (uintptr_t)v->second;
    b = getSysClockStamp();
    times.push_back({a, b, "MapA", "Random access via find()"});

    // Scan mapB looking/adding keys via find().
    for (const auto &k : keys)
      if (auto v = mapB.find(k); v != mapB.end()) dummy |= (uintptr_t)v->second;
    b = getSysClockStamp();
    times.push_back({a, b, "MapB", "Random access via find()"});
  }

  // Count the number of unique pages for the keys and val in each map.
  std::tuple<size_t, size_t> measureFrag() const {
    std::set<uintptr_t> mapAPages;
    for (const auto &k : mapA) {
      uintptr_t keyPageAddr = (uintptr_t)&k.first & PAGE_MASK;
      uintptr_t valPageAddr = (uintptr_t)&k.second & PAGE_MASK;
      mapAPages.insert(keyPageAddr);
      mapAPages.insert(valPageAddr);
    }

    std::set<uintptr_t> mapBPages;
    for (const auto &k : mapB) {
      uintptr_t keyPageAddr = (uintptr_t)&k.first & PAGE_MASK;
      uintptr_t valPageAddr = (uintptr_t)&k.second & PAGE_MASK;
      mapBPages.insert(keyPageAddr);
      mapBPages.insert(valPageAddr);
    }
    return {mapAPages.size(), mapBPages.size()};
  }
};

std::ostream &operator<<(std::ostream &os, const TimeSpan &ts) {
  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(ts.endTime -
                                                                 ts.startTime)
                .count();
  return os << ts.prefix << ", " << ts.title << ", " << ns << ", nanoseconds";
}

template <typename MapTypeA, typename MapTypeB>
std::ostream &operator<<(std::ostream &os, const Owner<MapTypeA, MapTypeB> &o) {
  auto [nMapAPages, nMapBPages] = o.measureFrag();
  os << "--> Input keys:  " << o.nKeys << " keys" << std::endl
     << "--> MapA type:   " << o.mapAName << std::endl
     << "--> MapB type:   " << o.mapBName << std::endl
     << "--> MapA:        " << o.mapA.size() << " keys across " << nMapAPages
     << " pages" << std::endl
     << "--> MapB:        " << o.mapB.size() << " keys across " << nMapBPages
     << " pages" << std::endl;
  for (const auto &ts : o.times) os << ts << std::endl;
  return os;
}

// Generate 'nKeys' random values to use as keys into the maps.
static Keys initKeys(size_t nKeys) {
  auto vals = Keys(nKeys);
  auto seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::minstd_rand0 rng(seed);
  std::generate_n(vals.begin(), nKeys, [&rng]() { return rng(); });
  return vals;
}

// Initialize the maps and perform the test measurements.
static void runTest(unsigned nKeys, unsigned id) {
  // Initialize the seed map.  This provides the random keys for the maps, and
  // later on is used to visit each key.
  Keys keys = initKeys(nKeys);
  Owner<MapA, MapB> foo;

  // Conduct the measurements.
  foo.populate(keys);
  foo.randomAccess(keys);
  std::cout << "----------{ Trial " + std::to_string(id) << " }----------"
            << std::endl
            << foo << std::endl;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <# of trials> <# of keys to measure>"
              << std::endl;
    return 0;
  }
  const unsigned nTrials = std::atoi(argv[1]);
  const unsigned nKeys = std::atoi(argv[2]);
  for (unsigned i = 0; i < nTrials; ++i) runTest(nKeys, i + 1);
  return 0;
}
