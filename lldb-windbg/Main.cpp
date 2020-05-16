#include "lldb/Target/TargetList.h"
#include "lldb/core/Debugger.h"
#include "lldb/lldb-forward.h"

using namespace llvm;
using namespace lldb;
using namespace lldb_private;

int main() {
  auto debugger = Debugger::CreateInstance();
  LoadDependentFiles load_dependent_files = eLoadDependentsNo;
  TargetSP target_sp;
  auto& targetList = debugger->GetTargetList();
  assert(targetList.CreateTarget(*debugger, "", "",load_dependent_files,nullptr,
                           target_sp).Success());
}