#ifndef _MY_PARSER_H_
#define _MY_PARSER_H_

#include "tlv.h"
#include "mytypes.h"

/*从包的名字区域读取所有的名字组件
  data：指向包某位置的指针
  names：用于存储读取到的名字组件集
  total_len：名字区域的总长度
  len：data+(*len)始终指向当前待处理位置
  返回值：读取成功返回True(1)，如不符合编码规范返回False(0)*/
BOOL read_name_components(uint8_t *data, NAMES *names, uint32_t total_len, uint32_t *len)
{
    uint32_t type = 0;
    uint64_t number = 0;
    uint32_t read_len = 0;        //已读取的字节数
    NAME_COMPONENT tmp_component; //临时存储读取到的名字组件
    uint32_t original_len = (*len);
    names->num = 0;
    names->names = NULL;
    while (read_len < total_len)
    {
        if (!read_type(data, &type, len))
        {
            return False;
        }
        if (!read_var_number(data, &number, len))
        {
            return False;
        }
        //读取一个名字组件并保存在tmp_component中
        tmp_component.type = type;
        tmp_component.len = number;
        tmp_component.value = (uint8_t *)malloc(sizeof(uint8_t) * number);
        memcpy(tmp_component.value, data+(*len), sizeof(uint8_t) * number);
        //将名字组件放入到names中
        names->num += 1;                                                                             //更新组件数量
        names->names = (NAME_COMPONENT *)realloc(names->names, sizeof(NAME_COMPONENT) * names->num); //重新分配空间
        memcpy(names->names + names->num - 1, &tmp_component, sizeof(NAME_COMPONENT));               //复制内存
        (*len) += number;
        read_len = (*len) - original_len;
    }
    return True;
}
/*从包的名字区域中读取名字
  data：指向包某位置的指针
  names：用于存储读取到的名字组件集
  len：data+(*len)始终指向当前待处理位置
  返回值：读取成功返回True(1)，如不符合编码规范返回False(0)*/
BOOL read_name(uint8_t *data, NAMES *names, uint32_t *len)
{
    uint32_t type = 0;
    uint64_t number = 0;
    if (!read_type(data, &type, len) || type != Name)
    {
        return False;
    }
    if (!read_var_number(data, &number, len))
    {
        return False;
    }
    if (!read_name_components(data, names, number, len))
    {
        return False;
    }
    return True;
}

/*从DATA包中提取名字
  data：指向包某位置的指针
  names：用于存储读取到的名字组件集
  len：data+(*len)始终指向当前待处理位置
  返回值：读取成功返回True(1)，如不符合编码规范返回False(0)*/
BOOL extract_name_from_data(uint8_t *data, NAMES *names, uint32_t *len)
{
    uint32_t type = 0;
    uint64_t number = 0;
    if (!read_type(data, &type, len) || type != Data)
    {
        return False;
    }
    if (!read_var_number(data, &number, len))
    {
        return False;
    }
    if (!read_name(data, names, len))
    {
        return False;
    }
    return True;
}

/*查找一个名字组件是否是追踪标签
  name：名字的值
  len：名字值的长度
  返回值：是追踪标签返回True(1)，否则返回False(0)*/
BOOL check_trace_tag(NAME_COMPONENT name)
{
    if (name.type != TraceTag || name.len != 4)
    {
        return False;
    }
    uint8_t trace_tag[4] = {0x4B, 0x49, 0x54, 0x45};
    for (int i = 0; i < 4; i++)
    {
        if (name.value[i] != trace_tag[i])
        {
            return False;
        }
    }
    return True;
}

/*在NAMES中的名字组件里查找是否有追踪标签
  names：名字组件集
  返回值：追踪标签的下标，未找到返回-1*/
int find_trace_tag(NAMES names)
{
    for (int i = 0; i < names.num; i++)
    {
        //检查名字组件是否为追踪标签
        NAME_COMPONENT tmp_component = names.names[i];
        if (check_trace_tag(tmp_component)) // DONE 重写判断追踪标签的条件
        {
            return i;
        }
    }
    return -1;
}

/*在NAMES中的名字组件中提取追踪前缀
  names：名字组件集
  trace_prefix：保存追踪前缀
  返回值：是否是TI/TD数据名，是返回True(1)，否则返回False(0)*/
BOOL extract_trace_prefix(NAMES names, NAMES *trace_prefix)
{
    int trace_tag_index = find_trace_tag(names);
    if (trace_tag_index < 0)
    {
        return False;
    }
    if (trace_prefix == NULL)
    {
        trace_prefix = (NAMES *)malloc(sizeof(NAMES));
    }
    trace_prefix->num = 0;
    trace_prefix->names = NULL;
    for (int i = 0; i < names.num; i++)
    {
        if (i != trace_tag_index)
        {
            if (names.names[i].type != GenericNameComponent)
                break;
            trace_prefix->num += 1;
            trace_prefix->names = (NAME_COMPONENT *)realloc(trace_prefix->names, sizeof(NAME_COMPONENT) * trace_prefix->num); //重新分配空间
            trace_prefix->names[trace_prefix->num - 1] = names.names[i];
            trace_prefix->names[trace_prefix->num - 1].value = malloc(sizeof(uint8_t)*names.names[i].len);
            memcpy(trace_prefix->names[trace_prefix->num - 1].value, names.names[i].value, names.names[i].len); //复制内存
        }
    }
    return True;
}

#endif