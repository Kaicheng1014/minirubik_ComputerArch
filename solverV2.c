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

// ==========================================
// 2. 核心搜尋：非遞迴 Bounded DFS
// ==========================================
// 執行一次限定 bound 的深度優先搜尋
// 找到解回傳總步數；沒找到則回傳 -1，並透過 next_bound 回傳下一個最小的 f 值
int bounded_dfs(uint16_t start_perm, uint16_t start_ori, int bound, int *next_bound) {
    int sp = 0; // Stack Pointer (堆疊指標)
    
    // 初始化根節點 (Root)
    stack[sp].perm = start_perm;
    stack[sp].ori = start_ori;
    stack[sp].g = 0;
    stack[sp].last_face = -1;
    stack[sp].next_move = 0;
    
    int min_out_of_bound = 100; // 記錄超過 bound 的最小 f 值 (充當無限大)
    
    while (sp >= 0) {
        // 如果當前節點的 9 種轉法都試過了，進行 Backtrack (退回上一層)
        if (stack[sp].next_move >= 9) {
            sp--; 
            continue;
        }
        
        int m = stack[sp].next_move++; // 取得要嘗試的轉法，並推進狀態機
        int face = m / 3;
        
        // 剪枝 1：同面連續轉動剪枝 (例如 R 接著 R2，完全多餘)
        if (face == stack[sp].last_face) {
            continue;
        }
        
        // O(1) 狀態轉移：直接查表得到新狀態編號！完全避開複雜數學運算
        uint16_t next_p = perm_trans[stack[sp].perm][m];
        uint16_t next_o = ori_trans[stack[sp].ori][m];
        uint8_t g = stack[sp].g + 1;
        
        // 檢查是否抵達解答 (目標狀態的排列與方向編號皆為 0)
        if (next_p == 0 && next_o == 0) {
            solution[g - 1] = m;
            return g; 
        }
        
        // 取得啟發值 h = max(PDB_perm, PDB_ori)
        uint8_t h_p = perm_pdb[next_p];
        uint8_t h_o = ori_pdb[next_o];
        uint8_t h = (h_p > h_o) ? h_p : h_o;
        
        int f = g + h;
        
        // 剪枝 2：IDA* 邊界檢查
        if (f > bound) {
            if (f < min_out_of_bound) {
                min_out_of_bound = f;
            }
            continue;
        }
        
        // 若 f <= bound，將下一個狀態 Push 進 Stack 繼續深搜
        solution[g - 1] = m; // 暫存可能路徑
        if (sp < 11) {       // 安全防護，避免超出直徑
            sp++;
            stack[sp].perm = next_p;
            stack[sp].ori = next_o;
            stack[sp].g = g;
            stack[sp].last_face = face;
            stack[sp].next_move = 0;
        }
    }
    
    *next_bound = min_out_of_bound;
    return -1; // 此次 bound 沒找到解
}



int main() {
    printf("Solver skeleton compiled. Ready for IDA*.\n");
    return 0;
}