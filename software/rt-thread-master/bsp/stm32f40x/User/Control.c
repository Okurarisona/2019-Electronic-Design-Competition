/*
 * Control.c
 *
 *  Created on: 2019��3��20��
 *      Author: zengwangfa
 *      Notes:  �˶��ܿ���
 */
 
#define LOG_TAG "Control"

#include <rtthread.h>
#include <elog.h>
#include <stdlib.h>
#include <math.h>

#include "Control.h"
#include "PID.h"
#include "rc_data.h"
#include "ret_data.h"
#include <stdio.h>

#include "focus.h"
#include "led.h"
#include "servo.h"
#include "PropellerControl.h"
#include "propeller.h"
#include "sensor.h"


#define PITCH_YUNTAI_MED  750
#define YAW_YUNTAI_MEN  1800
int target_x = 160;//Ŀ������
int target_y = 120;


int coords_x = -128;//��ǰ����
int coords_y = 0;

Cyc Cycle = {0,0,250,250,0};
Rectange_Type Rectange = {100,100};
Trigonometric_Type Sin = {0,0};
Trigonometric_Type Cos = {0,PI};
Star_Type Star = {0,50};

uint16 Pitch_Axis_Output_Limit_Left(int16 value)
{
		//������+500   ������-500
		value = (value) > PITCH_YUNTAI_MED+300 ? PITCH_YUNTAI_MED+300 : value ;//�����޷�
		value = (value) < PITCH_YUNTAI_MED-300 ? PITCH_YUNTAI_MED-300 : value;//�����޷�
	
		return value ;
}

uint16 Yaw_Axis_Output_Limit_Right(int16 value)
{
		//������+500   ������-500
		value = (value) > YAW_YUNTAI_MEN+400 ? YAW_YUNTAI_MEN+400 : value ;//�����޷�
		value = (value) < YAW_YUNTAI_MEN-400 ? YAW_YUNTAI_MEN-400 : value;//�����޷�
	
		return value ;
}

uint8 cycle_flag = 0;
/* ������x,y,�뾶r*/
void draw_cycle(int *x,int *y,int r)
{	

			if(0 == cycle_flag){//�ϰ�Բ
					(*x)++;
					if((*x)>=r){
							(*x) = r;
							cycle_flag = 1;
					}
					*y = -sqrt((r*r)- ((*x)*(*x))); 
			}
			else if(1 == cycle_flag){
					(*x)--;
					if((*x)<=(-r)){
							(*x) = (-r);
							cycle_flag = 0;
					}
					*y = sqrt((r*r)- ((*x)*(*x))); 
			}
			
			//rt_thread_mdelay(1);

}

void DrawCycle(int *x,int *y,int r)
{	
	static float Angle = 0;
	static int rec = 1;
	Angle = atan2(*y,*x);
	if(rec)
	{
		*x = r * cos(Angle);
		*y = r * sin(Angle);
		rec = 0;
	}
	if(*x>r*cos(PI/4))
	{
		(*y)++;
		 *x = sqrt( r*r - ((*y)*(*y)));
	}
	else if(*x < r*cos(PI*3/4))
	{
		(*y)--;
		*x = sqrt( r*r - ((*y)*(*y)));
	}
	else
	{
		if(*y >= r*sin(PI/4) )
		{
			(*x)--;
			*y = sqrt((r*r)- ((*x)*(*x)));
		}
		if(*y <= (r*sin(-PI/4)) )
		{
			(*x)++;
			*y = -(sqrt((r*r)- ((*x)*(*x))));
		}
	}
	
}
void DrawCyc(Cyc *cyc)
{
	cyc->Angle+=0.01f;
	cyc->x = cyc->a * cos(cyc->Angle);
	cyc->y = cyc->b * sin(cyc->Angle);
}

void DrawRetange(Rectange_Type *Rec)
{
	static int Flag = 1, Size_X,Size_Y;
	if(Flag)
	{
		Flag = 0;
		Size_X = Rec->x;
		Size_Y = Rec->y;
	}
	if(Rec->y>=Size_Y)
	{
		Rec->x--;
	}
	if(Rec->x <= -Size_X)
	{
		Rec->y--;		
	}
	if(Rec->y<=-Size_Y)
	{
		Rec->x++;
		
	}
	if(Rec->x>=Size_X)
	{
		Rec->y++;
	}

}
void DrawSin(Trigonometric_Type *Sin)
{
	Sin->x += 1;
	Sin->y = 100*sin(Sin->x/100);
}
void DrawCos(Trigonometric_Type *Cos)
{
	Cos->x += 1;
	Cos->y = 100*cos(Cos->x/100);
}

void DrawStar(Star_Type *Star)
{
	static int t = 0;
	static int Mode = 1;
	static char str[100];
	switch(Mode)
	{
		case 1:t++;Star->x = t*cos(0.4f*PI);
				Star->y = 50 - t*sin(0.4f*PI);
				if(t>=100)
				{
					Mode  = 2;
					t = 0;
				}
				break;
		case 2:t++;Star->x = 31- t*cos(0.2f*PI);
				Star->y = -45 + t*sin(0.2f*PI);
				if(t>=100)
				{
					Mode  = 3;
					t = 0;
				}break;
		case 3:t++;Star->x = -50 +t;
				Star->y = 14 ;
				if(t>=100)
				{
					Mode  = 4;
					t = 0;
				}break;
		case 4:t++;Star->x = 50 - t*cos(0.2f*PI);
				Star->y = 14 - t*sin(0.2f*PI);
				if(t>=100)
				{
					Mode  = 5;
					t = 0;
				}break;
		case 5:t++;Star->x = -31 + t*cos(0.4f*PI);
				   Star->y = -45 +  t*sin(0.4f*PI);
				if(t>=100)
				{
					Mode  = 1;
					t = 0;
				}break;
	}
	
}



int test_x = -250;
int test_y = 0;
void Two_Axis_Yuntai_Control(void)
{

	
	
//		coords_x = get_persent_x();//��ȡ С��X��
//	  coords_y = get_persent_y();//��ȡ С��Y��

		
    //yuntai_pid_control(coords_x,target_x,coords_y,target_y);
		//DrawCyc(&Cycle);
		//DrawSin(&Sin);
		//DrawCos(&Cos);
	DrawStar(&Star);

// 		PropellerPower.leftMiddle  = YAW_YUNTAI_MEN   + Total_Controller.Yaw_Angle_Control.Control_OutPut;   //ˮƽ
//		PropellerPower.rightMiddle = PITCH_YUNTAI_MED - Total_Controller.Pitch_Angle_Control.Control_OutPut; //����    �����  ������Ƹ����Ķ�� Y��
		PropellerPower.leftMiddle = YAW_YUNTAI_MEN + Cycle.x;
		PropellerPower.rightMiddle = PITCH_YUNTAI_MED + Cycle.y;

		PropellerPower.leftMiddle  = Yaw_Axis_Output_Limit_Right(PropellerPower.leftMiddle);  //��̨
		PropellerPower.rightMiddle = Pitch_Axis_Output_Limit_Left(PropellerPower.rightMiddle);			
	
		TIM4_PWM_CH1_D12(PropellerPower.leftMiddle);  //����   D12
		TIM4_PWM_CH2_D13(PropellerPower.rightMiddle); //����   D13
}


 

/*����� ������yaw MSH���� */
static int coords(int argc, char **argv)
{
    int result = 0;
    if (argc != 3){
        rt_kprintf("Error! Proper Usage: coords 50 50");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1])<10000 && atoi(argv[2])<10000  ){
				
				coords_x = atoi(argv[1]);
				coords_y = atoi(argv[2]);
			
		}
		else {
				log_e("Error! The  value is out of range!");
		}

		
_exit:
    return result;
}
MSH_CMD_EXPORT(coords,ag: coords 50 50);


























void Depth_PID_Control(float expect_depth,float sensor_depth)
{
		
		Total_Controller.High_Position_Control.Expect = expect_depth ; //���������ң��������
		Total_Controller.High_Position_Control.FeedBack = sensor_depth;  //��ǰ��ȷ���
		PID_Control(&Total_Controller.High_Position_Control);//�߶�λ�ÿ�����
	
		robot_upDown(&Total_Controller.High_Position_Control.Control_OutPut);		//��ֱ�ƽ�������
}



void Gyro_Control(void)//���ٶȻ�
{

//  	ƫ����ǰ������
//  	Total_Controller.Yaw_Gyro_Control.FeedBack=Yaw_Gyro;


//		PID_Control_Div_LPF(&Total_Controller.Yaw_Gyro_Control);
//		Yaw_Gyro_Control_Expect_Delta=1000*(Total_Controller.Yaw_Gyro_Control.Expect-Last_Yaw_Gyro_Control_Expect)
//			/Total_Controller.Yaw_Gyro_Control.PID_Controller_Dt.Time_Delta;
//		//**************************ƫ����ǰ������**********************************
//		Total_Controller.Yaw_Gyro_Control.Control_OutPut+=Yaw_Feedforward_Kp*Total_Controller.Yaw_Gyro_Control.Expect
//			+Yaw_Feedforward_Kd*Yaw_Gyro_Control_Expect_Delta;//ƫ����ǰ������
//		Total_Controller.Yaw_Gyro_Control.Control_OutPut=constrain_float(Total_Controller.Yaw_Gyro_Control.Control_OutPut,
//																																		 -Total_Controller.Yaw_Gyro_Control.Control_OutPut_Limit,
//																																		 Total_Controller.Yaw_Gyro_Control.Control_OutPut_Limit);
//		Last_Yaw_Gyro_Control_Expect=Total_Controller.Yaw_Gyro_Control.Expect;
//		

}

/*����� ������yaw MSH���� */
static int depth(int argc, char **argv)
{
    int result = 0;
    if (argc != 2){
        rt_kprintf("Error! Proper Usage: RoboticArm_openvalue_set 1600");
				result = -RT_ERROR;
        goto _exit;
    }
		if(atoi(argv[1])<100){
				Expect_Depth = atoi(argv[1]);
		}
		else {
				log_e("Error! The  value is out of range!");
		}

		
_exit:
    return result;
}
MSH_CMD_EXPORT(depth,ag: depth 10);





/*�������� �޸�MSH���� */
static int unlock(int argc, char **argv) //ֻ���� 0~3.0f
{
		ControlCmd.All_Lock = UNLOCK;
		return 0;
}
MSH_CMD_EXPORT(unlock,unlock);


/*�������� �޸�MSH���� */
static int lock(int argc, char **argv) //ֻ���� 0~3.0f
{
		ControlCmd.All_Lock = LOCK;
		return 0;
}
MSH_CMD_EXPORT(lock,lock);

