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
    int i, j, k, l, tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    if (M == 32 && N == 32) 
    {
        for (i = 0; i < 32; i=i+8) {
            for (j = 0; j < 32; j=j+8) {
                if (i == j) {
                    for (k = 0; k < 8; k++) {
                        tmp = A[i+k][j];
                        tmp1 = A[i+k][j+1];
                        tmp2 = A[i+k][j+2];
                        tmp3 = A[i+k][j+3];
                        tmp4 = A[i+k][j+4];
                        tmp5 = A[i+k][j+5];
                        tmp6 = A[i+k][j+6];
                        tmp7 = A[i+k][j+7];
                        
                        B[i+k][j+7] = tmp7;
                        B[i+k][j+6] = tmp6;
                        B[i+k][j+5] = tmp5;
                        B[i+k][j+4] = tmp4;
                        B[i+k][j+3] = tmp3;
                        B[i+k][j+2] = tmp2;
                        B[i+k][j+1] = tmp1;
                        B[i+k][j] = tmp;
                    }
                    for (k = 0; k < 8; k++) {
                        for (l = k; l < 8; l++) {
                            tmp = B[i+k][j+l];
                            B[i+k][j+l] = B[j+l][i+k];
                            B[j+l][i+k] = tmp;
                        }
                    }
                } else {
                    for (k = 0; k < 8; k++) {
                        for (l = 0; l < 8; l++) {
                            tmp = A[i+k][j+l];
                            B[j+l][i+k] = tmp;
                        }
                    }
                }
            }
        }
    }
    else if (M == 64 && N == 64) {
        for (i = 0; i < N; i = i + 8) {
            for (j = 0; j < M; j = j + 8) {
                if (i == j) {

                    for (k = 0; k < 8; k++) {
                        if (k == 4) {
                            // trans B[i,j][i+4,j+4]
                            for (k = 0; k < 4; k++) {
                                for (l = k; l < 4; l++) {
                                    tmp = B[i+k][j+l];
                                    B[i+k][j+l] = B[j+l][i+k];
                                    B[j+l][i+k] = tmp;
                                }
                            }
                            k = 4;
                        }

                        tmp = A[i+k][j];
                        tmp1 = A[i+k][j+1];
                        tmp2 = A[i+k][j+2];
                        tmp3 = A[i+k][j+3];
                        tmp4 = A[i+k][j+4];
                        tmp5 = A[i+k][j+5];
                        tmp6 = A[i+k][j+6];
                        tmp7 = A[i+k][j+7];
                        
                        B[i+k][j+7] = tmp7;
                        B[i+k][j+6] = tmp6;
                        B[i+k][j+5] = tmp5;
                        B[i+k][j+4] = tmp4;
                        B[i+k][j+3] = tmp3;
                        B[i+k][j+2] = tmp2;
                        B[i+k][j+1] = tmp1;
                        B[i+k][j] = tmp;
                    }
                    // trans B[i+4,j+4][i+8,j+8] 
                    for (k = 4; k < 8; k++) {
                        for (l = k; l < 8; l++) {
                            tmp = B[i+k][j+l];
                            B[i+k][j+l] = B[j+l][i+k];
                            B[j+l][i+k] = tmp;
                        }
                    }

                    // row 3,4 in cache B[i+4,j][i+6,j+2]
                    tmp = B[i+4][j]; tmp1 = B[i+4][j+1]; tmp2 = B[i+5][j]; tmp3 = B[i+5][j+1];
                    
                    // trans B[i,j+6][i+2,j+8] , block 2
                    for (k = 0; k < 2; k++) {
                        for (l = 6; l < 8; l++) {
                            tmp4 = B[i+k][j+l];
                            B[i+k][j+l] = B[j+l][i+k];
                            B[j+l][i+k] = tmp4;
                        }
                    }

                    // insert tmp
                    tmp4 = B[j][i+4]; tmp5 = B[j+1][i+4]; tmp6 = B[j][i+5]; tmp7 = B[j+1][i+5];
                    B[j][i+4] = tmp; B[j+1][i+4] = tmp1; B[j][i+5] = tmp2; B[j+1][i+5] = tmp3;
                    // row 1 done, load row 3, now row 3,4
                    B[i+4][j] = tmp4; B[i+4][j+1] = tmp5; B[i+5][j] = tmp6; B[i+5][j+1] = tmp7;

                    // store block 4 B[i+6,j+2][i+8,j+4]
                    tmp4 = B[i+6][j+2]; tmp5 = B[i+6][j+3]; tmp6 = B[i+7][j+2]; tmp7 = B[i+7][j+3];

                    // load row 2 , insert into block 4
                    tmp = B[j+2][i+6]; tmp1 = B[j+3][i+6]; tmp2 = B[j+2][i+7]; tmp3 = B[j+3][i+7];  
                    B[j+2][i+6] = tmp4; B[j+3][i+6] = tmp5; B[j+2][i+7] = tmp6; B[j+3][i+7] = tmp7;  

                    // trans B[i+2,j+4][i+4,j+6] , row 2 done
                    for (k = 2; k < 4; k++) {
                        for (l = 4; l < 6; l++) {
                            tmp4 = B[i+k][j+l];
                            B[i+k][j+l] = B[j+l][i+k];
                            B[j+l][i+k] = tmp4;
                        }
                    }

                    B[i+6][j+2] = tmp; B[i+6][j+3] = tmp1; B[i+7][j+2] = tmp2; B[i+7][j+3] = tmp3;

                } else {
                    // trans A[i,j][i+4,j+4]
                    for (k = 0; k < 4; k++) {
                        for (l = 0; l < 4; l++) {
                            tmp = A[i+k][j+l];
                            B[j+l][i+k] = tmp;
                        }
                    }

                    tmp = A[i][j+4];
                    tmp1 = A[i][j+5];
                    tmp2 = A[i][j+6];
                    tmp3 = A[i][j+7];

                    tmp4 = A[i+1][j+4];
                    tmp5 = A[i+1][j+5];
                    tmp6 = A[i+1][j+6];
                    tmp7 = A[i+1][j+7];

                    // trans A[i+4,j] [i+8,j+4]
                    for (k = 0; k < 4; k++) {
                        for (l = 0; l < 4; l++) {
                            B[j+l][i+4+k] = A[i+4+k][j+l];
                        }
                    }

                    // trans A[i+4,j+4] [i+8,j+8]
                    for (k = 0; k < 4; k++) {
                        for (l = 0; l < 4; l++) {
                            B[j+4+l][i+4+k] = A[i+4+k][j+4+l];
                        }
                    }

                    // trans A[i,j+4] [i+4,j+8]
                    for (k = 2; k < 4; k++) {
                        for (l = 0; l < 4; l++) {
                            B[j+4+l][i+k] = A[i+k][j+4+l];
                        }
                    }

                    B[j+4][i] = tmp;
                    B[j+5][i] = tmp1;
                    B[j+6][i] = tmp2;
                    B[j+7][i] = tmp3;

                    B[j+4][i+1] = tmp4;
                    B[j+5][i+1] = tmp5;
                    B[j+6][i+1] = tmp6;
                    B[j+7][i+1] = tmp7;

                }
            }
        }

    } else if (M == 61 && N == 67) {

        for (j = 0; j < M; j=j+8) {
            for (i = 0; i < N; i=i+8) {
                // 8*5 A[i,56][i+7,61], col 8
                if (j == 56) {
                    for (k = 0; k < 5; k++) {
                        tmp = A[i][j+k];
                        tmp1 = A[i+1][j+k];
                        tmp2 = A[i+2][j+k];

                        if (i != 64) {
                            tmp3 = A[i+3][j+k];
                            tmp4 = A[i+4][j+k];
                            tmp5 = A[i+5][j+k];
                            tmp6 = A[i+6][j+k];
                            tmp7 = A[i+7][j+k];
                        }

                        B[j+k][i] = tmp;
                        B[j+k][i+1] = tmp1;
                        B[j+k][i+2] = tmp2;
                        
                        if (i != 64) {
                            B[j+k][i+3] = tmp3;
                            B[j+k][i+4] = tmp4;
                            B[j+k][i+5] = tmp5;
                            B[j+k][i+6] = tmp6;
                            B[j+k][i+7] = tmp7;
                        }

                    }
                    continue;         
                } 
                l = 8;
                if (i == 64) {
                    l = 3;
                }
                for (k = 0; k < l; k++) {
                    tmp = A[i+k][j];
                    tmp1 = A[i+k][j+1];
                    tmp2 = A[i+k][j+2];
                    tmp3 = A[i+k][j+3];
                    tmp4 = A[i+k][j+4];
                    tmp5 = A[i+k][j+5];
                    tmp6 = A[i+k][j+6];
                    tmp7 = A[i+k][j+7];
                        
                    B[j+7][i+k] = tmp7;
                    B[j+6][i+k] = tmp6;
                    B[j+5][i+k] = tmp5;
                    B[j+4][i+k] = tmp4;
                    B[j+3][i+k] = tmp3;
                    B[j+2][i+k] = tmp2;
                    B[j+1][i+k] = tmp1;
                    B[j][i+k] = tmp;
                }
            }
        }

    } else {
        for (i = 0; i < N; i++) {
            for (j = 0; j < M; j++) {
                tmp = A[i][j];
                B[j][i] = tmp;
            }
        }
    }
}

char transpose_test1_desc[] = "Transpose test1";
void transpose_test1(int M, int N, int A[N][M], int B[M][N])
{
    // int i, j, k, l, tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    // if (M == 32 && N == 32) {
    // }
    // else 
    // {
    //     for (i = 0; i < N; i++) {
    //         for (j = 0; j < M; j++) {
    //             tmp = A[i][j];
    //             B[j][i] = tmp;
    //         }
    //     } 
    // }
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

