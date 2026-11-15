/**
 * @file include/retdec/capstone2llvmir/riscv/riscv_defs.h
 * @brief Additional (on top of Capstone) definitions for RISC-V translator.
 */

#ifndef RETDEC_CAPSTONE2LLVMIR_RISCV_RISCV_DEFS_H
#define RETDEC_CAPSTONE2LLVMIR_RISCV_RISCV_DEFS_H

#include <capstone/riscv.h>

namespace retdec {
namespace capstone2llvmir {

/**
 * Capstone does not expose the program counter as a RISC-V register, because
 * it is never encoded in general-purpose register operands. The translator
 * still creates it for parity with the other architecture backends.
 */
enum
{
	RISCV_REG_PC = RISCV_REG_ENDING + 1,
};

} // namespace capstone2llvmir
} // namespace retdec

#endif
