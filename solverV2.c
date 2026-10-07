#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pdb.h"

const char* MOVE_NAMES[] = {"R", "R2", "R'", "B", "B2", "B'", "D", "D2", "D'"};
const int FACTORIAL[] = {1, 1, 2, 6, 24, 120, 720, 5040};

// ==========================================
// 1. 初始化轉換工具 (將字串轉換為查表用的整數編號)
// ==========================================
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
        index = index * 3 + ori[i];
    }
    return index;
}

// 解析命令列的字串狀態，並防呆檢查
int parse_state(const char *input, uint16_t *start_p, uint16_t *start_o) {
    if (strlen(input) != 14) return 0;
    
    uint8_t perm[7], ori[7];
    int ori_sum = 0;
    
    // 解析字元 ('1'~'7' 及 '1'~'3')
    for (int i = 0; i < 14; ++i) {
        int limit = (i < 7) ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit) return 0;
        
        if (i < 7) {
            perm[i] = input[i] - '1';
            // 檢查重複的排列
            for (int j = 0; j < i; j++) {
                if (perm[i] == perm[j]) return 0;
            }
        } else {
            ori[i - 7] = input[i] - '1';
            ori_sum += ori[i - 7];
        }
    }
    
    // 魔術方塊物理定律：方向總和必須是 3 的倍數
    if (ori_sum % 3 != 0) return 0;
    
    *start_p = perm_to_index(perm);
    *start_o = ori_to_index(ori);
    return 1;
}

// ==========================================
// 2. IDA* 靜態堆疊與 Bounded DFS
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
    
    while (sp >= 0) {
        if (stack[sp].next_move >= 9) {
            sp--; 
            continue;
        }
        
        int m = stack[sp].next_move++;
        int face = m / 3;
        
        if (face == stack[sp].last_face) continue;
        
        // O(1) 查表狀態轉移
        uint16_t next_p = perm_trans[stack[sp].perm][m];
        uint16_t next_o = ori_trans[stack[sp].ori][m];
        uint8_t g = stack[sp].g + 1;
        
        if (next_p == 0 && next_o == 0) {
            solution[g - 1] = m;
            return g; 
        }
        
        uint8_t h_p = perm_pdb[next_p];
        uint8_t h_o = ori_pdb[next_o];
        uint8_t h = (h_p > h_o) ? h_p : h_o;
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

// ==========================================
// 3. 系統入口與輸出
// ==========================================
int main(int argc, char **argv) {
    uint16_t start_p = 0, start_o = 0;
    
    // 解析命令列參數傳入的初始狀態
    if (argc != 2 || !parse_state(argv[1], &start_p, &start_o)) {
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n", argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }

    // 若已經是復原狀態，直接印出換行
    if (start_p == 0 && start_o == 0) {
        putchar('\n');
        return 0;
    }

    uint8_t h_p = perm_pdb[start_p];
    uint8_t h_o = ori_pdb[start_o];
    int bound = (h_p > h_o) ? h_p : h_o;
    int ans_length = -1;

    // IDA* 迭代
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

    // 完全依照原格式輸出解法，以空白分隔
    const char *separator = "";
    for (int i = 0; i < ans_length; i++) {
        printf("%s%s", separator, MOVE_NAMES[solution[i]]);
        separator = " ";
    }
    putchar('\n');
    
    // 確保輸出不會卡在緩衝區
    if (fflush(stdout) != 0 || ferror(stdout)) return 1;

    return 0;
}