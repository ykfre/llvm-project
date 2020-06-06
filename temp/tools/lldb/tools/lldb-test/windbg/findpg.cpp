//
// This module implements exported debugger extension commands
//

// C/C++ standard headers
// Other external headers
// Windows headers
// Original headers
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// Other external headers
// Windows headers
#include "RunArgs.h"
#include <engextcpp.hpp>
#include <tchar.h>
#include <vector>
#include <map>
#include <mutex>
#include <vector>

#define THROW_IF(condition, message)                                           \
  if ((condition)) {                                                           \
    THROW(std::string(#message));                                              \
  }

#define THROW_IF_FALSE(condition, message)                                     \
  if (!(condition)) {                                                          \
    THROW(std::string(#message));                                              \
  }

#define THROW(message)                                                         \
  std::string throw_message{"message: "};                                      \
  throw_message += std::string(message);                                       \
  throw_message += ", file: ";                                                 \
  throw_message += std::string(__FILE__);                                      \
  throw_message += ", line: ";                                                 \
  throw_message += std::to_string(__LINE__);                                   \
  throw std::runtime_error(throw_message)



extern bool run(RunArgs runArgs);
std::string read_file(const std::string &file_path) {
  std::ifstream file(file_path, std::ifstream::binary);
  THROW_IF_FALSE("file", "failed to open " + file_path)
  // get length of file:
  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  std::vector<char> result(length);
  file.read(result.data(), result.size());
  auto stringResult = std::string(result.begin(), result.end());
  stringResult.push_back('\0');
  return stringResult;
}

class Event {
public:
  Event() { m_handle = CreateEvent(nullptr, false, false, nullptr); }
  ~Event() {
    if (m_handle) {
      CloseHandle(m_handle);
    }
  }

  void notify() { SetEvent(m_handle); }

  void wait() { WaitForSingleObject(m_handle, INFINITE); }

private:
  HANDLE m_handle;
};


class DebugEvents : public IDebugEventCallbacks {
public:
  STDMETHODIMP
  QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID *Interface) {
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugEventCallbacks))) {
      *Interface = (IDebugEventCallbacks *)this;
      AddRef();
      return S_OK;
    } else {
      return E_NOINTERFACE;
    }
  }

  STDMETHODIMP_(ULONG)
  AddRef() {
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
  }

  STDMETHODIMP_(ULONG)
  Release() {
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
  }

  // IDebugEventCallbacks.

  // The engine calls GetInterestMask once when
  // the event callbacks are set for a client.
  STDMETHODIMP
  GetInterestMask(_Out_ PULONG Mask) {
    *Mask = DEBUG_EVENT_BREAKPOINT;
    return 0;
  };

  // A breakpoint event is generated when
  // a breakpoint exception is received and
  // it can be mapped to an existing breakpoint.
  // The callback method is given a reference
  // to the breakpoint and should release it when
  // it is done with it.
  STDMETHODIMP Breakpoint(_In_ PDEBUG_BREAKPOINT Bp);


  // Exceptions include breaks which cannot
  // be mapped to an existing breakpoint
  // instance.
  STDMETHODIMP Exception(THIS_ _In_ PEXCEPTION_RECORD64 Exception,
                         _In_ ULONG FirstChance) {
    return 0;
  };

  // Any of these values can be zero if they
  // cannot be provided by the engine.
  // Currently the kernel does not return thread
  // or process change events.
  STDMETHODIMP CreateThread(THIS_ _In_ ULONG64 Handle, _In_ ULONG64 DataOffset,
                            _In_ ULONG64 StartOffset) {
    return 0;
  };
  ;
  STDMETHODIMP ExitThread(THIS_ _In_ ULONG ExitCode) { return 0; };
  ;

  // Any of these values can be zero if they
  // cannot be provided by the engine.
  STDMETHODIMP CreateProcessA(THIS_ _In_ ULONG64 ImageFileHandle,
                              _In_ ULONG64 Handle, _In_ ULONG64 BaseOffset,
                              _In_ ULONG ModuleSize, _In_opt_ PCSTR ModuleName,
                              _In_opt_ PCSTR ImageName, _In_ ULONG CheckSum,
                              _In_ ULONG TimeDateStamp,
                              _In_ ULONG64 InitialThreadHandle,
                              _In_ ULONG64 ThreadDataOffset,
                              _In_ ULONG64 StartOffset) {
    return 0;
  }
  _Analysis_noreturn_ STDMETHOD(ExitProcess)(THIS_ _In_ ULONG ExitCode) {
    return 0;
  };
  ;

  // Any of these values may be zero.
  STDMETHODIMP LoadModule(THIS_ _In_ ULONG64 ImageFileHandle,
                          _In_ ULONG64 BaseOffset, _In_ ULONG ModuleSize,
                          _In_opt_ PCSTR ModuleName, _In_opt_ PCSTR ImageName,
                          _In_ ULONG CheckSum, _In_ ULONG TimeDateStamp) {
    return 0;
  };
  ;
  STDMETHODIMP UnloadModule(THIS_ _In_opt_ PCSTR ImageBaseName,
                            _In_ ULONG64 BaseOffset) {
    return 0;
  };
  ;

  STDMETHODIMP SystemError(THIS_ _In_ ULONG Error, _In_ ULONG Level) {
    return 0;
  };
  ;

  // Session status is synchronous like the other
  // wait callbacks but it is called as the state
  // of the session is changing rather than at
  // specific events so its return value does not
  // influence waiting.  Implementations should just
  // return DEBUG_STATUS_NO_CHANGE.
  // Also, because some of the status
  // notifications are very early or very
  // late in the session lifetime there may not be
  // current processes or threads when the notification
  // is generated.
  STDMETHODIMP SessionStatus(THIS_ _In_ ULONG Status) { return 0; };
  ;

  // The following callbacks are informational
  // callbacks notifying the provider about
  // changes in debug state.  The return value
  // of these callbacks is ignored.  Implementations
  // can not call back into the engine.

  // Debuggee state, such as registers or data spaces,
  // has changed.
  STDMETHODIMP ChangeDebuggeeState(THIS_ _In_ ULONG Flags,
                                   _In_ ULONG64 Argument) {
    return 0;
  };
  ;
  // Engine state has changed.
  STDMETHODIMP ChangeEngineState(THIS_ _In_ ULONG Flags,
                                 _In_ ULONG64 Argument) {
    return 0;
  };
  ;
  // Symbol state has changed.
  STDMETHODIMP ChangeSymbolState(THIS_ _In_ ULONG Flags,
                                 _In_ ULONG64 Argument) {
    return 0;
  };
  ;
};

class EXT_CLASS : public ExtExtension {
public:
  Event m_t;

  void Uninitialize() override {
    m_Client->SetOutputCallbacks(m_oldOutputCallbacks);
    TerminateThread(m_executeExthread.native_handle(), 0);
  }
  EXT_COMMAND_METHOD(execute);

  bool m_initialized = false;
  PDEBUG_OUTPUT_CALLBACKS m_oldOutputCallbacks = nullptr;
  HRESULT Initialize();
  void execute(const std::string &filePath,
               const std::string &command_file_path);
  void *allocateMemory(size_t size);
  void deallocateMemory(void *addr);

  void execute2(const std::string &filePath,
                const std::string &command_file_path);
  void addBp(void *addr) {
    PDEBUG_BREAKPOINT bp = nullptr;
    m_Control->AddBreakpoint(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &bp);
    bp->AddFlags(DEBUG_BREAKPOINT_ENABLED);
    bp->SetOffset(size_t(addr));
    m_bpAndCounters[(size_t)addr] += 1;
  }
  int getThreadId();

  int64_t getModule(size_t ip) {
    ULONG moudlesNUm = 0;
    ULONG unloadedModuleNum = 0;
    m_Symbols3->GetNumberModules(&moudlesNUm, &unloadedModuleNum);
    for (int i = 0; i < moudlesNUm + unloadedModuleNum; i++) {
      ULONG64 moduleStart = 0;
      m_Symbols->GetModuleByIndex(i, &moduleStart);
      if (i < moudlesNUm + unloadedModuleNum - 1) {
        ULONG64 nextModule = 0;
        m_Symbols->GetModuleByIndex(i + 1, &nextModule);
        if (ip < nextModule && moduleStart <= ip) {
          return (int64_t)moduleStart;
        }
      }
    }
    ULONG64 moduleStart = 0;
    auto result = m_Symbols->GetModuleByIndex(
        moudlesNUm + unloadedModuleNum - 1, &moduleStart);
    THROW_IF_FALSE(SUCCEEDED(result), "Failed to get module");
    return (int64_t)moduleStart;
  }

  size_t readMemory(void *addr, void *buf, size_t size) {
    ULONG bytesRead = 0;
    if (!SUCCEEDED(m_Data->ReadVirtual((size_t)addr, buf, size, &bytesRead))) {
      return 0;
    }
    return bytesRead;
  }

  size_t writeMemory(void *addr, const void *buf, size_t size) {
    ULONG bytesRead = 0;
    if (!SUCCEEDED(m_Data->WriteVirtual((size_t)addr, (void *)buf, size,
                                        &bytesRead))) {
      return 0;
    }
    return bytesRead;
  }

  std::string getCorrectRegisterName(const std::string &registerName) {
    std::string registerNameLower = registerName;

    std::transform(registerNameLower.begin(), registerNameLower.end(),
                   registerNameLower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (registerNameLower == "eflags") {
      registerNameLower = "efl";
    }
    return registerNameLower;
  }

  std::bitset<128> getRegisterValue(const std::string &registerName) {
    ULONG index;

    std::string registerNameLower = getCorrectRegisterName(registerName);
    DEBUG_VALUE value;
    HRESULT result =
        m_Registers->GetIndexByName(registerNameLower.c_str(), &index);

    if (!SUCCEEDED(result)) {
      result =
          m_Registers2->GetPseudoIndexByName(registerNameLower.c_str(), &index);
      THROW_IF_FALSE(SUCCEEDED(result),
                     "failed get index value " + registerNameLower);

      m_Registers2->GetPseudoValues(DEBUG_REGSRC_FRAME, 1, nullptr, index,
                                    &value);
    } else {
      result = m_Registers2->GetValue(index, &value);
    }

    THROW_IF_FALSE(SUCCEEDED(result), "failed get index value " + registerName);
    std::bitset<128> Returnvalue;
    int registerSize = 8;
    if (registerNameLower.find("xmm") != -1) {
      registerSize = 16;
    }
    memcpy(&Returnvalue, value.RawBytes, registerSize);

    return Returnvalue;
  }

  void setRegisterValue(const std::string &registerName,
                        std::bitset<128> value) {

    DEBUG_VALUE debugValue;
    std::string registerNameLower = getCorrectRegisterName(registerName);
    int registerSize = 8;
    int type = 4;
    if (registerNameLower.find("xmm") != -1) {
      registerSize = 16;
      type = 0xb;
    }

    memcpy(debugValue.RawBytes, &value, registerSize);
    ULONG index = 0;
    HRESULT result =
        m_Registers2->GetIndexByName(registerNameLower.c_str(), &index);
    if (!SUCCEEDED(result)) {
      result =
          m_Registers2->GetPseudoIndexByName(registerNameLower.c_str(), &index);
      THROW_IF_FALSE(SUCCEEDED(result),
                     "failed set index name " + registerNameLower);
      m_Registers2->SetPseudoValues(DEBUG_REGSRC_FRAME, 1, nullptr, index,
                                    &debugValue);
    } else {
      debugValue.Type = type;
      result = m_Registers2->SetValue(index, &debugValue);
    }
    THROW_IF_FALSE(SUCCEEDED(result),
                   "failed set index name " + registerNameLower);
  }

  void resumeThread() {
    std::stringstream command;
    auto result = m_Control->Execute(DEBUG_OUTPUT_NORMAL, "g", 0);

    THROW_IF_FALSE(SUCCEEDED(result), "resuming failed");
  }

  void suspendThread() {}

  lldb::ExpressionResults
  RunThreadPlan(lldb_private::ExecutionContext &exe_ctx, int tid) {
    registerEvent(tid, std::make_shared<Event>());
    resumeThread();
    waitFor(tid);

    return lldb::ExpressionResults::eExpressionCompleted;
  }

  void registerEvent(int tid, std::shared_ptr<Event> event) {
    m_tidToMutexes[tid].push_back(event);
  }

  void waitFor(int tid) {
    m_tidToMutexes[tid][m_tidToMutexes.size() - 1]->wait();
  }

  void notify() {
    int tid = getThreadId();
    if (m_tidToMutexes.find(tid) == m_tidToMutexes.end()) {
      return;
    }
    auto mutexes = m_tidToMutexes[tid];
    if (mutexes.empty()) {
      return;
    }

    auto currentMutex = mutexes.at(mutexes.size() - 1);
    mutexes.pop_back();
    currentMutex->notify();
  }

  void executeInternal(const std::string &filePath,
                       const std::string &commandFilePath);

  std::map<int, std::vector<std::shared_ptr<Event>>> m_tidToMutexes;

  std::map<size_t, int> m_bpAndCounters;
  DebugEvents m_debugEvents;
  std::thread m_executeExthread;
  std::vector<std::pair<std::string, std::string>> m_argsQueue;
};

class StdioOutputCallbacks : public IDebugOutputCallbacks {
public:
  void setExtension(Extension *extension) { m_extension = extension; }
  STDMETHODIMP
  QueryInterface(THIS_ _In_ REFIID InterfaceId, _Out_ PVOID *Interface) {
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks))) {
      *Interface = (IDebugOutputCallbacks *)this;
      AddRef();
      return S_OK;
    } else {
      return E_NOINTERFACE;
    }
  }

  STDMETHODIMP_(ULONG)
  AddRef(THIS) {
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
  }

  STDMETHODIMP_(ULONG)
  Release(THIS) {
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
  }

  STDMETHODIMP
  Output(THIS_ _In_ ULONG Mask, _In_ PCSTR Text) {
    UNREFERENCED_PARAMETER(Mask);
    m_text += std::string(Text);
    if (nullptr != m_extension->m_oldOutputCallbacks) {
      m_extension->m_oldOutputCallbacks->Output(Mask, Text);
    }
    return S_OK;
  }

  void clear() { m_text.clear(); }
  std::string m_text;

private:
  Extension *m_extension = nullptr;
};

StdioOutputCallbacks g_OutputCb;

void *EXT_CLASS::allocateMemory(size_t size) {
  std::stringstream command;
  g_OutputCb.clear();
  command << ".dvalloc " << std::hex << size;
  Out(command.str().c_str());
  if (!SUCCEEDED(m_Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                                    command.str().c_str(), 0))) {
    return 0;
  }
  int index = g_OutputCb.m_text.rfind(" ");
  std::string text = g_OutputCb.m_text.substr(index + 1);
  text.erase(std::remove(text.begin(), text.end(), '`'), text.end());
  text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
  std::stringstream ss;
  ss << "0x" << text;
  size_t address;
  ss >> std::hex >> address;
  return (void *)address;
}

void EXT_CLASS::deallocateMemory(void *address) {
  std::stringstream command;
  command << ".dvfree 0x" << std::hex << (size_t)address  << " 0";
  Out(command.str().c_str());
  auto result =
      m_Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, command.str().c_str(), 0);
  THROW_IF_FALSE(SUCCEEDED(result), "deallocate failed");
}

int EXT_CLASS::getThreadId() { return getRegisterValue("$tid").to_ulong(); }


class Stream3 : public lldb_private::Stream {
public:
  Stream3(Extension &extension) : m_extension(extension){};

  void Flush() override {}

  size_t WriteImpl(const void *src, size_t src_len) override {
    for (int i = 0; i < src_len; i++) {
      std::string s;
      s.push_back(((char *)src)[i]);
      m_extension.Out(s.c_str());
    }
    return src_len;
  }

private:
  Extension &m_extension;
};

////////////////////////////////////////////////////////////////////////////////
//
// prototypes
//

////////////////////////////////////////////////////////////////////////////////
//
// variables
//

// EXT_DECLARE_GLOBALS must be used to instantiate
// the framework's assumed globals.
EXT_DECLARE_GLOBALS();


inline void EXT_CLASS::executeInternal(const std::string &filePath,
                                       const std::string &commandFilePath) {
  RunArgs args;
  args.allocateMemory = [&](size_t size) { return allocateMemory(size); };
  args.deallocateMemory = [&](void *address) { deallocateMemory(address); };

  args.expression = read_file(commandFilePath);
  args.file_path = filePath;
  args.getThreadId = [&] { return getThreadId(); };
  args.readMemory = [&](void *addr, void *buf, size_t size) {
    return readMemory(addr, buf, size);
  };
  args.writeMemory = [&](void *addr, const void *buf, size_t size) {
    return writeMemory(addr, buf, size);
  };
  args.resumeThread = [&] { resumeThread(); };
  args.suspendThread = [&] { suspendThread(); };
  args.start_address = (void *)g_ExtInstance.getModule(
      g_ExtInstance.getRegisterValue("rip").to_ullong());
  args.stream = std::make_shared<Stream3>(g_ExtInstance);
  args.getRegisterValue = [&](const std::string &regName) {
    return getRegisterValue(regName);
  };
  args.frameIndex = getRegisterValue("$frame").to_ulong();
  auto tid = getThreadId();
  args.runThreadPlan = [&](lldb_private::ExecutionContext &exe_ctx) {
    return RunThreadPlan(exe_ctx, tid);
  };
  args.bpAddress = true;
  args.addBreakpoint = [&](void *addr) { addBp(addr); };
  args.setRegisterValue = [&](const std::string &registerName,
                              std::bitset<128> value) {
    return setRegisterValue(registerName, value);
  };
  run(args);
}

HRESULT DebugEvents::Breakpoint(_In_ PDEBUG_BREAKPOINT Bp) {
  {
    ULONG breakType;
    ULONG temp;
    Bp->GetType(&breakType, &temp);
    if (breakType == DEBUG_BREAKPOINT_CODE) {
      ULONG64 bpOffset;
      Bp->GetOffset(&bpOffset);
      if (g_ExtInstance.m_bpAndCounters.find(bpOffset) !=
          g_ExtInstance.m_bpAndCounters.end()) {
        if (g_ExtInstance.m_bpAndCounters[bpOffset] > 0) {
          g_ExtInstance.notify();
        }
      }
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// implementations
//


void a() {
  while (1) {
  }
}

// debug event callback
IDebugClient5 *g_pDebugClient5;
IDebugControl *g_pDebugControl;


HRESULT EXT_CLASS::Initialize() {
  ::HRESULT hr = ::DebugCreate(__uuidof(::IDebugClient5),
                               reinterpret_cast<void **>(&g_pDebugClient5));
  if (FAILED(hr)) {
    return hr;
  }

  hr = g_pDebugClient5->SetEventCallbacks(&m_debugEvents);
  if (!SUCCEEDED(hr)) {
    return hr;
  }
  
  m_executeExthread = std::thread([this]() {
    while (true) {
      m_t.wait();
      Out("starting executing\n");
      auto args = m_argsQueue.at(m_argsQueue.size() - 1);
      m_argsQueue.pop_back();
      executeInternal(args.first, args.second);

      Out("ending executing\n");
    }
  });

  return 0;
}

// Does main stuff and throws an exception when
void EXT_CLASS::execute2(const std::string &filePath,
                         const std::string &command_file_path) {
  if (!m_initialized) {
    m_Client->GetOutputCallbacks(&m_oldOutputCallbacks);
    g_OutputCb.setExtension(&g_ExtInstance);
    m_Client->SetOutputCallbacks(&g_OutputCb);
    m_initialized = true;
  }
  m_argsQueue.push_back(
      std::make_pair(filePath, command_file_path));
   m_t.notify();
}

EXT_COMMAND(execute, "Displays base addresses of PatchGuard pages", "") {
  try {
    execute2("C:\\Users\\idowe\\source\\repos\\Project1\\Debug\\Project1.exe",
             "C:\\Users\\idowe\\Desktop\\temp\\b.txt");
  } catch (std::exception &e) {
    // As an exception string does not appear on Windbg,
    // we need to handle it manually.
    Err("%s\n", e.what());
  }
  
}
