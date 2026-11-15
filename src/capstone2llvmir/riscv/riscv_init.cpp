/**
 * @file src/capstone2llvmir/riscv/riscv_init.cpp
 * @brief Initializations for RISC-V implementation of
 *        @c Capstone2LlvmIrTranslator.
 */

#include "capstone2llvmir/riscv/riscv_impl.h"

namespace retdec {
namespace capstone2llvmir {

void Capstone2LlvmIrTranslatorRiscV_impl::initializeArchSpecific()
{
	// Nothing.
}

void Capstone2LlvmIrTranslatorRiscV_impl::initializeRegNameMap()
{
	_reg2name =
	{
		{RISCV_REG_PC, "pc"},
	};
}

void Capstone2LlvmIrTranslatorRiscV_impl::initializeRegTypeMap()
{
	auto* i32 = llvm::IntegerType::getInt32Ty(_module->getContext());
	auto* i64 = llvm::IntegerType::getInt64Ty(_module->getContext());
	auto* f32 = llvm::Type::getFloatTy(_module->getContext());
	auto* f64 = llvm::Type::getDoubleTy(_module->getContext());

	auto* defTy = _basicMode == CS_MODE_RISCV64 ? i64 : i32;
	std::map<uint32_t, llvm::Type*> r2t;

	for (uint32_t r = RISCV_REG_X0; r <= RISCV_REG_X31; ++r)
	{
		r2t.emplace(r, defTy);
	}
	for (uint32_t r = 0; r < 32; ++r)
	{
		r2t.emplace(RISCV_REG_F0_32 + (2 * r), f32);
		r2t.emplace(RISCV_REG_F0_64 + (2 * r), f64);
	}
	r2t.emplace(RISCV_REG_PC, defTy);

	_reg2type = std::move(r2t);
}

void Capstone2LlvmIrTranslatorRiscV_impl::initializePseudoCallInstructionIDs()
{
	_callInsnIds =
	{
		RISCV_INS_C_JAL,
		RISCV_INS_C_JALR,
	};

	_branchInsnIds =
	{
		RISCV_INS_C_J,
	};

	_condBranchInsnIds =
	{
		RISCV_INS_BEQ,
		RISCV_INS_BNE,
		RISCV_INS_BLT,
		RISCV_INS_BGE,
		RISCV_INS_BLTU,
		RISCV_INS_BGEU,
		RISCV_INS_C_BEQZ,
		RISCV_INS_C_BNEZ,
	};
}

std::map<
		std::size_t,
		void (Capstone2LlvmIrTranslatorRiscV_impl::*)(
				cs_insn*,
				cs_riscv*,
				llvm::IRBuilder<>&)>
Capstone2LlvmIrTranslatorRiscV_impl::_i2fm =
{
	{RISCV_INS_ADD, &Capstone2LlvmIrTranslatorRiscV_impl::translateAdd},
	{RISCV_INS_ADDI, &Capstone2LlvmIrTranslatorRiscV_impl::translateAddImm},
	{RISCV_INS_ADDIW, &Capstone2LlvmIrTranslatorRiscV_impl::translateAddImmW},
	{RISCV_INS_ADDW, &Capstone2LlvmIrTranslatorRiscV_impl::translateAddW},
	{RISCV_INS_AND, &Capstone2LlvmIrTranslatorRiscV_impl::translateAnd},
	{RISCV_INS_ANDI, &Capstone2LlvmIrTranslatorRiscV_impl::translateAnd},
	{RISCV_INS_AUIPC, &Capstone2LlvmIrTranslatorRiscV_impl::translateAuipc},
	{RISCV_INS_BEQ, &Capstone2LlvmIrTranslatorRiscV_impl::translateCondBranch},
	{RISCV_INS_BGE, &Capstone2LlvmIrTranslatorRiscV_impl::translateCondBranch},
	{RISCV_INS_BGEU, &Capstone2LlvmIrTranslatorRiscV_impl::translateCondBranch},
	{RISCV_INS_BLT, &Capstone2LlvmIrTranslatorRiscV_impl::translateCondBranch},
	{RISCV_INS_BLTU, &Capstone2LlvmIrTranslatorRiscV_impl::translateCondBranch},
	{RISCV_INS_BNE, &Capstone2LlvmIrTranslatorRiscV_impl::translateCondBranch},
	{RISCV_INS_CSRRC, &Capstone2LlvmIrTranslatorRiscV_impl::translateCsr},
	{RISCV_INS_CSRRCI, &Capstone2LlvmIrTranslatorRiscV_impl::translateCsr},
	{RISCV_INS_CSRRS, &Capstone2LlvmIrTranslatorRiscV_impl::translateCsr},
	{RISCV_INS_CSRRSI, &Capstone2LlvmIrTranslatorRiscV_impl::translateCsr},
	{RISCV_INS_CSRRW, &Capstone2LlvmIrTranslatorRiscV_impl::translateCsr},
	{RISCV_INS_CSRRWI, &Capstone2LlvmIrTranslatorRiscV_impl::translateCsr},
	{RISCV_INS_C_ADD, &Capstone2LlvmIrTranslatorRiscV_impl::translateAdd},
	{RISCV_INS_C_ADDI, &Capstone2LlvmIrTranslatorRiscV_impl::translateAddImm},
	{RISCV_INS_C_ADDI16SP, &Capstone2LlvmIrTranslatorRiscV_impl::translateAddi16sp},
	{RISCV_INS_C_ADDI4SPN, &Capstone2LlvmIrTranslatorRiscV_impl::translateAddi4spn},
	{RISCV_INS_C_ADDIW, &Capstone2LlvmIrTranslatorRiscV_impl::translateAddImmW},
	{RISCV_INS_C_ADDW, &Capstone2LlvmIrTranslatorRiscV_impl::translateAddW},
	{RISCV_INS_C_AND, &Capstone2LlvmIrTranslatorRiscV_impl::translateAnd},
	{RISCV_INS_C_ANDI, &Capstone2LlvmIrTranslatorRiscV_impl::translateAnd},
	{RISCV_INS_C_BEQZ, &Capstone2LlvmIrTranslatorRiscV_impl::translateCondBranch},
	{RISCV_INS_C_BNEZ, &Capstone2LlvmIrTranslatorRiscV_impl::translateCondBranch},
	{RISCV_INS_C_EBREAK, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_C_FLD, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedLoad},
	{RISCV_INS_C_FLDSP, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedLoad},
	{RISCV_INS_C_FLW, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedLoad},
	{RISCV_INS_C_FLWSP, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedLoad},
	{RISCV_INS_C_FSD, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedStore},
	{RISCV_INS_C_FSDSP, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedStore},
	{RISCV_INS_C_FSW, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedStore},
	{RISCV_INS_C_FSWSP, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedStore},
	{RISCV_INS_C_J, &Capstone2LlvmIrTranslatorRiscV_impl::translateJal},
	{RISCV_INS_C_JAL, &Capstone2LlvmIrTranslatorRiscV_impl::translateJal},
	{RISCV_INS_C_JALR, &Capstone2LlvmIrTranslatorRiscV_impl::translateJalr},
	{RISCV_INS_C_JR, &Capstone2LlvmIrTranslatorRiscV_impl::translateJalr},
	{RISCV_INS_C_LD, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedLoad},
	{RISCV_INS_C_LDSP, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedLoad},
	{RISCV_INS_C_LI, &Capstone2LlvmIrTranslatorRiscV_impl::translateLi},
	{RISCV_INS_C_LUI, &Capstone2LlvmIrTranslatorRiscV_impl::translateLui},
	{RISCV_INS_C_LW, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedLoad},
	{RISCV_INS_C_LWSP, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedLoad},
	{RISCV_INS_C_MV, &Capstone2LlvmIrTranslatorRiscV_impl::translateMove},
	{RISCV_INS_C_NOP, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_C_OR, &Capstone2LlvmIrTranslatorRiscV_impl::translateOr},
	{RISCV_INS_C_SD, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedStore},
	{RISCV_INS_C_SDSP, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedStore},
	{RISCV_INS_C_SLLI, &Capstone2LlvmIrTranslatorRiscV_impl::translateSll},
	{RISCV_INS_C_SRAI, &Capstone2LlvmIrTranslatorRiscV_impl::translateSra},
	{RISCV_INS_C_SRLI, &Capstone2LlvmIrTranslatorRiscV_impl::translateSrl},
	{RISCV_INS_C_SUB, &Capstone2LlvmIrTranslatorRiscV_impl::translateSub},
	{RISCV_INS_C_SUBW, &Capstone2LlvmIrTranslatorRiscV_impl::translateSubW},
	{RISCV_INS_C_SW, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedStore},
	{RISCV_INS_C_SWSP, &Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedStore},
	{RISCV_INS_C_UNIMP, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_C_XOR, &Capstone2LlvmIrTranslatorRiscV_impl::translateXor},
	{RISCV_INS_DIV, &Capstone2LlvmIrTranslatorRiscV_impl::translateDiv},
	{RISCV_INS_DIVU, &Capstone2LlvmIrTranslatorRiscV_impl::translateDiv},
	{RISCV_INS_DIVUW, &Capstone2LlvmIrTranslatorRiscV_impl::translateDivW},
	{RISCV_INS_DIVW, &Capstone2LlvmIrTranslatorRiscV_impl::translateDivW},
	{RISCV_INS_EBREAK, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_ECALL, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_FADD_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FADD_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FCVT_D_L, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_D_LU, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_D_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_D_W, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_D_WU, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_L_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_L_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_LU_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_LU_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_S_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_S_L, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_S_LU, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_S_W, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_S_WU, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_W_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_W_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_WU_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FCVT_WU_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert},
	{RISCV_INS_FDIV_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FDIV_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FENCE, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_FENCE_I, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_FENCE_TSO, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_FEQ_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpCompare},
	{RISCV_INS_FEQ_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpCompare},
	{RISCV_INS_FLD, &Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory},
	{RISCV_INS_FLE_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpCompare},
	{RISCV_INS_FLE_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpCompare},
	{RISCV_INS_FLT_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpCompare},
	{RISCV_INS_FLT_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpCompare},
	{RISCV_INS_FLW, &Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory},
	{RISCV_INS_FMADD_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpFma},
	{RISCV_INS_FMADD_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpFma},
	{RISCV_INS_FMAX_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FMAX_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FMIN_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FMIN_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FMSUB_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpFma},
	{RISCV_INS_FMSUB_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpFma},
	{RISCV_INS_FMUL_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FMUL_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FMV_D_X, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpMove},
	{RISCV_INS_FMV_W_X, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpMove},
	{RISCV_INS_FMV_X_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpMove},
	{RISCV_INS_FMV_X_W, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpMove},
	{RISCV_INS_FNMADD_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpFma},
	{RISCV_INS_FNMADD_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpFma},
	{RISCV_INS_FNMSUB_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpFma},
	{RISCV_INS_FNMSUB_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpFma},
	{RISCV_INS_FSD, &Capstone2LlvmIrTranslatorRiscV_impl::translateStoreMemory},
	{RISCV_INS_FSGNJ_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpSignInject},
	{RISCV_INS_FSGNJ_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpSignInject},
	{RISCV_INS_FSGNJN_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpSignInject},
	{RISCV_INS_FSGNJN_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpSignInject},
	{RISCV_INS_FSGNJX_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpSignInject},
	{RISCV_INS_FSGNJX_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpSignInject},
	{RISCV_INS_FSQRT_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpUnary},
	{RISCV_INS_FSQRT_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpUnary},
	{RISCV_INS_FSUB_D, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FSUB_S, &Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary},
	{RISCV_INS_FSW, &Capstone2LlvmIrTranslatorRiscV_impl::translateStoreMemory},
	{RISCV_INS_JAL, &Capstone2LlvmIrTranslatorRiscV_impl::translateJal},
	{RISCV_INS_JALR, &Capstone2LlvmIrTranslatorRiscV_impl::translateJalr},
	{RISCV_INS_LB, &Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory},
	{RISCV_INS_LBU, &Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory},
	{RISCV_INS_LD, &Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory},
	{RISCV_INS_LH, &Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory},
	{RISCV_INS_LHU, &Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory},
	{RISCV_INS_LUI, &Capstone2LlvmIrTranslatorRiscV_impl::translateLui},
	{RISCV_INS_LW, &Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory},
	{RISCV_INS_LWU, &Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory},
	{RISCV_INS_MRET, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_MUL, &Capstone2LlvmIrTranslatorRiscV_impl::translateMul},
	{RISCV_INS_MULH, &Capstone2LlvmIrTranslatorRiscV_impl::translateMulh},
	{RISCV_INS_MULHSU, &Capstone2LlvmIrTranslatorRiscV_impl::translateMulh},
	{RISCV_INS_MULHU, &Capstone2LlvmIrTranslatorRiscV_impl::translateMulh},
	{RISCV_INS_MULW, &Capstone2LlvmIrTranslatorRiscV_impl::translateMulW},
	{RISCV_INS_OR, &Capstone2LlvmIrTranslatorRiscV_impl::translateOr},
	{RISCV_INS_ORI, &Capstone2LlvmIrTranslatorRiscV_impl::translateOr},
	{RISCV_INS_REM, &Capstone2LlvmIrTranslatorRiscV_impl::translateRem},
	{RISCV_INS_REMU, &Capstone2LlvmIrTranslatorRiscV_impl::translateRem},
	{RISCV_INS_REMUW, &Capstone2LlvmIrTranslatorRiscV_impl::translateRemW},
	{RISCV_INS_REMW, &Capstone2LlvmIrTranslatorRiscV_impl::translateRemW},
	{RISCV_INS_SB, &Capstone2LlvmIrTranslatorRiscV_impl::translateStoreMemory},
	{RISCV_INS_SD, &Capstone2LlvmIrTranslatorRiscV_impl::translateStoreMemory},
	{RISCV_INS_SFENCE_VMA, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_SH, &Capstone2LlvmIrTranslatorRiscV_impl::translateStoreMemory},
	{RISCV_INS_SLL, &Capstone2LlvmIrTranslatorRiscV_impl::translateSll},
	{RISCV_INS_SLLI, &Capstone2LlvmIrTranslatorRiscV_impl::translateSll},
	{RISCV_INS_SLLIW, &Capstone2LlvmIrTranslatorRiscV_impl::translateSllW},
	{RISCV_INS_SLLW, &Capstone2LlvmIrTranslatorRiscV_impl::translateSllW},
	{RISCV_INS_SLT, &Capstone2LlvmIrTranslatorRiscV_impl::translateSlt},
	{RISCV_INS_SLTI, &Capstone2LlvmIrTranslatorRiscV_impl::translateSlt},
	{RISCV_INS_SLTIU, &Capstone2LlvmIrTranslatorRiscV_impl::translateSltu},
	{RISCV_INS_SLTU, &Capstone2LlvmIrTranslatorRiscV_impl::translateSltu},
	{RISCV_INS_SRA, &Capstone2LlvmIrTranslatorRiscV_impl::translateSra},
	{RISCV_INS_SRAI, &Capstone2LlvmIrTranslatorRiscV_impl::translateSra},
	{RISCV_INS_SRAIW, &Capstone2LlvmIrTranslatorRiscV_impl::translateSraW},
	{RISCV_INS_SRAW, &Capstone2LlvmIrTranslatorRiscV_impl::translateSraW},
	{RISCV_INS_SRET, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_SRL, &Capstone2LlvmIrTranslatorRiscV_impl::translateSrl},
	{RISCV_INS_SRLI, &Capstone2LlvmIrTranslatorRiscV_impl::translateSrl},
	{RISCV_INS_SRLIW, &Capstone2LlvmIrTranslatorRiscV_impl::translateSrlW},
	{RISCV_INS_SRLW, &Capstone2LlvmIrTranslatorRiscV_impl::translateSrlW},
	{RISCV_INS_SUB, &Capstone2LlvmIrTranslatorRiscV_impl::translateSub},
	{RISCV_INS_SUBW, &Capstone2LlvmIrTranslatorRiscV_impl::translateSubW},
	{RISCV_INS_SW, &Capstone2LlvmIrTranslatorRiscV_impl::translateStoreMemory},
	{RISCV_INS_UNIMP, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_URET, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_WFI, &Capstone2LlvmIrTranslatorRiscV_impl::translateSystem},
	{RISCV_INS_XOR, &Capstone2LlvmIrTranslatorRiscV_impl::translateXor},
	{RISCV_INS_XORI, &Capstone2LlvmIrTranslatorRiscV_impl::translateXor},
};

} // namespace capstone2llvmir
} // namespace retdec
