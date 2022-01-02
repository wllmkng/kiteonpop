#ifndef _MYTYPES_H_
#define _MYTYPES_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BOOL uint8_t
#define True 1
#define False 0

/*要用到的TLV_TYPE的取值*/
enum
{
    Invalid = 0,
    Interest = 5,
    Data = 6,
    Name = 7,
    GenericNameComponent = 8,
    ImplicitSha256DigestComponent = 1,
    ParametersSha256DigestComponent = 2,
    CanBePrefix = 33,
    MustBeFresh = 18,
    ForwardingHint = 30,
    Nonce = 10,
    InterestLifetime = 12,
    HopLimit = 34,
    ApplicationParameters = 36,
    InterestSignatureInfo = 44,
    InterestSignatureValue = 46,
    MetaInfo = 20,
    Content = 21,
    SignatureInfo = 22,
    SignatureValue = 23,
    ContentType = 24,
    FreshnessPeriod = 25,
    FinalBlockId = 26,
    SignatureType = 27,
    KeyLocator = 28,
    KeyDigest = 29,
    SignatureNonce = 38,
    SignatureTime = 40,
    SignatureSeqNum = 42,
    LinkDelegation = 31,
    LinkPreference = 30,

    NameComponentMin = 1,
    NameComponentMax = 65535,

    AppPrivateBlock1 = 128,
    AppPrivateBlock2 = 32767,

    TraceTag = 32
};

/*一个名字组件
  type：名字的类型
  len：名字值的长度
  value：名字的值
  使用free_name_component函数释放内存*/
typedef struct
{
    uint32_t type;
    uint64_t len;
    uint8_t *value;
} NAME_COMPONENT;
/*一个包中的所有名字组件
  num：名字组件的数量
  names：名字组件数组
  使用free_names函数释放内存*/
typedef struct
{
    uint32_t num;
    NAME_COMPONENT *names;
} NAMES;

BOOL eq_name_component(NAME_COMPONENT n1, NAME_COMPONENT n2)
{
    if (n1.type == n2.type && n1.len == n2.len)
    {
        for (int i = 0; i < n1.len; i++)
        {
            if (n1.value[i] != n2.value[i])
            {
                return False;
            }
        }
        return True;
    }
    return False;
}

BOOL eq_names(NAMES ns1, NAMES ns2)
{
    if (ns1.num == ns2.num)
    {
        for (int i = 0; i < ns1.num; i++)
        {
            if (!eq_name_component(ns1.names[i], ns2.names[i]))
            {
                return False;
            }
        }
        return True;
    }
    return False;
}

/*释放内存*/
void free_name_component(NAME_COMPONENT *name)
{
    name->type = 0;
    if (name->value != NULL)
    {
        free(name->value);
        name->value = NULL;
    }
}
/*释放内存*/
void free_names(NAMES *names)
{
    if (names->names == NULL)
    {
        names->num = 0;
        return;
    }
    for (int i = 0; i < names->num; i++)
    {
        free_name_component(&(names->names[i]));
    }
    free(names->names);
    names->num = 0;
    names->names = NULL;
}

void name_to_str(NAMES names, char *str)
{
    int pos = 0;
    for (int i = 0; i < names.num; i++)
    {
        str[pos++] = '/';
        for (int j = 0; j < names.names[i].len; j++)
        {
            str[pos++] = names.names[i].value[j];
        }
    }
    str[pos] = '\0';
}

#endif