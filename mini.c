#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定義常數：C=可移動角塊數量, O=方向組合數(3^6), N=總狀態數(7! * 3^6)
enum { C = 7, O = 729, N = 3674160 };

typedef struct {
    unsigned char p[C], o[C]; // p: 排列 (位置), o: 方向
} state_t;

// 執行 1/4 轉動 (f 是轉動的面：0=R, 1=B, 2=D)
static state_t turn(state_t s, int f)
{
    // 把上一版的 source 和 twist 兩個二維陣列「壓縮」成三個字串
    // 字串長度 14，前 7 個字元代表「新位置的來源索引」，後 7 個字元代表「扭轉方向增量」
    static const char map[][15] = {
        "14203561202100", // R 面的規則
        "01245630001212", // B 面的規則
        "02531460000000", // D 面的規則
    };
    state_t t;
    for (int i = 0; i < C; ++i) {
        // 利用 - '0' 將字元 ('0'~'9') 轉換回整數值 (0~9)
        int j = map[f][i] - '0';
        t.p[i] = s.p[j];
        // 加上對應的扭轉量，並取 3 的餘數
        t.o[i] = (s.o[j] + map[f][i + C] - '0') % 3;
    }
    return t;
}

// 計算當前狀態的唯一編號 (與上一版邏輯相同，只是變數命名變短了)
static size_t rank_state(state_t s)
{
    size_t p = 0, o = 0;
    for (int i = 0; i < C; ++i) {
        int n = 0;
        for (int j = i + 1; j < C; ++j)
            n += s.p[j] < s.p[i];
        p = p * (C - i) + n;
        if (i < 6)
            o = o * 3 + s.o[i];
    }
    return p * O + o; // 組合排列與方向
}

int main(int argc, char **argv)
{
    state_t s;
    unsigned seen = 0, sum = 0;
    
    // 檢查參數是否給了 14 個字元 (7個位置 + 7個方向)
    if (argc != 2 || strlen(argv[1]) != 14)
        return 2;
        
    // 解析輸入的 14 個字元
    for (int i = 0; i < C; ++i) {
        unsigned p = (unsigned) (argv[1][i] - '1');
        unsigned o = (unsigned) (argv[1][i + C] - '1');
        
        // 1. 檢查數字是否越界 (p < 7, o < 3)
        // 2. 檢查有沒有重複的小方塊 (利用 bitmask 位元遮罩 seen)
        // (seen >> p & 1) 代表檢查第 p 個 bit 是否已經被設為 1 
        if (p >= C || o >= 3 || seen >> p & 1)
            return 2;
            
        s.p[i] = p;
        s.o[i] = o;
        seen |= 1U << p; // 將第 p 個 bit 設為 1，標記這個方塊已經出現過了
        sum += o;
    }
    // 檢查方向總和是否符合魔方物理定律 (必須是 3 的倍數)
    if (sum % 3)
        return 2;

    // step 陣列用來記錄抵達各狀態的最短路徑第一步
    // 使用 calloc 會自動填 0，因此 0 代表「未訪問過」
    unsigned char *step = calloc(N, 1);
    state_t *queue = malloc(sizeof(*queue) * N);
    size_t head = 0, tail = 1;
    
    if (!step || !queue)
        return free(step), free(queue), 1;
        
    // 佇列起點放入「已復原狀態」
    queue[0] = (state_t) {{0, 1, 2, 3, 4, 5, 6}, {0}};
    
    // 倒推法的 BFS 廣度優先搜尋
    while (head < tail) {
        state_t from = queue[head++];
        for (int f = 0; f < 3; ++f) { // 測試 3 個面 (R, B, D)
            state_t to = from;
            // 連續做 1次轉(順轉)、2次轉(180度)、3次轉(逆轉)
            for (int n = 0; n < 3; ++n) {
                // 將狀態「轉一次」並計算新狀態的編號
                size_t rank = rank_state(to = turn(to, f)); 
                
                // 如果該狀態不是復原狀態(rank != 0) 且還沒被走過
                if (rank && !step[rank]) {
                    // 這裡非常巧妙：
                    // n=0 (轉1次)，倒推回正向解法需要逆轉 (對應索引 2)
                    // n=1 (轉2次)，倒推回正向解法仍是轉2次 (對應索引 1)
                    // n=2 (轉3次)，倒推回正向解法需要順轉 (對應索引 0)
                    // 所以用 (3 - n) 算出正確的逆操作代碼，並儲存為 1~9 的數字
                    step[rank] = f * 3 + 3 - n; 
                    queue[tail++] = to;
                }
            }
        }
    }
    free(queue);
    
    if (tail != N) // 確保 3674160 個狀態都有被遍歷到
        return free(step), 1;
        
    const char *sep = "";
    
    // 從使用者輸入的狀態開始，一步一步順著 step 表走回復原狀態(rank=0)
    for (size_t rank; (rank = rank_state(s));) {
        // 從 1~9 減 1 變回 0~8 的操作代碼
        int move = step[rank] - 1; 
        
        // 印出解法：
        // "RBD"[move/3] 選出轉哪個面
        // {"", "2", "'"}[move%3] 選出是順轉、180度還是逆轉
        printf(
            "%s%c%s", sep, "RBD" [move / 3],
            (const char *[]) { "", "2", "'" }[move % 3]);
        sep = " ";
        
        // 執行這個轉動，更新魔方狀態以便進入下一步
        // (如果是逆轉 move%3=2，迴圈會跑 3 次；180度跑 2 次)
        for (int n = move % 3 + 1; n--;)
            s = turn(s, move / 3);
    }
    
    // putchar 印出最後的換行，並檢查所有輸出是否成功
    int error = putchar('\n') < 0 || fflush(stdout) || ferror(stdout);
    free(step);
    return error;
}
