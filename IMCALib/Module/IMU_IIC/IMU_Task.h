#ifndef __IMU_TASK_
#define __IMU_TASK_

#include "mytype.h"


typedef struct{
    float angle; //陀螺仪yaw轴的实时角度
    float last_angle; //上一次角度
    float angle_diff; //前后两次直接作差得到的差值
    float angle_diff_process; //经过处理后的角度差值
    float total_angle; //转过的总角度
    int32_t round_cnt; //圈数
    
}IMU_t;


extern IMU_t IMU_Yaw;

void GetImuAngle(IMU_t *IMU);



#endif
