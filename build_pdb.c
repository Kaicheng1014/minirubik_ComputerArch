#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ==========================================
// 1. 巨集與常數定義
// ==========================================
#define PERM_STATES 5040   // 7 顆可動角塊的位置排列總數 (7! = 5040)
#define ORI_STATES 729     // 6 顆可動角塊的方向變化總數 (3^6 = 729，第 7 顆由總和決定)
#define NUM_MOVES 9        // 9 種基本轉法 (R, R2, R', B, B2, B', D, D2, D')

// 階乘表，用於快速計算 Lehmer code
const int FACTORIAL[] = {1, 1, 2, 6, 24, 120, 720, 5040};

// ==========================================
// 2. 編號轉換函數 (Index Conversion)
// ==========================================

// 將 7 個角塊的位置排列轉換為 0 ~ 5039 的唯一整數編號
int perm_to_index(const uint8_t *perm) {
    int index = 0;
    for (int i = 0; i < 7; i++) {
        int count = 0;
        for (int j = i + 1; j < 7; j++) {
            if (perm[j] < perm[i]) {
                count++;
            }
        }
        index += count * FACTORIAL[6 - i];
    }
    return index;
}

// 將 6 個角塊的方向陣列轉換為 0 ~ 728 的整數編號
int ori_to_index(const uint8_t *ori) {
    int index = 0;
    for (int i = 0; i < 6; i++) {
        index = index * 3 + ori[i];
    }
    return index;
}

// ==========================================
// 2.5 反向解碼函數 (Index to Array)
// ==========================================

// 將 0 ~ 5039 的編號還原為 7 個角塊的排列陣列
void index_to_perm(int index, uint8_t *perm) {
    uint8_t unused[7] = {0, 1, 2, 3, 4, 5, 6};
    for (int i = 0; i < 7; i++) {
        int fact = FACTORIAL[6 - i];
        int k = index / fact;
        index %= fact;
        perm[i] = unused[k];
        // 將用過的數字移除，後面的數字往前遞補
        for (int j = k; j < 6 - i; j++) {
            unused[j] = unused[j + 1];
        }
    }
}

// 將 0 ~ 728 的編號還原為 6 個角塊的方向陣列，並推算第 7 個
void index_to_ori(int index, uint8_t *ori) {
    int sum = 0;
    // 取得前 6 顆角塊的方向 (0, 1, 2)
    for (int i = 5; i >= 0; i--) {
        ori[i] = index % 3;
        sum += ori[i];
        index /= 3;
    }
    // 魔術方塊的物理特性：所有角塊的方向總和必須是 3 的倍數
    ori[6] = (3 - (sum % 3)) % 3;
}

// ==========================================
// 3. 魔術方塊轉動邏輯 (Move Generators)
// ==========================================

// 定義 R, B, D 三個基本面的 4-cycle 置換與 Twist 影響
// 角塊索引定義 (固定 UFL)：0:UBL, 1:UBR, 2:UFR, 3:DFL, 4:DFR, 5:DBR, 6:DBL
// 轉動時，牽涉的 4 個角塊位置會順序輪替，部分面的轉動會改變方向(Twist)

const uint8_t BASE_PERM[3][4] = {
    {1, 5, 4, 2}, // R 面順時針牽涉的角塊
    {0, 1, 5, 6}, // B 面順時針牽涉的角塊
    {3, 4, 5, 6}  // D 面順時針牽涉的角塊
};

// 方向變化 (0: 不變, 1: 順時針轉, 2: 逆時針轉)
const uint8_t BASE_TWIST[3][4] = {
    {1, 2, 1, 2}, // R 面轉動的 Twist
    {1, 2, 1, 2}, // B 面轉動的 Twist
    {0, 0, 0, 0}  // D 面轉動的 Twist (底層轉動不改變 U/D 朝向)
};

// 執行一次基本轉動
void apply_move(uint8_t *perm, uint8_t *ori, int move_id) {
    int face = move_id / 3;       // 0: R, 1: B, 2: D
    int turns = (move_id % 3) + 1; // 1: 順時針 90度, 2: 180度, 3: 逆時針 90度
    
    for (int t = 0; t < turns; t++) {
        uint8_t old_perm[7], old_ori[7];
        memcpy(old_perm, perm, 7);
        memcpy(old_ori, ori, 7);
        
        // 執行 4-cycle 的置換與方向更新
        for (int i = 0; i < 4; i++) {
            int curr = BASE_PERM[face][i];
            int next = BASE_PERM[face][(i + 1) % 4];
            perm[next] = old_perm[curr];
            // 更新方向：舊方向加上轉動造成的變化，並取 3 的餘數
            ori[next] = (old_ori[curr] + BASE_TWIST[face][i]) % 3;
        }
    }
}

// ==========================================
// 4. 主程式：建表與檔案輸出
// ==========================================
int main() {
    printf("=== 2x2 Rubik's Cube PDB Generator (Option 2) ===\n");
    printf("Target: Generating Transition Tables & Independent PDBs\n");

    FILE *f = fopen("pdb.h", "w");
    if (!f) {
        perror("Error: Failed to create pdb.h");
        return 1;
    }

    fprintf(f, "#ifndef PDB_H\n#define PDB_H\n\n");
    fprintf(f, "#include <stdint.h>\n\n");

    fprintf(f, "// Transition Tables and PDB arrays will be generated here.\n");

    fprintf(f, "\n#endif // PDB_H\n");
    fclose(f);

    printf("Skeleton initialized successfully. pdb.h framework created.\n");
    return 0;
}