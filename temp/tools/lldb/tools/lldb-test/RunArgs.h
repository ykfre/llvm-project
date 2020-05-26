#pragma once
#include "lldb/API/SBDebugger.h"
#include "lldb/Host/HostInfoBase.h"
#include "lldb/Symbol/ClangASTContext.h"
#include "lldb/Target/DynamicLoader.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/TargetList.h"
#include "lldb/core/Debugger.h"
#include "lldb/core/Module.h"
#include "lldb/core/Section.h"
#include "lldb/lldb-forward.h"
#include "lldb/target/target.h"
#include <bitset>
#include <memory>
#include <string>

struct RunArgs {
  void *start_address = nullptr;
  std::string file_path;
  std::function<void()> suspendThread;
  std::function<void()> resumeThread;
  std::function<size_t(void *, void *, size_t)> readMemory;

  std::function<size_t(void *, const void *, size_t)> writeMemory;
  std::function<int()> getThreadId;
  std::function<std::bitset<128>(const std::string &)> getRegisterValue;
  std::function<void(const std::string &, std::bitset<128>)> setRegisterValue;
  std::shared_ptr<lldb_private::Stream> stream;
  std::function<void *(size_t)> allocateMemory;
  std::function<void(void *memory)> deallocateMemory;
  std::function<void(void *memory)> addBreakpoint;
  std::function<lldb::ExpressionResults(
      lldb_private::ExecutionContext &exe_ctx)>
      runThreadPlan;
  std::string expression;
  int frameIndex = -1;
  bool bpAddress;
};
