#ifndef _MY_TLV_H_
#define _MY_TLV_H_

#include "mytypes.h"

/*大端序转小端序
  value：需要转换的大端序数字的起始地址
  size：数字的字节数，取值为{2,4,8}
  返回值：转换后的小端序数字*/
uint64_t big_to_native(uint8_t *value, size_t size)
{
    uint64_t ans = 0;
    switch (size)
    {
    case 2:
    {
        ans = ((uint64_t) * (value) << 8) | ((uint64_t) * (value + 1));
        break;
    }
    case 4:
    {
        ans = ((uint64_t) * (value) << 24) | ((uint64_t) * (value + 1) << 16) | ((uint64_t) * (value + 2) << 8) | ((uint64_t) * (value + 3));
        break;
    }
    case 8:
    {
        ans = ((uint64_t) * (value) << 56) | ((uint64_t) * (value + 1) << 48) | ((uint64_t) * (value + 2) << 40) | ((uint64_t) * (value + 3) << 32) | ((uint64_t) * (value + 4) << 24) | ((uint64_t) * (value + 5) << 16) | ((uint64_t) * (value + 6) << 8) | ((uint64_t) * (value + 7));
        break;
    }
    }
    return ans;
}

/*读取TLV编码的数字VAR-NUMBER-1(3,5,9)，结果在number中
  data：指向包某位置的指针
  number：用于存储读取到的VAR_NUMBER
  len：data+(*len)始终指向当前待处理位置
  返回值：读取成功返回True(1)，如不符合编码规范返回False(0)*/
BOOL read_var_number(uint8_t *data, uint64_t *number, uint32_t *len)
{
    uint8_t first_octec = *(data + *len);
    (*len)++;
    if (first_octec <= 0xFC)
    {
        *number = first_octec;
        return True;
    }
    switch (first_octec)
    {
    case 0xFD:
    {
        uint16_t value = 0;
        memcpy(&value, data + *len, 2);
        (*len) += 2;
        *number = big_to_native((uint8_t *)&value, 2);
        if (*number <= 0xFC)
            return False;
        return True;
    }
    case 0xFE:
    {
        uint32_t value = 0;
        memcpy(&value, data + *len, 4);
        (*len) += 4;
        *number = big_to_native((uint8_t *)&value, 4);
        if (*number <= 0xFFFF)
            return False;
        return True;
    }
    default:
    {
        uint64_t value = 0;
        memcpy(&value, data + *len, 8);
        (*len) += 8;
        *number = big_to_native((uint8_t *)&value, 8);
        if (*number <= 0xFFFFFFFF)
            return False;
        return True;
    }
    }
}

/*读取类型结，果在type中
  data：指向包某位置的指针
  type：用于存储读取到的TLV_TYPE
  len：data+(*len)始终指向当前待处理位置
  返回值：读取成功返回True(1)，如不符合编码规范返回False(0)*/
BOOL read_type(uint8_t *data, uint32_t *type, uint32_t *len)
{
    uint64_t number = 0;
    if (!read_var_number(data, &number, len) || number == Invalid || number > 0xFFFFFFFF)
    {
        return False;
    }
    *type = (uint32_t)number;
    return True;
}

/*读取Non-Negative Integer，结果在nn_integer中
  data：指向包某位置的指针
  size：要读取的Non-Negative Integer长度，取值为{1,2,4,8}
  nn_integer：用于存储读取到的Non-Negative Integer
  len：data+(*len)始终指向当前待处理位置
  返回值：读取成功返回True(1)，如不符合编码规范返回False(0)*/
BOOL read_non_negative_integer(uint8_t *data, size_t size, uint64_t *nn_integer, uint32_t *len)
{
    if (size != 1 && size != 2 && size != 4 && size != 8)
    {
        return False;
    }
    uint64_t number = 0;
    if (!read_var_number(data + *len, &number, len))
    {
        return False;
    }
    *nn_integer = number;
    return True;
}

#endif