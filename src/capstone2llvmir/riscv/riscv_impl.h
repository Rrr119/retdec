/**
 * @file src/capstone2llvmir/riscv/riscv_impl.h
 * @brief RISC-V implementation of @c Capstone2LlvmIrTranslator.
 */

#ifndef CAPSTONE2LLVMIR_RISCV_RISCV_IMPL_H
#define CAPSTONE2LLVMIR_RISCV_RISCV_IMPL_H

#include "retdec/capstone2llvmir/riscv/riscv.h"
#include "capstone2llvmir/capstone2llvmir_impl.h"

namespace retdec {
namespace capstone2llvmir {

class Capstone2LlvmIrTranslatorRiscV_impl :
		public Capstone2LlvmIrTranslator_impl<cs_riscv, cs_riscv_op>,
		public Capstone2LlvmIrTranslatorRiscV
{
	public:
		Capstone2LlvmIrTranslatorRiscV_impl(
				llvm::Module* m,
				cs_mode basic = CS_MODE_RISCV32,
				cs_mode extra = CS_MODE_RISCVC);

	public:
		virtual bool isAllowedBasicMode(cs_mode m) override;
		virtual bool isAllowedExtraMode(cs_mode m) override;
		virtual uint32_t getArchByteSize() override;

		virtual bool isCallInstruction(cs_insn& i) const override;
		virtual bool isReturnInstruction(cs_insn& i) const override;
		virtual bool isBranchInstruction(cs_insn& i) const override;
		virtual bool isCondBranchInstruction(cs_insn& i) const override;

	protected:
		virtual void initializeArchSpecific() override;
		virtual void initializeRegNameMap() override;
		virtual void initializeRegTypeMap() override;
		virtual void initializePseudoCallInstructionIDs() override;
		virtual void generateEnvironmentArchSpecific() override;
		virtual void generateDataLayout() override;
		virtual void generateRegisters() override;
		virtual uint32_t getCarryRegister() override;

		virtual void translateInstruction(
				cs_insn* i,
				llvm::IRBuilder<>& irb) override;

		virtual llvm::Value* loadRegister(
				uint32_t r,
				llvm::IRBuilder<>& irb,
				llvm::Type* dstType = nullptr,
				eOpConv ct = eOpConv::THROW) override;
		virtual llvm::Value* loadOp(
				cs_riscv_op& op,
				llvm::IRBuilder<>& irb,
				llvm::Type* ty = nullptr,
				bool lea = false) override;
		virtual llvm::Instruction* storeRegister(
				uint32_t r,
				llvm::Value* val,
				llvm::IRBuilder<>& irb,
				eOpConv ct = eOpConv::SEXT_TRUNC_OR_BITCAST) override;
		virtual llvm::Instruction* storeOp(
				cs_riscv_op& op,
				llvm::Value* val,
				llvm::IRBuilder<>& irb,
				eOpConv ct = eOpConv::SEXT_TRUNC_OR_BITCAST) override;

		virtual bool isOperandRegister(cs_riscv_op& op) override;

	protected:
		uint32_t getInstructionWord(const cs_insn* i) const;
		uint32_t getJalRd(const cs_insn* i) const;
		uint32_t getJalrRd(const cs_insn* i) const;
		uint32_t getJalrRs1(const cs_insn* i) const;
		uint32_t getCompressedJrRs1(const cs_insn* i) const;
		uint32_t getCsrNumber(const cs_insn* i) const;

		llvm::Value* getBranchTarget(cs_insn* i, int64_t offset);
		llvm::Value* getSignExtendedImmediate(
				llvm::Value* value,
				unsigned bitSize,
				llvm::IRBuilder<>& irb);
		std::pair<llvm::Value*, llvm::Value*> loadOp0AndLast(
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		llvm::Value* loadCompressedMemoryAddress(
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
	protected:
		void translateAdd(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSub(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateAnd(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateOr(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateXor(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSll(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSrl(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSra(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSlt(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSltu(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateAddImm(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateAddImmW(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateAddW(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSubW(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSllW(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSrlW(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSraW(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateMul(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateMulh(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateMulW(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateDiv(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateRem(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateDivW(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateRemW(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateLui(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateAuipc(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateJal(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateJalr(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateCondBranch(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateLoadMemory(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateStoreMemory(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateMove(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateLi(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateAddi16sp(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateAddi4spn(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateCompressedLoad(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateCompressedStore(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateCsr(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);
		void translateSystem(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateAmo(cs_insn* i, cs_riscv* ci, llvm::IRBuilder<>& irb);

		void translateFpBinary(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateFpUnary(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateFpCompare(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateFpConvert(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateFpMove(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateFpFma(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);
		void translateFpSignInject(
				cs_insn* i,
				cs_riscv* ci,
				llvm::IRBuilder<>& irb);

	protected:
		static std::map<
				std::size_t,
				void (Capstone2LlvmIrTranslatorRiscV_impl::*)(
						cs_insn*,
						cs_riscv*,
						llvm::IRBuilder<>&)> _i2fm;
};

} // namespace capstone2llvmir
} // namespace retdec

#endif
