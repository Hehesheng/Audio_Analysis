#include "main.h"

u8 arg = 10;

//定义一个测试用的列表和3个列表项
List_t TestList;      //测试用列表
ListItem_t ListItem1; //测试用列表项1
ListItem_t ListItem2; //测试用列表项2
ListItem_t ListItem3; //测试用列表项3

char taskListInfo[1024];
EventGroupHandle_t EventGroupHandle;

int main()
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); //设置中断
    delay_init(168);                                //delay初始化，系统主频168MHz
    uart_init(115200);                              //USART1波特率设置为115200

    Adc1_Init(); //adc初始化
    Dac1_Init(); //DAC初始化
    GPIO_ALLInit();
    // usmart_init(84);
    TIM3_PWM_Init(1000 - 1, 8400 - 1); // 84M/(8400*100) = 10hz
    TIM_SetCompare1(TIM3, 500);        // 比较值CCR1为500

    printf("STM32 Terminal\r\n");
    printf("STM32F407 Hehe$ ");

    xTaskCreate((TaskFunction_t)start_task,
                (const char *)"start_task",
                (uint16_t)START_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)START_TASK_PRIO,
                (TaskHandle_t)&StartTask_Handler);

    vTaskStartScheduler(); //任务调度开始

    return 0;
}

void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();
    //led task create
    xTaskCreate((TaskFunction_t)led_task,
                (const char *)"led_task",
                (uint16_t)LED_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)LED_TASK_PRIO,
                (TaskHandle_t)&LedTask_Handler);
    //adc task create
    xTaskCreate((TaskFunction_t)ADC_task,
                (const char *)"ADC_task",
                (uint16_t)ADC_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)ADC_TASK_PRIO,
                (TaskHandle_t)&ADCTask_Handler);
    //list task create
    xTaskCreate((TaskFunction_t)list_task,
                (const char *)"list_task",
                (uint16_t)LIST_STK_SIZE,
                (void *)NULL,
                (UBaseType_t)LIST_TASK_PRIO,
                (TaskHandle_t)&ListTask_Handler);
    EventGroupHandle = xEventGroupCreate();
    vTaskDelete(NULL);
    taskEXIT_CRITICAL();
    while (1)
        ;
}

void led_task(void *pvParameters)
{
    while (1)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_7);
        delay_ms(250);
        GPIO_ResetBits(GPIOA, GPIO_Pin_7);
        delay_ms(250);
    }
}

void ADC_task(void *pvParameters)
{
    u8 i;
    u32 res;
    while (1)
    {
        res = 0;
        printf("ADC is Ready!\n");
        xEventGroupSetBits(EventGroupHandle, ADCEventBit);
        xEventGroupWaitBits(EventGroupHandle,
                            EnterEventBit,
                            pdTRUE, pdTRUE,
                            portMAX_DELAY);
        for (i = 0; i < adc_size; i++)
        {
            res += Get_Adc1(ADC_Channel_5);
        }
        taskENTER_CRITICAL();
        printf("ADC res: %ld\n", res / 10);
        taskEXIT_CRITICAL();
        delay_ms(3000);
    }
}

void list_task(void *pvParameters)
{
    vListInitialise(&TestList);
    vListInitialiseItem(&ListItem1);
    vListInitialiseItem(&ListItem2);
    vListInitialiseItem(&ListItem3);

    ListItem1.xItemValue = 40; //ListItem1列表项值为40
    ListItem2.xItemValue = 60; //ListItem2列表项值为60
    ListItem3.xItemValue = 50; //ListItem3列表项值为50

    //第二步：打印列表和其他列表项的地址
    printf("/***************List and ListItem Address*************/\r\n");
    printf("Item                              ADD                  \r\n");
    printf("TestList                          %#x                  \r\n", (int)&TestList);
    printf("TestList->pxIndex                 %#x                  \r\n", (int)TestList.pxIndex);
    printf("TestList->xListEnd                %#x                  \r\n", (int)(&TestList.xListEnd));
    printf("ListItem1                         %#x                  \r\n", (int)&ListItem1);
    printf("ListItem2                         %#x                  \r\n", (int)&ListItem2);
    printf("ListItem3                         %#x                  \r\n", (int)&ListItem3);
    printf("/************************END**************************/\r\n");
    printf("Enter to continue\r\n\r\n\r\n");

    // xEventGroupSetBits(EventGroupHandle, EnterEventBit);
    xEventGroupWaitBits(EventGroupHandle,
                        ADCEventBit,
                        pdTRUE, pdTRUE,
                        portMAX_DELAY);
    while (!(USART_RX_STA & USART_RX_OK))
        ; //等待输入
    USART_RX_STA = 0;

    //第三步：向列表TestList添加列表项ListItem1，并通过串口打印所有
    //列表项中成员变量pxNext和pxPrevious的值，通过这两个值观察列表
    //项在列表中的连接情况。
    vListInsert(&TestList, &ListItem1); //插入列表项ListItem1
    printf("/******************添加列表项ListItem1*****************/\r\n");
    printf("项目                              地址            \r\n");
    printf("TestList->xListEnd->pxNext        %#x          \r\n", (int)(TestList.xListEnd.pxNext));
    printf("ListItem1->pxNext                 %#x          \r\n", (int)(ListItem1.pxNext));
    printf("/*******************前后向连接分割线********************/\r\n");
    printf("TestList->xListEnd->pxPrevious    %#x          \r\n", (int)(TestList.xListEnd.pxPrevious));
    printf("ListItem1->pxPrevious             %#x          \r\n", (int)(ListItem1.pxPrevious));
    printf("/************************结束**************************/\r\n");
    printf("Enter to continue\r\n\r\n\r\n");

    // xEventGroupSetBits(EventGroupHandle, EnterEventBit);
    xEventGroupWaitBits(EventGroupHandle,
                        ADCEventBit,
                        pdTRUE, pdTRUE,
                        portMAX_DELAY);
    while (!(USART_RX_STA & USART_RX_OK))
        ; //等待输入
    USART_RX_STA = 0;

    //第四步：向列表TestList添加列表项ListItem2，并通过串口打印所有
    //列表项中成员变量pxNext和pxPrevious的值，通过这两个值观察列表
    //项在列表中的连接情况。
    vListInsert(&TestList, &ListItem2); //插入列表项ListItem2
    printf("/******************添加列表项ListItem2*****************/\r\n");
    printf("项目                              地址            \r\n");
    printf("TestList->xListEnd->pxNext        %#x          \r\n", (int)(TestList.xListEnd.pxNext));
    printf("ListItem1->pxNext                 %#x          \r\n", (int)(ListItem1.pxNext));
    printf("ListItem2->pxNext                 %#x          \r\n", (int)(ListItem2.pxNext));
    printf("/*******************前后向连接分割线********************/\r\n");
    printf("TestList->xListEnd->pxPrevious    %#x          \r\n", (int)(TestList.xListEnd.pxPrevious));
    printf("ListItem1->pxPrevious             %#x          \r\n", (int)(ListItem1.pxPrevious));
    printf("ListItem2->pxPrevious             %#x          \r\n", (int)(ListItem2.pxPrevious));
    printf("/************************结束**************************/\r\n");
    printf("Enter to continue\r\n\r\n\r\n");

    // xEventGroupSetBits(EventGroupHandle, EnterEventBit);
    xEventGroupWaitBits(EventGroupHandle,
                        ADCEventBit,
                        pdTRUE, pdTRUE,
                        portMAX_DELAY);
    while (!(USART_RX_STA & USART_RX_OK))
        ; //等待输入
    USART_RX_STA = 0;

    //第五步：向列表TestList添加列表项ListItem3，并通过串口打印所有
    //列表项中成员变量pxNext和pxPrevious的值，通过这两个值观察列表
    //项在列表中的连接情况。
    vListInsert(&TestList, &ListItem3); //插入列表项ListItem3
    printf("/******************添加列表项ListItem3*****************/\r\n");
    printf("项目                              地址            \r\n");
    printf("TestList->xListEnd->pxNext        %#x          \r\n", (int)(TestList.xListEnd.pxNext));
    printf("ListItem1->pxNext                 %#x          \r\n", (int)(ListItem1.pxNext));
    printf("ListItem3->pxNext                 %#x          \r\n", (int)(ListItem3.pxNext));
    printf("ListItem2->pxNext                 %#x          \r\n", (int)(ListItem2.pxNext));
    printf("/*******************前后向连接分割线********************/\r\n");
    printf("TestList->xListEnd->pxPrevious    %#x          \r\n", (int)(TestList.xListEnd.pxPrevious));
    printf("ListItem1->pxPrevious             %#x          \r\n", (int)(ListItem1.pxPrevious));
    printf("ListItem3->pxPrevious             %#x          \r\n", (int)(ListItem3.pxPrevious));
    printf("ListItem2->pxPrevious             %#x          \r\n", (int)(ListItem2.pxPrevious));
    printf("/************************结束**************************/\r\n");
    printf("Enter to continue\r\n\r\n\r\n");

    // xEventGroupSetBits(EventGroupHandle, EnterEventBit);
    xEventGroupWaitBits(EventGroupHandle,
                        ADCEventBit,
                        pdTRUE, pdTRUE,
                        portMAX_DELAY);
    while (!(USART_RX_STA & USART_RX_OK))
        ; //等待输入
    USART_RX_STA = 0;

    //第六步：删除ListItem2，并通过串口打印所有列表项中成员变量pxNext和
    //pxPrevious的值，通过这两个值观察列表项在列表中的连接情况。
    uxListRemove(&ListItem2); //删除ListItem2
    printf("/******************删除列表项ListItem2*****************/\r\n");
    printf("项目                              地址            \r\n");
    printf("TestList->xListEnd->pxNext        %#x          \r\n", (int)(TestList.xListEnd.pxNext));
    printf("ListItem1->pxNext                 %#x          \r\n", (int)(ListItem1.pxNext));
    printf("ListItem3->pxNext                 %#x          \r\n", (int)(ListItem3.pxNext));
    printf("/*******************前后向连接分割线********************/\r\n");
    printf("TestList->xListEnd->pxPrevious    %#x          \r\n", (int)(TestList.xListEnd.pxPrevious));
    printf("ListItem1->pxPrevious             %#x          \r\n", (int)(ListItem1.pxPrevious));
    printf("ListItem3->pxPrevious             %#x          \r\n", (int)(ListItem3.pxPrevious));
    printf("/************************结束**************************/\r\n");
    printf("Enter to continue\r\n\r\n\r\n");

    // xEventGroupSetBits(EventGroupHandle, EnterEventBit);
    xEventGroupWaitBits(EventGroupHandle,
                        ADCEventBit,
                        pdTRUE, pdTRUE,
                        portMAX_DELAY);
    while (!(USART_RX_STA & USART_RX_OK))
        ; //等待输入
    USART_RX_STA = 0;

    //第七步：删除ListItem2，并通过串口打印所有列表项中成员变量pxNext和
    //pxPrevious的值，通过这两个值观察列表项在列表中的连接情况。
    TestList.pxIndex = TestList.pxIndex->pxNext; //pxIndex向后移一项，这样pxIndex就会指向ListItem1。
    vListInsertEnd(&TestList, &ListItem2);       //列表末尾添加列表项ListItem2
    printf("/***************在末尾添加列表项ListItem2***************/\r\n");
    printf("项目                              地址               \r\n");
    printf("TestList->pxIndex                 %#x            \r\n", (int)TestList.pxIndex);
    printf("TestList->xListEnd->pxNext        %#x            \r\n", (int)(TestList.xListEnd.pxNext));
    printf("ListItem2->pxNext                 %#x          \r\n", (int)(ListItem2.pxNext));
    printf("ListItem1->pxNext                 %#x          \r\n", (int)(ListItem1.pxNext));
    printf("ListItem3->pxNext                 %#x          \r\n", (int)(ListItem3.pxNext));
    printf("/*******************前后向连接分割线********************/\r\n");
    printf("TestList->xListEnd->pxPrevious    %#x          \r\n", (int)(TestList.xListEnd.pxPrevious));
    printf("ListItem2->pxPrevious             %#x          \r\n", (int)(ListItem2.pxPrevious));
    printf("ListItem1->pxPrevious             %#x          \r\n", (int)(ListItem1.pxPrevious));
    printf("ListItem3->pxPrevious             %#x          \r\n", (int)(ListItem3.pxPrevious));
    printf("/************************结束**************************/\r\n\r\n\r\n");

    vTaskList(taskListInfo);
    taskENTER_CRITICAL();
    printf("%s\n", taskListInfo);
    taskEXIT_CRITICAL();

    while (1)
    {
    }
}
