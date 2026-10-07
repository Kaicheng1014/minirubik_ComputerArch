#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pdb.h"

const char* MOVE_NAMES[] = {"R", "R2", "R'", "B", "B2", "B'", "D", "D2", "D'"};
const int FACTORIAL[] = {1, 1, 2, 6, 24, 120, 720, 5040};

// RV32I 優化 1：消除 m / 3 的除法運算，改為 9 bytes 查表
const int8_t MOVE_TO_FACE[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};

uint16_t perm_to_index(const uint8_t *perm) {
    int index = 0;
    for (int i = 0; i < 7; i++) {
        int count = 0;
        for (int j = i + 1; j < 7; j++) {
            if (perm[j] < perm[i]) count++;
        }
        index += count * FACTORIAL[6 - i];
    }
    return index;
}

uint16_t ori_to_index(const uint8_t *ori) {
    int index = 0;
    for (int i = 0; i < 6; i++) {
        // 這裡的乘法只在初始化跑一次，不影響搜尋迴圈效能
        index = (index << 1) + index + ori[i]; // index * 3 + ori[i]
    }
    return index;
}

int parse_state(const char *input, uint16_t *start_p, uint16_t *start_o) {
    if (strlen(input) != 14) return 0;
    
    uint8_t perm[7], ori[7];
    int ori_sum = 0;
    
    for (int i = 0; i < 14; ++i) {
        int limit = (i < 7) ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit) return 0;
        
        if (i < 7) {
            perm[i] = input[i] - '1';
            for (int j = 0; j < i; j++) {
                if (perm[i] == perm[j]) return 0;
            }
        } else {
            ori[i - 7] = input[i] - '1';
            ori_sum += ori[i - 7];
        }
    }
    
    if (ori_sum % 3 != 0) return 0;
    
    *start_p = perm_to_index(perm);
    *start_o = ori_to_index(ori);
    return 1;
}

// ==========================================
// 2. IDA* 靜態堆疊與 RV32I 優化 Bounded DFS
// ==========================================
typedef struct {
    uint16_t perm;
    uint16_t ori;
    uint8_t g;
    int8_t last_face;
    int8_t next_move;
} StackFrame;

StackFrame stack[12];
uint8_t solution[12];

int bounded_dfs(uint16_t start_perm, uint16_t start_ori, int bound, int *next_bound) {
    int sp = 0;
    stack[sp].perm = start_perm;
    stack[sp].ori = start_ori;
    stack[sp].g = 0;
    stack[sp].last_face = -1;
    stack[sp].next_move = 0;
    
    int min_out_of_bound = 100;
    
    // 強制將 2D 陣列轉為 1D 指標，避免編譯器產生 mul 指令
    const uint16_t *p_trans_1d = (const uint16_t *)perm_trans;
    const uint16_t *o_trans_1d = (const uint16_t *)ori_trans;
    
    while (sp >= 0) {
        if (stack[sp].next_move >= 9) {
            sp--; 
            continue;
        }
        
        int m = stack[sp].next_move++;
        
        // 優化 1：查表取代除法 (face = m / 3)
        int face = MOVE_TO_FACE[m];
        
        if (face == stack[sp].last_face) continue;
        
        // RV32I 優化 2：Shift-Add 取代乘法
        // 計算 1D 偏移量: index * 9 + m  => (index << 3) + index + m
        int p_idx = (stack[sp].perm << 3) + stack[sp].perm + m;
        int o_idx = (stack[sp].ori << 3) + stack[sp].ori + m;
        
        uint16_t next_p = p_trans_1d[p_idx];
        uint16_t next_o = o_trans_1d[o_idx];
        uint8_t g = stack[sp].g + 1;
        
        if (next_p == 0 && next_o == 0) {
            solution[g - 1] = m;
            return g; 
        }
        
        uint8_t h_p = perm_pdb[next_p];
        uint8_t h_o = ori_pdb[next_o];
        
        // RV32I 優化 3：Branchless Max (無分支比大小)
        // 避免編譯器產生 jump 指令導致 pipeline flush
        int diff = h_o - h_p;
        int mask = diff >> 31; // 如果 h_p > h_o，diff 為負，mask 為 -1(全1)；否則為 0
        uint8_t h = h_o - (diff & mask); 
        
        int f = g + h;
        
        if (f > bound) {
            if (f < min_out_of_bound) min_out_of_bound = f;
            continue;
        }
        
        solution[g - 1] = m;
        if (sp < 11) {
            sp++;
            stack[sp].perm = next_p;
            stack[sp].ori = next_o;
            stack[sp].g = g;
            stack[sp].last_face = face;
            stack[sp].next_move = 0;
        }
    }
    
    *next_bound = min_out_of_bound;
    return -1;
}

int main(int argc, char **argv) {
    uint16_t start_p = 0, start_o = 0;
    
    if (argc != 2 || !parse_state(argv[1], &start_p, &start_o)) {
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n", argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }

    if (start_p == 0 && start_o == 0) {
        putchar('\n');
        return 0;
    }

    uint8_t h_p = perm_pdb[start_p];
    uint8_t h_o = ori_pdb[start_o];
    int diff = h_o - h_p;
    int bound = h_o - (diff & (diff >> 31)); // Branchless max
    int ans_length = -1;

    while (1) {
        int next_bound;
        ans_length = bounded_dfs(start_p, start_o, bound, &next_bound);
        
        if (ans_length != -1) break;
        
        if (next_bound > 11) {
            fprintf(stderr, "No solution found.\n");
            return 1;
        }
        bound = next_bound;
    }

    const char *separator = "";
    for (int i = 0; i < ans_length; i++) {
        printf("%s%s", separator, MOVE_NAMES[solution[i]]);
        separator = " ";
    }
    putchar('\n');
    
    if (fflush(stdout) != 0 || ferror(stdout)) return 1;

    return 0;
}