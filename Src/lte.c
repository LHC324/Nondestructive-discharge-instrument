#include "LTE.h"
#include "systemTimer.h"
#include "usart.h"
#include <string.h>

const AT_COMMAND atCmdLteInit[] = // LTE模块AT指令
    {
        {"+++", "a", T_2S},
        {"a", "+ok", T_2S},
        {"AT+E=OFF\r\n", "OK", T_500MS},

        {"AT+HEARTDT=7777772E796E7061782E636F6D\r\n", "OK", T_500MS},

        {"AT+WKMOD=NET\r\n", "OK", T_500MS},
        {"AT+SOCKAEN=ON\r\n", "OK", T_500MS},
        {"AT+SOCKASL=LONG\r\n", "OK", T_500MS},
        {"AT+SOCKA=TCP,clouddata.usr.cn,15000\r\n", "OK", T_500MS},

        {"AT+REGEN=ON\r\n", "OK", T_500MS},
        {"AT+REGTP=CLOUD\r\n", "OK", T_500MS},
        {"AT+CLOUD=00019639000000000034,SkdGAzyl\r\n", "OK", T_500MS},
        {"AT+Z\r\n", "OK", T_500MS},
};
#define atCmdLteInitSize sizeof(atCmdLteInit) / sizeof(AT_COMMAND)

void LTEconnect(void) //连接服务器
{
    uint8_t i;

    for (i = 0; i < atCmdLteInitSize; i++)
    {
        Uartx_SendStr(&Uart2, atCmdLteInit[i].Cmd, strlen(atCmdLteInit[i].Cmd));
        //        Delay_ms(500);
    }
}

void LTEenable(bool Enable) //模块启停
{
    if (Enable == TRUE)
    {
        LTE_POWER_KEY = 1;
    }
    else
    {
        LTE_POWER_KEY = 0;
    }
}
