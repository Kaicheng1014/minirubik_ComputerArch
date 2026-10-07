#include <stdio.h>
#include <stdint.h>
// 載入我們在 PC 端算好並輸出的 109.5 KiB 大表
#include "pdb.h"

// 9 種基本轉法名稱，對應編號 0~8
const char* MOVE_NAMES[] = {"R", "R2", "R'", "B", "B2", "B'", "D", "D2", "D'"};

// ==========================================
// 1. 非遞迴 IDA* 的靜態堆疊結構 (Static Stack)
// ==========================================
// 用來取代 function call stack，手動紀錄每次深搜的狀態
typedef struct {
    uint16_t perm;      // 當前的位置編號 (0~5039)
    uint16_t ori;       // 當前的方向編號 (0~728)
    uint8_t g;          // 已經走過的步數 (Depth / g-value)
    int8_t last_face;   // 上一步轉動的面 (0:R, 1:B, 2:D, -1:起始無)，用於同面剪枝
    int8_t next_move;   // 狀態機：記住這個節點下一個要嘗試的轉法 (0~8)
} StackFrame;

// 魔術方塊 (限定3面) 最大直徑為 11，宣告 12 絕對安全且極度省記憶體
StackFrame stack[12];
uint8_t solution[12]; // 用來記錄最終找到的解法路徑

int main() {
    printf("Solver skeleton compiled. Ready for IDA*.\n");
    return 0;
}