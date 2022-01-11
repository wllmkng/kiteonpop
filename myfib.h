#ifndef _MYFIB_H_
#define _MYFIB_H_

#include "mytypes.h"
#include "map.h"

typedef struct MYFIB_ENTRY
{
    NAMES names;              //名字
    int port;                 //端口
    struct MYFIB_ENTRY *next; //链表下一项
    uint64_t time;            //时间戳
} MYFIB_ENTRY;

typedef struct
{
    struct entity *esw; //交换机
    int eswid;
    MYFIB_ENTRY *head; //链表头
} MYFIB;

void free_myfib(MYFIB *myfib)
{
    MYFIB_ENTRY *e = myfib->head;
    while (e)
    {
        MYFIB_ENTRY *pre = e;
        e = e->next;
        free_names(&(pre->names));
        free(pre);
    }
}
bool eq_myfib_f(int_or_ptr_t k1, int_or_ptr_t k2)
{
    return false;
}
void free_myfib_f(int_or_ptr_t myfib)
{
    free_myfib((MYFIB *)myfib.p);
    free(myfib.p);
}

BOOL eq_myfib_entry(MYFIB_ENTRY e1, MYFIB_ENTRY e2)
{
    return eq_names(e1.names, e2.names);
}

/*创建并初始化一个交换机的FIB表*/
MYFIB *myfib(struct entity *esw)
{
    MYFIB *fib = malloc(sizeof(MYFIB));
    fib->esw = esw;
    fib->eswid = entity_get_dpid(esw);
    fib->head = NULL;
    return fib;
}

/*寻找FIB表项与指定名字的匹配数量*/
BOOL myfib_match_name(MYFIB_ENTRY *e, NAMES names, uint32_t *match_num)
{
    *match_num = 0;
    NAMES e_name = e->names;
    for (int i = 0, j = 0; i < names.num && j < e_name.num; i++, j++)
    {
        if (eq_name_component(names.names[i], e_name.names[j]))
        {
            (*match_num)++;
        }
        else
        {
            break;
        }
    }
    if ((*match_num) == e_name.num)
        return True;
    return False;
}
/*在指定交换机的FIB表中根据名字查找表项*/
MYFIB_ENTRY *myfib_find_entry_by_name(MYFIB *myfib, NAMES names, BOOL *full_match)
{
    uint32_t match_num = 0; //匹配的名字组件个数
    uint32_t max_match_num = 0;
    MYFIB_ENTRY *e = myfib->head;
    MYFIB_ENTRY *ans = NULL;
    while (e)
    {
        *full_match = myfib_match_name(e, names, &match_num);
        if (match_num > max_match_num)
        {
            max_match_num = match_num;
            ans = e;
        }
        //向后移动
        e = e->next;
    }
    return ans;
}

/*在指定交换机的FIB表中添加一项*/
void myfib_add_entry(MYFIB *myfib, NAMES *names, int port, uint64_t time)
{
    MYFIB_ENTRY *n = malloc(sizeof(MYFIB_ENTRY));
    n->names = *names;
    n->port = port;
    n->time = time;
    BOOL full_match;
    MYFIB_ENTRY *old = myfib_find_entry_by_name(myfib, *names, &full_match);
    if (old != NULL && eq_myfib_entry(*n, *old)) //如果有相同的项就替换
    {
        old->port = port;
        old->time = time;
        for (int i = 0; i < old->names.num; i++)
        {
            if (names->names[i].value != old->names.names[i].value)
            {
                free(names->names[i].value);
            }
            names->names[i].value = old->names.names[i].value;
        }
        old->names = *names;
        free(n);
        return;
    }
    n->next = myfib->head;
    myfib->head = n;
}

typedef struct KITEFIB_ENTRY
{
    MYFIB *myfib;
    struct entity *esw;
    int eswid;
    struct KITEFIB_ENTRY *next;
} KITEFIB_ENTRY;

typedef struct
{
    NAMES trace_prefix;
    KITEFIB_ENTRY *head;
} KITEFIB;

bool eq_kitefib_f(int_or_ptr_t k1, int_or_ptr_t k2)
{
    return false;
}
void free_kitefib_f(int_or_ptr_t k)
{
    KITEFIB_ENTRY *e = ((KITEFIB *)(k.p))->head;
    while (e)
    {
        KITEFIB_ENTRY *pre = e;
        e = e->next;
        free_myfib(e->myfib);
        free(pre);
    }
}

typedef struct
{
    uint32_t num;     // trace_prefix的个数
    NAMES names[256]; //数组，保存所有的trace_prefix
} ALL_TRACE_PREFIX;

ALL_TRACE_PREFIX *all_tp()
{
    ALL_TRACE_PREFIX *atp = malloc(sizeof(ALL_TRACE_PREFIX));
    atp->num = 0;
    for (int i = 0; i < 256; i++)
    {
        atp->names[i].names = NULL;
        atp->names[i].num = 0;
    }
    return atp;
}
void all_tp_addtp(ALL_TRACE_PREFIX *atp, NAMES names)
{
    for (int i = 0; i < names.num; i++)
    {
        atp->names[atp->num].num += 1;
        atp->names[atp->num].names = (NAME_COMPONENT *)realloc(atp->names[atp->num].names, sizeof(NAME_COMPONENT) * atp->names[atp->num].num); //重新分配空间
        atp->names[atp->num].names[i] = names.names[i];
        atp->names[atp->num].names[i].value = malloc(sizeof(uint8_t) * names.names[i].len);
        memcpy(atp->names[atp->num].names[i].value, names.names[i].value, names.names[i].len); //复制内存
    }
    atp->num++;
    xinfo("KITE test all_tp_addtp\n");
}
bool eq_atp_f(int_or_ptr_t a1, int_or_ptr_t a2)
{
    return false;
}
void free_atp_f(int_or_ptr_t a)
{
    for (int i = 0; i < ((ALL_TRACE_PREFIX *)(a.p))->num; i++)
    {
        free_names(&(((ALL_TRACE_PREFIX *)(a.p))->names[i]));
    }
    free(a.p);
}

/*创建并初始化网络的某个追踪前缀对应的KITE FIB表*/
KITEFIB *nw_kitefib(NAMES trace_prefix)
{
    KITEFIB *fib = malloc(sizeof(KITEFIB));
    fib->trace_prefix = trace_prefix;
    fib->head = NULL;
    return fib;
}

/*向指定KITE FIB表中添加一个FIB表项*/
void kitefib_add_entry(KITEFIB *kitefib, struct entity *esw, NAMES *names, int port, uint64_t time)
{
    KITEFIB_ENTRY *e = kitefib->head;
    //xinfo("KITE test 3.36\n");
    while (e)
    {
        //xinfo("KITE test 3.37 %p\n", esw);
        if (e->eswid == entity_get_dpid(esw))
        {
            //xinfo("KITE test 3.38 eswid: %d\n", entity_get_dpid(esw));
            break;
        }
        e = e->next;
    }
    //xinfo("KITE test 3.39\n");
    if (e == NULL)
    {
        KITEFIB_ENTRY *n = malloc(sizeof(KITEFIB_ENTRY));
        n->esw = esw;
        n->eswid = entity_get_dpid(esw);
        n->myfib = myfib(esw);
        myfib_add_entry(n->myfib, names, port, time);
        n->next = kitefib->head;
        kitefib->head = n;
    }
    else
    {
        myfib_add_entry(e->myfib, names, port, time);
    }
}

/**/

#endif