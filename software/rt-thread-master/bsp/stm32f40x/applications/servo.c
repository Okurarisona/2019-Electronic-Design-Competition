/*
 * servo.c
 *
 *  Created on: 2019年3月30日
 *      Author: zengwangfa
 *      Notes:  舵机设备
 */
#define  LOG_TAG    "servo"
#include <stdio.h>
#include <stdlib.h>
#include <rtthread.h>
#include <elog.h>

#include "sys.h"

#include "DataType.h"
#include "Control.h"
#include "ioDevices.h"
#include "servo.h"
#include "propeller.h"
#include "flash.h"
#include "ret_data.h"
#include "rc_data.h"
#include "focus.h"
#include "DeviceThread.h"
#include "PID.h"
/*---------------------- Constant / Macro Definitions -----------------------*/

#define RoboticArm_MedValue  1500
#define YunTai_MedValue  		 2000

/*----------------------- Variable Declarations -----------------------------*/

ServoType RoboticArm = {
		 .MaxValue = 2000, 		//机械臂 正向最大值
		 .MinValue = 1000,	  //机械臂 反向
		 .MedValue = 1500,
	   .Speed  = 5//机械臂当前值
};  //机械臂
ServoType  YunTai = {
		 .MaxValue = 1700, 		// 正向最大值
		 .MinValue = 1400,	  // 反向
		 .MedValue = 1100,
	   .Speed  = 10//云台转动速度
};      //云台


uint16 propeller_power = 1500;



/*----------------------- Function Implement --------------------------------*/


/*******************************************
* 函 数 名：Servo_Output_Limit
* 功    能：舵机输出限制
* 输入参数：输入值：舵机结构体地址 
* 返 回 值：None
* 注    意：
********************************************/
void Servo_Output_Limit(ServoType *Servo)
{
		Servo->CurrentValue = Servo->CurrentValue > Servo->MaxValue ? Servo->MaxValue : Servo->CurrentValue ;//正向限幅
		Servo->CurrentValue = Servo->CurrentValue < Servo->MinValue ? Servo->MinValue : Servo->CurrentValue ;//反向限幅
	
}







/*  小车舵机 控制*/
void Servo_Dir_Control(short offest) //PWM越大 ,往右
{

	
		YunTai.CurrentValue = YunTai.MedValue + (offest);
	
		Servo_Output_Limit(&YunTai); //限幅
		TIM4_PWM_CH4_D15(YunTai.CurrentValue);  //输出

}


/* 云台控制 

当前x，期望x;当前y，期望y
*/
void yuntai_pid_control(short present_x,short expect_x,short persent_y,short expect_y) //
{

    /* 航向角 为 X*/
		Total_Controller.Yaw_Angle_Control.FeedBack = present_x;  //当前深度反馈
		Total_Controller.Yaw_Angle_Control.Expect = expect_x ; //期望深度由遥控器给定	
	
		/* 俯仰角 为 Y*/
		Total_Controller.Pitch_Angle_Control.FeedBack = persent_y;  //当前深度反馈
		Total_Controller.Pitch_Angle_Control.Expect = expect_y ; //期望深度由遥控器给定
	
	
		PID_Control(&Total_Controller.Pitch_Angle_Control);//高度位置控制器
		PID_Control(&Total_Controller.Yaw_Angle_Control);//高度位置控制器
	
}

/*【推进器】 修改 【正向最大值】MSH方法 */
static int pitch_yuntai(int argc, char **argv)
{

    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: Propoller_Test <0~100>");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 5000){

				PropellerPower.rightMiddle = atoi(argv[1]);

				log_i("Current propeller power:  %d",atoi(argv[1]) );
		}
		
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(pitch_yuntai,ag: two_axis_yuntai 1000 1000);



/*【推进器】 修改 【正向最大值】MSH方法 */
static int yaw_yuntai(int argc, char **argv)
{

    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: Propoller_Test <0~100>");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 5000){

				PropellerPower.leftMiddle = atoi(argv[1]);

				log_i("Current propeller power:  %d",atoi(argv[1]) );
		}
		
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(yaw_yuntai,ag: two_axis_yuntai 1000 1000);


/**
  * @brief  RoboticArm_Control(机械臂控制)
  * @param  控制指令 0x00：不动作  0x01：张开  0x02：关闭
  * @retval None
  * @notice 
  */
void RoboticArm_Control(uint8 *action)
{
		switch(*action)
		{
				case 0x01:RoboticArm.CurrentValue += RoboticArm.Speed;
									if(RoboticArm.CurrentValue >= RoboticArm.MaxValue){device_hint_flag |= 0x01;}//机械臂到头标志
									else {device_hint_flag &= 0xFE;}; //清除机械臂到头标志

									break;
				case 0x02:RoboticArm.CurrentValue -= RoboticArm.Speed;
									if(RoboticArm.CurrentValue <= RoboticArm.MinValue){device_hint_flag |= 0x01;}//机械臂到头标志
									else {device_hint_flag &= 0xFE;}; //清除机械臂到头标志

									break;
				default:break;
		}
		Servo_Output_Limit(&RoboticArm);//机械臂舵机限幅
		TIM4_PWM_CH3_D14(RoboticArm.CurrentValue);
		*action = 0x00; //清除控制字
}




/**
  * @brief  YunTai_Control(云台控制)
  * @param  控制指令 0x00：不动作  0x01：向上  0x02：向下
  * @retval None
  * @notice 
  */

void YunTai_Control(uint8 *action)
{
		switch(*action)
		{
				case 0x01:YunTai.CurrentValue += YunTai.Speed;  //向上
						if(YunTai.CurrentValue <= YunTai.MaxValue){device_hint_flag |= 0x02;}//云台到头标志
						else {device_hint_flag &= 0xFD;}; //清除云台到头标志

						break;  
						
				case 0x02:YunTai.CurrentValue -= YunTai.Speed; //向下
						if(YunTai.CurrentValue >= YunTai.MinValue){device_hint_flag |= 0x02;}//云台到头标志
						else {device_hint_flag &= 0xFD;}; //清除云台到头标志

						break;  

				case 0x03:YunTai.CurrentValue = YunTai.MedValue;break;   //归中
				default: break;
		}
		Servo_Output_Limit(&YunTai);
		//TIM4_PWM_CH4_D15(YunTai.CurrentValue); 
		*action = 0x00; //清除控制字
}







short _test_value = 0;
/*【吸取器】 修改 【测试】 MSH方法 */
static int xiquqi_value_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: robotic_arm_currentValue_set <0~3000>");
				result = -RT_ERROR;
        goto _exit;
    }

		if(atoi(argv[1]) <= 3000 ){		
				_test_value = atoi(argv[1]);
				log_i(" Value:  %d",_test_value);
				TIM3_PWM_CH3_B0(_test_value);
		}
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(xiquqi_value_set,ag: xiquqi_value_set <0~3000>);





/*【云台】舵机 修改 【当前】 MSH方法 */
static int robotic_arm_currentValue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: robotic_arm_currentValue_set <0~3000>");
				result = -RT_ERROR;
        goto _exit;
    }

		if(atoi(argv[1]) <= 3000 ){		
				RoboticArm.CurrentValue = atoi(argv[1]);
				log_i(" Value:  %d",RoboticArm.CurrentValue);
				TIM4_PWM_CH3_D14(RoboticArm.CurrentValue);


		}
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(robotic_arm_currentValue_set,ag: robotic_arm_currentValue_set <0~3000>);





/*【机械臂】舵机 修改 速度值 */
static int robotic_arm_speed_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: RoboticArm_Speed 10");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 255 && atoi(argv[1]) > 0){
				RoboticArm.Speed = atoi(argv[1]);
				Flash_Update();
				log_i("Write_Successed! RoboticArm.Speed:  %d",RoboticArm.Speed);
		}
		
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(robotic_arm_speed_set,ag: robotic_arm_speed_set 10);


/*【机械臂】舵机 修改 【正向最大值】MSH方法 */
static int robotic_arm_maxValue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: RoboticArm_Maxvalue_set 1600");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 5000 && atoi(argv[1]) >= 1500){
				RoboticArm.MaxValue = atoi(argv[1]);
				Flash_Update();
				log_i("Write_Successed!  RoboticArm.MaxValue:  %d",RoboticArm.MaxValue);
		}
		
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(robotic_arm_maxValue_set,ag: robotic_arm_openvalue_set 2000);




/*【机械臂】舵机 修改 【反向最大值】 MSH方法 */
static int robotic_arm_minValue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: RoboticArm_minvalue_set 1150");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 3000 &&  atoi(argv[1]) >= 500){
				RoboticArm.MinValue = atoi(argv[1]);
				Flash_Update();
				log_i("Write_Successed!  RoboticArm.MinValue:  %d",RoboticArm.MinValue);
		}
		else {
				log_e("Error! The value is out of range!");
		}

		
		
_exit:
    return result;
}
MSH_CMD_EXPORT(robotic_arm_minValue_set,ag: robotic_arm_closevalue_set 1500);


/*【机械臂】舵机 修改 速度值 */
static int yuntai_speed_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: yuntai_speed_set 5");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 255 && atoi(argv[1]) > 0){
				YunTai.Speed = atoi(argv[1]);
				Flash_Update();
				log_i("Write_Successed! YunTai.Speed:  %d",YunTai.Speed);
		}
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(yuntai_speed_set,ag: yuntai_speed_set 5);

/*【云台】舵机 修改 【正向最大值】MSH方法 */
static int yuntai_maxValue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: YunTai_maxvalue_set 1600");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 5000){
				YunTai.MaxValue = atoi(argv[1]);
				Flash_Update();
				log_i("Write_Successed! YunTai.MaxValue:  %d",YunTai.MaxValue);
		}
		
		else {
				log_e("Error! The value is out of range!");
		}
_exit:
    return result;
}
MSH_CMD_EXPORT(yuntai_maxValue_set,ag: yuntai_maxvalue_set 2500);




/*【云台】舵机 修改 【反向最大值】 MSH方法 */
static int yuntai_minValue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: YunTai_minvalue_set 1150");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 3000){
				YunTai.MinValue = atoi(argv[1]);
				Flash_Update();
				log_i("Write_Successed! YunTai.MinValue:  %d",YunTai.MinValue);
		}
		else {
				log_e("Error! The value is out of range!");
		}

_exit:
    return result;
}
MSH_CMD_EXPORT(yuntai_minValue_set,ag: yuntai_arm_closevalue_set 1500);

/*【云台】舵机 修改 【反向最大值】 MSH方法 */
static int yuntai_medValue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: YunTai_medvalue_set 2000");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 3000){
				YunTai.MedValue = atoi(argv[1]);
				Flash_Update();
				log_i("Write_Successed! YunTai.MedValue):  %d",YunTai.MedValue);
		}
		else {
				log_e("Error! The value is out of range!");
		}

_exit:
    return result;
}
MSH_CMD_EXPORT(yuntai_medValue_set,ag: yuntai_arm_medvalue_set 2000);



/*【云台】舵机 修改 【当前】 MSH方法 */
static int yuntai_currentValue_set(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        log_e("Error! Proper Usage: YunTai_medvalue_set 2000");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1]) <= 3000 && atoi(argv[1]) >= 500){
				YunTai.CurrentValue = atoi(argv[1]);
				log_i("Write_Successed! Current YunTai.CurrentValue:  %d",YunTai.CurrentValue);
		}
		else {
				log_e("Error! The value is out of range!");
		}

_exit:
    return result;
}
MSH_CMD_EXPORT(yuntai_currentValue_set,ag: yuntai_currentValue_set 1500);


/*【云台】舵机 修改 【当前】 MSH方法 */
static int yuntai(int argc, char **argv)
{
    int result = 0;
		ServoType servo;
    if (argc > 1){
        log_e("Error! Proper Usage: YunTai_medvalue_set 2000");
				result = -RT_ERROR;
        goto _exit;
    }
		rt_kprintf("Set Min Value:");
		scanf("%d",(int *)(&servo.MinValue));

		if(servo.MinValue <= 7000 ){		
				log_i("Write_Successed! servo.MinValue  %d",servo.MinValue);
		}
		else {
				log_e("Error! The value is out of range!");
		}

_exit:
    return result;
}
MSH_CMD_EXPORT(yuntai,ag: yuntai_currentValue_set 1500);


