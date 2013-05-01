// Copyright (c) 2012-2013 Zeex
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

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#ifdef _WIN32
  #include <windows.h>
#else
  #ifndef _GNU_SOURCE
    #define _GNU_SOURCE 1 // for dl_addr()
  #endif
  #include <dlfcn.h>
#endif
#include <subhook.h>
#include "plugin.h"
#include "version.h"
#include "amxjit/compiler.h"
#if JIT_ASMJIT
  #include "amxjit/compiler-asmjit.h"
#endif
#if JIT_LLVM
  #include "amxjit/compiler-llvm.h"
#endif
#include "amxjit/disasm.h"
#include "amxjit/jit.h"

#if defined __GNUC__ && !defined __MINGW32__
  #define USE_OPCODE_MAP 1
#endif

extern void *pAMXFunctions;

typedef void (*LogprintfType)(const char *format, ...);
static LogprintfType logprintf;

typedef std::map<AMX*, amxjit::JIT*> AmxToJitMap;
static AmxToJitMap amxToJit;

static SubHook ExecHook;
#if USE_OPCODE_MAP
  static cell *opcodeMap = 0;
#endif

static std::string GetModulePath(void *address,
                                 std::size_t maxLength = FILENAME_MAX)
{
  #ifdef _WIN32
    std::vector<char> name(maxLength + 1);
    if (address != 0) {
      MEMORY_BASIC_INFORMATION mbi;
      VirtualQuery(address, &mbi, sizeof(mbi));
      GetModuleFileName((HMODULE)mbi.AllocationBase, &name[0], maxLength);
    }
    return std::string(&name[0]);
  #else
    std::vector<char> name(maxLength + 1);
    if (address != 0) {
      Dl_info info;
      dladdr(address, &info);
      strncpy(&name[0], info.dli_fname, maxLength);
    }  
    return std::string(&name[0]);
  #endif
}

static std::string GetFileName(const std::string &path) {
  std::string::size_type pos = path.find_last_of("/\\");
  if (pos != std::string::npos) {
    return path.substr(pos + 1);
  }
  return path;
}

static int AMXAPI amx_Exec_JIT(AMX *amx, cell *retval, int index) {
  #if USE_OPCODE_MAP
    if ((amx->flags & AMX_FLAG_BROWSE) == AMX_FLAG_BROWSE) {
      assert(::opcodeMap != 0);
      *retval = reinterpret_cast<cell>(::opcodeMap);
      return AMX_ERR_NONE;
    }
  #endif
  AmxToJitMap::iterator iterator = ::amxToJit.find(amx);
  if (iterator == ::amxToJit.end()) {
    SubHook::ScopedRemove r(&ExecHook);
    return amx_Exec(amx, retval, index);
  } else {
    amxjit::JIT *jit = iterator->second;
    return jit->Exec(index, retval);
  }
}

static int AMXAPI amx_Debug_JIT(AMX *amx) {
  return AMX_ERR_NONE;
}

class CompileErrorHandler : public amxjit::CompileErrorHandler {
 public:
  virtual void Execute(const amxjit::Instruction &instr) {
    logprintf("[jit] Invalid or unsupported instruction at address %p:",
              instr.GetAddress());
    logprintf("[jit]   => %s", instr.AsString().c_str());
  }
};

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
  return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) {
  logprintf = (LogprintfType)ppData[PLUGIN_DATA_LOGPRINTF];
  pAMXFunctions = reinterpret_cast<void*>(ppData[PLUGIN_DATA_AMX_EXPORTS]);

  void *ptr = SubHook::ReadDst(((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec]);
  if (ptr != 0) {
    std::string module = GetFileName(GetModulePath(ptr));
    if (!module.empty()) {
      logprintf("  JIT must be loaded before '%s'", module.c_str());
      return false;
    }
  }

  #if USE_OPCODE_MAP
    // Get opcode table before we hook amx_Exec().
    AMX amx = {0};
    amx.flags |= AMX_FLAG_BROWSE;
    amx_Exec(&amx, reinterpret_cast<cell*>(&::opcodeMap), 0);
    amx.flags &= ~AMX_FLAG_BROWSE;
  #endif

  ExecHook.Install(((void**)pAMXFunctions)[PLUGIN_AMX_EXPORT_Exec],
                        (void*)amx_Exec_JIT);

  logprintf("  JIT plugin v%s is OK.", PROJECT_VERSION_STRING);
  return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() {
  for (AmxToJitMap::iterator iterator = amxToJit.begin();
       iterator != amxToJit.end(); iterator++) {
    delete iterator->second;
  }
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) {
  amxjit::JIT *jit = new amxjit::JIT(amx);

  const char *backendString = getenv("JIT_BACKEND");
  if (backendString == 0) {
    backendString = "asmjit";
  }

  amxjit::Compiler *compiler = 0;
  #if JIT_ASMJIT
    if (std::strcmp(backendString, "asmjit") == 0) {
      compiler = new amxjit::CompilerAsmjit;
    }
  #endif
  #if JIT_LLVM
    if (std::strcmp(backendString, "llvm") == 0) {
      compiler = new amxjit::CompilerLLVM;
    }
  #endif

  if (compiler != 0) {
    CompileErrorHandler errorHandler;
    if (!jit->Compile(compiler, &errorHandler)) {
      delete jit;
    } else {
      #ifdef DEBUG
        amx_SetDebugHook(amx, amx_Debug_JIT);
      #endif
      ::amxToJit.insert(std::make_pair(amx, jit));
    }
  } else {
    logprintf("[jit] Unknown backend '%s'", backendString);
    delete jit;
  }

  delete compiler;
  return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) {
  AmxToJitMap::iterator iterator = amxToJit.find(amx);
  if (iterator != amxToJit.end()) {
    delete iterator->second;
    amxToJit.erase(iterator);
  }
  return AMX_ERR_NONE;
}
