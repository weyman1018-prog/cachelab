
#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>

/* 模拟缓存行，只保存判断命中和执行 LRU 所需的信息 */
typedef struct {
    int valid;                     // 有效位：0 为空，1 为已装入块
    unsigned long long tag;         // 当前内存块的标记
    unsigned long long last_used;   // 最近一次访问的序号
} CacheLine;

/* 打印命令行使用说明 */
void print_usage(void)
{
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
    printf("  -h        Show help\n");
    printf("  -v        Show each cache access result\n");
    printf("  -s <s>    Number of set index bits\n");
    printf("  -E <E>    Number of lines per set\n");
    printf("  -b <b>    Number of block offset bits\n");
    printf("  -t <file> Trace file path\n");
}

/* 程序入口：argc 为参数数量，argv 保存参数字符串 */
int main(int argc, char *argv[])
{
    int verbose = 0;          // 是否打印每次访问结果
    int s = -1;               // 组索引位数
    int E = -1;               // 每组缓存行数
    int b = -1;               // 块内偏移位数
    int option;               // getopt 返回的选项字符
    char *trace_path = NULL;  // trace 文件路径

    /* getopt：逐次解析选项，optarg 指向该选项的参数 */
    while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (option) {
        case 'h':
            print_usage();
            return 0;

        case 'v':
            verbose = 1;
            break;

        case 's':
            s = atoi(optarg);  // atoi：将数字字符串转换为 int
            break;

        case 'E':
            E = atoi(optarg);
            break;

        case 'b':
            b = atoi(optarg);
            break;

        case 't':
            trace_path = optarg;
            break;

        default:
            print_usage();
            return 1;
        }
    }

    /* 检查必需参数和基本取值范围 */
    if (s < 0 || E <= 0 || b < 0 || trace_path == NULL) {
        fprintf(stderr, "Missing or invalid arguments.\n");
        print_usage();
        return 1;
    }

    /* 当前 x86-64 环境下，避免移位达到或超过 64 位 */
    if (s >= 64 || b >= 64 || s + b >= 64) {
        fprintf(stderr, "Require s + b < 64.\n");
        return 1;
    }

    size_t set_count = (size_t)1 << s;  // 缓存组数：2^s

    /* 在乘法前检查总内存大小是否会溢出 */
    if (set_count > SIZE_MAX / sizeof(CacheLine) / (size_t)E) {
        fprintf(stderr, "Cache is too large.\n");
        return 1;
    }

    size_t line_count = set_count * (size_t)E;  // 总缓存行数

    // malloc：申请连续内存；cache 指向第一条缓存行
    CacheLine *cache = malloc(line_count * sizeof(CacheLine));

    if (cache == NULL) {
        fprintf(stderr, "Failed to allocate cache.\n");
        return 1;
    }

    /* 初始化：所有缓存行均为空 */
    for (size_t i = 0; i < line_count; i++) {  // i：缓存行数组下标
        cache[i].valid = 0;
        cache[i].tag = 0;
        cache[i].last_used = 0;
    }

    // fopen：以只读模式打开文件；trace 为文件流指针
    FILE *trace = fopen(trace_path, "r");

    if (trace == NULL) {
        perror(trace_path);  // 打印文件路径和打开失败的原因
        free(cache);
        return 1;
    }

    unsigned long long counter = 0;  // 访问序号
    int hits = 0;                    // 命中次数
    int misses = 0;                  // 未命中次数
    int evictions = 0;               // 驱逐次数

    char operation;               // 操作类型：I、L、S、M
    unsigned long long address;   // 访问的内存地址
    int size;                     // 访问字节数

    /* fscanf：读取操作、十六进制地址和长度 */
    while (fscanf(trace, " %c %llx,%d",
                  &operation, &address, &size) == 3) {
        if (operation == 'I') {
            continue;  // 忽略指令访问
        }

        unsigned long long set_mask = (1ULL << s) - 1; // 组索引掩码
        size_t set_index = (address >> b) & set_mask;  // 目标组号
        unsigned long long tag = address >> (s + b);  // 目标块标记

        // 指向目标组的第一条缓存行
        CacheLine *set = cache + set_index * (size_t)E;

        int hit_index = -1;    // 命中的组内行号，-1 表示未找到
        int empty_index = -1;  // 第一条空行的组内行号

        /* 查找命中行，同时记录第一条空行 */
        for (int i = 0; i < E; i++) {  // i：组内行号
            if (set[i].valid && set[i].tag == tag) {
                hit_index = i;
                break;
            }

            if (!set[i].valid && empty_index == -1) {
                empty_index = i;
            }
        }

        counter++;

        int accessed_index = -1;  // 本次访问最终使用的组内行号

        if (verbose) {
            printf("%c %llx,%d", operation, address, size);
        }

        if (hit_index != -1) {
            /* 命中：更新最近访问序号 */
            hits++;
            set[hit_index].last_used = counter;
            accessed_index = hit_index;

            if (verbose) {
                printf(" hit");
            }
        } else {
            /* 未命中：准备装入目标块 */
            misses++;

            if (verbose) {
                printf(" miss");
            }

            int target_index = empty_index;  // 新块将占用的行号

            if (target_index == -1) {
                /* 组已满：选择 last_used 最小的行 */
                target_index = 0;

                for (int i = 1; i < E; i++) {  // i：参与比较的行号
                    if (set[i].last_used <
                        set[target_index].last_used) {
                        target_index = i;
                    }
                }

                evictions++;

                if (verbose) {
                    printf(" eviction");
                }
            }

            /* 装入目标块 */
            set[target_index].valid = 1;
            set[target_index].tag = tag;
            set[target_index].last_used = counter;
            accessed_index = target_index;
        }

        /* M 的第二次访问：同一块已在缓存中，必定命中 */
        if (operation == 'M') {
            counter++;
            hits++;
            set[accessed_index].last_used = counter;

            if (verbose) {
                printf(" hit");
            }
        }

        if (verbose) {
            printf("\n");
        }
    }

    fclose(trace);  // 关闭文件流
    free(cache);    // 释放模拟缓存

    printSummary(hits, misses, evictions);  // 输出最终统计结果
    return 0;
}