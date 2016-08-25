////////////////////////////////////////////////////////////////////////////////
//
// The University of Illinois/NCSA
// Open Source License (NCSA)
//
// Copyright (c) 2014-2015, Advanced Micro Devices, Inc. All rights reserved.
//
// Developed by:
//
//                 AMD Research and AMD HSA Software Development
//
//                 Advanced Micro Devices, Inc.
//
//                 www.amd.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
//  - Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimers.
//  - Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimers in
//    the documentation and/or other materials provided with the distribution.
//  - Neither the names of Advanced Micro Devices, Inc,
//    nor the names of its contributors may be used to endorse or promote
//    products derived from this Software without specific prior written
//    permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS WITH THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#include "hsail_finalize.h"
#include "hsail_helper.h"
#include "tools.h"
#include "hsa/hsa.h"
#include "hsa/hsa_ext_finalize.h"
#include <stdio.h>
#include <string.h>

int compile_code_object(hsail_module_t* list,
  hsail_runtime_t* run,
  hsail_finalize_t* fin) {

    hsa_status_t err;
    hsa_ext_program_t program;
    // Create program
    memset(&program, 0, sizeof(hsa_ext_program_t));
    err = run->table_1_00.hsa_ext_program_create(run->machine_model, run->profile,
        HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, &program);
    check(Create the program, err);

    // Add the BRIG module to hsa program.
    hsail_module_t* tmp = list;
    while (tmp != NULL && err == HSA_STATUS_SUCCESS) {
       err = run->table_1_00.hsa_ext_program_add_module(program, tmp->module);
       tmp = tmp->next;
    }
    check(Adding the brig module to the program, err);

    // Determine the agents ISA.
    hsa_isa_t isa;
    err = hsa_agent_get_info(run->agent, HSA_AGENT_INFO_ISA, &isa);
    check(Query the agents isa, err);

    // Finalize the program and extract the code object.
    hsa_ext_control_directives_t control_directives;
    memset(&control_directives, 0, sizeof(hsa_ext_control_directives_t));
    err = run->table_1_00.hsa_ext_program_finalize(program, isa, 0, control_directives,
        "", HSA_CODE_OBJECT_TYPE_PROGRAM, &(fin->code_object));
    check(Finalizing the program, err);

    // Destroy the program, it is no longer needed.
    err = run->table_1_00.hsa_ext_program_destroy(program);
    check(Destroying the program, err);
    return 0;
}

int generate_executable(hsail_runtime_t* run, hsail_finalize_t* fin) {
   // Create the empty executable.
  hsa_status_t err;
  err = hsa_executable_create(run->profile, HSA_EXECUTABLE_STATE_UNFROZEN,
    "", &(fin->executable));
  check(Create the executable, err);

  // Load the code object.
  err = hsa_executable_load_code_object(fin->executable, run->agent, fin->code_object, "");
  check(Loading the code object, err);

  // Freeze the executable; it can now be queried for symbols.
  err = hsa_executable_freeze(fin->executable, "");
  check(Freeze the executable, err);
  return 0;
}

// Assuming list.name fits in 512 easely
int extract_dispatch_info(hsail_module_t* list,
  hsail_runtime_t* run,
  hsail_finalize_t* fin) {

    /*
    * Extract the symbol from the executable
    * & extract dispatch information from the symbol.
    */
    char buff[512];
    hsa_status_t err = HSA_STATUS_SUCCESS;
    hsail_module_t* tmp = list;
    while (tmp != NULL && err == HSA_STATUS_SUCCESS) {
      sprintf(buff, "&__%s_kernel", tmp->name);
      // deprecated but get_symbol_by_name doesn't exists
      hsa_executable_symbol_t symbol;
      err = hsa_executable_get_symbol(fin->executable, NULL, buff,
        run->agent, 0, &symbol);
      if (err != HSA_STATUS_SUCCESS) break;
      err = get_symbol_info(symbol, &(tmp->pkt_info));
      tmp = tmp->next;
    }
    check(Extract the symbol information from the executable, err);
    return 0;
}

int finalize_modules(hsail_module_t* list,
  hsail_runtime_t* run,
  hsail_finalize_t* fin) {

    if (compile_code_object(list, run, fin) != 0
        || generate_executable(run, fin) != 0
        || extract_dispatch_info(list, run, fin) != 0) {
          return 1;
      }
    return 0;
}
