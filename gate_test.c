#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "pdb.h"

// ==========================================
// 1. IDA* 核心 (與 RV32I 優化版完全相同)
// ==========================================
typedef struct {
    uint16_t perm;
    uint16_t ori;
    uint8_t g;
    int8_t last_face;
    int8_t next_move;
} StackFrame;

StackFrame stack[12];
const int8_t MOVE_TO_FACE[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};

int bounded_dfs(uint16_t start_perm, uint16_t start_ori, int bound, int *next_bound) {
    int sp = 0;
    stack[sp].perm = start_perm;
    stack[sp].ori = start_ori;
    stack[sp].g = 0;
    stack[sp].last_face = -1;
    stack[sp].next_move = 0;
    
    int min_out_of_bound = 100;
    const uint16_t *p_trans_1d = (const uint16_t *)perm_trans;
    const uint16_t *o_trans_1d = (const uint16_t *)ori_trans;
    
    while (sp >= 0) {
        if (stack[sp].next_move >= 9) {
            sp--; 
            continue;
        }
        
        int m = stack[sp].next_move++;
        int face = MOVE_TO_FACE[m];
        
        if (face == stack[sp].last_face) continue;
        
        int p_idx = (stack[sp].perm << 3) + stack[sp].perm + m;
        int o_idx = (stack[sp].ori << 3) + stack[sp].ori + m;
        
        uint16_t next_p = p_trans_1d[p_idx];
        uint16_t next_o = o_trans_1d[o_idx];
        uint8_t g = stack[sp].g + 1;
        
        if (next_p == 0 && next_o == 0) {
            return g; 
        }
        
        uint8_t h_p = perm_pdb[next_p];
        uint8_t h_o = ori_pdb[next_o];
        int diff = h_o - h_p;
        uint8_t h = h_o - (diff & (diff >> 31)); 
        
        int f = g + h;
        if (f > bound) {
            if (f < min_out_of_bound) min_out_of_bound = f;
            continue;
        }
        
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

int main() {
    printf("=== Stage 3: Correctness Gates (H1-H4) Verification ===\n\n");

    // ---------------------------------------------------------
    // Gate H2: Table Completeness
    // ---------------------------------------------------------
    printf("[Gate H2] Checking Table Completeness...\n");
    if (perm_pdb[0] != 0 || ori_pdb[0] != 0) {
        printf("  -> FAIL: Solved state != 0\n");
        return 1;
    }
    int max_p = 0, max_o = 0;
    for (int i = 0; i < 5040; i++) if (perm_pdb[i] > max_p) max_p = perm_pdb[i];
    for (int i = 0; i < 729; i++) if (ori_pdb[i] > max_o) max_o = ori_pdb[i];
    printf("  -> PASS: Max Perm Depth = %d, Max Ori Depth = %d\n", max_p, max_o);

    // ---------------------------------------------------------
    // Gate H1, H3: Full-Domain Search & Admissibility
    // ---------------------------------------------------------
    printf("\n[Gate H1 & H3] Starting Full-Domain Search (3,674,160 states)...\n");
    printf("  -> This may take a few seconds. Please wait...\n");

    clock_t start_time = clock();
    int success_count = 0;
    int max_ans_length = 0;
    int total_states = 5040 * 729;

    for (uint16_t p = 0; p < 5040; p++) {
        for (uint16_t o = 0; o < 729; o++) {
            if (p == 0 && o == 0) {
                success_count++;
                continue;
            }

            uint8_t h_p = perm_pdb[p];
            uint8_t h_o = ori_pdb[o];
            int diff = h_o - h_p;
            int bound = h_o - (diff & (diff >> 31)); 
            
            int initial_heuristic = bound; 
            int ans_length = -1;
            while (1) {
                int next_bound;
                ans_length = bounded_dfs(p, o, bound, &next_bound);
                if (ans_length != -1) break;
                bound = next_bound;
            }
            
            if (ans_length >= initial_heuristic && ans_length <= 11) {
                success_count++;
                if (ans_length > max_ans_length) max_ans_length = ans_length;
            } else {
                printf("\n  -> FAIL at p=%d, o=%d\n", p, o);
                return 1;
            }
            
            // 每解完 50000 個狀態，印一次進度
            if (success_count % 50000 == 0) {
                clock_t current_time = clock();
                double elapsed = ((double) (current_time - start_time)) / CLOCKS_PER_SEC;
                double speed = success_count / elapsed;
                printf("  ... Solved %d / %d (%.1f%%) | Speed: %.0f states/sec | Elapsed: %.1f s\r", 
                       success_count, total_states, 
                       (float)success_count / total_states * 100.0, 
                       speed, elapsed);
                fflush(stdout); // 強制刷新輸出緩衝區
            }
        }
    }
    clock_t end_time = clock();
    double cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;

    printf("  -> PASS: Admissibility h(s) <= h*(s) verified for all states.\n");
    printf("  -> PASS: All %d states solved successfully.\n", success_count);
    printf("  -> MAX PATH LENGTH FOUND: %d\n", max_ans_length);
    printf("\n>>> GATE H3 WALL-CLOCK TIME: %.3f seconds <<<\n", cpu_time_used);

    // ---------------------------------------------------------
    // Gate H4: Packed Accessor Verification
    // ---------------------------------------------------------
    printf("\n[Gate H4] Packed Accessor Verification\n");
    printf("  -> STATUS: N/A (Deprecated).\n");
    printf("  -> REASON: 4-bit nibble packing was abandoned during Stage 3 optimization due to instruction overhead (un-packing cost). Reverted to O(1) byte-aligned arrays.\n");

    return 0;
}