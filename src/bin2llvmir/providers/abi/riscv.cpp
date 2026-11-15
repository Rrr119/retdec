/**
 * @file src/bin2llvmir/providers/abi/riscv.cpp
 * @brief ABI information for RISC-V.
 */

#include "retdec/bin2llvmir/providers/abi/riscv.h"
#include "retdec/capstone2llvmir/riscv/riscv_defs.h"

using namespace llvm;

namespace retdec {
namespace bin2llvmir {

AbiRiscV::AbiRiscV(llvm::Module* m, Config* c) :
		Abi(m, c)
{
	_regs.reserve(RISCV_REG_ENDING);
	_id2regs.resize(RISCV_REG_ENDING, nullptr);
	_regStackPointerId = RISCV_REG_SP;
	_regZeroReg = RISCV_REG_X0;

	_regSyscallId = RISCV_REG_A7;
	_regSyscallReturn = RISCV_REG_A0;
	_syscallRegs =
	{
		RISCV_REG_A0,
		RISCV_REG_A1,
		RISCV_REG_A2,
		RISCV_REG_A3,
		RISCV_REG_A4,
		RISCV_REG_A5,
	};

	_defcc = isRiscV64()
			? CallingConvention::ID::CC_RISCV64
			: CallingConvention::ID::CC_RISCV;
}

bool AbiRiscV::isGeneralPurposeRegister(const llvm::Value* val) const
{
	uint32_t rid = getRegisterId(val);
	return RISCV_REG_X0 <= rid && rid <= RISCV_REG_X31;
}

bool AbiRiscV::isNopInstruction(cs_insn* insn)
{
	return insn->id == RISCV_INS_C_NOP
			|| std::string(insn->mnemonic) == "nop";
}

} // namespace bin2llvmir
} // namespace retdec
