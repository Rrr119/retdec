/**
 * @file include/retdec/capstone2llvmir/riscv/riscv.h
 * @brief RISC-V specialization of translator's abstract public interface.
 */

#ifndef RETDEC_CAPSTONE2LLVMIR_RISCV_RISCV_H
#define RETDEC_CAPSTONE2LLVMIR_RISCV_RISCV_H

#include "retdec/capstone2llvmir/capstone2llvmir.h"
#include "retdec/capstone2llvmir/riscv/riscv_defs.h"

namespace retdec {
namespace capstone2llvmir {

/**
 * RISC-V specialization of translator's abstract public interface.
 */
class Capstone2LlvmIrTranslatorRiscV : virtual public Capstone2LlvmIrTranslator
{
};

using Capstone2LlvmIrTranslatorRiscv = Capstone2LlvmIrTranslatorRiscV;

} // namespace capstone2llvmir
} // namespace retdec

#endif
