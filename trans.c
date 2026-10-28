/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i;
    int j;
    int r;
    int c;
    // int x;
    // int y;


    int a0, a1, a2, a3;  // 暂存一行中的前 4 个元素
    int a4, a5, a6, a7;  // 暂存一行中的后 4 个元素

    if (M == 32 && N == 32) {
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                for (r = i; r < i + 8; r++) {
                    // 先连续读完 A 中这一行的 8 个元素
                    a0 = A[r][j];
                    a1 = A[r][j + 1];
                    a2 = A[r][j + 2];
                    a3 = A[r][j + 3];
                    a4 = A[r][j + 4];
                    a5 = A[r][j + 5];
                    a6 = A[r][j + 6];
                    a7 = A[r][j + 7];

                    // 再写入 B，避免写 B 打断上面的连续读取
                    B[j][r]     = a0;
                    B[j + 1][r] = a1;
                    B[j + 2][r] = a2;
                    B[j + 3][r] = a3;
                    B[j + 4][r] = a4;
                    B[j + 5][r] = a5;
                    B[j + 6][r] = a6;
                    B[j + 7][r] = a7;
                }
            }
        }

        return;  // 当前尺寸已处理完毕
    }

    if (M == 64 && N == 64) {

        for(i = 0;i < N;i+=8){
            for(j = 0;j < M;j+=8){
                for(r = i;r < i + 4 ;r++){
                    a0 = A[r][j];
                    a1 = A[r][j+1];
                    a2 = A[r][j+2];
                    a3 = A[r][j+3];
                    a4 = A[r][j+4];
                    a5 = A[r][j+5];
                    a6 = A[r][j+6];
                    a7 = A[r][j+7];


                    //
                    B[j][r] = a0;
                    B[j+1][r] = a1;
                    B[j+2][r] = a2;
                    B[j+3][r] = a3;

                    //
                    B[j][r+4] = a4;
                    B[j+1][r+4] = a5;
                    B[j+2][r+4] = a6;
                    B[j+3][r+4] = a7;

                }
for (i = 0; i < N; i += 8) {
    for (j = 0; j < M; j += 8) {

        /* 第一阶段：读取 A 的上四行 */
        for (r = i; r < i + 4; r++) {
            /* 一次读完连续八个元素 */
            a0 = A[r][j];
            a1 = A[r][j + 1];
            a2 = A[r][j + 2];
            a3 = A[r][j + 3];
            a4 = A[r][j + 4];
            a5 = A[r][j + 5];
            a6 = A[r][j + 6];
            a7 = A[r][j + 7];

            /* P 转置后放入最终位置 */
            B[j][r]     = a0;
            B[j + 1][r] = a1;
            B[j + 2][r] = a2;
            B[j + 3][r] = a3;

            /* Q 转置后暂存在B[j]分配缓存行的后4位，充分利用空间 */
            B[j][r + 4]     = a4;
            B[j + 1][r + 4] = a5;
            B[j + 2][r + 4] = a6;
            B[j + 3][r + 4] = a7;
        }

/* 第二阶段：每次完成 B 上半区的一行，并搬走该行的暂存值 */
        for (c = j; c < j + 4; c++) {
    /* 先保存 Q 的暂存值，避免覆盖后丢失 */
            a0 = B[c][i + 4];
            a1 = B[c][i + 5];
            a2 = B[c][i + 6];
            a3 = B[c][i + 7];

    /* 读取 R 的一列，转置后对应 B 的一行 */
            a4 = A[i + 4][c];
            a5 = A[i + 5][c];
            a6 = A[i + 6][c];
            a7 = A[i + 7][c];

    /* R 转置后填入右上区域，这一行至此完成 */
            B[c][i + 4] = a4;
            B[c][i + 5] = a5;
            B[c][i + 6] = a6;
            B[c][i + 7] = a7;

    /* 将 Q 的暂存值搬到左下区域的最终位置 */
            B[c + 4][i]     = a0;
            B[c + 4][i + 1] = a1;
            B[c + 4][i + 2] = a2;
            B[c + 4][i + 3] = a3;
}

/* 第三阶段：将 A 的右下 4×4 转置到 B 的右下区域 */
for (r = i + 4; r < i + 8; r++) {
    /* 先读取一行的四个元素 */
    a0 = A[r][j + 4];
    a1 = A[r][j + 5];
    a2 = A[r][j + 6];
    a3 = A[r][j + 7];

    /* 再写入转置后的位置 */
    B[j + 4][r] = a0;
    B[j + 5][r] = a1;
    B[j + 6][r] = a2;
    B[j + 7][r] = a3;
    }
}
                }
            }
        }

        return;  // 当前尺寸已处理完毕
    }

    for (i = 0;i<N;i+=16){
        for(j=0;j<M;j+=16){
            for(r = i; r < i + 16 && r < N; r++){
                for(c = j; c < j + 16 && c < M; c++)
                    B[c][r] = A[r][c];
            }
        }
    }



}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

