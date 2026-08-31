#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <windows.h>

// 应用名称
#define APP_NAME "MEM_BAR"

/**
 * @brief 打印消息
 * @param tag 消息标签
 * @param format 格式化字符串
 * @param ... 不定长参数
 */
void print_msg(const char *tag, const char *format, ...)
{
    printf("[%s]: ", tag);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

// 内存块名称大小
#define NAME_SIZE 64
// 内存ROM类型识别关键字
const const char *rom_keyword_list[] = {"ROM", "Rom", "rom", "Flash", "FLASH", "flash", "RO_"};
// 内存RAM类型识别关键字
const const char *ram_keyword_list[] = {"ER$$", "RW_", "ER_RW", "ER_ZI", "RAM", "Ram", "ram"};

/**
 * @brief 内存类型枚举
 */
typedef enum mem_type_t
{
    MEM_UNKNOW = 0,
    MEM_ROM,
    MEM_RAM,
    MEM_TYPE_END
} mem_type_t;

/**
 * @brief 内存块描述信息结构体
 */
typedef struct mem_info_t
{
    // 内存块名称
    char name[NAME_SIZE];
    // 内存块类别
    mem_type_t type;
    // 内存块基地址
    uint32_t base;
    // 内存块使用量
    uint32_t used;
    // 内存块总大小
    uint32_t total;
    // 下一个节点指针
    void *next;
} mem_info_t;

/**
 * @brief 获取内存类型的字符串
 * @param type 文件类型枚举
 * @return 文件类型字符串
 */
const char *get_mem_type_str(mem_type_t type)
{
    switch (type)
    {
    case MEM_ROM:
        return "ROM";
    case MEM_RAM:
        return "RAM";
    default:
        break;
    }
    return "UNKNOW";
}

/**
 * @brief 通过文件名匹配关键字来识别内存类型
 * @param name 内存块名称
 * @return 内存类型
 */
mem_type_t identify_mem_type_by_name(const char *name)
{
    size_t rom_keyword_count = sizeof(rom_keyword_list) / sizeof(rom_keyword_list[0]);
    size_t ram_keyword_count = sizeof(ram_keyword_list) / sizeof(ram_keyword_list[0]);
    for (size_t i = 0; i < rom_keyword_count; i++)
    {
        if (strstr(name, rom_keyword_list[i]))
            return MEM_ROM;
    }
    for (size_t i = 0; i < ram_keyword_count; i++)
    {
        if (strstr(name, ram_keyword_list[i]))
            return MEM_RAM;
    }
    return MEM_UNKNOW;
}

/**
 * @brief 从map文件里搜索内存块
 * @param f 文件指针
 * @return 返回解析出来的内存信息节点链表头
 */
mem_info_t *search_mem_region(FILE *f)
{
#define ram_title "Execution Region "
#define base_title "Exec base: 0x"
#define used_title "Size: 0x"
#define total_title "Max: 0x"
    if (f == NULL)
        return NULL;
    mem_info_t *mem_info_head = NULL;
    mem_info_t *mem_info = NULL;
    size_t title_offset = strlen(ram_title);
    size_t base_offset = strlen(base_title);
    size_t used_offset = strlen(used_title);
    size_t total_offset = strlen(total_title);
    char line[1024];
    uint32_t row_cnt = 0;
    uint32_t row_record = 0xffffffff;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        char *title_pos = strstr(line, ram_title);
        row_cnt++;
        if (title_pos != NULL)
        {
            char *info_pos = strchr(title_pos, '(');
            if (mem_info_head == NULL)
            {
                mem_info_head = (mem_info_t *)malloc(sizeof(mem_info_t));
                mem_info = mem_info_head;
            }
            else
            {
                mem_info->next = (mem_info_t *)malloc(sizeof(mem_info_t));
                mem_info = mem_info->next;
            }
            // 保存RAM名称
            size_t name_len = info_pos - title_pos - title_offset - 1;
            name_len = name_len >= (NAME_SIZE - 1) ? (NAME_SIZE - 1) : name_len;
            strncpy(mem_info->name, title_pos + title_offset, name_len);
            mem_info->name[name_len] = '\0';
            // 解析并保存RAM信息
            char *base_pos = strstr(info_pos, base_title);
            char *used_pos = strstr(info_pos, used_title);
            char *total_pos = strstr(info_pos, total_title);
            sscanf(base_pos, base_title "%x", &mem_info->base);
            sscanf(used_pos, used_title "%x", &mem_info->used);
            sscanf(total_pos, total_title "%x", &mem_info->total);
            // 识别内存类型
            mem_info->type = identify_mem_type_by_name(mem_info->name);
            mem_info->next = NULL;
            row_record = row_cnt;
        }
        // 判断RAM类型
        if (mem_info != NULL && mem_info->type == MEM_UNKNOW && row_cnt > row_record && row_cnt < (row_record + 5))
        {
            if (strstr(line, " RW "))
            {
                mem_info->type = MEM_RAM;
            }
            else if (strstr(line, " RO "))
            {
                mem_info->type = MEM_ROM;
            }
        }
    }
    return mem_info_head;
}

/**
 * @brief 交换两个节点的值，但是next指针不变
 * @param a A节点
 * @param b B节点
 */
void swap_mem_info_value(mem_info_t *a, mem_info_t *b)
{
    mem_info_t temp_info;
    mem_info_t *a_next = a->next;
    mem_info_t *b_next = b->next;
    if (a != b)
    {
        temp_info = *a;
        memcpy(&temp_info, a, sizeof(mem_info_t));
        memcpy(a, b, sizeof(mem_info_t));
        memcpy(b, &temp_info, sizeof(mem_info_t));
    }
    a->next = a_next;
    b->next = b_next;
}

/**
 * @brief 排序条件
 * @param a 当前节点
 * @param b 下一节点
 * @return 是否需要交换值
 */
uint8_t sort_condition_by_type_base(mem_info_t *a, mem_info_t *b)
{
    if (a->type != b->type)
        return a->type > b->type;
    return a->base > b->base;
}

/**
 * @brief 内存信息节点排序
 * @param list 待排序的列表
 * @param condition 排序条件判断函数,返回真值表示需要交换两个节点值
 */
void mem_info_list_sort(mem_info_t *list, uint8_t (*condition)(mem_info_t *a, mem_info_t *b))
{
    mem_info_t *current, *next;

    for (current = list; current != NULL; current = current->next)
    {
        for (next = current->next; next != NULL; next = next->next)
        {
            if (condition(current, next))
            {
                swap_mem_info_value(current, next);
            }
        }
    }
}

#define USED_ICON_932_SHIFT_JIS \
    (char[]) { 0x81, 0xa1, 0x00 }
#define USED_ICON_936_GB2312 \
    (char[]) { 0xa1, 0xf6, 0x00 }
#define USED_ICON_949_KR \
    (char[]) { 0xa1, 0xe1, 0x00 }
#define USED_ICON_950_BIG5 \
    (char[]) { 0xa1, 0xbd, 0x00 }
#define USED_ICON_1361_JOHAB \
    (char[]) { 0xd9, 0x71, 0x00 }
#define USED_ICON_95001_UTF8 \
    (char[]) { 0xe2, 0x96, 0xa0, 0x00 }

int main(int argc, char const *argv[])
{
    char *used_char = "#";
    uint32_t console_cp = GetConsoleOutputCP();
    print_msg(APP_NAME, "Console cp is %d", console_cp);
    switch (console_cp)
    {
    case 936:
        used_char = USED_ICON_936_GB2312;
        break;
    case 932:
        used_char = USED_ICON_932_SHIFT_JIS;
        break;
    case 949:
        used_char = USED_ICON_949_KR;
        break;
    case 950:
        used_char = USED_ICON_950_BIG5;
        break;
    case 1361:
        used_char = USED_ICON_1361_JOHAB;
        break;
    case 65001:
        used_char = USED_ICON_95001_UTF8;
        break;
    default:
        print_msg(APP_NAME, "No supported encoding found, use '#' as used icon");
        break;
    }
    print_msg(APP_NAME, "Open map %s", argv[1]);
    FILE *fp_map = fopen(argv[1], "r+");
    if (!fp_map)
    {
        print_msg(APP_NAME, "%s open failed!", argv[1]);
        return -1;
    }
    mem_info_t *mem_region_list = search_mem_region(fp_map);
    mem_info_list_sort(mem_region_list, sort_condition_by_type_base);
    if (mem_region_list)
    {
        uint8_t type_shown[MEM_TYPE_END] = {0};
        printf("Memory Address Range |     Status Bar     |  Rate        Used Size             Total Size       Name\n");
        for (mem_info_t *mem_rg = mem_region_list, *next = NULL; mem_rg != NULL; next = mem_rg->next, free(mem_rg), mem_rg = next)
        {
            if (!type_shown[mem_rg->type])
            {
                type_shown[mem_rg->type] = 1;
                printf("%s:\n", get_mem_type_str(mem_rg->type));
            }
            printf("0x%08X-%08X: |", mem_rg->base, mem_rg->base + mem_rg->total);
            for (uint32_t i = 0, pos = (mem_rg->used * 20.0 + 0.5) / mem_rg->total; i < 20; i++)
            {
                printf(i < pos ? used_char : "_");
            }
            printf("|%5.1f%%  %7.2lfKiB(%08X)   %7.2lfKiB(%08X)  %s\n",
                   mem_rg->used * 100.0 / mem_rg->total,
                   mem_rg->used / 1024.0, mem_rg->used,
                   mem_rg->total / 1024.0, mem_rg->total,
                   mem_rg->name);
        }
    }
    else
    {
        print_msg(APP_NAME, "Parse map file failed!");
        return -2;
    }
    fclose(fp_map);
    return 0;
}
