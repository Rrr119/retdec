/**
 * @file src/bin2llvmir/providers/calling_convention/riscv/riscv_conv.cpp
 * @brief Calling convention of RISC-V architecture.
 */

#include "retdec/bin2llvmir/providers/abi/abi.h"
#include "retdec/bin2llvmir/providers/calling_convention/riscv/riscv_conv.h"
#include "retdec/capstone2llvmir/riscv/riscv_defs.h"

namespace retdec {
namespace bin2llvmir {

RiscVCallingConvention::RiscVCallingConvention(const Abi* a) :
		CallingConvention(a)
{
	_paramRegs =
	{
		RISCV_REG_A0,
		RISCV_REG_A1,
		RISCV_REG_A2,
		RISCV_REG_A3,
		RISCV_REG_A4,
		RISCV_REG_A5,
		RISCV_REG_A6,
		RISCV_REG_A7,
	};
	_paramFPRegs =
	{
		RISCV_REG_F10_32,
		RISCV_REG_F11_32,
		RISCV_REG_F12_32,
		RISCV_REG_F13_32,
		RISCV_REG_F14_32,
		RISCV_REG_F15_32,
		RISCV_REG_F16_32,
		RISCV_REG_F17_32,
	};
	_paramDoubleRegs =
	{
		RISCV_REG_F10_64,
		RISCV_REG_F11_64,
		RISCV_REG_F12_64,
		RISCV_REG_F13_64,
		RISCV_REG_F14_64,
		RISCV_REG_F15_64,
		RISCV_REG_F16_64,
		RISCV_REG_F17_64,
	};

	_returnRegs =
	{
		RISCV_REG_A0,
		RISCV_REG_A1,
	};
	_returnFPRegs =
	{
		RISCV_REG_F10_32,
		RISCV_REG_F11_32,
	};
	_returnDoubleRegs =
	{
		RISCV_REG_F10_64,
		RISCV_REG_F11_64,
	};
}

CallingConvention::Ptr RiscVCallingConvention::create(const Abi* a)
{
	if (!a->isRiscV())
	{
		return nullptr;
	}
	return std::make_unique<RiscVCallingConvention>(a);
}

} // namespace bin2llvmir
} // namespace retdec
