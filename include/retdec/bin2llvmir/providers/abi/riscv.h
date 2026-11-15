/**
 * @file include/retdec/bin2llvmir/providers/abi/riscv.h
 * @brief ABI information for RISC-V.
 */

#ifndef RETDEC_BIN2LLVMIR_PROVIDERS_ABI_RISCV_H
#define RETDEC_BIN2LLVMIR_PROVIDERS_ABI_RISCV_H

#include "retdec/bin2llvmir/providers/abi/abi.h"

namespace retdec {
namespace bin2llvmir {

class AbiRiscV : public Abi
{
	public:
		AbiRiscV(llvm::Module* m, Config* c);

	public:
		virtual bool isGeneralPurposeRegister(
				const llvm::Value* val) const override;
		virtual bool isNopInstruction(cs_insn* insn) override;
};

} // namespace bin2llvmir
} // namespace retdec

#endif
