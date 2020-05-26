#include "runArgs.h"

#include <Windows.h>
#include <algorithm>
#include <iostream>
#include <thread>

std::thread g_thread;
void *allocate_memory(size_t size) {
  char* data = new char[size];
  DWORD dummy;
  VirtualProtect(data, size, PAGE_EXECUTE_READWRITE, &dummy);
  return data;
}
void deallocate_memory(void *data) { delete[] data; }
int getThreadId() { return GetThreadId(g_thread.native_handle()); };
size_t ReadMemory(void *addr, void *buf, size_t size) {
  memcpy(buf, (void *)addr, size);
  return size;
}

size_t WriteMemory(void *addr, const void *buf, size_t size) {
  memcpy(addr, buf, size);
  return size;
}

class Stream2 : public lldb_private::Stream {
  void Flush() override {}

  size_t WriteImpl(const void *src, size_t src_len) override {
    for (int i = 0; i < src_len; i++) {
      std::cout << ((char *)src)[i];
    }
    return src_len;
  }
};

void resumeThread() { ResumeThread(g_thread.native_handle()); }

void suspedThread() { SuspendThread(g_thread.native_handle()); }

lldb::ExpressionResults runThreadPlan(lldb_private::ExecutionContext& exe_ctx) {

  auto event = CreateEventA(nullptr, false, false, "me");
  exe_ctx.GetProcessPtr()->Resume();
  WaitForSingleObject(event, INFINITE);
  Sleep(300);
  return lldb::eExpressionCompleted;
}

extern bool run(RunArgs args);

std::bitset<128> getRegisterValue(const std::string &registerName) {
  std::bitset<128> value;
  CONTEXT context;
  context.ContextFlags = CONTEXT_ALL;
  GetThreadContext(g_thread.native_handle(), &context);
  std::string registerNameLower = registerName;
  std::transform(registerNameLower.begin(), registerNameLower.end(),
                 registerNameLower.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (registerNameLower == "rax") {
    return context.Rax;
  } else if (registerNameLower == "rbx") {
    return context.Rbx;
  } else if (registerNameLower == "rcx") {
    return context.Rcx;
  } else if (registerNameLower == "rdx") {
    return context.Rdx;
  } else if (registerNameLower == "rdi") {
    return context.Rdi;
  } else if (registerNameLower == "rsi") {
    return context.Rsi;
  } else if (registerNameLower == "r8") {
    return context.R8;
  } else if (registerNameLower == "r9") {
    return context.R9;
  } else if (registerNameLower == "r10") {
    return context.R10;
  } else if (registerNameLower == "r11") {
    return context.R11;
  } else if (registerNameLower == "r12") {
    return context.R12;
  } else if (registerNameLower == "r13") {
    return context.R13;
  } else if (registerNameLower == "r14") {
    return context.R14;
  } else if (registerNameLower == "r15") {
    return context.R15;
  } else if (registerNameLower == "rbp") {
    return context.Rbp;
  } else if (registerNameLower == "rsp") {
    return context.Rsp;
  } else if (registerNameLower == "rip") {
    return context.Rip;
  } else if (registerNameLower == "eflags") {
    return context.EFlags;
  } else if (registerNameLower == "xmm0") {
    memcpy(&value, &context.Xmm0, 16);
  } else if (registerNameLower == "xmm1") {
    memcpy(&value, &context.Xmm1, 16);
  } else if (registerNameLower == "xmm2") {
    memcpy(&value, &context.Xmm2, 16);
  } else if (registerNameLower == "xmm3") {
    memcpy(&value, &context.Xmm3, 16);
  } else if (registerNameLower == "xmm4") {
    memcpy(&value, &context.Xmm4, 16);
  } else if (registerNameLower == "xmm5") {
    memcpy(&value, &context.Xmm5, 16);
  } else if (registerNameLower == "xmm6") {
    memcpy(&value, &context.Xmm6, 16);
  } else if (registerNameLower == "xmm7") {
    memcpy(&value, &context.Xmm7, 16);
  } else if (registerNameLower == "xmm8") {
    memcpy(&value, &context.Xmm8, 16);
  } else if (registerNameLower == "xmm9") {
    memcpy(&value, &context.Xmm9, 16);
  } else if (registerNameLower == "xmm10") {
    memcpy(&value, &context.Xmm10, 16);
  } else if (registerNameLower == "xmm11") {
    memcpy(&value, &context.Xmm11, 16);
  } else if (registerNameLower == "xmm12") {
    memcpy(&value, &context.Xmm12, 16);
  } else if (registerNameLower == "xmm13") {
    memcpy(&value, &context.Xmm13, 16);
  } else if (registerNameLower == "xmm14") {
    memcpy(&value, &context.Xmm14, 16);
  } else if (registerNameLower == "xmm15") {
    memcpy(&value, &context.Xmm15, 16);
  } else {
    __debugbreak();
  }
  return value;
}

void setRegisterValue(const std::string &registerName, std::bitset<128> value) {
  CONTEXT context;
  context.ContextFlags = CONTEXT_ALL;
  GetThreadContext(g_thread.native_handle(), &context);
  std::string registerNameLower = registerName; 

    std::transform(registerNameLower.begin(), registerNameLower.end(),
                 registerNameLower.begin(),
      [](unsigned char c) { return std::tolower(c); });
  if (registerNameLower == "rax") {
    context.Rax = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "rbx") {
    context.Rbx = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "rcx") {
    context.Rcx = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "rdx") {
    context.Rdx = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "rdi") {
    context.Rdi = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "rsi") {
    context.Rsi = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "r8") {
    context.R8 = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "r9") {
    context.R9 = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "r10") {
    context.R10 = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "r11") {
    context.R11 = (DWORD64)(DWORD64)value.to_ullong();
  } else if (registerNameLower == "r12") {
    context.R12 = (DWORD64)(DWORD64)value.to_ullong();
  } else if (registerNameLower == "r13") {
    context.R13 = (DWORD64)(DWORD64)value.to_ullong();
  } else if (registerNameLower == "r14") {
    context.R14 = (DWORD64)(DWORD64)value.to_ullong();
  } else if (registerNameLower == "r15") {
    context.R15 = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "rbp") {
    context.Rbp = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "rsp") {
    context.Rsp = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "rip") {
    context.Rip = (DWORD64)value.to_ullong();
  } else if (registerNameLower == "eflags") {
    context.EFlags = value.to_ullong();
  } else if (registerNameLower == "xmm0") {
    memcpy(&context.Xmm0, &value, 16);
  } else if (registerNameLower == "xmm1") {
    memcpy(&context.Xmm1, &value, 16);
  } else if (registerNameLower == "xmm2") {
    memcpy(&context.Xmm2, &value, 16);
  } else if (registerNameLower == "xmm3") {
    memcpy(&context.Xmm3, &value, 16);
  } else if (registerNameLower == "xmm4") {
    memcpy(&context.Xmm4, &value, 16);
  } else if (registerNameLower == "xmm5") {
    memcpy(&context.Xmm5, &value, 16);
  } else if (registerNameLower == "xmm6") {
    memcpy(&context.Xmm6, &value, 16);
  } else if (registerNameLower == "xmm7") {
    memcpy(&context.Xmm7, &value, 16);
  } else if (registerNameLower == "xmm8") {
    memcpy(&context.Xmm8, &value, 16);
  } else if (registerNameLower == "xmm9") {
    memcpy(&context.Xmm9, &value, 16);
  } else if (registerNameLower == "xmm10") {
    memcpy(&context.Xmm10, &value, 16);
  } else if (registerNameLower == "xmm11") {
    memcpy(&context.Xmm11, &value, 16);
  } else if (registerNameLower == "xmm12") {
    memcpy(&context.Xmm12, &value, 16);
  } else if (registerNameLower == "xmm13") {
    memcpy(&context.Xmm13, &value, 16);
  } else if (registerNameLower == "xmm14") {
    memcpy(&context.Xmm14, &value, 16);
  } else if (registerNameLower == "xmm15") {
    memcpy(&context.Xmm15, &value, 16);
  } else {
    __debugbreak();
  }
  SetThreadContext(g_thread.native_handle(), &context);
}


int main() {
  RunArgs args;
  args.allocateMemory = allocate_memory;
  args.deallocateMemory = deallocate_memory;
  args.expression = "#include <string>\n;printf(((std::string*) &s)->c_str());while(1){}";
  args.file_path = "C:\\Users\\idowe\\source\\repos\\Project1\\Debug\\Project1.dll";
  auto module = LoadLibraryA(args.file_path.c_str());
  void *endlessThreadProc = GetProcAddress(module, "endlessThread");
  g_thread = std::thread((void (*)(void *, void *))endlessThreadProc,
                         (void *)OpenEventA, (void *)SetEvent);
  Sleep(100);
  SuspendThread(g_thread.native_handle());
  args.getThreadId = getThreadId;
  args.readMemory = ReadMemory;
  args.writeMemory = WriteMemory;
  args.resumeThread = resumeThread;
  args.suspendThread = suspedThread;
  args.start_address = module;
  args.stream = std::make_shared<Stream2>();
  args.getRegisterValue = getRegisterValue;
  args.setRegisterValue = setRegisterValue;
  args.runThreadPlan = runThreadPlan;
  run(args);
  return 0;
}