#include <stdio.h>

void HostTest_PowerMonitor(void);
void HostTest_ButtonEstop(void);
void HostTest_MotorTb6612(void);
void HostTest_Encoder(void);
void HostTest_LineSensor(void);
void HostTest_TofStp23l(void);
void HostTest_VisionUart(void);
void HostTest_Gimbal(void);

int main(void)
{
    HostTest_PowerMonitor();
    HostTest_ButtonEstop();
    HostTest_MotorTb6612();
    HostTest_Encoder();
    HostTest_LineSensor();
    HostTest_TofStp23l();
    HostTest_VisionUart();
    HostTest_Gimbal();

    puts("host module tests 01-08: PASS");
    return 0;
}