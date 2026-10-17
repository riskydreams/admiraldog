// admiraldog test environment for riscv-tests (replaces the upstream "p" env).
//
// The upstream env boots in machine mode and reports through CSRs, mret and
// ecall, none of which exist in plain RV32I. This one needs no CSRs:
//   - the test starts at _start, at the address the linker script gives;
//   - pass/fail is a single SW to `tohost` (a memory-mapped word the runner
//     watches): 1 means pass, (testnum << 1) | 1 means test `testnum` failed;
//   - after reporting, the program spins on `j .`.
#ifndef ADMIRALDOG_RISCV_TEST_H
#define ADMIRALDOG_RISCV_TEST_H

#define TESTNUM gp

#define RVTEST_RV32U .option norvc;
#define RVTEST_RV64U RVTEST_RV32U

#define RVTEST_CODE_BEGIN \
  .section .text.init;    \
  .align 2;               \
  .globl _start;          \
_start:

#define RVTEST_CODE_END \
  unimp

#define RVTEST_PASS        \
  fence;                   \
  li TESTNUM, 1;           \
  la t5, tohost;           \
  sw TESTNUM, 0(t5);       \
1: j 1b;

#define RVTEST_FAIL              \
  fence;                         \
1: beqz TESTNUM, 1b;             \
  sll TESTNUM, TESTNUM, 1;       \
  or TESTNUM, TESTNUM, 1;        \
  la t5, tohost;                 \
  sw TESTNUM, 0(t5);             \
2: j 2b;

#define EXTRA_DATA

#define RVTEST_DATA_BEGIN \
  EXTRA_DATA              \
  .align 4;

#define RVTEST_DATA_END

#endif
