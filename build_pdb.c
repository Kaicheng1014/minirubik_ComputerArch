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
// 3. 主程式：建表與檔案輸出
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