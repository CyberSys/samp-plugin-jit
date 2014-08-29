// Copyright (c) 2012-2014 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef AMXJIT_COMPILER_ASMJIT_H
#define AMXJIT_COMPILER_ASMJIT_H

#include <cstddef>
#include <map>
#include <asmjit/core.h>
#include <asmjit/x86.h>
#include "amxptr.h"
#include "compiler.h"
#include "macros.h"

namespace amxjit {

class CompilerAsmjit: public Compiler {
 public:
  typedef void (CompilerAsmjit::*EmitIntrinsicMethod)();

  CompilerAsmjit();
  virtual ~CompilerAsmjit();

 protected:
  virtual bool Prepare(AMXPtr amx);
  virtual bool Process(const Instruction &instr);
  virtual CompileOutput *Finish(bool error);

 protected:
  virtual void load_pri(cell address);
  virtual void load_alt(cell address);
  virtual void load_s_pri(cell offset);
  virtual void load_s_alt(cell offset);
  virtual void lref_pri(cell address);
  virtual void lref_alt(cell address);
  virtual void lref_s_pri(cell offset);
  virtual void lref_s_alt(cell offset);
  virtual void load_i();
  virtual void lodb_i(cell number);
  virtual void const_pri(cell value);
  virtual void const_alt(cell value);
  virtual void addr_pri(cell offset);
  virtual void addr_alt(cell offset);
  virtual void stor_pri(cell address);
  virtual void stor_alt(cell address);
  virtual void stor_s_pri(cell offset);
  virtual void stor_s_alt(cell offset);
  virtual void sref_pri(cell address);
  virtual void sref_alt(cell address);
  virtual void sref_s_pri(cell offset);
  virtual void sref_s_alt(cell offset);
  virtual void stor_i();
  virtual void strb_i(cell number);
  virtual void lidx();
  virtual void lidx_b(cell shift);
  virtual void idxaddr();
  virtual void idxaddr_b(cell shift);
  virtual void align_pri(cell number);
  virtual void align_alt(cell number);
  virtual void lctrl(cell index, cell cip);
  virtual void sctrl(cell index);
  virtual void move_pri();
  virtual void move_alt();
  virtual void xchg();
  virtual void push_pri();
  virtual void push_alt();
  virtual void push_c(cell value);
  virtual void push(cell address) ;
  virtual void push_s(cell offset);
  virtual void pop_pri();
  virtual void pop_alt();
  virtual void stack(cell value);
  virtual void heap(cell value);
  virtual void proc();
  virtual void ret();
  virtual void retn();
  virtual void call(cell address);
  virtual void jump_pri();
  virtual void jump(cell address);
  virtual void jzer(cell address);
  virtual void jnz(cell address);
  virtual void jeq(cell address);
  virtual void jneq(cell address);
  virtual void jless(cell address);
  virtual void jleq(cell address);
  virtual void jgrtr(cell address);
  virtual void jgeq(cell address);
  virtual void jsless(cell address);
  virtual void jsleq(cell address);
  virtual void jsgrtr(cell address);
  virtual void jsgeq(cell address);
  virtual void shl();
  virtual void shr();
  virtual void sshr();
  virtual void shl_c_pri(cell value);
  virtual void shl_c_alt(cell value);
  virtual void shr_c_pri(cell value);
  virtual void shr_c_alt(cell value);
  virtual void smul();
  virtual void sdiv();
  virtual void sdiv_alt();
  virtual void umul();
  virtual void udiv();
  virtual void udiv_alt();
  virtual void add();
  virtual void sub();
  virtual void sub_alt();
  virtual void and_();
  virtual void or_();
  virtual void xor_();
  virtual void not_();
  virtual void neg();
  virtual void invert();
  virtual void add_c(cell value);
  virtual void smul_c(cell value);
  virtual void zero_pri();
  virtual void zero_alt();
  virtual void zero(cell address);
  virtual void zero_s(cell offset);
  virtual void sign_pri();
  virtual void sign_alt();
  virtual void eq();
  virtual void neq();
  virtual void less();
  virtual void leq();
  virtual void grtr();
  virtual void geq();
  virtual void sless();
  virtual void sleq();
  virtual void sgrtr();
  virtual void sgeq();
  virtual void eq_c_pri(cell value);
  virtual void eq_c_alt(cell value);
  virtual void inc_pri();
  virtual void inc_alt();
  virtual void inc(cell address);
  virtual void inc_s(cell offset);
  virtual void inc_i();
  virtual void dec_pri();
  virtual void dec_alt();
  virtual void dec(cell address);
  virtual void dec_s(cell offset);
  virtual void dec_i();
  virtual void movs(cell num_bytes);
  virtual void cmps(cell num_bytes);
  virtual void fill(cell num_bytes);
  virtual void halt(cell error_code);
  virtual void bounds(cell value);
  virtual void sysreq_pri();
  virtual void sysreq_c(cell index, const char *name);
  virtual void sysreq_d(cell address, const char *name);
  virtual void switch_(const CaseTable &case_table);
  virtual void casetbl();
  virtual void swap_pri();
  virtual void swap_alt();
  virtual void push_adr(cell offset);
  virtual void nop();
  virtual void break_();

 private:
  bool EmitIntrinsic(const char *name);
  void float_();
  void floatabs();
  void floatadd();
  void floatsub();
  void floatmul();
  void floatdiv();
  void floatsqroot();
  void floatlog();

 private:
  void EmitRuntimeInfo();
  void EmitInstrTable();
  void EmitExec();
  void EmitExecHelper();
  void EmitHaltHelper();
  void EmitJumpHelper();
  void EmitSysreqCHelper();
  void EmitSysreqDHelper();

 private:
  const AsmJit::Label &GetLabel(cell address);

 private:
  AMXPtr current_amx_;

  AsmJit::X86Assembler asm_;
  AsmJit::Label rib_start_label_;
  AsmJit::Label exec_ptr_label_;
  AsmJit::Label ebp_label_;
  AsmJit::Label esp_label_;
  AsmJit::Label reset_ebp_label_;
  AsmJit::Label reset_esp_label_;
  AsmJit::Label exec_label_;
  AsmJit::Label exec_helper_label_;
  AsmJit::Label halt_helper_label_;
  AsmJit::Label jump_helper_label_;
  AsmJit::Label sysreq_c_helper_label_;
  AsmJit::Label sysreq_d_helper_label_;

  typedef std::map<cell, AsmJit::Label> LabelMap;
  LabelMap label_map_;

  typedef std::map<cell, std::ptrdiff_t> InstrMap;
  InstrMap instr_map_;

  AsmJit::Logger *logger_;

 private:
  AMXJIT_DISALLOW_COPY_AND_ASSIGN(CompilerAsmjit);
};

class CompileOutputAsmjit: public CompileOutput {
 public:
  CompileOutputAsmjit(void *code);
  virtual ~CompileOutputAsmjit();

  virtual void *GetCode() const;
  virtual EntryPoint GetEntryPoint() const;

  virtual void Delete();

 private:
  void *code_;

 private:
  AMXJIT_DISALLOW_COPY_AND_ASSIGN(CompileOutputAsmjit);
};

} // namespace amxjit

#endif // !AMXJIT_COMPILER_ASMJIT_H
