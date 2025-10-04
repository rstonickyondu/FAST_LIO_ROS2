#pragma once
#include <chrono>
#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <iostream>

// Lightweight header-only profiling helpers. Enabled when FASTLIO_PROFILE defined.
// Usage:
//   FASTLIO_SCOPE("deskew");
//   FASTLIO_SCOPE_ACCUM("map_incremental", some_double_var);
// Dump suggestions: aggregate using FASTLIOProfilerRegistry::instance().snapshot();

#ifdef FASTLIO_PROFILE
namespace fastlio_profile {
struct ScopeResult { double total_ms{0}; uint64_t count{0};};
class Registry {
 public:
  static Registry &instance() { static Registry r; return r; }
  void add(const std::string &name, double ms) {
    std::lock_guard<std::mutex> lk(m_);
    auto &e = data_[name];
    e.total_ms += ms; e.count++; }
  std::unordered_map<std::string, ScopeResult> snapshot() {
    std::lock_guard<std::mutex> lk(m_); return data_; }
 private: std::mutex m_; std::unordered_map<std::string, ScopeResult> data_; };
class Scope {
 public:
  explicit Scope(const char *name): name_(name), start_(Clock::now()) {}
  ~Scope(){ auto end = Clock::now();
    double ms = std::chrono::duration<double,std::milli>(end-start_).count();
    Registry::instance().add(name_, ms);
  }
 private:
  using Clock = std::chrono::steady_clock; const char *name_; Clock::time_point start_; };
}
#define FASTLIO_JOIN2(a,b) a##b
#define FASTLIO_JOIN(a,b) FASTLIO_JOIN2(a,b)

#define FASTLIO_SCOPE(name) fastlio_profile::Scope FASTLIO_JOIN(fastlio_scope_obj_, __LINE__)(name);

#define FASTLIO_SCOPE_ACCUM(name, accum_var) \
  auto FASTLIO_JOIN(fastlio_scope_begin_, __LINE__) = std::chrono::steady_clock::now(); \
  struct FASTLIO_JOIN(fastlio_scope_guard_, __LINE__) { double *acc; std::chrono::steady_clock::time_point b; \
    ~FASTLIO_JOIN(fastlio_scope_guard_, __LINE__)(){ auto e=std::chrono::steady_clock::now(); *acc += std::chrono::duration<double>(e-b).count(); } \
  } FASTLIO_JOIN(fastlio_scope_guard_inst_, __LINE__){&(accum_var), FASTLIO_JOIN(fastlio_scope_begin_, __LINE__)};
#else
#define FASTLIO_SCOPE(name)
#define FASTLIO_SCOPE_ACCUM(name, accum_var)
#endif
