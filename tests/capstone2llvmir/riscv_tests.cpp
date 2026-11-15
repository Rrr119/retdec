/**
 * @file tests/capstone2llvmir/riscv_tests.cpp
 * @brief Capstone2LlvmIrTranslatorRiscV unit tests.
 */

#include <llvm/IR/InstIterator.h>

#include "capstone2llvmir/capstone2llvmir_tests.h"
#include "retdec/capstone2llvmir/riscv/riscv.h"

using namespace ::testing;

namespace retdec {
namespace capstone2llvmir {
namespace tests {

class Capstone2LlvmIrTranslatorRiscVTests :
		public Capstone2LlvmIrTranslatorTests,
		public ::testing::WithParamInterface<cs_mode>
{
	protected:
		virtual void initKeystoneEngine() override
		{
			// Keystone does not provide a RISC-V backend. The tests below
			// therefore use fixed instruction encodings and emulate_bin().
		}

		virtual void initCapstone2LlvmIrTranslator() override
		{
			switch (GetParam())
			{
				case CS_MODE_RISCV32:
					_translator =
							Capstone2LlvmIrTranslator::createRiscV32(&_module);
					break;
				case CS_MODE_RISCV64:
					_translator =
							Capstone2LlvmIrTranslator::createRiscV64(&_module);
					break;
				default:
					throw std::runtime_error("Unknown RISC-V mode.");
			}
		}

	protected:
		bool is64() const
		{
			return GetParam() == CS_MODE_RISCV64;
		}
};

struct PrintCapstoneModeToString_RiscV
{
	template <class ParamType>
	std::string operator()(const TestParamInfo<ParamType>& info) const
	{
		switch (info.param)
		{
			case CS_MODE_RISCV32: return "CS_MODE_RISCV32";
			case CS_MODE_RISCV64: return "CS_MODE_RISCV64";
			default: return "UNHANDLED CS_MODE";
		}
	}
};

INSTANTIATE_TEST_SUITE_P(
		InstantiateRiscVWithAllModes,
		Capstone2LlvmIrTranslatorRiscVTests,
		::testing::Values(CS_MODE_RISCV32, CS_MODE_RISCV64),
		PrintCapstoneModeToString_RiscV());

TEST(RiscVFactoryTests, acceptsCompressedBitInBasicMode)
{
	llvm::LLVMContext context;
	llvm::Module module("test", context);
	auto translator = Capstone2LlvmIrTranslator::createArch(
			CS_ARCH_RISCV,
			&module,
			static_cast<cs_mode>(CS_MODE_RISCV64 | CS_MODE_RISCVC));

	ASSERT_NE(translator, nullptr);
	EXPECT_EQ(translator->getBasicMode(), CS_MODE_RISCV64);
	EXPECT_TRUE(translator->getExtraMode() & CS_MODE_RISCVC);
	EXPECT_EQ(translator->getArchBitSize(), 64);
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, add)
{
	setRegisters({
		{RISCV_REG_A1, 0x1234},
		{RISCV_REG_A2, 0x20},
	});

	// add a0, a1, a2
	emulate_bin("33 85 c5 00", 0x1000);

	EXPECT_JUST_REGISTERS_LOADED({
		RISCV_REG_A1,
		RISCV_REG_A2,
	});
	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 0x1254},
	});
	EXPECT_NO_MEMORY_LOADED_STORED();
	EXPECT_NO_VALUE_CALLED();
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, addiNegativeImmediate)
{
	setRegisters({
		{RISCV_REG_A1, 0x1234},
	});

	// addi a0, a1, -4
	emulate_bin("13 85 c5 ff", 0x1000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 0x1230},
	});
	EXPECT_NO_MEMORY_LOADED_STORED();
	EXPECT_NO_VALUE_CALLED();
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, loadWord)
{
	setRegisters({
		{RISCV_REG_A1, 0x1000},
	});
	setMemory({
		{0x1004, StoredValue(0x12345678, 32)},
	});

	// lw a0, 4(a1)
	emulate_bin("03 a5 45 00", 0x2000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 0x12345678},
	});
	EXPECT_JUST_MEMORY_LOADED({0x1004});
	EXPECT_NO_MEMORY_STORED();
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, storeWord)
{
	setRegisters({
		{RISCV_REG_A0, 0xabcdef01},
		{RISCV_REG_A1, 0x1000},
	});

	// sw a0, 4(a1)
	emulate_bin("23 a2 a5 00", 0x2000);

	EXPECT_JUST_MEMORY_STORED({
		{0x1004, StoredValue(0xabcdef01, 32)},
	});
	EXPECT_NO_MEMORY_LOADED();
	EXPECT_NO_VALUE_CALLED();
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, jalStoresReturnAddressAndCalls)
{
	// jal ra, 8
	emulate_bin("ef 00 80 00", 0x1000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_RA, 0x1004},
	});
	EXPECT_JUST_VALUES_CALLED({
		{_translator->getCallFunction(), {0x1008}},
	});
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, retCallsReturnPseudoFunction)
{
	setRegisters({
		{RISCV_REG_RA, 0x12345678},
	});

	// ret == jalr x0, 0(ra)
	emulate_bin("67 80 00 00", 0x1000);

	EXPECT_JUST_VALUES_CALLED({
		{_translator->getReturnFunction(), {0x12345678}},
	});
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, compressedAddiAndInstructionBoundaries)
{
	setRegisters({
		{RISCV_REG_A0, 1},
	});

	// Two independent 2-byte instructions: c.addi a0, 1; c.addi a0, 1
	emulate_bin("05 05 05 05", 0x1000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 3},
	});
	EXPECT_NO_MEMORY_LOADED_STORED();
	EXPECT_NO_VALUE_CALLED();
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, compressedMove)
{
	setRegisters({
		{RISCV_REG_A1, 0x1234},
	});

	// c.mv a0, a1
	emulate_bin("2e 85", 0x1000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 0x1234},
	});
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, compressedLoadStoreWord)
{
	setRegisters({
		{RISCV_REG_A1, 0x2000},
	});
	setMemory({
		{0x2004, StoredValue(0x11223344, 32)},
	});

	// c.lw a0, 4(a1); c.sw a0, 4(a1)
	emulate_bin("c8 41 c8 c1", 0x1000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 0x11223344},
	});
	EXPECT_JUST_MEMORY_STORED({
		{0x2004, StoredValue(0x11223344, 32)},
	});
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, compressedConditionalBranch)
{
	setRegisters({
		{RISCV_REG_A0, 0},
	});

	// c.beqz a0, 2
	emulate_bin("09 c1", 0x1000);

	EXPECT_JUST_VALUES_CALLED({
		{_translator->getCondBranchFunction(), {true, 0x1002}},
	});
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, compressedJump)
{
	// c.j 2
	emulate_bin("09 a0", 0x1000);

	EXPECT_JUST_VALUES_CALLED({
		{_translator->getBranchFunction(), {0x1002}},
	});
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, rv64AddiwSignExtends)
{
	if (!is64())
	{
		SUCCEED();
		return;
	}

	setRegisters({
		{RISCV_REG_A1, 0x7fffffff},
	});

	// addiw a0, a1, 1
	emulate_bin("1b 85 15 00", 0x1000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 0xffffffff80000000ULL},
	});
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, rv64CompressedAddiwSignExtends)
{
	if (!is64())
	{
		SUCCEED();
		return;
	}

	setRegisters({
		{RISCV_REG_A0, 0x7fffffff},
	});

	// c.addiw a0, 1
	emulate_bin("05 25", 0x1000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 0xffffffff80000000ULL},
	});
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, rv64LoadStoreDoubleword)
{
	if (!is64())
	{
		SUCCEED();
		return;
	}

	setRegisters({
		{RISCV_REG_A1, 0x2000},
	});
	setMemory({
		{0x2008, StoredValue(0x1122334455667788ULL, 64)},
	});

	// ld a0, 8(a1); sd a0, 8(a1)
	emulate_bin("03 b5 85 00 23 b4 a5 00", 0x1000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 0x1122334455667788ULL},
	});
	EXPECT_JUST_MEMORY_STORED({
		{0x2008, StoredValue(0x1122334455667788ULL, 64)},
	});
}

TEST_P(Capstone2LlvmIrTranslatorRiscVTests, multiply)
{
	setRegisters({
		{RISCV_REG_A1, 7},
		{RISCV_REG_A2, 9},
	});

	// mul a0, a1, a2
	emulate_bin("33 85 c5 02", 0x1000);

	EXPECT_JUST_REGISTERS_STORED({
		{RISCV_REG_A0, 63},
	});
}

} // namespace tests
} // namespace capstone2llvmir
} // namespace retdec
