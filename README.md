## Template——stm32f4标准空白例程

### V1.0.5 

**settings.h**加入，**AD9834**驱动加入  

### V1.0.2 USART优化

添加**USART2**的相关函数，在**usart.h**中可以选择性开启串口  
优化代码结构，结构模块化，方便以后添加变得串口  
使用**USARTx_printf(string)**可以选择发送串口（x为串口编号）  


### V1.0.0

添加**ADC&DAC**模块，默认添加**USART1、delay**函数  


