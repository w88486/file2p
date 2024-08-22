#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arr.h"
// 查找最大值

int max(int arr[], int n)
{
    int max = arr[0];
    for (int i = 1; i < n; i++)
    {
        if (arr[i] > max)
        {
            max = arr[i];
        }
    }
    return max;
}

// 查找最小值
int min(int arr[], int n)
{
    int min = arr[0];
    for (int i = 1; i < n; i++)
    {
        if (arr[i] < min)
        {
            min = arr[i];
        }
    }
    return min;
}
// 查找平均值
double avg(int arr[], int n)
{
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += arr[i];
    }
    return sum / n;
}

// 查找元素
int find(int arr[], int len, int num)
{
    int ret = -1;
    for (int i = 0; i < len; ++i)
    {
        if (arr[i] == num)
        {
            ret = i;
            break;
        }
    }
    return ret;
}

// 交换两个数
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}
// 交换两个字符
void swapc(char *a, char *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}
// 冒泡排序
void Bub_sort(int arr[], int len)
{
    for (int i = 0; i < len - 1; ++i)
    {
        for (int j = 0; j < len - i - 1; ++j)
        {
            if (arr[j] > arr[j + 1])
            {
                swap(&arr[j], &arr[j + 1]);
            }
        }
    }
}

// 选择排序
void sel_sort(int arr[], int len)
{
    for (int i = 0; i < len - 1; ++i)
    {
        //假设第一个是最小
        int mini = i;
        for (int j = i + 1; j < len; ++j)
        {
            //比最大值还大，记录最大值index
            if (arr[j] < arr[mini])
            {
                mini = j;
            }
        }
        //交换第一个位置值和最大值
        swap(&arr[i], &arr[mini]);
    }
}

// 插入排序
void ins_sort(int arr[], int len)
{
    for (int i = 1; i < len; ++i)
    {
        int tmp = arr[i];
        for (int j = i; j > 0; --j)
        {
            //向后移动
            if (tmp < arr[j - 1])
            {
                arr[j] = arr[j - 1];
            }
            //直到当前比较位置的数小于当前要插入的数
            else
            {
                //插入元素
                arr[j] = tmp;
                break;
            }
        }
    }
}

// 快速排序
void recur_sort(int arr[], int start, int end)
{
    if (start >= end)
    {
        return;
    }
    int mid_val = arr[end];
    int left = start;
    int right = end - 1;
    while (left < right)
    {
        while (arr[left] < mid_val && left < right)
        {
            ++left;
        }
        while (arr[right] > mid_val && left < right)
        {
            --right;
        }
        swap(&arr[left], &arr[right]);
    }
    if (arr[left] <= mid_val)
    {
        ++left;
    }
    else
    {
        swap(&arr[left], &arr[end]);
    }
    recur_sort(arr, start, left - 1);
    recur_sort(arr, left + 1, end);
}

// 排序
void sort(int arr[], int n)
{
    recur_sort(arr, 0, n - 1);
}

// 翻转数组
void reverse(int *arr, int len)
{
    for (int i = 0; i < len / 2 /* i < len / 2 */; ++i)
    {
        swap(&arr[i], &arr[len - i - 1]);
    }
}
// 翻转字符串
void reversec(char *arr, int len)
{
    for (int i = 0; i < len / 2 /* i < len / 2 */; ++i)
    {
        swapc(&arr[i], &arr[len - i - 1]);
    }
}

// 输出数组
void print_arr(int arr[], int len)
{
    for (int i = 0; i < len; ++i)
    {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

// 初始化可变数组
void init(struct list *l)
{
    l->capacity = 5;
    l->size = 0;
    l->data = malloc(l->capacity * sizeof(char*));
}
// 扩展数组容量
void change_capacity(struct list *l)
{
    // 容量加自身的一半
    l->capacity += l->capacity >> 1;
    // 重新分配内存
    l->data = realloc(l->data, l->capacity * sizeof(char*));
}
// 检查数组容量是否已满，已满则扩展数组
void check_capacity(struct list *l)
{
    if (l->size >= l->capacity)
    {
        change_capacity(l);
    }
}
// 获取数组长度
int size(struct list *l)
{
    return l->size;
}
// 获取index位置的数组元素
char* get(struct list *l, int index)
{
    // 判断参数是否合法
    if (NULL == l || index >= l->size || index < 0)
    {
        return NULL;
    }
    return l->data[index];
}
// 在在末尾添加元素value
void add(struct list *l, char *value)
{
    // 判断参数是否合法
    if (NULL == l)
    {
        return;
    }
    // 检查数组容量是否足够
    check_capacity(l);
    char *elem = malloc(strlen(value) + 1);
    strncpy(elem, value, strlen(value) + 1);
    // 添加元素
    l->data[l->size++] = elem;
}
// 在index位置插入元素value
// void insert(struct list *l, int index, int value)
// {
//     // 判断参数是否合法
//     if (NULL == l || l->size < index || index < 0)
//     {
//         return;
//     }
//     // 检查数组容量是否足够
//     check_capacity(l);
//     // 移动并添加元素
//     for (int i = l->size - 1; i >= index; --i)
//     {
//         l->data[i + 1] = l->data[i];
//     }
//     l->data[index] = value;
//     ++l->size;
// }
// 删除index位置的元素
void delete(struct list *l, int index)
{
    // 判断参数是否合法
    if (NULL == l || l->size <= index || index < 0)
    {
        return;
    }
    // 将index以后的元素往前移动一个位置
    free(l->data[index]);
    l->data[index] = NULL;
}
// 显示可变数组
// void display(struct list *l)
// {
//     for (int i = 0; i < l->size; ++i)
//     {
//         printf("%d ", l->data[i]);
//     }
//     printf("\n");
// }
// 释放可变数组的资源
void fini(struct list *l)
{
    if (NULL == l || 0 == l->capacity)
    {
        return;
    }
    for (int i = 0; i < l->size; i++)
    {
        if (NULL != l->data[i])
        {
            free(l->data[i]);
        }
    }
    
    free(l->data);
    l->data = NULL;
    l->size = 0;
    l->capacity = 0;
    free(l);
}

int find_nnull(struct list *l)
{
    for (int i = 0; i < l->size; ++i)
    {
        if (NULL != l->data[i])
        {
            return i;
        }
    }
    return -1;
}