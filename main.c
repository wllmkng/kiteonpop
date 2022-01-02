#include "xlog/xlog.h"
#include "pop_api.h"
#include "parser.h"
#include "myfib.h"
#include "utils.h"
#include "route.h"
#include "cppfuncs.h"

#include <time.h>

#define NDN_TYPE_INTEREST 0x01
#define NDN_TYPE_DATA 0x02
#define NDN_TYPE_TRACEDATA 0x03

/*TODO 计算名字对应的哈希*/
uint8_t calc_hash(NAME_COMPONENT name)
{
    uint8_t hash = calc_hash_cpp(name.value, name.len);
    //xinfo("KITE test hash=%.2x\n", hash);
    return hash;
}

/*WORKING 匹配数据包中的字段，寻找是否与某个保存过的追踪前缀相符*/
BOOL match_trace_prefix(struct packet *pkt, struct map *env, ALL_TRACE_PREFIX *atp, NAMES *trace_prefix)
{
    char field_names[16][12];
    BOOL flag = False;
    for (int i = 0; i < 16; i++)
    {
        memcpy(field_names[i], "name_hash_", strlen("name_hash_"));
        field_names[i][strlen("name_hash_")] = 'a' + i;
        field_names[i][strlen("name_hash_") + 1] = '\0';
    }
    for (int i = 0; i < atp->num && !flag; i++)
    {
        //比较记录过的第i个追踪前缀
        NAMES tp = atp->names[i];
        flag = False;
        for (int j = 0; j < tp.num && j < 4; j++) //POP assertion num(fields)<8
        {
            //xinfo("KITE test hash=%.2x\n", calc_hash(tp.names[j]));
            if (!test_equal(pkt, field_names[j], value_from_8l(calc_hash(tp.names[j]))))
            {
                //xinfo("KITE test false hash=%.2x\n", value_from_8l(calc_hash(tp.names[j])));
                flag = False;
                break;
            }
            if (j == tp.num - 1)
            {
                flag = True;
                *trace_prefix = tp;
            }
        }
    }
    //xinfo("KITE test flag=%d\n", flag);
    return flag;
}

/*根据之前处理TD保存在kitefib中的信息生成一条路由路径*/
struct route *kite_gen_trace_route(struct packet *pkt, struct map *env, KITEFIB *kitefib)
{
    struct route *r = route();
    KITEFIB_ENTRY *ke = kitefib->head;
    int in_port = read_packet_inport(pkt);
    struct entity *in_esw = read_packet_inswitch(pkt);
    route_add_edge(r, edge(NULL, 0, in_esw, in_port)); //在路径中添加一条边
    xinfo("KITE route ((%d,%d)->(%d,%d)) added\n", 0, 0, entity_get_dpid(in_esw), in_port);
    while (ke)
    {
        MYFIB *myfib = ke->myfib;
        int eswid = ke->eswid;
        struct entity *esw = get_switch(eswid);
        if (myfib == NULL)
        {
            ke = ke->next;
            continue;
        }
        MYFIB_ENTRY *me = myfib->head;
        while (me)
        {
            int out_port = me->port; //该交换机的出端口号
            /*
            struct entity *out_esw;  //应发送到的entity
            int out_esw_in_port;     // out_esw的入端口号
            int n;
            BOOL flag = False;
            
            const struct entity_adj *adjs = get_entity_adjs(esw, &n);
            //xinfo("KITE test adj_num=%d\n", n);
            for (int i = 0; i < n; i++)
            {
                //xinfo("KITE test adj\n");
                if (adjs[i].out_port == out_port)
                {
                    out_esw = adjs[i].adj_entity;
                    out_esw_in_port = adjs[i].adj_in_port;
                    flag = True;
                    break;
                }
            }
            if (flag)
            {
                route_add_edge(r, edge(NULL, 0, esw, out_port)); //在路径中添加一条边
                xinfo("KITE route ((%d,%d)->(%d,%d)) added\n", 0, 0, entity_get_dpid(esw), out_port);
                route_add_edge(r, edge(esw, out_port, out_esw, out_esw_in_port)); //在路径中添加一条边
                xinfo("KITE route ((%d,%d)->(%d,%d)) added\n", entity_get_dpid(esw), out_port, entity_get_dpid(out_esw), out_esw_in_port);
            }
            */
            route_add_edge(r, edge(esw, out_port, NULL, 0)); //在路径中添加一条边
            xinfo("KITE route ((%d,%d)->(%d,%d)) added\n", entity_get_dpid(esw), out_port, 0, 0);
            me = me->next;
        }
        ke = ke->next;
    }
    return r;
}

void init_f(struct map *env)
{
    xinfo("f init\n");
}

struct route *f(struct packet *pkt, struct map *env)
{
    struct route *r = NULL;

    struct entity *esw = read_packet_inswitch(pkt);
    xinfo("KITE packet received\n");
    entity_print(esw);

    /* inspect network header */
    pull_header(pkt);
    xinfo("KITE read ethernet head\n");
    /*
    uint64_t dst = value_to_48(read_packet(pkt, "dl_dst"));
    uint64_t src = value_to_48(read_packet(pkt, "dl_src"));
    xinfo("KITE dst MAC: %x\n", dst);
    xinfo("KITE src MAC: %x\n", src);
    */
    /* call handler */
    if (strcmp(read_header_type(pkt), "ndn") == 0)
    {
        xinfo("KITE ndn\n");
    }
    else
    {
        xinfo("KITE unknown\n");
        xinfo("KITE unknown protocol: %s.\n", read_header_type(pkt));
        r = route();
        return r;
    }

    if (test_equal(pkt, "type", value_from_8(NDN_TYPE_TRACEDATA))) //是TraceData
    {

        /*DONE KITE处理TD：
               提取追踪标签
               将名字和接口信息保存起来
        */
        // data：指向一个DATA包的开头
        // esw：提交PacketIn的交换机
        // port：POF交换机上有状态转发模块从PIT中找到的转发端口
        // FIXME 应从NDN前置头中提取这些信息
        int length;
        //xinfo("KITE test -1\n");
        uint8_t *data = read_payload(pkt, &length);
        //xinfo("KITE test 0\n");
        int port = value_to_32(read_packet(pkt, "data_out_ports"));
        port = big_to_native(&port, 4);
        //xinfo("KITE test 1\n");
        //从DATA包中读取所有的名字
        NAMES names;
        uint32_t len = 0; //处理的总字节数，在读取数据包内容的函数调用过程中传递，data+len始终指向当前待处理的位置
        if (!extract_name_from_data(data, &names, &len))
        {
            //错误
            r = route();
            free_names(&names);
            return r; //返回空路径
        }
        //xinfo("KITE test 2\n");
        //查找是否含有追踪标签
        int trace_tag_index;
        if ((trace_tag_index = find_trace_tag(names)) < 0)
        {
            //未找到追踪标签
            //跳出
            r = route();
            free_names(&names);
            return r; //返回空路径
        }
        //xinfo("KITE test 3\n");
        //提取追踪前缀并保存
        NAMES trace_prefix;
        extract_trace_prefix(names, &trace_prefix);
        //test
        uint8_t tmp_str[1024];
        name_to_str(trace_prefix, tmp_str);
        xinfo("KITE prefix extracted: %s\n", tmp_str);

        int_or_ptr_t atp = map_read(env, PTR("all_trace_prefix"));
        if (atp.p == NULL)
        {
            ALL_TRACE_PREFIX *n_atp = all_tp();
            all_tp_addtp(n_atp, trace_prefix);
            map_add_key(env, PTR("all_trace_prefix"), PTR(n_atp), eq_atp_f, free_atp_f);
        }
        else
        {
            BOOL flag = True;
            ALL_TRACE_PREFIX *f_atp = (ALL_TRACE_PREFIX *)(atp.p);
            for (int i = 0; i < f_atp->num; i++)
            {
                if (eq_names(trace_prefix, f_atp->names[i]))
                {
                    flag = False;
                    break;
                }
            }
            if (flag)
            {
                all_tp_addtp(f_atp, trace_prefix);
            }
        }

        //test start
        atp = map_read(env, PTR("all_trace_prefix"));
        if (atp.p != NULL)
        {
            ALL_TRACE_PREFIX *f_atp = (ALL_TRACE_PREFIX *)(atp.p);
            for (int i = 0; i < f_atp->num; i++)
            {
                name_to_str(f_atp->names[i], tmp_str);
                xinfo("KITE prefix stored[%d]: %s\n", i, tmp_str);
            }
        }
        else
        {
            xinfo("KITE prefix store failed\n");
        }
        //test end

        char kitefibkey[1024];
        memcpy(kitefibkey, "kitefib:", strlen("kitefib:"));
        //xinfo("KITE test 3.1\n");
        name_to_str(trace_prefix, kitefibkey + strlen("kitefib:"));
        //xinfo("KITE test 3.2\n");
        int_or_ptr_t kitefib;
        //xinfo("KITE test kitefibkey: %s\n", kitefibkey);
        kitefib = map_read(env, PTR(kitefibkey));
        //xinfo("KITE test 3.3\n");
        if (kitefib.p == NULL)
        {
            KITEFIB *n_kfib = nw_kitefib(trace_prefix);
            n_kfib->head = NULL;
            //xinfo("KITE test 3.35\n");
            kitefib_add_entry(n_kfib, esw, &trace_prefix, port, time(NULL));
            //xinfo("KITE test 3.4\n");
            map_add_key(env, PTR(kitefibkey), PTR(n_kfib), eq_kitefib_f, free_kitefib_f);
            //xinfo("KITE test 3.41\n");
        }
        else
        {
            kitefib_add_entry((KITEFIB *)(kitefib.p), esw, &trace_prefix, port, time(NULL));
        }

        //test start
        //xinfo("KITE test 3.5\n");
        atp = map_read(env, PTR("all_trace_prefix"));
        //xinfo("KITE test 4\n");
        if (atp.p != NULL)
        {
            ALL_TRACE_PREFIX *f_atp = (ALL_TRACE_PREFIX *)(atp.p);
            //xinfo("KITE test 5\n");
            for (int i = 0; i < f_atp->num; i++)
            {
                memcpy(kitefibkey, "kitefib:", strlen("kitefib:"));
                name_to_str(f_atp->names[i], kitefibkey + strlen("kitefib:"));
                kitefib = map_read(env, PTR(kitefibkey));
                if (kitefib.p != NULL)
                {
                    KITEFIB *f_kitefib = (KITEFIB *)(kitefib.p);
                    KITEFIB_ENTRY *ke = f_kitefib->head;
                    while (ke)
                    {
                        MYFIB *myfib = ke->myfib;
                        int eswid = ke->eswid;
                        struct entity *esw = get_switch(eswid);
                        if (myfib == NULL)
                        {
                            ke = ke->next;
                            continue;
                        }
                        MYFIB_ENTRY *me = myfib->head;
                        while (me)
                        {
                            xinfo("KITE FIB ENTRY\neswid: %d\nport: %d\ntime: %lld\n", myfib->eswid, me->port, me->time);
                            me = me->next;
                        }
                        ke = ke->next;
                    }
                }
                else
                {
                    xinfo("KITE FIB store failed\n");
                }
            }
        }
        else
        {
            xinfo("KITE prefix store failed\n");
        }
        //test end

        //释放内存
        //xinfo("KITE test 6\n");
        free_names(&names);
        //xinfo("KITE test 7\n");
        r = route();
        return r;
    }
    else if (test_equal(pkt, "type", value_from_8(NDN_TYPE_INTEREST)))
    {
        /*TODO 根据消费者Interest生成路径：
            将Interest名字与保存的TD信息中的名字对比
            匹配后找到对应的TD信息中的接口

        */
        int_or_ptr_t atp = map_read(env, PTR("all_trace_prefix"));
        NAMES trace_prefix;
        if (!match_trace_prefix(pkt, env, (ALL_TRACE_PREFIX *)(atp.p), &trace_prefix)) //没有跟当前兴趣包名字符合的追踪前缀
        {
            r = route();
            return r;
        }
        char kitefibkey[1024];
        memcpy(kitefibkey, "kitefib:", strlen("kitefib:"));
        name_to_str(trace_prefix, kitefibkey + strlen("kitefib:"));
        int_or_ptr_t kitefib;
        kitefib = map_read(env, PTR(kitefibkey));
        if (kitefib.p == NULL)
        {
            r = route();
            return r;
        }
        return kite_gen_trace_route(pkt, env, (KITEFIB *)(kitefib.p));
    }
    r = route();
    return r;
}