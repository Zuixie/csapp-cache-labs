#include "cachelab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <getopt.h>

#define SUCCESS 0
#define ERROR_CACHE_NULL 0x01
#define ERROR_CACHE_INIT_ARGS 0x02
#define ERROR_CACHE_INIT_MALLOC_SET_PTR 0x03
#define ERROR_CACHE_INIT_MALLOC_SET 0x04
#define ERROR_CACHE_INIT_MALLOC_LINE 0x05
#define ERROR_CACHE_INIT_MALLOC_BLOCK 0x06
#define ERROR_COMMAND_INIT_MALLOC_COMMAND 0x07
#define ERROR_COMMAND_INIT_COMMAND_NULL 0x08
#define ERROR_COMMAND_INIT_FILENAME_NULL 0x09
#define ERROR_COMMAND_INIT_FILE_OPEN 0x0a
#define ERROR_COMMAND_INIT_FILE_CLOSE 0x0b
#define ERROR_COMMAND_INIT_READLINE 0x0c
#define ERROR_COMMAND_INIT_UDEF_CMD 0x0d
#define ERROR_COMMAND_INIT_UDEF_OP 0x0e
#define ERROR_EXEC_COMMAND_NULL 0x10
#define ERROR_EXEC_CACHE_NULL 0x11
#define ERROR_EXEC_UDEF_OP 0x12
#define ERROR_PARSE_ARGS_NULL 0x13
#define ERROR_PARSE_ARGS_FILENAME_LONG 0x14

#define COMMAND_RESULT_NONE 0x00 // 00b
#define COMMAND_RESULT_MISS_INT 0x01 // 01b
#define COMMAND_RESULT_MISS_STR "miss" // 01b
#define COMMAND_RESULT_HIT_INT 0x02 // 10b
#define COMMAND_RESULT_HIT_STR "hit" // 10b
#define COMMAND_RESULT_EVICTION_INT 0x03 // 11b
#define COMMAND_RESULT_EVICTION_STR "eviction" // 11b

#define OP_INS_LOAD 'I'
#define OP_DATA_STORE 'S'
#define OP_DATA_LOAD 'L'
#define OP_DATA_MODIFY 'M'

#define ADDRESS_BIT_NUM 64
#define MAX_LINE_BUFF 1024
#define MAX_FILE_NAME 256

struct ArgsOpt
{
    int verbose;
    int set_bit_num;
    int line_num;
    int block_bit_num;
    char filename[MAX_FILE_NAME];
};

struct Command
{
    unsigned char op;
    char vaild;
    unsigned short size;
    unsigned short result_count;
    unsigned long result; // 0x9 -> 001001b -> 01(miss), 10(hit), 00(end) -> parse from right to left   
    unsigned long address;
    struct Command* next; // link next command, end with NULL
};

struct Line
{
    char vaild;
    unsigned long tag;
    char *block;
    struct Line *next;
};

struct Set
{
    struct Line *line_head;
};

struct Cache 
{
    int set_bit_num; // s
    int tag_bit_num; // t
    int address_bit_num; // m
    int block_bit_num; // b
    unsigned int line_num; // E
    unsigned long set_num; // S
    unsigned long block_num; // B 

    struct Set* set_arr;
};


// 1. 解析输入的命令 结构化
extern char *optarg;

int parse_args_opt(int argc, char **argv, struct ArgsOpt *argsOpt); // TODO
void usage();

int command_malloc(struct Command **command);
int fill_command(struct Command *command, char* str);
int parse_command_with_file(struct Command *command, char *filename);

int deinit_command(struct Command *command);

// 2. 运行输入的命令(再展开)

// 2.1 根据输入初始化缓存系统(cache)
static int init_cache_args(struct Cache *cache, int s, int E, int b, int m);
static int init_cache_malloc_line(struct Line **line_pptr, unsigned long block_num);
int init_cache(struct Cache *cache, int s, int E, int b, int m);
int deinit_cache(struct Cache *cache);

// 2.2 根据输入的操作类型
static int execute_data_load(unsigned long tag, unsigned long set_index, struct Command *cmd, struct Cache *cache);
static int execute_data_modify(unsigned long tag, unsigned long set_index, struct Command *cmd, struct Cache *cache);
static int execute_data_store(unsigned long tag, unsigned long set_index, struct Command *cmd, struct Cache *cache);
static int add_result_to_cmd(struct Command *cmd, unsigned long result);
int execute(struct Command *command_head, struct Cache *cache);


// 3. 输出或统计结果
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;
void print_detail(struct Command *command);

#ifdef DEBUG
void test_cache_init()
{
    int ret;
    struct Cache cache;
    ret = init_cache(&cache, 4, 4, 4, ADDRESS_BIT_NUM);
    printf("ret:%d\n", ret);
    if (ret == SUCCESS)
    {
        printf("t:%d; S:%lu; B:%lu\n", cache.tag_bit_num, cache.set_num, cache.block_num);
        printf("vaild:%d\n", cache.set_arr[0].line_head->next->next->vaild);
    }
    deinit_cache(&cache);

}

void test_command_init()
{
    struct Command* command = NULL;
    struct Command* temp;
    int ret;

    if (command_malloc(&command) != SUCCESS) 
    {
        printf("malloc fail\n");
        return;
    }

    ret = parse_command_with_file(command, "./traces/yi.trace");
    printf("ret:%d\n", ret);

    if (ret != 0)
    {
        return;
    }
    
    temp = command->next;
    while(temp != NULL) 
    {
        printf("vaild:%d, op:%c, ad:%lx\n", temp->vaild, temp->op, temp->address);
        temp = temp->next;
    }

    deinit_command(command);
    free(command);

}

void test_exec()
{
    // 1. cache init
    int ret;
    struct Cache cache;
    ret = init_cache(&cache, 4, 1, 4, ADDRESS_BIT_NUM);
    printf("cache init ret:%d\n", ret);

    // 2. cmd init
    struct Command* command = NULL;

    if (command_malloc(&command) != SUCCESS) 
    {
        printf("malloc fail\n");
        return;
    }

    ret = parse_command_with_file(command, "./traces/yi.trace");
    printf("cmd init ret:%d\n", ret);

    // 3. exec
    ret = execute(command, &cache);
    printf("exec ret:%d\n", ret);

    print_detail(command);
}

void test_args(int argc, char**argv)
{
    int ret;
    struct ArgsOpt opt;
    ret = parse_args_opt(argc, argv, &opt);
    printf("ret:%d\n", ret);
    printf("v:%d s:%d b:%d E:%d t:%s\n", opt.verbose, opt.set_bit_num, opt.block_bit_num, opt.line_num, opt.filename);
}
#endif

int main(int argc, char** argv)
{
#ifdef DEBUG
    // test_cache_init();
    // test_command_init();
    // test_exec();
    // test_args(argc, argv);
    printf("this is not\n");
    
#endif
    int ret;
    struct ArgsOpt *opt;
    struct Command* command;
    struct Cache cache;

    if ((opt = (struct ArgsOpt*)malloc(sizeof(struct ArgsOpt))) == NULL) 
    {
        printf("malloc fail\n");
        exit(0);
    }

    if ((ret = parse_args_opt(argc, argv, opt)) != SUCCESS) 
    {
        printf("parse args fail %d\n", ret);
        return 1;
    }

    // init cache
    ret = init_cache(&cache, opt->set_bit_num, opt->line_num, opt->block_bit_num, ADDRESS_BIT_NUM);
    if (ret != SUCCESS) 
    {
        printf("init cache fail %d\n", ret);
        return 1;
    }

    // init command
    if (command_malloc(&command) != SUCCESS) 
    {
        printf("malloc fail\n");
        return 1;
    }

    ret = parse_command_with_file(command, opt->filename);
    if (ret != SUCCESS) 
    {
        printf("init parse command fail %d\n", ret);
        return 1;
    }

    // exec
    ret = execute(command, &cache);
    if (ret != SUCCESS) 
    {
        printf("exec fail %d\n", ret);
        return 1;
    }

    if (opt->verbose == 1) 
    {
        print_detail(command);
    }

    printSummary(hit_count, miss_count, eviction_count);
    
    free(command);
    free(opt);
    
    return 0;
}

void usage()
{
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
    printf("\t-h: Optional help flag that prints usage info\n");
    printf("\t-v: Optional verbose flag that displays trace info\n");
    printf("\t-s <s>: Number of set index bits (S = 2s is the number of sets)\n");
    printf("\t-E <E>: Associativity (number of lines per set)\n");
    printf("\t-b <b>: Number of block bits (B = 2b is the block size)\n");
    printf("\t-t <tracefile>: Name of the valgrind trace to replay\n");
    
    exit(0);
}

int parse_args_opt(int argc, char **argv, struct ArgsOpt *argsOpt)
{
    int len;
    char ch;
    if (argsOpt == NULL)
    {
        return ERROR_PARSE_ARGS_NULL;
    }
    if ((argc == 1) || (argc > 2 && argc < 9)) 
    {
        usage();
    }

    while ((ch = getopt(argc, argv, "hvs:E:b:t:")) != -1) 
    {
        switch (ch) 
        {
            case 'v':
                argsOpt->verbose = 1;
                break;
            case 's':
                argsOpt->set_bit_num = atoi(optarg);
                break;
            case 'E':
                argsOpt->line_num = atoi(optarg);
                break;
            case 'b':
                argsOpt->block_bit_num = atoi(optarg);
                break;
            case 't':
                len = strlen(optarg);
                if (len + 1 > MAX_FILE_NAME)
                {
                    return ERROR_PARSE_ARGS_FILENAME_LONG;
                }
                memcpy(argsOpt->filename, optarg, len+1);
                break;
            case 'h':
            case '?':
            default:
                usage();
        }
    }

    return SUCCESS;
    
}

int command_malloc(struct Command **command)
{
    struct Command *ptr;
    if ((ptr = (struct Command*)malloc(sizeof(struct Command))) == NULL) 
    {
        return ERROR_COMMAND_INIT_MALLOC_COMMAND;
    }

    ptr->op = 0;
    ptr->vaild = 0;
    ptr->size = 0;
    ptr->result = 0;
    ptr->result_count = 0;
    ptr->address = 0;
    ptr->next = NULL;

    *command = ptr;
    return SUCCESS;
}

int fill_command(struct Command *command, char* str) 
{
    char *temp;
    int len = strlen(str);
    if (str[len-1] == '\n')
        str[len-1] = '\0';
    // delete head space
    while((*str) && *str == ' ')  str++;
    
    // get op
    if ((temp = strchr(str, ' ')) == NULL) 
    {
        return ERROR_COMMAND_INIT_UDEF_CMD;
    }
    if ((temp - str) != 1) 
    {
        return ERROR_COMMAND_INIT_UDEF_CMD;
    }
    switch (str[0])
    {
        case OP_INS_LOAD:
            command->vaild = 0;
            break;
        case OP_DATA_LOAD:
        case OP_DATA_MODIFY:
        case OP_DATA_STORE:
            command->vaild = 1;
            break;
        default:
            return ERROR_COMMAND_INIT_UDEF_OP;
    }
    command->op = str[0];
    
    // get address and size
    str = temp;
    while((*str) && *str == ' ')  str++;
    if ((temp = strchr(str, ',')) == NULL) 
    {
        return ERROR_COMMAND_INIT_UDEF_CMD;
    }
    *temp = 0; 
    command->address = strtoul(str, NULL, 16);
    command->size = atoi(temp + 1);
    return SUCCESS;
}

int parse_command_with_file(struct Command *command, char *filename)
{
    if (command == NULL)
    {
        return ERROR_COMMAND_INIT_COMMAND_NULL;
    }

    if (filename == NULL)
    {
        return ERROR_COMMAND_INIT_FILENAME_NULL;
    }

    int ret = 0;
    FILE *fp;
    char buff[MAX_LINE_BUFF];
    struct Command *command_temp;
    // open file 
    if((fp = fopen(filename, "r")) == NULL)
    {
        return ERROR_COMMAND_INIT_FILE_OPEN;
    }
    // read line
    while(!feof(fp))
    {
        if ((fgets(buff, MAX_LINE_BUFF, fp)) == NULL)
        {
            if (!feof(fp))
                return ERROR_COMMAND_INIT_READLINE;
            else
                continue;
        }
        if (buff[0] == '\n' || buff[0] == '\0') 
        {
            continue;
        }
        // malloc command
        if ((ret = command_malloc(&command_temp)) != SUCCESS)
        {
            return ret;
        }
        // fill command
        if ((ret = fill_command(command_temp, buff)) != SUCCESS) 
        {
            return ret;
        }
        command->next = command_temp;
        command = command->next;
    }

    // close file
    if (fclose(fp))
    {
        return ERROR_COMMAND_INIT_FILE_CLOSE;
    }

    return SUCCESS;    
}

int deinit_command(struct Command *command)
{
    if (command == NULL) 
        return SUCCESS;
    command = command->next;
    struct Command *temp;
    
    while(command != NULL) 
    {
        temp = command;
        command = command->next;
        free(temp);
    }
    
    return SUCCESS;
}

static int init_cache_args(struct Cache *cache, int s, int E, int b, int m)
{
    if (s < 0 || m <= 0 || b < 0 || E <= 0 || (s+b > m))
    {
        return ERROR_CACHE_INIT_ARGS;
    }
    
    cache->set_bit_num = s;
    cache->set_num = 1 << s;
    cache->block_bit_num = b;
    cache->block_num = 1 << b;
    cache->address_bit_num = m;
    cache->tag_bit_num = m - s - b;
    cache->line_num = E;

    return SUCCESS;
}

static int init_cache_malloc_line(struct Line **line_pptr, unsigned long block_num) 
{
    struct Line *ptr = NULL;
    if ((ptr = (struct Line*)malloc(sizeof(struct Line))) == NULL) 
    {
        return ERROR_CACHE_INIT_MALLOC_LINE;
    }

    // malloc block
    if ((ptr->block = (char*)malloc(block_num)) == NULL)
    {
        return ERROR_CACHE_INIT_MALLOC_BLOCK;
    }
    ptr->vaild = 0;
    ptr->next = NULL;

   *line_pptr = ptr;

    return SUCCESS;
}

int init_cache(struct Cache *cache, int s, int E, int b, int m)
{
    int err = SUCCESS;

    if (cache == NULL)
    {
        return ERROR_CACHE_NULL;
    }

    if ((err = init_cache_args(cache, s, E, b, m)) != SUCCESS) 
    {
        return err;
    }

    unsigned long set_num = cache->set_num;
    unsigned int line_num = cache->line_num;
    unsigned long block_num = cache->block_num;
    struct Set* set_arr = NULL;
    
    // malloc sets
    if((set_arr = (struct Set*)malloc(set_num * sizeof(struct Set))) == NULL)
    {
        return ERROR_CACHE_INIT_MALLOC_SET;
    }
    
    // malloc line & block
    for (int i = 0; i < set_num; i++)
    {
        // malloc line && create link
        struct Line* line_head = NULL;
        struct Line* line_ptr = NULL;

        if ((err = (init_cache_malloc_line(&line_head, block_num))) != SUCCESS)
        {
            return err;
        }

        line_ptr = line_head;
        for (int j = 1; j < line_num; j++)
        {
            struct Line *temp;
            if ((err = (init_cache_malloc_line(&temp, block_num))) != SUCCESS)
            {
                return err;
            }
            line_ptr->next = temp;
            line_ptr = line_ptr->next;
        }

        set_arr[i].line_head = line_head;
    }
    cache->set_arr = set_arr;

    return SUCCESS;
}

int deinit_cache(struct Cache *cache)
{
    if (cache == NULL || cache->set_arr == NULL) 
        return SUCCESS;
    
    // free sets
    int set_num = cache->set_num;
    struct Set* set_ptr = cache->set_arr;
    for (int i = 0; i < set_num; i++)
    {
        struct Line *line_head = set_ptr[i].line_head;
        struct Line *line_temp = NULL;
        // free line links
        while(line_head != NULL)
        {
            line_temp = line_head;
            line_head = line_head->next;

            if (line_temp->block != NULL)
            {
                free(line_temp->block);
            }
            free(line_temp);
        }

    }
    free(set_ptr);
    cache->set_arr = NULL;

    return SUCCESS;
}

static int execute_each(struct Command* cmd, struct Cache *cache)
{
    if (cmd->vaild == 0) 
    {
        return SUCCESS;
    }

    unsigned char op = cmd->op;
    unsigned long address = cmd->address;
    unsigned long s = cache->set_bit_num;
    unsigned long b = cache->block_bit_num;
    int tag_bit_num = cache->tag_bit_num;
    int ret;

    unsigned long tag = address >> (s + b);
    unsigned long set_index = address << tag_bit_num >> (tag_bit_num + b);

    switch(op)
    {
        case OP_DATA_LOAD:
            if ((ret = execute_data_load(tag, set_index, cmd, cache)) != SUCCESS)
                return ret;
            break;
        case OP_DATA_STORE:
            if ((ret = execute_data_store(tag, set_index, cmd, cache)) != SUCCESS)
                return ret;  
            break;
        case OP_DATA_MODIFY:
            if ((ret = execute_data_modify(tag, set_index, cmd, cache)) != SUCCESS)
                return ret;
            break;
        default:
            return ERROR_EXEC_UDEF_OP;
    }
    
    return SUCCESS;
}

int execute(struct Command *command_head, struct Cache *cache)
{
    if (command_head == NULL) 
        return SUCCESS;
    if (cache == NULL)
        return ERROR_CACHE_NULL;

    struct Command *cmd = command_head->next;
    int ret = 0;

    while (cmd != NULL)
    {
        if ((ret = execute_each(cmd, cache)) != SUCCESS) 
        {
            return ret;
        }
        cmd = cmd->next;
    }
    return 0;
}

static int add_result_to_cmd(struct Command *cmd, unsigned long result)
{
    int ret_count = cmd->result_count;
    unsigned long cur_result = cmd->result;
    cmd->result = cur_result | (result << (ret_count << 1));
    cmd->result_count = ret_count + 1;

    return SUCCESS;
}

static int execute_data_load(unsigned long tag, unsigned long set_index, struct Command *cmd, struct Cache *cache)
{
    // printf("exec load tag:%lu set:%lu\n", tag, set_index);
    struct Line *line_head = cache->set_arr[set_index].line_head;

    struct Line *pre_line = NULL;
    struct Line *cur_line = line_head;
    unsigned long result = COMMAND_RESULT_MISS_INT; 

    while(cur_line->next !=  NULL) 
    {
        if (cur_line->vaild == 0)
        {
            // miss
            break;
        }
        if (cur_line->tag == tag) 
        {
            // hit
            result = COMMAND_RESULT_HIT_INT;
            break;
        }

        pre_line = cur_line;
        cur_line = cur_line->next;
    }

    // check last one

    if (result == COMMAND_RESULT_HIT_INT || (cur_line->vaild != 0 && cur_line->tag == tag))
    {
        add_result_to_cmd(cmd, COMMAND_RESULT_HIT_INT);
        hit_count++;
        // printf("hit\n");
    }
    else if (result == COMMAND_RESULT_MISS_INT) 
    {
        add_result_to_cmd(cmd, COMMAND_RESULT_MISS_INT);
        miss_count++;
        // printf("miss\n");
        
        if (cur_line->vaild != 0) 
        {
            // cur_line vaild is true -> need eviction
            add_result_to_cmd(cmd, COMMAND_RESULT_EVICTION_INT);
            eviction_count++;
            // printf("eviction\n");
        }

        cur_line->vaild = 1;
        cur_line->tag = tag;
        // handle block ...
    }

    if (line_head != cur_line)
    {
        // move to head
        pre_line->next = cur_line->next;
        cur_line->next = line_head;
        line_head = cur_line;
        cache->set_arr[set_index].line_head = line_head;
    }

    return SUCCESS;
}

static int execute_data_modify(unsigned long tag, unsigned long set_index, struct Command *cmd, struct Cache *cache)
{
    execute_data_load(tag, set_index, cmd, cache);
    execute_data_store(tag, set_index, cmd, cache);
    return 0;
}
static int execute_data_store(unsigned long tag, unsigned long set_index, struct Command *cmd, struct Cache *cache)
{
    execute_data_load(tag, set_index, cmd, cache);
    return 0;
}

void print_detail(struct Command *command)
{
    if (command == NULL) 
    {   
        return;
    }
    unsigned long result = 0;
    unsigned long result_one = 0;
    
    while(command != NULL) 
    {
        if (command->vaild == 0) 
        {
            command = command->next;
            continue;
        }
        printf("%c %lx,%d", command->op, command->address, command->size);
        result = command->result;
        while(result != 0) 
        {
            result_one = result & 0x03;
            switch (result_one)
            {
                case COMMAND_RESULT_HIT_INT:
                    printf(" %s", COMMAND_RESULT_HIT_STR);
                    break;
                case COMMAND_RESULT_MISS_INT:
                    printf(" %s", COMMAND_RESULT_MISS_STR);
                    break;
                case COMMAND_RESULT_EVICTION_INT:
                    printf(" %s", COMMAND_RESULT_EVICTION_STR);
                    break;
            }
            result >>= 2;
        }
        printf("\n");
        command = command->next;
    }
}