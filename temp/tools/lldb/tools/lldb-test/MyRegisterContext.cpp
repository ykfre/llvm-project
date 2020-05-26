//===-- MyRegisterContext.cpp --------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#if defined(__x86_64__) || defined(_M_X64)

#include "lldb/Host/windows/HostThreadWindows.h"
#include "lldb/Host/windows/windows.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"
#include "lldb/lldb-private-types.h"
#include <iostream>
#include "MyRegisterContext.h"
#include "Plugins/Process/Utility/RegisterContext_x86.h"
#include "Plugins/Process/Utility/lldb-x86-register-enums.h"
#include "TargetThreadWindows.h"

#include "llvm/ADT/STLExtras.h"

using namespace lldb;
using namespace lldb_private;

#define DEFINE_GPR(reg, alt) #reg, alt, 8, 0, eEncodingUint, eFormatHexUppercase
#define DEFINE_GPR_BIN(reg, alt) #reg, alt, 8, 0, eEncodingUint, eFormatBinary
#define DEFINE_FPU_XMM(reg)                                                    \
#reg, NULL, 16, 0, eEncodingUint, eFormatVectorOfUInt64,                     \
      {dwarf_##reg##_x86_64, dwarf_##reg##_x86_64, LLDB_INVALID_REGNUM,        \
       LLDB_INVALID_REGNUM, lldb_##reg##_x86_64 },                             \
       nullptr, nullptr, nullptr, 0

namespace {

// This enum defines the layout of the global RegisterInfo array.  This is
// necessary because lldb register sets are defined in terms of indices into
// the register array. As such, the order of RegisterInfos defined in global
// registers array must match the order defined here. When defining the
// register set layouts, these values can appear in an arbitrary order, and
// that determines the order that register values are displayed in a dump.
enum RegisterIndex {
  eRegisterIndexRax,
  eRegisterIndexRbx,
  eRegisterIndexRcx,
  eRegisterIndexRdx,
  eRegisterIndexRdi,
  eRegisterIndexRsi,
  eRegisterIndexRbp,
  eRegisterIndexRsp,
  eRegisterIndexR8,
  eRegisterIndexR9,
  eRegisterIndexR10,
  eRegisterIndexR11,
  eRegisterIndexR12,
  eRegisterIndexR13,
  eRegisterIndexR14,
  eRegisterIndexR15,
  eRegisterIndexRip,
  eRegisterIndexRflags,

  eRegisterIndexXmm0,
  eRegisterIndexXmm1,
  eRegisterIndexXmm2,
  eRegisterIndexXmm3,
  eRegisterIndexXmm4,
  eRegisterIndexXmm5,
  eRegisterIndexXmm6,
  eRegisterIndexXmm7,
  eRegisterIndexXmm8,
  eRegisterIndexXmm9,
  eRegisterIndexXmm10,
  eRegisterIndexXmm11,
  eRegisterIndexXmm12,
  eRegisterIndexXmm13,
  eRegisterIndexXmm14,
  eRegisterIndexXmm15
};

// Array of all register information supported by Windows x86
RegisterInfo g_register_infos[] = {
    //  Macro auto defines most stuff     eh_frame                  DWARF
    //  GENERIC
    //  GDB                  LLDB                  VALUE REGS    INVALIDATE REGS
    //  ================================  =========================
    //  ======================  =========================
    //  ===================  =================     ==========    ===============
    {DEFINE_GPR(rax, nullptr),
     {dwarf_rax_x86_64, dwarf_rax_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_rax_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(rbx, nullptr),
     {dwarf_rbx_x86_64, dwarf_rbx_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_rbx_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(rcx, nullptr),
     {dwarf_rcx_x86_64, dwarf_rcx_x86_64, LLDB_REGNUM_GENERIC_ARG1,
      LLDB_INVALID_REGNUM, lldb_rcx_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(rdx, nullptr),
     {dwarf_rdx_x86_64, dwarf_rdx_x86_64, LLDB_REGNUM_GENERIC_ARG2,
      LLDB_INVALID_REGNUM, lldb_rdx_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(rdi, nullptr),
     {dwarf_rdi_x86_64, dwarf_rdi_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_rdi_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(rsi, nullptr),
     {dwarf_rsi_x86_64, dwarf_rsi_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_rsi_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(rbp, "fp"),
     {dwarf_rbp_x86_64, dwarf_rbp_x86_64, LLDB_REGNUM_GENERIC_FP,
      LLDB_INVALID_REGNUM, lldb_rbp_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(rsp, "sp"),
     {dwarf_rsp_x86_64, dwarf_rsp_x86_64, LLDB_REGNUM_GENERIC_SP,
      LLDB_INVALID_REGNUM, lldb_rsp_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(r8, nullptr),
     {dwarf_r8_x86_64, dwarf_r8_x86_64, LLDB_REGNUM_GENERIC_ARG3,
      LLDB_INVALID_REGNUM, lldb_r8_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(r9, nullptr),
     {dwarf_r9_x86_64, dwarf_r9_x86_64, LLDB_REGNUM_GENERIC_ARG4,
      LLDB_INVALID_REGNUM, lldb_r9_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(r10, nullptr),
     {dwarf_r10_x86_64, dwarf_r10_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_r10_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(r11, nullptr),
     {dwarf_r11_x86_64, dwarf_r11_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_r11_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(r12, nullptr),
     {dwarf_r12_x86_64, dwarf_r12_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_r12_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(r13, nullptr),
     {dwarf_r13_x86_64, dwarf_r13_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_r13_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(r14, nullptr),
     {dwarf_r14_x86_64, dwarf_r14_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_r14_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(r15, nullptr),
     {dwarf_r15_x86_64, dwarf_r15_x86_64, LLDB_INVALID_REGNUM,
      LLDB_INVALID_REGNUM, lldb_r15_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR(rip, "pc"),
     {dwarf_rip_x86_64, dwarf_rip_x86_64, LLDB_REGNUM_GENERIC_PC,
      LLDB_INVALID_REGNUM, lldb_rip_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_GPR_BIN(eflags, "flags"),
     {LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_REGNUM_GENERIC_FLAGS,
      LLDB_INVALID_REGNUM, lldb_rflags_x86_64},
     nullptr,
     nullptr,
     nullptr,
     0},
    {DEFINE_FPU_XMM(xmm0)},
    {DEFINE_FPU_XMM(xmm1)},
    {DEFINE_FPU_XMM(xmm2)},
    {DEFINE_FPU_XMM(xmm3)},
    {DEFINE_FPU_XMM(xmm4)},
    {DEFINE_FPU_XMM(xmm5)},
    {DEFINE_FPU_XMM(xmm6)},
    {DEFINE_FPU_XMM(xmm7)},
    {DEFINE_FPU_XMM(xmm8)},
    {DEFINE_FPU_XMM(xmm9)},
    {DEFINE_FPU_XMM(xmm10)},
    {DEFINE_FPU_XMM(xmm11)},
    {DEFINE_FPU_XMM(xmm12)},
    {DEFINE_FPU_XMM(xmm13)},
    {DEFINE_FPU_XMM(xmm14)},
    {DEFINE_FPU_XMM(xmm15)}};

static size_t k_num_register_infos = llvm::array_lengthof(g_register_infos);

// Array of lldb register numbers used to define the set of all General Purpose
// Registers
uint32_t g_gpr_reg_indices[] = {
    eRegisterIndexRax, eRegisterIndexRbx, eRegisterIndexRcx,
    eRegisterIndexRdx, eRegisterIndexRdi, eRegisterIndexRsi,
    eRegisterIndexRbp, eRegisterIndexRsp, eRegisterIndexR8,
    eRegisterIndexR9,  eRegisterIndexR10, eRegisterIndexR11,
    eRegisterIndexR12, eRegisterIndexR13, eRegisterIndexR14,
    eRegisterIndexR15, eRegisterIndexRip, eRegisterIndexRflags};

uint32_t g_fpu_reg_indices[] = {
    eRegisterIndexXmm0,  eRegisterIndexXmm1,  eRegisterIndexXmm2,
    eRegisterIndexXmm3,  eRegisterIndexXmm4,  eRegisterIndexXmm5,
    eRegisterIndexXmm6,  eRegisterIndexXmm7,  eRegisterIndexXmm8,
    eRegisterIndexXmm9,  eRegisterIndexXmm10, eRegisterIndexXmm11,
    eRegisterIndexXmm12, eRegisterIndexXmm13, eRegisterIndexXmm14,
    eRegisterIndexXmm15};

RegisterSet g_register_sets[] = {
    {"General Purpose Registers", "gpr",
     llvm::array_lengthof(g_gpr_reg_indices), g_gpr_reg_indices},
    {"Floating Point Registers", "fpu", llvm::array_lengthof(g_fpu_reg_indices),
     g_fpu_reg_indices}};
} // namespace

size_t MyRegisterContext::GetRegisterCount() {
  return llvm::array_lengthof(g_register_infos);
}

lldb_private::RegisterInfo *
MyRegisterContext::GetRegisterInfoAtIndex(size_t reg) {
  if (reg < k_num_register_infos)
    return &g_register_infos[reg];
  return NULL;
}

size_t MyRegisterContext::GetRegisterSetCount() {
  return llvm::array_lengthof(g_register_sets);
}

RegisterSet *MyRegisterContext::GetRegisterSet(size_t reg_set) {
  return &g_register_sets[reg_set];
}

bool MyRegisterContext::ReadRegister(const RegisterInfo *reg_info,
                                     RegisterValue &reg_value) {
  std::bitset<128> value;
  if (reg_info == nullptr)
    return false;

  const uint32_t reg = reg_info->kinds[eRegisterKindLLDB];

  switch (reg) {
  case lldb_rax_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("Rax").to_ullong());
    break;
  case lldb_rbx_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("rbx").to_ullong());
    break;
  case lldb_rcx_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("rcx").to_ullong());
    break;
  case lldb_rdx_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("rdx").to_ullong());
    break;
  case lldb_rdi_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("rdi").to_ullong());
    break;
  case lldb_rsi_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("rsi").to_ullong());
    break;
  case lldb_r8_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("r8").to_ullong());
    break;
  case lldb_r9_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("r9").to_ullong());
    break;
  case lldb_r10_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("r10").to_ullong());
    break;
  case lldb_r11_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("r11").to_ullong());
    break;
  case lldb_r12_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("r12").to_ullong());
    break;
  case lldb_r13_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("r13").to_ullong());
    break;
  case lldb_r14_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("r14").to_ullong());
    break;
  case lldb_r15_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("r15").to_ullong());
    break;
  case lldb_rbp_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("rbp").to_ullong());
    break;
  case lldb_rsp_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("rsp").to_ullong());
    break;
  case lldb_rip_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("rip").to_ullong());
    break;
  case lldb_rflags_x86_64:
    reg_value.SetUInt64(m_getRegisterValueFunc("EFLAGS").to_ullong());
    break;
  case lldb_xmm0_x86_64:
     value= m_getRegisterValueFunc("Xmm0");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm1_x86_64:
    value = m_getRegisterValueFunc("Xmm1");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm2_x86_64:
    value = m_getRegisterValueFunc("Xmm2");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm3_x86_64:
    value = m_getRegisterValueFunc("Xmm3");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm4_x86_64:
    value = m_getRegisterValueFunc("Xmm4");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm5_x86_64:
    value = m_getRegisterValueFunc("Xmm5");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm6_x86_64:
    value = m_getRegisterValueFunc("Xmm6");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm7_x86_64:
    value = m_getRegisterValueFunc("Xmm7");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm8_x86_64:
    value = m_getRegisterValueFunc("Xmm8");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm9_x86_64:
    value = m_getRegisterValueFunc("Xmm9");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm10_x86_64:
    value = m_getRegisterValueFunc("Xmm10");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm11_x86_64:
    value = m_getRegisterValueFunc("Xmm11");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm12_x86_64:
    value = m_getRegisterValueFunc("Xmm12");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm13_x86_64:
    value = m_getRegisterValueFunc("Xmm13");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm14_x86_64:
    value = m_getRegisterValueFunc("Xmm14");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  case lldb_xmm15_x86_64:
    value = m_getRegisterValueFunc("Xmm15");
    reg_value.SetBytes(&value, reg_info->byte_size, endian::InlHostByteOrder());
    break;
  }
  return true;
}

MyRegisterContext::MyRegisterContext(
    std::function<std::bitset<128>(const std::string &)> getRegisterValueFunc,
    std::function<void(const std::string &, std::bitset<128>)>
        setRegisterValueFunc,
    const ArchSpec &target_arch, Thread &thread, uint32_t concrete_frame_idx)
    : RegisterContext(thread, concrete_frame_idx) {
  m_getRegisterValueFunc = getRegisterValueFunc;
  m_setRegisterValueFunc = setRegisterValueFunc;
  m_arch = target_arch;
}

bool MyRegisterContext::WriteRegister(const RegisterInfo *reg_info,
                                      const RegisterValue &reg_value) {
  // Since we cannot only write a single register value to the inferior, we
  // need to make sure our cached copy of the register values are fresh.
  // Otherwise when writing EAX, for example, we may also overwrite some other
  // register with a stale value.
  std::bitset<128> value;

  switch (reg_info->kinds[eRegisterKindLLDB]) {
  case lldb_rax_x86_64:
    m_setRegisterValueFunc("Rax", reg_value.GetAsUInt64());
    break;
  case lldb_rbx_x86_64:
    m_setRegisterValueFunc("Rbx", reg_value.GetAsUInt64());
    break;
  case lldb_rcx_x86_64:
    m_setRegisterValueFunc("Rcx", reg_value.GetAsUInt64());
    break;
  case lldb_rdx_x86_64:
    m_setRegisterValueFunc("Rdx", reg_value.GetAsUInt64());
    break;
  case lldb_rdi_x86_64:
    m_setRegisterValueFunc("Rdi", reg_value.GetAsUInt64());

    break;
  case lldb_rsi_x86_64:
    m_setRegisterValueFunc("Rsi", reg_value.GetAsUInt64());

    break;
  case lldb_r8_x86_64:
    m_setRegisterValueFunc("R8", reg_value.GetAsUInt64());

    break;
  case lldb_r9_x86_64:
    m_setRegisterValueFunc("R9", reg_value.GetAsUInt64());

    break;
  case lldb_r10_x86_64:
    m_setRegisterValueFunc("R10", reg_value.GetAsUInt64());

    break;
  case lldb_r11_x86_64:
    m_setRegisterValueFunc("R11", reg_value.GetAsUInt64());

    break;
  case lldb_r12_x86_64:
    m_setRegisterValueFunc("R12", reg_value.GetAsUInt64());

    break;
  case lldb_r13_x86_64:
    m_setRegisterValueFunc("R13", reg_value.GetAsUInt64());

    break;
  case lldb_r14_x86_64:
    m_setRegisterValueFunc("R14", reg_value.GetAsUInt64());

    break;
  case lldb_r15_x86_64:
    m_setRegisterValueFunc("R15", reg_value.GetAsUInt64());

    break;
  case lldb_rbp_x86_64:
    m_setRegisterValueFunc("RBP", reg_value.GetAsUInt64());

    break;
  case lldb_rsp_x86_64:
    m_setRegisterValueFunc("Rsp", reg_value.GetAsUInt64());

    break;
  case lldb_rip_x86_64:
    m_setRegisterValueFunc("Rip", reg_value.GetAsUInt64());

    break;
  case lldb_rflags_x86_64:
    m_setRegisterValueFunc("EFlags", reg_value.GetAsUInt64());

    break;
  case lldb_xmm0_x86_64:
    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);

    break;
  case lldb_xmm1_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm2_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm3_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm4_x86_64:
    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm5_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm6_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm7_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm8_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm9_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm10_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm11_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm12_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm13_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm14_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  case lldb_xmm15_x86_64:

    memcpy(&value, reg_value.GetBytes(), 16);

    m_setRegisterValueFunc("Xmm0", value);
    break;
  }

  return true;
}

void MyRegisterContext::InvalidateAllRegisters() {}

bool MyRegisterContext::ReadAllRegisterValues(lldb::DataBufferSP &data_sp) {

  int index = 0;
  data_sp.reset(new DataBufferHeap(sizeof(CONTEXT), 0));
  for (int i = 0; i < GetRegisterCount(); i++) {
    auto regInfo = GetRegisterInfoAtIndex(i);
    RegisterValue regValue;
    ReadRegister(regInfo, regValue);
    memcpy(data_sp->GetBytes() + index, regValue.GetBytes(),
           regValue.GetByteSize());
    index += regValue.GetByteSize();
  }

  return true;
}

bool MyRegisterContext::WriteAllRegisterValues(
    const lldb::DataBufferSP &data_sp) {
  int index = 0;
  for (int i = 0; i < GetRegisterCount(); i++) {
    lldb_private::RegisterInfo *regInfo = GetRegisterInfoAtIndex(i);
    std::cout << regInfo->name;
    RegisterValue regValue(data_sp->GetBytes() + index, regInfo->byte_size,
                           eByteOrderLittle);
    WriteRegister(regInfo, regValue);
    index += regValue.GetByteSize();
  }
  return true;
}

uint32_t
MyRegisterContext::ConvertRegisterKindToRegisterNumber(lldb::RegisterKind kind,
                                                       uint32_t num) {
  const uint32_t num_regs = GetRegisterCount();

  assert(kind < kNumRegisterKinds);
  for (uint32_t reg_idx = 0; reg_idx < num_regs; ++reg_idx) {
    const RegisterInfo *reg_info = GetRegisterInfoAtIndex(reg_idx);

    if (reg_info->kinds[kind] == num)
      return reg_idx;
  }

  return LLDB_INVALID_REGNUM;
}

#endif // defined(__x86_64__) || defined(_M_X64)
