// 请实现生产者消费者模型，完善下面程序：
#include <stdio.h>
#include <unistd.h>

#include "blockq.h"

void* gateway_pool_build(void* pool_void);
static void* consumer(void* task);