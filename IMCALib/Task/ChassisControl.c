/*************************************************************************************************************************
 * @File   ChassisControl.c        
 * @Brief  �����˵��̿��ƣ��ӵ������Ͻǵ����ʱ�뿪ʼ�������̶�Ӧ�����IDΪ1 ~ 4�������ĸ�����Ŀ����ǹ���CAN1�ϣ�
 *         PID��Ӧ�Ĳ������ڴ������Ӹ��ص�����µ��Եõ���
 *
*************************************************************************************************************************/

#include "ChassisControl.h"
#include "RC_Task.h"
#include "CanBus_Task.h"
#include "ramp.h"
#include "stdio.h"


uint8_t Chassis_Ctrl; //���̿��Ʊ�־λ
pid_t  Moto_Chassis_Pid_Pos[4];  //���̵��λ�û�PID�ṹ��
pid_t  Moto_Chassis_Pid_Spd[4];  //���̵���ٶȻ�PID�ṹ��
chassis_t  Chassis;  /*���ڵ���*/

//J_Scope������ʾ
float J_Scope_rcinput_y;
float J_Scope_rcinput_x;


void ChassisTask(void)
{
    
    ChassisDataUpdate(); 
    ChassisPidCalc(); 
    ChassisDataCanSend();
    
}



void ChassisDataUpdate(void)
{
    int16_t chassis_vx_channel, chassis_vy_channel, chassis_vw_channel;
    
    /*ң������������*/
    chassis_vx_channel = RcDeadlineLimit(remote_control.ch1, 10)*CHASSIS_MAXSPEED_RPM/660;
    chassis_vy_channel = RcDeadlineLimit(remote_control.ch2, 10)*CHASSIS_MAXSPEED_RPM/660;
    chassis_vw_channel = RcDeadlineLimit(remote_control.ch3, 10)*CHASSIS_MAXSPEED_RPM/660;

    
    #if defined (SKYGUARD_CHASSIS)
    
        Chassis.vx = remote_control.ch1*GUARD_CHASSIS_MAXSPEED_RPM/660; //����ƽ��
    
        Chassis.fr_motor_rpm_201 = -Chassis.vx;
        Chassis.fl_motor_rpm_202 = -Chassis.vx;
    
    #else
    
/****************************************б�²���*********************************************************/
        //һ�׵�ͨ�˲�����б����Ϊ�����ٶ�����
        FirstOrderFilterCali(&chassis_cmd_slow_set_vx, chassis_vx_channel);
        FirstOrderFilterCali(&chassis_cmd_slow_set_vy, chassis_vy_channel);

        Chassis.vx = chassis_cmd_slow_set_vx.out; //����ƽ��
        Chassis.vy = chassis_cmd_slow_set_vy.out; //ǰ���˶�
        Chassis.vw = chassis_vw_channel; //��ת

/*******************************************END***********************************************************/
        
        J_Scope_rcinput_x = Chassis.vx;
        J_Scope_rcinput_y = Chassis.vy;
        
        Chassis.fr_motor_rpm_201 = +Chassis.vx - Chassis.vy + Chassis.vw;
        Chassis.fl_motor_rpm_202 = +Chassis.vx + Chassis.vy + Chassis.vw;
        Chassis.rl_motor_rpm_203 = -Chassis.vx + Chassis.vy + Chassis.vw;
        Chassis.rr_motor_rpm_204 = -Chassis.vx - Chassis.vy + Chassis.vw;
        
    #endif
    
}



void ChassisPidCalc(void)
{
    
    pid_calc(&Moto_Chassis_Pid_Spd[FRON_RIGH_201], Chassis_Motor[FRON_RIGH_201].speed_rpm, Chassis.fr_motor_rpm_201*REDUCTION_RATIO_3508);
    pid_calc(&Moto_Chassis_Pid_Spd[FRON_LEFT_202], Chassis_Motor[FRON_LEFT_202].speed_rpm, Chassis.fl_motor_rpm_202*REDUCTION_RATIO_3508);
    pid_calc(&Moto_Chassis_Pid_Spd[REAR_LEFT_203], Chassis_Motor[REAR_LEFT_203].speed_rpm, Chassis.rl_motor_rpm_203*REDUCTION_RATIO_3508);
    pid_calc(&Moto_Chassis_Pid_Spd[REAR_RIGH_204], Chassis_Motor[REAR_RIGH_204].speed_rpm, Chassis.rr_motor_rpm_204*REDUCTION_RATIO_3508);
    
}


void ChassisDataCanSend(void)
{
   
//    if((Chassis.fr_motor_rpm_201 != 0)&&(Chassis.fl_motor_rpm_202 != 0)&&(Chassis.rl_motor_rpm_203 != 0)&&(Chassis.rr_motor_rpm_204 != 0))
//    {
        SetChassisMotorCurrent(&hcan1, Moto_Chassis_Pid_Spd[FRON_RIGH_201].pos_out, Moto_Chassis_Pid_Spd[FRON_LEFT_202].pos_out, \
                     Moto_Chassis_Pid_Spd[REAR_LEFT_203].pos_out, Moto_Chassis_Pid_Spd[REAR_RIGH_204].pos_out);
        
//    }
//    else
//    {
//        SetChassisMotorCurrent(&hcan1, 0, 0, 0, 0);
//    }
//    
    
}


