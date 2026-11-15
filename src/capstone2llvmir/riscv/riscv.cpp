/**
 * @file src/capstone2llvmir/riscv/riscv.cpp
 * @brief RISC-V implementation of @c Capstone2LlvmIrTranslator.
 */

#include <set>

#include "capstone2llvmir/riscv/riscv_impl.h"

namespace retdec {
namespace capstone2llvmir {

namespace {

llvm::ConstantInt* getSignedInt(llvm::IntegerType* type, int64_t value)
{
	return llvm::ConstantInt::get(
			type,
			static_cast<uint64_t>(value),
			true);
}

} // anonymous namespace

Capstone2LlvmIrTranslatorRiscV_impl::Capstone2LlvmIrTranslatorRiscV_impl(
		llvm::Module* m,
		cs_mode basic,
		cs_mode extra)
		:
		Capstone2LlvmIrTranslator_impl(CS_ARCH_RISCV, basic, extra, m)
{
	initialize();
}

//
//==============================================================================
// Mode query & modification methods.
//==============================================================================
//

bool Capstone2LlvmIrTranslatorRiscV_impl::isAllowedBasicMode(cs_mode m)
{
	return m == CS_MODE_RISCV32 || m == CS_MODE_RISCV64;
}

bool Capstone2LlvmIrTranslatorRiscV_impl::isAllowedExtraMode(cs_mode m)
{
	const auto allowed = static_cast<unsigned>(CS_MODE_RISCVC)
			| static_cast<unsigned>(CS_MODE_BIG_ENDIAN);
	return (static_cast<unsigned>(m) & ~allowed) == 0;
}

uint32_t Capstone2LlvmIrTranslatorRiscV_impl::getArchByteSize()
{
	switch (_basicMode)
	{
		case CS_MODE_RISCV32:
			return 4;
		case CS_MODE_RISCV64:
			return 8;
		default:
			throw GenericError("Unhandled RISC-V mode in getArchByteSize().");
	}
}

//
//==============================================================================
// Control flow classification.
//==============================================================================
//

bool Capstone2LlvmIrTranslatorRiscV_impl::isCallInstruction(cs_insn& i) const
{
	switch (i.id)
	{
		case RISCV_INS_C_JAL:
		case RISCV_INS_C_JALR:
			return true;
		case RISCV_INS_JAL:
			return getJalRd(&i) != RISCV_REG_X0;
		case RISCV_INS_JALR:
			return getJalrRd(&i) != RISCV_REG_X0;
		default:
			return false;
	}
}

bool Capstone2LlvmIrTranslatorRiscV_impl::isReturnInstruction(cs_insn& i) const
{
	switch (i.id)
	{
		case RISCV_INS_C_JR:
			return getCompressedJrRs1(&i) == RISCV_REG_RA;
		case RISCV_INS_JALR:
			return getJalrRd(&i) == RISCV_REG_X0
					&& getJalrRs1(&i) == RISCV_REG_RA;
		default:
			return false;
	}
}

bool Capstone2LlvmIrTranslatorRiscV_impl::isBranchInstruction(cs_insn& i) const
{
	switch (i.id)
	{
		case RISCV_INS_C_J:
			return true;
		case RISCV_INS_C_JR:
			return getCompressedJrRs1(&i) != RISCV_REG_RA;
		case RISCV_INS_JAL:
			return getJalRd(&i) == RISCV_REG_X0;
		case RISCV_INS_JALR:
			return getJalrRd(&i) == RISCV_REG_X0
					&& getJalrRs1(&i) != RISCV_REG_RA;
		default:
			return false;
	}
}

bool Capstone2LlvmIrTranslatorRiscV_impl::isCondBranchInstruction(
		cs_insn& i) const
{
	static const std::set<uint32_t> ids =
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
	return ids.count(i.id) != 0;
}

//
//==============================================================================
// Instruction and register helpers.
//==============================================================================
//

uint32_t Capstone2LlvmIrTranslatorRiscV_impl::getInstructionWord(
		const cs_insn* i) const
{
	uint32_t word = 0;
	const std::size_t n = i->size < 4 ? i->size : 4;
	for (std::size_t j = 0; j < n; ++j)
	{
		word |= static_cast<uint32_t>(i->bytes[j]) << (8 * j);
	}
	return word;
}

uint32_t Capstone2LlvmIrTranslatorRiscV_impl::getJalRd(const cs_insn* i) const
{
	switch (i->id)
	{
		case RISCV_INS_C_J:
			return RISCV_REG_X0;
		case RISCV_INS_C_JAL:
			return RISCV_REG_RA;
		default:
			return RISCV_REG_X0 + ((getInstructionWord(i) >> 7) & 0x1f);
	}
}

uint32_t Capstone2LlvmIrTranslatorRiscV_impl::getJalrRd(const cs_insn* i) const
{
	switch (i->id)
	{
		case RISCV_INS_C_JR:
			return RISCV_REG_X0;
		case RISCV_INS_C_JALR:
			return RISCV_REG_RA;
		default:
			return RISCV_REG_X0 + ((getInstructionWord(i) >> 7) & 0x1f);
	}
}

uint32_t Capstone2LlvmIrTranslatorRiscV_impl::getJalrRs1(
		const cs_insn* i) const
{
	if (i->id == RISCV_INS_C_JR || i->id == RISCV_INS_C_JALR)
	{
		return RISCV_REG_X0 + ((getInstructionWord(i) >> 7) & 0x1f);
	}
	return RISCV_REG_X0 + ((getInstructionWord(i) >> 15) & 0x1f);
}

uint32_t Capstone2LlvmIrTranslatorRiscV_impl::getCompressedJrRs1(
		const cs_insn* i) const
{
	return RISCV_REG_X0 + ((getInstructionWord(i) >> 7) & 0x1f);
}

uint32_t Capstone2LlvmIrTranslatorRiscV_impl::getCsrNumber(
		const cs_insn* i) const
{
	return (getInstructionWord(i) >> 20) & 0xfff;
}

llvm::Value* Capstone2LlvmIrTranslatorRiscV_impl::getBranchTarget(
		cs_insn* i,
		int64_t offset)
{
	return getSignedInt(
			getDefaultType(),
			static_cast<int64_t>(i->address) + offset);
}

llvm::Value* Capstone2LlvmIrTranslatorRiscV_impl::getSignExtendedImmediate(
		llvm::Value* value,
		unsigned bitSize,
		llvm::IRBuilder<>& irb)
{
	auto* t = getDefaultType();
	if (bitSize >= t->getBitWidth())
	{
		return irb.CreateSExtOrTrunc(value, t);
	}
	auto* narrow = irb.CreateTrunc(value, irb.getIntNTy(bitSize));
	return irb.CreateSExt(narrow, t);
}

std::pair<llvm::Value*, llvm::Value*>
Capstone2LlvmIrTranslatorRiscV_impl::loadOp0AndLast(
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	if (ci->op_count == 2)
	{
		return std::make_pair(
				loadOp(ci->operands[0], irb),
				loadOp(ci->operands[1], irb));
	}
	if (ci->op_count >= 3)
	{
		return std::make_pair(
				loadOp(ci->operands[1], irb),
				loadOp(ci->operands[2], irb));
	}
	throwUnexpectedOperands(_insn);
	return {nullptr, nullptr};
}

llvm::Value* Capstone2LlvmIrTranslatorRiscV_impl::loadCompressedMemoryAddress(
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	if (ci->op_count != 3 || ci->operands[1].type != RISCV_OP_IMM
			|| ci->operands[2].type != RISCV_OP_REG)
	{
		throwUnexpectedOperands(_insn);
		return nullptr;
	}

	auto* base = loadRegister(ci->operands[2].reg, irb);
	auto* disp = getSignedInt(getDefaultType(), ci->operands[1].imm);
	return irb.CreateAdd(base, disp);
}

//
//==============================================================================
// Environment initialization.
//==============================================================================
//

void Capstone2LlvmIrTranslatorRiscV_impl::generateEnvironmentArchSpecific()
{
	// Nothing.
}

void Capstone2LlvmIrTranslatorRiscV_impl::generateDataLayout()
{
	switch (_basicMode)
	{
		case CS_MODE_RISCV32:
			_module->setDataLayout("e-p:32:32:32-i64:64-n32-S128");
			break;
		case CS_MODE_RISCV64:
			_module->setDataLayout("e-p:64:64:64-i64:64-i128:128-n32:64-S128");
			break;
		default:
			throw GenericError("Unhandled RISC-V mode in generateDataLayout().");
	}
}

void Capstone2LlvmIrTranslatorRiscV_impl::generateRegisters()
{
	for (const auto& p : _reg2type)
	{
		createRegister(p.first, _regLt);
	}
}

uint32_t Capstone2LlvmIrTranslatorRiscV_impl::getCarryRegister()
{
	return RISCV_REG_INVALID;
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateInstruction(
		cs_insn* i,
		llvm::IRBuilder<>& irb)
{
	_insn = i;
	auto* ci = &i->detail->riscv;
	const std::string mnemonic = i->mnemonic;
	if (mnemonic.rfind("amo", 0) == 0
			|| mnemonic.rfind("lr.", 0) == 0
			|| mnemonic.rfind("sc.", 0) == 0)
	{
		translateAmo(i, ci, irb);
		return;
	}

	auto fIt = _i2fm.find(i->id);
	if (fIt != _i2fm.end() && fIt->second != nullptr)
	{
		(this->*fIt->second)(i, ci, irb);
	}
	else
	{
		throwUnhandledInstructions(i);
		translatePseudoAsmGeneric(i, ci, irb);
	}
}

//
//==============================================================================
// Register and operand access.
//==============================================================================
//

llvm::Value* Capstone2LlvmIrTranslatorRiscV_impl::loadRegister(
		uint32_t r,
		llvm::IRBuilder<>& irb,
		llvm::Type* dstType,
		eOpConv ct)
{
	if (r == RISCV_REG_INVALID)
	{
		return nullptr;
	}
	if (r == RISCV_REG_X0)
	{
		return getSignedInt(getDefaultType(), 0);
	}
	if (r == RISCV_REG_PC)
	{
		return getThisInsnAddress(_insn);
	}

	auto* gv = getRegister(r);
	if (gv == nullptr)
	{
		throw GenericError("loadRegister() unhandled RISC-V register.");
	}

	llvm::Value* value = irb.CreateLoad(gv);
	if (dstType != nullptr)
	{
		value = generateTypeConversion(irb, value, dstType, ct);
	}
	return value;
}

llvm::Value* Capstone2LlvmIrTranslatorRiscV_impl::loadOp(
		cs_riscv_op& op,
		llvm::IRBuilder<>& irb,
		llvm::Type* ty,
		bool lea)
{
	switch (op.type)
	{
		case RISCV_OP_REG:
		{
			auto* value = loadRegister(op.reg, irb);
			return value ? value : llvm::UndefValue::get(
					ty ? ty : getDefaultType());
		}
		case RISCV_OP_IMM:
			return getSignedInt(getDefaultType(), op.imm);
		case RISCV_OP_MEM:
		{
			auto* base = loadRegister(op.mem.base, irb);
			auto* disp = getSignedInt(getDefaultType(), op.mem.disp);
			auto* addr = base ? irb.CreateAdd(base, disp) : disp;
			if (lea)
			{
				return addr;
			}

			auto* valueType = ty ? ty : getDefaultType();
			addr = irb.CreateIntToPtr(
					addr,
					llvm::PointerType::get(valueType, 0));
			return irb.CreateLoad(addr);
		}
		case RISCV_OP_INVALID:
		default:
			return llvm::UndefValue::get(ty ? ty : getDefaultType());
	}
}

llvm::Instruction* Capstone2LlvmIrTranslatorRiscV_impl::storeRegister(
		uint32_t r,
		llvm::Value* val,
		llvm::IRBuilder<>& irb,
		eOpConv ct)
{
	if (r == RISCV_REG_INVALID || r == RISCV_REG_X0)
	{
		return nullptr;
	}

	auto* gv = getRegister(r);
	if (gv == nullptr)
	{
		throw GenericError("storeRegister() unhandled RISC-V register.");
	}

	auto* type = gv->getValueType();
	if (type->isFloatingPointTy())
	{
		ct = eOpConv::FPCAST_OR_BITCAST;
	}
	val = generateTypeConversion(irb, val, type, ct);
	return irb.CreateStore(val, gv);
}

llvm::Instruction* Capstone2LlvmIrTranslatorRiscV_impl::storeOp(
		cs_riscv_op& op,
		llvm::Value* val,
		llvm::IRBuilder<>& irb,
		eOpConv ct)
{
	switch (op.type)
	{
		case RISCV_OP_REG:
			return storeRegister(op.reg, val, irb, ct);
		case RISCV_OP_MEM:
		{
			auto* base = loadRegister(op.mem.base, irb);
			auto* disp = getSignedInt(getDefaultType(), op.mem.disp);
			auto* addr = base ? irb.CreateAdd(base, disp) : disp;
			addr = irb.CreateIntToPtr(
					addr,
					llvm::PointerType::get(val->getType(), 0));
			return irb.CreateStore(val, addr);
		}
		case RISCV_OP_IMM:
		case RISCV_OP_INVALID:
		default:
			throw GenericError("Invalid RISC-V store operand.");
	}
}

bool Capstone2LlvmIrTranslatorRiscV_impl::isOperandRegister(cs_riscv_op& op)
{
	return op.type == RISCV_OP_REG;
}

//
//==============================================================================
// Integer arithmetic and logical instructions.
//==============================================================================
//

void Capstone2LlvmIrTranslatorRiscV_impl::translateAdd(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* result = irb.CreateAdd(values.first, values.second);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSub(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* result = irb.CreateSub(values.first, values.second);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateAnd(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* result = irb.CreateAnd(values.first, values.second);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateOr(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* result = irb.CreateOr(values.first, values.second);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateXor(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* result = irb.CreateXor(values.first, values.second);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSll(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* shift = irb.CreateZExtOrTrunc(
			values.second,
			values.first->getType());
	auto* result = irb.CreateShl(values.first, shift);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSrl(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* shift = irb.CreateZExtOrTrunc(
			values.second,
			values.first->getType());
	auto* result = irb.CreateLShr(values.first, shift);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSra(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* shift = irb.CreateZExtOrTrunc(
			values.second,
			values.first->getType());
	auto* result = irb.CreateAShr(values.first, shift);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSlt(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* cmp = irb.CreateICmpSLT(values.first, values.second);
	storeOp(ci->operands[0], irb.CreateZExt(cmp, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSltu(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* cmp = irb.CreateICmpULT(values.first, values.second);
	storeOp(ci->operands[0], irb.CreateZExt(cmp, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateAddImm(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOp0AndLast(ci, irb);
	auto* result = irb.CreateAdd(values.first, values.second);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateAddImmW(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOp0AndLast(ci, irb);
	auto* i32 = irb.getInt32Ty();
	auto* add = irb.CreateAdd(
			irb.CreateTrunc(values.first, i32),
			irb.CreateTrunc(values.second, i32));
	storeOp(ci->operands[0], irb.CreateSExt(add, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateAddW(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* i32 = irb.getInt32Ty();
	auto* result = irb.CreateAdd(
			irb.CreateTrunc(values.first, i32),
			irb.CreateTrunc(values.second, i32));
	storeOp(ci->operands[0], irb.CreateSExt(result, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSubW(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* i32 = irb.getInt32Ty();
	auto* result = irb.CreateSub(
			irb.CreateTrunc(values.first, i32),
			irb.CreateTrunc(values.second, i32));
	storeOp(ci->operands[0], irb.CreateSExt(result, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSllW(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* i32 = irb.getInt32Ty();
	auto* value = irb.CreateTrunc(values.first, i32);
	auto* shift = irb.CreateTrunc(values.second, i32);
	auto* result = irb.CreateShl(value, shift);
	storeOp(ci->operands[0], irb.CreateSExt(result, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSrlW(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* i32 = irb.getInt32Ty();
	auto* result = irb.CreateLShr(
			irb.CreateTrunc(values.first, i32),
			irb.CreateTrunc(values.second, i32));
	storeOp(ci->operands[0], irb.CreateSExt(result, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSraW(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);
	auto values = loadOpBinaryOrTernaryOp1Op2(ci, irb);
	auto* i32 = irb.getInt32Ty();
	auto* result = irb.CreateAShr(
			irb.CreateTrunc(values.first, i32),
			irb.CreateTrunc(values.second, i32));
	storeOp(ci->operands[0], irb.CreateSExt(result, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateMul(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(_insn, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* result = irb.CreateMul(std::get<1>(values), std::get<2>(values));
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateMulh(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(_insn, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* lhs = std::get<1>(values);
	auto* rhs = std::get<2>(values);
	const auto narrowBits = lhs->getType()->getIntegerBitWidth();
	auto* wideType = irb.getIntNTy(narrowBits * 2);

	switch (_insn->id)
	{
		case RISCV_INS_MULH:
			lhs = irb.CreateSExt(lhs, wideType);
			rhs = irb.CreateSExt(rhs, wideType);
			break;
		case RISCV_INS_MULHSU:
			lhs = irb.CreateSExt(lhs, wideType);
			rhs = irb.CreateZExt(rhs, wideType);
			break;
		default:
			lhs = irb.CreateZExt(lhs, wideType);
			rhs = irb.CreateZExt(rhs, wideType);
			break;
	}

	auto* product = irb.CreateMul(lhs, rhs);
	auto* high = irb.CreateTrunc(
			irb.CreateLShr(product, narrowBits),
			getDefaultType());
	storeOp(ci->operands[0], high, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateMulW(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(_insn, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* i32 = irb.getInt32Ty();
	auto* result = irb.CreateMul(
			irb.CreateTrunc(std::get<1>(values), i32),
			irb.CreateTrunc(std::get<2>(values), i32));
	storeOp(ci->operands[0], irb.CreateSExt(result, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateDiv(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(_insn, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* lhs = std::get<1>(values);
	auto* rhs = std::get<2>(values);
	llvm::Value* result = nullptr;
	switch (_insn->id)
	{
		case RISCV_INS_DIV:
			result = irb.CreateSDiv(lhs, rhs);
			break;
		case RISCV_INS_DIVU:
			result = irb.CreateUDiv(lhs, rhs);
			break;
		default:
			throw GenericError("Unexpected RISC-V division instruction.");
	}
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateRem(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(_insn, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* lhs = std::get<1>(values);
	auto* rhs = std::get<2>(values);
	llvm::Value* result = nullptr;
	switch (_insn->id)
	{
		case RISCV_INS_REM:
			result = irb.CreateSRem(lhs, rhs);
			break;
		case RISCV_INS_REMU:
			result = irb.CreateURem(lhs, rhs);
			break;
		default:
			throw GenericError("Unexpected RISC-V remainder instruction.");
	}
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateDivW(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(_insn, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* i32 = irb.getInt32Ty();
	auto* lhs = irb.CreateTrunc(std::get<1>(values), i32);
	auto* rhs = irb.CreateTrunc(std::get<2>(values), i32);
	auto* result = _insn->id == RISCV_INS_DIVW
			? irb.CreateSDiv(lhs, rhs)
			: irb.CreateUDiv(lhs, rhs);
	storeOp(ci->operands[0], irb.CreateSExt(result, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateRemW(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(_insn, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* i32 = irb.getInt32Ty();
	auto* lhs = irb.CreateTrunc(std::get<1>(values), i32);
	auto* rhs = irb.CreateTrunc(std::get<2>(values), i32);
	auto* result = _insn->id == RISCV_INS_REMW
			? irb.CreateSRem(lhs, rhs)
			: irb.CreateURem(lhs, rhs);
	storeOp(ci->operands[0], irb.CreateSExt(result, getDefaultType()), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateLui(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(_insn, ci, irb);
	auto* value = loadOp(ci->operands[1], irb);
	unsigned bits = _insn->id == RISCV_INS_C_LUI ? 6 : 20;
	value = getSignExtendedImmediate(value, bits, irb);
	value = irb.CreateShl(
			value,
			getSignedInt(getDefaultType(), 12));
	storeOp(ci->operands[0], value, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateAuipc(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(_insn, ci, irb);
	auto* imm = loadOp(ci->operands[1], irb);
	imm = getSignExtendedImmediate(imm, 20, irb);
	auto* shifted = irb.CreateShl(
			imm,
			getSignedInt(getDefaultType(), 12));
	auto* value = irb.CreateAdd(
			getThisInsnAddress(_insn),
			shifted);
	storeOp(ci->operands[0], value, irb);
}

//
//==============================================================================
// Control flow instructions.
//==============================================================================
//

void Capstone2LlvmIrTranslatorRiscV_impl::translateJal(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	if (ci->op_count < 1 || ci->operands[ci->op_count - 1].type != RISCV_OP_IMM)
	{
		throwUnexpectedOperands(_insn);
		translatePseudoAsmGeneric(_insn, ci, irb);
		return;
	}

	const auto rd = getJalRd(_insn);
	if (rd != RISCV_REG_X0)
	{
		storeRegister(rd, getNextInsnAddress(_insn), irb);
	}

	auto* target = getBranchTarget(
			_insn,
			ci->operands[ci->op_count - 1].imm);
	if (rd == RISCV_REG_X0)
	{
		generateBranchFunctionCall(irb, target);
	}
	else
	{
		generateCallFunctionCall(irb, target);
	}
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateJalr(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	if (ci->op_count < 1)
	{
		if (ci->op_count == 0 && _insn->id == RISCV_INS_JALR)
		{
			auto* target = loadRegister(RISCV_REG_RA, irb);
			generateReturnFunctionCall(irb, target);
			return;
		}
		throwUnexpectedOperands(_insn);
		translatePseudoAsmGeneric(_insn, ci, irb);
		return;
	}

	const auto rd = getJalrRd(_insn);
	const auto rs1 = getJalrRs1(_insn);
	auto* base = loadRegister(rs1, irb);
	int64_t imm = 0;
	if (ci->op_count >= 3 && ci->operands[2].type == RISCV_OP_IMM)
	{
		imm = ci->operands[2].imm;
	}
	auto* target = irb.CreateAdd(
			base,
			getSignedInt(getDefaultType(), imm));

	if (rd != RISCV_REG_X0)
	{
		storeRegister(rd, getNextInsnAddress(_insn), irb);
		generateCallFunctionCall(irb, target);
	}
	else if (rs1 == RISCV_REG_RA)
	{
		generateReturnFunctionCall(irb, target);
	}
	else
	{
		generateBranchFunctionCall(irb, target);
	}
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateCondBranch(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY_OR_TERNARY(_insn, ci, irb);

	llvm::Value* lhs = nullptr;
	llvm::Value* rhs = nullptr;
	int64_t offset = 0;
	if (ci->op_count == 2)
	{
		lhs = loadOp(ci->operands[0], irb);
		rhs = getSignedInt(getDefaultType(), 0);
		offset = ci->operands[1].imm;
	}
	else
	{
		lhs = loadOp(ci->operands[0], irb);
		rhs = loadOp(ci->operands[1], irb);
		offset = ci->operands[2].imm;
	}

	llvm::Value* cond = nullptr;
	switch (_insn->id)
	{
		case RISCV_INS_BEQ:
		case RISCV_INS_C_BEQZ:
			cond = irb.CreateICmpEQ(lhs, rhs);
			break;
		case RISCV_INS_BNE:
		case RISCV_INS_C_BNEZ:
			cond = irb.CreateICmpNE(lhs, rhs);
			break;
		case RISCV_INS_BLT:
			cond = irb.CreateICmpSLT(lhs, rhs);
			break;
		case RISCV_INS_BGE:
			cond = irb.CreateICmpSGE(lhs, rhs);
			break;
		case RISCV_INS_BLTU:
			cond = irb.CreateICmpULT(lhs, rhs);
			break;
		case RISCV_INS_BGEU:
			cond = irb.CreateICmpUGE(lhs, rhs);
			break;
		default:
			throw GenericError("Unexpected RISC-V conditional branch.");
	}

	generateCondBranchFunctionCall(
			irb,
			cond,
			getBranchTarget(_insn, offset));
}

//
//==============================================================================
// Memory instructions.
//==============================================================================
//

void Capstone2LlvmIrTranslatorRiscV_impl::translateLoadMemory(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(_insn, ci, irb);

	llvm::Type* loadType = nullptr;
	eOpConv conversion = eOpConv::SEXT_TRUNC_OR_BITCAST;
	switch (_insn->id)
	{
		case RISCV_INS_LB:
			loadType = irb.getInt8Ty();
			break;
		case RISCV_INS_LBU:
			loadType = irb.getInt8Ty();
			conversion = eOpConv::ZEXT_TRUNC_OR_BITCAST;
			break;
		case RISCV_INS_LH:
			loadType = irb.getInt16Ty();
			break;
		case RISCV_INS_LHU:
			loadType = irb.getInt16Ty();
			conversion = eOpConv::ZEXT_TRUNC_OR_BITCAST;
			break;
		case RISCV_INS_LW:
			loadType = irb.getInt32Ty();
			break;
		case RISCV_INS_LWU:
			loadType = irb.getInt32Ty();
			conversion = eOpConv::ZEXT_TRUNC_OR_BITCAST;
			break;
		case RISCV_INS_LD:
			loadType = irb.getInt64Ty();
			break;
		case RISCV_INS_FLW:
			loadType = irb.getFloatTy();
			conversion = eOpConv::FPCAST_OR_BITCAST;
			break;
		case RISCV_INS_FLD:
			loadType = irb.getDoubleTy();
			conversion = eOpConv::FPCAST_OR_BITCAST;
			break;
		default:
			throw GenericError("Unexpected RISC-V load instruction.");
	}

	auto* value = loadOp(ci->operands[1], irb, loadType);
	storeOp(ci->operands[0], value, irb, conversion);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateStoreMemory(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(_insn, ci, irb);

	llvm::Type* storeType = nullptr;
	switch (_insn->id)
	{
		case RISCV_INS_SB:
			storeType = irb.getInt8Ty();
			break;
		case RISCV_INS_SH:
			storeType = irb.getInt16Ty();
			break;
		case RISCV_INS_SW:
			storeType = irb.getInt32Ty();
			break;
		case RISCV_INS_SD:
			storeType = irb.getInt64Ty();
			break;
		case RISCV_INS_FSW:
			storeType = irb.getFloatTy();
			break;
		case RISCV_INS_FSD:
			storeType = irb.getDoubleTy();
			break;
		default:
			throw GenericError("Unexpected RISC-V store instruction.");
	}

	auto* value = loadOp(ci->operands[0], irb);
	if (storeType->isFloatingPointTy())
	{
		value = irb.CreateFPCast(value, storeType);
	}
	else
	{
		value = irb.CreateZExtOrTrunc(value, storeType);
	}
	storeOp(ci->operands[1], value, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedLoad(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	auto* addrValue = loadCompressedMemoryAddress(ci, irb);

	llvm::Type* loadType = nullptr;
	eOpConv conversion = eOpConv::SEXT_TRUNC_OR_BITCAST;
	switch (_insn->id)
	{
		case RISCV_INS_C_LW:
		case RISCV_INS_C_LWSP:
			loadType = irb.getInt32Ty();
			break;
		case RISCV_INS_C_LD:
		case RISCV_INS_C_LDSP:
			loadType = irb.getInt64Ty();
			break;
		case RISCV_INS_C_FLW:
		case RISCV_INS_C_FLWSP:
			loadType = irb.getFloatTy();
			conversion = eOpConv::FPCAST_OR_BITCAST;
			break;
		case RISCV_INS_C_FLD:
		case RISCV_INS_C_FLDSP:
			loadType = irb.getDoubleTy();
			conversion = eOpConv::FPCAST_OR_BITCAST;
			break;
		default:
			throw GenericError("Unexpected compressed RISC-V load.");
	}

	auto* ptr = irb.CreateIntToPtr(
			addrValue,
			llvm::PointerType::get(loadType, 0));
	auto* value = irb.CreateLoad(ptr);
	storeOp(ci->operands[0], value, irb, conversion);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateCompressedStore(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	auto* addrValue = loadCompressedMemoryAddress(ci, irb);

	llvm::Type* storeType = nullptr;
	switch (_insn->id)
	{
		case RISCV_INS_C_SW:
		case RISCV_INS_C_SWSP:
			storeType = irb.getInt32Ty();
			break;
		case RISCV_INS_C_SD:
		case RISCV_INS_C_SDSP:
			storeType = irb.getInt64Ty();
			break;
		case RISCV_INS_C_FSW:
		case RISCV_INS_C_FSWSP:
			storeType = irb.getFloatTy();
			break;
		case RISCV_INS_C_FSD:
		case RISCV_INS_C_FSDSP:
			storeType = irb.getDoubleTy();
			break;
		default:
			throw GenericError("Unexpected compressed RISC-V store.");
	}

	auto* value = loadOp(ci->operands[0], irb);
	if (storeType->isFloatingPointTy())
	{
		value = irb.CreateFPCast(value, storeType);
	}
	else
	{
		value = irb.CreateZExtOrTrunc(value, storeType);
	}
	auto* ptr = irb.CreateIntToPtr(
			addrValue,
			llvm::PointerType::get(storeType, 0));
	irb.CreateStore(value, ptr);
}

//
//==============================================================================
// Compressed instruction helpers.
//==============================================================================
//

void Capstone2LlvmIrTranslatorRiscV_impl::translateMove(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(_insn, ci, irb);
	storeOp(ci->operands[0], loadOp(ci->operands[1], irb), irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateLi(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(_insn, ci, irb);
	auto* value = getSignExtendedImmediate(
			loadOp(ci->operands[1], irb),
			6,
			irb);
	storeOp(ci->operands[0], value, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateAddi16sp(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(_insn, ci, irb);
	auto* sp = loadRegister(RISCV_REG_SP, irb);
	auto* result = irb.CreateAdd(sp, loadOp(ci->operands[1], irb));
	storeRegister(RISCV_REG_SP, result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateAddi4spn(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(_insn, ci, irb);
	auto* sp = loadOp(ci->operands[1], irb);
	auto* result = irb.CreateAdd(sp, loadOp(ci->operands[2], irb));
	storeOp(ci->operands[0], result, irb);
}

//
//==============================================================================
// System, CSR, atomic, and floating-point instructions.
//==============================================================================
//

void Capstone2LlvmIrTranslatorRiscV_impl::translateCsr(
		cs_insn*,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	if (ci->op_count < 2)
	{
		throwUnexpectedOperands(_insn);
		translatePseudoAsmGeneric(_insn, ci, irb);
		return;
	}

	auto* csr = getSignedInt(getDefaultType(), getCsrNumber(_insn));
	auto* source = loadOp(ci->operands[1], irb);
	source = irb.CreateZExtOrTrunc(source, getDefaultType());

	auto* fnc = getPseudoAsmFunction(
			_insn,
			getDefaultType(),
			llvm::ArrayRef<llvm::Type*>{
					getDefaultType(),
					getDefaultType()},
			"__asm_" + std::string(_insn->mnemonic));
	auto* call = irb.CreateCall(fnc, llvm::ArrayRef<llvm::Value*>{
			csr,
			source});
	storeOp(ci->operands[0], call, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateSystem(
		cs_insn* i,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	switch (i->id)
	{
		case RISCV_INS_FENCE:
		case RISCV_INS_FENCE_I:
		case RISCV_INS_FENCE_TSO:
		case RISCV_INS_C_NOP:
		case RISCV_INS_C_UNIMP:
		case RISCV_INS_UNIMP:
			return;
		default:
			translatePseudoAsmGeneric(i, ci, irb);
			return;
	}
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateAmo(
		cs_insn* i,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	if (ci->op_count < 2 || ci->operands[ci->op_count - 1].type != RISCV_OP_REG)
	{
		throwUnexpectedOperands(i);
		translatePseudoAsmGeneric(i, ci, irb);
		return;
	}

	std::string mnemonic = i->mnemonic;
	auto* addressValue = loadOp(ci->operands[ci->op_count - 1], irb);
	const bool isDouble = mnemonic.find(".d") != std::string::npos;
	auto* valueType = isDouble ? irb.getInt64Ty() : irb.getInt32Ty();
	auto* ptr = irb.CreateIntToPtr(
			addressValue,
			llvm::PointerType::get(valueType, 0));

	if (mnemonic.rfind("lr.", 0) == 0)
	{
		auto* loaded = irb.CreateLoad(ptr);
		storeOp(ci->operands[0], loaded, irb);
		return;
	}

	auto* storedValue = loadOp(ci->operands[1], irb);
	storedValue = irb.CreateZExtOrTrunc(storedValue, valueType);
	if (mnemonic.rfind("sc.", 0) == 0)
	{
		irb.CreateStore(storedValue, ptr);
		storeOp(
				ci->operands[0],
				getSignedInt(getDefaultType(), 0),
				irb);
		return;
	}

	auto* old = irb.CreateLoad(ptr);
	llvm::Value* updated = nullptr;
	if (mnemonic.find("amoswap") != std::string::npos)
	{
		updated = storedValue;
	}
	else if (mnemonic.find("amoadd") != std::string::npos)
	{
		updated = irb.CreateAdd(old, storedValue);
	}
	else if (mnemonic.find("amoand") != std::string::npos)
	{
		updated = irb.CreateAnd(old, storedValue);
	}
	else if (mnemonic.find("amoor") != std::string::npos)
	{
		updated = irb.CreateOr(old, storedValue);
	}
	else if (mnemonic.find("amoxor") != std::string::npos)
	{
		updated = irb.CreateXor(old, storedValue);
	}
	else if (mnemonic.find("amomaxu") != std::string::npos)
	{
		updated = irb.CreateSelect(
				irb.CreateICmpUGT(old, storedValue),
				old,
				storedValue);
	}
	else if (mnemonic.find("amominu") != std::string::npos)
	{
		updated = irb.CreateSelect(
				irb.CreateICmpULT(old, storedValue),
				old,
				storedValue);
	}
	else if (mnemonic.find("amomax") != std::string::npos)
	{
		updated = irb.CreateSelect(
				irb.CreateICmpSGT(old, storedValue),
				old,
				storedValue);
	}
	else if (mnemonic.find("amomin") != std::string::npos)
	{
		updated = irb.CreateSelect(
				irb.CreateICmpSLT(old, storedValue),
				old,
				storedValue);
	}
	else
	{
		translatePseudoAsmGeneric(i, ci, irb);
		return;
	}

	irb.CreateStore(updated, ptr);
	storeOp(ci->operands[0], old, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateFpBinary(
		cs_insn* i,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(i, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* lhs = std::get<1>(values);
	auto* rhs = std::get<2>(values);

	llvm::Value* result = nullptr;
	switch (i->id)
	{
		case RISCV_INS_FADD_S:
		case RISCV_INS_FADD_D:
			result = irb.CreateFAdd(lhs, rhs);
			break;
		case RISCV_INS_FSUB_S:
		case RISCV_INS_FSUB_D:
			result = irb.CreateFSub(lhs, rhs);
			break;
		case RISCV_INS_FMUL_S:
		case RISCV_INS_FMUL_D:
			result = irb.CreateFMul(lhs, rhs);
			break;
		case RISCV_INS_FDIV_S:
		case RISCV_INS_FDIV_D:
			result = irb.CreateFDiv(lhs, rhs);
			break;
		case RISCV_INS_FMIN_S:
		case RISCV_INS_FMIN_D:
			result = irb.CreateSelect(
					irb.CreateFCmpOLT(lhs, rhs),
					lhs,
					rhs);
			break;
		case RISCV_INS_FMAX_S:
		case RISCV_INS_FMAX_D:
			result = irb.CreateSelect(
					irb.CreateFCmpOGT(lhs, rhs),
					lhs,
					rhs);
			break;
		default:
			throw GenericError("Unexpected RISC-V FP binary instruction.");
	}

	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateFpUnary(
		cs_insn* i,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(i, ci, irb);
	auto* value = loadOp(ci->operands[1], irb);
	llvm::Value* result = nullptr;
	switch (i->id)
	{
		case RISCV_INS_FSQRT_S:
		case RISCV_INS_FSQRT_D:
		{
			auto* fnc = llvm::Intrinsic::getDeclaration(
					_module,
					llvm::Intrinsic::sqrt,
					value->getType());
			result = irb.CreateCall(fnc, {value});
			break;
		}
		default:
			throw GenericError("Unexpected RISC-V FP unary instruction.");
	}
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateFpCompare(
		cs_insn* i,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(i, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* lhs = std::get<1>(values);
	auto* rhs = std::get<2>(values);

	llvm::Value* result = nullptr;
	switch (i->id)
	{
		case RISCV_INS_FEQ_S:
		case RISCV_INS_FEQ_D:
			result = irb.CreateFCmpOEQ(lhs, rhs);
			break;
		case RISCV_INS_FLT_S:
		case RISCV_INS_FLT_D:
			result = irb.CreateFCmpOLT(lhs, rhs);
			break;
		case RISCV_INS_FLE_S:
		case RISCV_INS_FLE_D:
			result = irb.CreateFCmpOLE(lhs, rhs);
			break;
		default:
			throw GenericError("Unexpected RISC-V FP compare instruction.");
	}
	storeOp(
			ci->operands[0],
			irb.CreateZExt(result, getDefaultType()),
			irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateFpConvert(
		cs_insn* i,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(i, ci, irb);
	auto* source = loadOp(ci->operands[1], irb);
	llvm::Type* destType = getRegisterType(ci->operands[0].reg);
	llvm::Value* result = nullptr;

	switch (i->id)
	{
		case RISCV_INS_FCVT_W_S:
		case RISCV_INS_FCVT_W_D:
			result = irb.CreateFPToSI(source, irb.getInt32Ty());
			result = irb.CreateSExt(result, getDefaultType());
			break;
		case RISCV_INS_FCVT_WU_S:
		case RISCV_INS_FCVT_WU_D:
			result = irb.CreateFPToUI(source, irb.getInt32Ty());
			result = irb.CreateZExt(result, getDefaultType());
			break;
		case RISCV_INS_FCVT_L_S:
		case RISCV_INS_FCVT_L_D:
			result = irb.CreateFPToSI(source, irb.getInt64Ty());
			result = irb.CreateSExtOrTrunc(result, getDefaultType());
			break;
		case RISCV_INS_FCVT_LU_S:
		case RISCV_INS_FCVT_LU_D:
			result = irb.CreateFPToUI(source, irb.getInt64Ty());
			result = irb.CreateZExtOrTrunc(result, getDefaultType());
			break;
		case RISCV_INS_FCVT_S_W:
		case RISCV_INS_FCVT_D_W:
			result = irb.CreateSIToFP(
					irb.CreateSExtOrTrunc(source, irb.getInt32Ty()),
					destType);
			break;
		case RISCV_INS_FCVT_S_WU:
		case RISCV_INS_FCVT_D_WU:
			result = irb.CreateUIToFP(
					irb.CreateZExtOrTrunc(source, irb.getInt32Ty()),
					destType);
			break;
		case RISCV_INS_FCVT_S_L:
		case RISCV_INS_FCVT_D_L:
			result = irb.CreateSIToFP(
					irb.CreateSExtOrTrunc(source, irb.getInt64Ty()),
					destType);
			break;
		case RISCV_INS_FCVT_S_LU:
		case RISCV_INS_FCVT_D_LU:
			result = irb.CreateUIToFP(
					irb.CreateZExtOrTrunc(source, irb.getInt64Ty()),
					destType);
			break;
		case RISCV_INS_FCVT_S_D:
		case RISCV_INS_FCVT_D_S:
			result = irb.CreateFPCast(source, destType);
			break;
		default:
			throw GenericError("Unexpected RISC-V FP conversion.");
	}

	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateFpMove(
		cs_insn* i,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_BINARY(i, ci, irb);
	auto* source = loadOp(ci->operands[1], irb);
	llvm::Type* destType = getRegisterType(ci->operands[0].reg);
	auto* bits = destType->isFloatingPointTy()
			? irb.getIntNTy(destType->getPrimitiveSizeInBits())
			: destType;
	auto* result = irb.CreateBitCast(source, bits);
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateFpFma(
		cs_insn* i,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_QUATERNARY(i, ci, irb);
	auto* a = loadOp(ci->operands[1], irb);
	auto* b = loadOp(ci->operands[2], irb);
	auto* c = loadOp(ci->operands[3], irb);
	auto* product = irb.CreateFMul(a, b);
	llvm::Value* result = nullptr;
	switch (i->id)
	{
		case RISCV_INS_FMADD_S:
		case RISCV_INS_FMADD_D:
			result = irb.CreateFAdd(product, c);
			break;
		case RISCV_INS_FMSUB_S:
		case RISCV_INS_FMSUB_D:
			result = irb.CreateFSub(product, c);
			break;
		case RISCV_INS_FNMSUB_S:
		case RISCV_INS_FNMSUB_D:
			result = irb.CreateFSub(
					llvm::ConstantFP::get(product->getType(), 0.0),
					irb.CreateFAdd(product, c));
			break;
		case RISCV_INS_FNMADD_S:
		case RISCV_INS_FNMADD_D:
			result = irb.CreateFSub(
					llvm::ConstantFP::get(product->getType(), 0.0),
					irb.CreateFSub(product, c));
			break;
		default:
			throw GenericError("Unexpected RISC-V FMA instruction.");
	}
	storeOp(ci->operands[0], result, irb);
}

void Capstone2LlvmIrTranslatorRiscV_impl::translateFpSignInject(
		cs_insn* i,
		cs_riscv* ci,
		llvm::IRBuilder<>& irb)
{
	EXPECT_IS_TERNARY(i, ci, irb);
	auto values = loadOpTernary(ci, irb);
	auto* lhs = std::get<1>(values);
	auto* rhs = std::get<2>(values);
	auto* bitsType = irb.getIntNTy(lhs->getType()->getPrimitiveSizeInBits());
	auto* lhsBits = irb.CreateBitCast(lhs, bitsType);
	auto* rhsBits = irb.CreateBitCast(rhs, bitsType);
	llvm::Value* signMask = getSignedInt(
			irb.getIntNTy(lhs->getType()->getPrimitiveSizeInBits()),
			1ULL << (bitsType->getIntegerBitWidth() - 1));
	signMask = irb.CreateZExtOrTrunc(signMask, bitsType);

	llvm::Value* result = nullptr;
	switch (i->id)
	{
		case RISCV_INS_FSGNJ_S:
		case RISCV_INS_FSGNJ_D:
			result = irb.CreateOr(
					irb.CreateAnd(lhsBits, irb.CreateNot(signMask)),
					irb.CreateAnd(rhsBits, signMask));
			break;
		case RISCV_INS_FSGNJN_S:
		case RISCV_INS_FSGNJN_D:
			result = irb.CreateOr(
					irb.CreateAnd(lhsBits, irb.CreateNot(signMask)),
					irb.CreateAnd(
							irb.CreateNot(rhsBits),
							signMask));
			break;
		case RISCV_INS_FSGNJX_S:
		case RISCV_INS_FSGNJX_D:
			result = irb.CreateXor(
					lhsBits,
					irb.CreateAnd(rhsBits, signMask));
			break;
		default:
			throw GenericError("Unexpected RISC-V sign injection instruction.");
	}

	storeOp(ci->operands[0], irb.CreateBitCast(result, lhs->getType()), irb);
}

} // namespace capstone2llvmir
} // namespace retdec
