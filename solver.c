#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定義魔術方塊狀態的常數
enum {
    CUBIES = 7,                 // 可移動的小方塊數量 (2x2x2 共有 8 個角塊，固定 1 個，剩 7 個)
    PERMUTATIONS = 5040,        // 7 個小方塊的排列數 (7!)
    ORIENTATIONS = 729,         // 小方塊的方向數 (3^6，第 7 個角塊的方向由前 6 個決定)
    STATES = PERMUTATIONS * ORIENTATIONS, // 總狀態數：5040 * 729 = 3674160
    MOVES = 9                   // 總共有 9 種基本轉動操作 (3個面 x 3種轉動方式)
};

// 儲存魔術方塊狀態的結構體
typedef struct {
    uint8_t p[CUBIES]; // p 代表排列 (permutation)，記錄每個位置上是哪個小方塊
    uint8_t o[CUBIES]; // o 代表方向 (orientation)，記錄每個小方塊的扭轉狀態 (0, 1, 2)
} state_t;

/*@ predicate valid_state(state_t *state) =
      // 確保 p 和 o 的數值在合法範圍內
      (\forall integer i; 0 <= i < CUBIES ==>
         state->p[i] < CUBIES && state->o[i] < 3) &&
      // 確保沒有重複的小方塊 (排列是唯一且不重複的)
      (\forall integer i, j; 0 <= i < j < CUBIES ==>
         state->p[i] != state->p[j]) &&
      // 確保所有小方塊的方向總和必須是 3 的倍數 (魔術方塊的物理限制定律)
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
 */

// 轉動操作的名稱字串 (R: 右面, B: 後面, D: 下面；包含順時針、180度、逆時針)
static const char *const move_names[MOVES] = {"R",  "R2", "R'", "B", "B2",
                                              "B'", "D",  "D2", "D'"};
// 每個操作的逆向操作對應索引 (例如 "R" 的逆向是 "R'")
static const uint8_t inverse_move[MOVES] = {2, 1, 0, 5, 4, 3, 8, 7, 6};

/* 每個目標位置將從 source[face][destination] 獲取小方塊。 */
// 轉動某個面時，小方塊「位置」的變化規則表
static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6}, // 轉動面 0 (R) 的變化
    {0, 1, 2, 4, 5, 6, 3}, // 轉動面 1 (B) 的變化
    {0, 2, 5, 3, 1, 4, 6}, // 轉動面 2 (D) 的變化
};
// 轉動某個面時，小方塊「方向 (扭轉)」的變化規則表
static const uint8_t twist[3][CUBIES] = {
    {1, 2, 0, 2, 1, 0, 0}, // 面 0 的扭轉增量
    {0, 0, 0, 1, 2, 1, 2}, // 面 1 的扭轉增量
    {0, 0, 0, 0, 0, 0, 0}, // 面 2 的扭轉增量
};

/* 三種面轉 90 度 (quarter-turns) 都會保持「前-上-左」角的那個方塊不變。 */
/*@ requires face < 3;
    assigns \nothing;
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.p[i] == state.p[source[face][i]];
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.o[i] == (state.o[source[face][i]] + twist[face][i]) % 3;
 */
// 執行一次 1/4 轉動 (面 face 順時針轉 90 度)
static state_t quarter_turn(state_t state, uint8_t face)
{
    state_t result;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant \forall integer j; 0 <= j < i ==>
          result.p[j] == state.p[source[face][j]];
        loop invariant \forall integer j; 0 <= j < i ==>
          result.o[j] == (state.o[source[face][j]] + twist[face][j]) % 3;
        loop assigns i, result.p[0..6], result.o[0..6];
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        // 找出轉動後該位置的小方塊原本在哪裡
        uint8_t from = source[face][i];
        // 更新位置
        result.p[i] = state.p[from];
        // 更新方向 (加上扭轉值並取 3 的餘數)
        result.o[i] = (uint8_t) ((state.o[from] + twist[face][i]) % 3U);
    }
    return result;
}

// 執行指定操作 (可能包含 1/4 轉、半轉、反轉)
static state_t apply_move(state_t state, uint8_t move)
{
    // 計算需要轉幾個 90 度 (例如 move=0 轉1次, move=1 轉2次, move=2 轉3次等同逆時針)
    uint8_t turns = (uint8_t) (move % 3U + 1U);
    for (uint8_t i = 0; i < turns; ++i)
        state = quarter_turn(state, (uint8_t) (move / 3U));// (move / 3) 決定轉哪一面
    return state;
}

/* 計算當前狀態的「編號 (rank)」，這將用於將狀態對應到一維陣列索引上 */
/*@ requires \valid_read(state);
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->p[i] < CUBIES;
    requires \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->o[i] < 3;
    assigns \nothing;
    ensures \result < STATES;
 */
static uint32_t rank_state(const state_t *state)
{
    uint32_t p = 0, o = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant (i == 0 ==> p == 0) && (i == 1 ==> p <= 6) &&
          (i == 2 ==> p <= 41) && (i == 3 ==> p <= 209) &&
          (i == 4 ==> p <= 839) && (i == 5 ==> p <= 2519) &&
          (i >= 6 ==> p <= 5039);
        loop assigns i, p;
        loop variant CUBIES - i;
     */

    // 1. 計算排列 (Permutation) 的編號 (利用康托展開 Cantor expansion)
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t smaller = 0;
        /*@ loop invariant i + 1 <= j <= CUBIES;
            loop invariant smaller <= j - i - 1;
            loop assigns j, smaller;
            loop variant CUBIES - j;
         */
        // 計算在第 i 位之後，有幾個比 state->p[i] 還小的數字
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j)
            if (state->p[j] < state->p[i])
                ++smaller;
        p = p * (CUBIES - i) + smaller;
    }
    /*@ loop invariant 0 <= i <= 6;
        loop invariant (i == 0 ==> o == 0) && (i == 1 ==> o < 3) &&
          (i == 2 ==> o < 9) && (i == 3 ==> o < 27) &&
          (i == 4 ==> o < 81) && (i == 5 ==> o < 243) &&
          (i == 6 ==> o < 729);
        loop assigns i, o;
        loop variant 6 - i;
     */
    // 2. 計算方向 (Orientation) 的編號 (當作 3 進位數處理，只需算前 6 個)
    for (uint8_t i = 0; i < 6; ++i)
        o = o * 3U + state->o[i];
    
    // 組合排列與方向，返回唯一的狀態編號
    return p * ORIENTATIONS + o;
}
/* 將「編號 (rank)」還原回實際的魔方狀態 (用於 BFS 初始化所有狀態) */
/*@ requires \valid(state); requires rank < STATES; assigns *state; */
static void unrank_state(uint32_t rank, state_t *state)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};// 可用的小方塊
    uint32_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS, f = 720; // f = 6!
    uint8_t sum = 0;

    // 1. 還原排列狀態
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t q = (uint8_t) (p / f);
        p %= f;
        state->p[i] = available[q];
        // 移除已經用掉的方塊，將後面的遞補上來
        for (uint8_t j = q; j + 1U < CUBIES - i; ++j)
            available[j] = available[j + 1U];
        if (i < 5)
            f /= 6U - i;
    }

    // 2. 還原方向狀態 (只需還原前 6 個，最後一個由前面決定)
    for (uint8_t i = 6; i-- > 0;) {
        state->o[i] = (uint8_t) (o % 3U);
        sum = (uint8_t) (sum + state->o[i]);
        o /= 3U;
    }
    state->o[6] = (uint8_t) ((3U - sum % 3U) % 3U);
}

/*@ requires \valid_read(state);
    requires \initialized(&state->p[0..6]) && \initialized(&state->o[0..6]);
    assigns \nothing;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures complete: valid_state(state) ==> \result != 0;
 */

 /* 檢查傳入的字串/狀態是否符合合法的魔術方塊規則 */
static int valid(const state_t *state)
{
    uint8_t sum = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant sum <= 2 * i;
        loop invariant sum == (i > 0 ? state->o[0] : 0) +
          (i > 1 ? state->o[1] : 0) + (i > 2 ? state->o[2] : 0) +
          (i > 3 ? state->o[3] : 0) + (i > 4 ? state->o[4] : 0) +
          (i > 5 ? state->o[5] : 0) + (i > 6 ? state->o[6] : 0);
        loop invariant \forall integer j; 0 <= j < i ==>
          state->p[j] < CUBIES && state->o[j] < 3;
        loop invariant \forall integer j, k; 0 <= j < k < i ==>
          state->p[j] != state->p[k];
        loop assigns i, sum;
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        // 檢查是否有超出範圍的值
        if (state->p[i] >= CUBIES || state->o[i] >= 3)
            return 0;
        /*@ loop invariant 0 <= j <= i;
            loop invariant \forall integer k; 0 <= k < j ==>
              state->p[k] != state->p[i];
            loop assigns j;
            loop variant i - j;
        */
       // 檢查是否有重複的方塊
        for (uint8_t j = 0; j < i; ++j)
            if (state->p[j] == state->p[i])
                return 0;
        sum = (uint8_t) (sum + state->o[i]);
    }
    // 確保所有方向值的總和是 3 的倍數
    return sum % 3U == 0;
}

// 建立魔術方塊所有狀態的最佳解查找表 (透過 BFS 廣度優先搜尋)
static uint8_t *build_table(uint8_t *diameter)
{
    // toward_solved 記錄從某個狀態要走向復原狀態所需走的第一步
    uint8_t *toward_solved = malloc(STATES);
    // BFS 使用的佇列 (Queue)
    uint32_t *queue = malloc((size_t) STATES * sizeof *queue);
    // 快取陣列：記錄每種排列/方向轉動後的下一種排列/方向，加速運算
    uint16_t permutation[3][PERMUTATIONS], orientation[3][ORIENTATIONS];
    uint32_t head = 0, tail = 1, level_end = 1;
    state_t state;
    if (!toward_solved || !queue) {
        free(toward_solved);
        free(queue);
        return NULL;
    }
    // 預先計算並快取所有的排列轉動結果
    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {
        unrank_state((uint32_t) rank * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            permutation[face][rank] =
                (uint16_t) (rank_state(&next) / ORIENTATIONS);
        }
    }
    // 預先計算並快取所有的方向轉動結果
    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orientation[face][rank] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }
    // 初始化 toward_solved 表格，將所有狀態標記為未訪問 (UINT8_MAX)
    memset(toward_solved, UINT8_MAX, STATES);
    queue[0] = 0;// 0 表示復原的狀態 (編號為 0)
    toward_solved[0] = 0;
    *diameter = 0;// diameter (直徑) 代表魔方「上帝之數」的步數

    // 從復原狀態開始向外擴展 (Reverse BFS)
    while (head < tail) {
        // 每訪問完同一深度的所有節點，直徑加 1
        if (head == level_end) {
            level_end = tail;
            ++*diameter;
        }
        uint32_t here = queue[head++];
        uint16_t p = (uint16_t) (here / ORIENTATIONS);
        uint16_t o = (uint16_t) (here % ORIENTATIONS);

        // 嘗試從當前狀態反向進行所有可能的 9 種轉動
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next_p = p, next_o = o;
            // 連續進行 1/4 轉、半轉、逆時針 1/4 轉
            for (uint8_t turn = 0; turn < 3; ++turn) {
                // 使用快取表快速求出轉動後的狀態
                next_p = permutation[face][next_p];
                next_o = orientation[face][next_o];
                uint32_t there = (uint32_t) next_p * ORIENTATIONS + next_o;

                // 如果這個狀態還沒被訪問過
                if (toward_solved[there] == UINT8_MAX) {
                    uint8_t move = (uint8_t) (face * 3U + turn);
                    // 記錄回到當前狀態所需的操作 (因為是倒推，所以記錄逆操作)
                    toward_solved[there] = inverse_move[move];
                    // 將新狀態加入佇列中
                    queue[tail++] = there;
                }
            }
        }
    }
    free(queue);

    // 如果還有狀態沒被遍歷到 (代表魔方圖不連通，發生錯誤)
    if (tail != STATES) {
        free(toward_solved);
        return NULL;
    }
    return toward_solved;
}

/*@ requires valid_read_string(input);
    requires \valid(state);
    assigns state->p[0..6], state->o[0..6];
    ensures \result != 0 ==> input[14] == '\0';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] == input[i] - '1';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->o[i] == input[i + CUBIES] - '1';
 */

 /* 解析使用者輸入的魔方狀態字串 (如 12345670000000) */
static int parse_state(const char *input, state_t *state)
{
    /*@ loop invariant 0 <= i <= 14;
        loop invariant i <= strlen(input);
        loop invariant i <= 7 ==> \initialized(&state->p[0..i-1]);
        loop invariant i >= 7 ==> \initialized(&state->p[0..6]);
        loop invariant i >= 7 ==> \initialized(&state->o[0..i-8]);
        loop invariant \forall integer j; 0 <= j < i && j < CUBIES ==>
          state->p[j] == input[j] - '1';
        loop invariant \forall integer j; 0 <= j < i - CUBIES ==>
          state->o[j] == input[j + CUBIES] - '1';
        loop assigns i, state->p[0..6], state->o[0..6];
        loop variant 14 - i;
     */

    // 前 7 個字元代表排列 (1~7)，後 7 個字元代表方向 (1~3)
    for (int i = 0; i < 14; ++i) {
        int limit = i < 7 ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit)
            return 0;// 格式錯誤

        // 將 '1'-'7' 或 '1'-'3' 轉換成 0-6 或 0-2 的內部表示法
        (i < 7 ? state->p : state->o)[i % 7] = (uint8_t) (input[i] - '1');
    }
    // 必須剛剛好 14 個字元，並且是合法的狀態
    return input[14] == '\0' && valid(state);
}

/* stdout is fully buffered off a terminal, so a write error surfaces at the
 * flush, not at the printf that queued the bytes. Every exit path that has
 * produced output goes through here.
 */

 /* 確保標準輸出完全成功，沒有發生錯誤或緩衝區卡住 */
static int output_failed(void)
{
    return fflush(stdout) != 0 || ferror(stdout);
}

// 內部自我測試 (驗證運算是否正確)
static int self_test(void)
{
    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};// 復原的狀態
    state_t state;

    // 測試：轉動後再反向轉動，是否能回到原狀態
    for (uint8_t move = 0; move < MOVES; ++move) {
        state = solved;
        state = apply_move(state, move);
        state = apply_move(state, inverse_move[move]);
        if (memcmp(&solved, &state, sizeof solved))
            return 0;
    }

    // 測試：狀態編號與解碼的雙向轉換是否正確
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank)
            return 0;
    }
    return 1;// 測試通過
}

int main(int argc, char **argv)
{
    state_t state;
    uint8_t diameter;

    // 若參數為 --self-test，執行自我測試及上帝之數計算
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        if (!self_test()) {
            fputs("self-test failed\n", stderr);
            return 1;
        }
        uint8_t *table = build_table(&diameter);
        if (!table) {
            fputs("could not build complete state table\n", stderr);
            return 1;
        }
        free(table);
        // 2x2x2 的上帝之數 (最遠狀態的步數) 應為 11 步 (若只轉三個面)
        if (diameter != 11) {
            fputs("BFS check failed\n", stderr);
            return 1;
        }
        puts("3674160 states; diameter 11");
        return output_failed();
    }

    // 解析命令列參數傳入的初始狀態
    if (argc != 2 || !parse_state(argv[1], &state)) {
        /* C99 5.1.2.2.1 lets argv[0] be null when argc is 0. */
        // 提示用法：前面 7 碼是位置 (1~7)，後面 7 碼是方向 (1~3)
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n",
                argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }

    // 建立查找表
    uint8_t *table = build_table(&diameter);
    if (!table) {
        fputs("could not build complete state table\n", stderr);
        return 1;
    }
    const char *separator = "";
    // 不斷從當前狀態查詢要進行哪一步才能靠近目標，直到抵達復原狀態 (rank 為 0)
    for (uint32_t rank = rank_state(&state); rank; rank = rank_state(&state)) {
        uint8_t move = table[rank];
        // 印出解法步驟
        printf("%s%s", separator, move_names[move]);
        separator = " ";
        // 執行該步驟更新當前狀態
        state = apply_move(state, move);
    }
    putchar('\n');
    free(table);
    return output_failed();
}
