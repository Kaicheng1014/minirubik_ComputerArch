#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PERM_STATES 5040   // 7! = 5,040 (位置狀態數)
#define ORI_STATES 729     // 3^6 = 729 (方向狀態數)
#define NUM_MOVES 9        // 9 種基本轉法 (R, R2, R', B, B2, B', D, D2, D')

// 定義魔方狀態結構 (只追蹤 7 個可動角塊)
typedef struct {
    uint8_t perm[7]; // 數值 0 ~ 6 (位置排列)
    uint8_t ori[7];  // 數值 0 ~ 2 (翻轉方向)
} CubeState;

// 階乘表，用於 Lehmer code 與排列編號的互換
const int FACTORIAL[] = {1, 1, 2, 6, 24, 120, 720, 5040};

// 將 7 個元素的排列轉為 0 ~ 5039 的唯一編號 (Lehmer code)
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

// 將 6 個角塊的方向轉為 0 ~ 728 的三進位編號
int ori_to_index(const uint8_t *ori) {
    int index = 0;
    for (int i = 0; i < 6; i++) {
        index = index * 3 + ori[i];
    }
    return index;
}

int main() {
    printf("build_pdb skeleton compiled successfully.\n");
    printf("PERM_STATES: %d, ORI_STATES: %d\n", PERM_STATES, ORI_STATES);
    return 0;
}