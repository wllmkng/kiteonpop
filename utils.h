#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char *reverse_str(char *str)
{
    char *ans = malloc(sizeof(char) * 65);
    int i,j;
    for(i=strlen(str)-1,j=0;i>=0;i--,j++)
    {
        ans[j]=str[i];
    }
    ans[j] = '\0';
    return ans;
}

char *int_to_str(uint64_t number)
{
    char *str = malloc(sizeof(char) * 65);
    if (number == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    int i = 0;
    while (number > 0)
    {
        str[i] = number % 10 + '0';
        number = number / 10;
        i++;
    }
    char *ans=reverse_str(str);
    free(str);
    return ans;
}

#endif