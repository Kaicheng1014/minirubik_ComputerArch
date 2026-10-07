# ==========================================
# 工具鏈與參數設定
# ==========================================
# Host 端 (你的 PC)
HOST_CC = gcc
HOST_CFLAGS = -O3 -Wall

# Target 端 (RISC-V)
RV_CC = riscv64-unknown-elf-gcc
RV_ARCH = -march=rv32i -mabi=ilp32
RV_CFLAGS = $(RV_ARCH) -O3 -static
EMU = qemu-riscv32
GDB = gdb-multiarch

# 來源檔案設定 (日後若要改成 solver.s，只需改這一行即可)
SOLVER_SRC = solverV2.c

# 預設執行的指令 (當你只打 make 時)
.PHONY: help table test host-run rv-build rv-run asm debug-server debug-client clean

help:
	@echo "=== 2x2 Rubik's Cube Solver Makefile ==="
	@echo "可用指令清單："
	@echo "  make table       - [Host] 編譯並執行 build_pdb.c，產生 pdb.h"
	@echo "  make test        - [Host] 編譯並執行 gate_test.c 進行全領域 H1~H4 測試"
	@echo "  make host-run    - [Host] 用 PC 直接編譯 $(SOLVER_SRC) 並執行 (快速驗證邏輯)"
	@echo "  make rv-build    - [RV32I] 用 RISC-V 工具鏈將 $(SOLVER_SRC) 編譯成 solver.elf"
	@echo "  make rv-run      - [RV32I] 透過 QEMU 模擬器執行 solver.elf"
	@echo "  make asm         - [RV32I] 將 $(SOLVER_SRC) 轉成 RISC-V 組合語言 (Stage 4 參考用)"
	@echo "  make clean       - 清除所有編譯出來的執行檔與暫存檔"
	@echo "========================================"

# ==========================================
# 1. 建表程式 (Table Generator)
# ==========================================
pdb.h: build_pdb.c
	@echo "\n[Host] Building and running PDB generator..."
	$(HOST_CC) $(HOST_CFLAGS) -o build_pdb build_pdb.c
	./build_pdb

table: pdb.h

# ==========================================
# 2. 測試程式 (Gate Test)
# ==========================================
test: gate_test.c pdb.h
	@echo "\n[Host] Building and running Gate Tests..."
	$(HOST_CC) $(HOST_CFLAGS) -o gate_test gate_test.c
	./gate_test

# ==========================================
# 3. Solver (Host 端快速驗證)
# ==========================================
host-run: $(SOLVER_SRC) pdb.h
	@echo "\n[Host] Building and running Solver locally..."
	$(HOST_CC) $(HOST_CFLAGS) -o host_solver $(SOLVER_SRC)
	./host_solver 21345671111111

# ==========================================
# 4. Solver (RISC-V Target 端與執行)
# ==========================================
solver.elf: $(SOLVER_SRC) pdb.h
	@echo "\n[RV32I] Cross-compiling Solver for RISC-V..."
	$(RV_CC) $(RV_CFLAGS) -o solver.elf $(SOLVER_SRC)

rv-build: solver.elf

rv-run: solver.elf
	@echo "\n[QEMU] Running RISC-V Solver..."
	$(EMU) solver.elf 21345671111111

# ==========================================
# 5. Stage 4 輔助工具 (組合語言轉換與 Debug)
# ==========================================
asm: $(SOLVER_SRC) pdb.h
	@echo "\n[RV32I] Generating assembly code (solver_compiler.s)..."
	$(RV_CC) $(RV_ARCH) -O3 -S $(SOLVER_SRC) -o solver_compiler.s

debug-server: solver.elf
	@echo "\n啟動 QEMU GDB Server (等待連線中)..."
	@echo "請開啟另一個終端機並輸入: make debug-client"
	$(EMU) -g 1234 solver.elf 21345671111111

debug-client:
	$(GDB) solver.elf -ex "target remote localhost:1234"

# ==========================================
# 6. 清除工具 (Clean)
# ==========================================
clean:
	@echo "\nCleaning up generated binaries..."
	rm -f build_pdb gate_test host_solver solver.elf solver_compiler.s