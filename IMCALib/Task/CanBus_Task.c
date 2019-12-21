/**********************************************************************************************************************
 * @file  CanBus_Task.c
 * @brief CAN1�˲��������ú�CAN�Ŀ�������CAN�����Ͻ��ձ��ĺͷ��ͱ��ĵ�CAN������
 *
**********************************************************************************************************************/

#include "CanBus_Task.h"


moto_measure_t  Chassis_Motor[4];  //���̵�������ṹ��
moto_measure_t  Gimbal_Motor[2];  //��̨��������ṹ��
uint16_t can_cnt;



/**********************************************************************************************************************
  * @Func	 CANFilterInit
  * @Brief   CAN1�˲�������
  * @Param	 CAN_HandleTypeDef* _hcan
  * @Retval	 None
 *********************************************************************************************************************/
void CANFilterInit(void)
{

	CAN_FilterTypeDef		CAN_FilterConfigStructure;

    /*filter config for can1*/
	CAN_FilterConfigStructure.FilterBank = 0;                      // filter 0
	CAN_FilterConfigStructure.FilterMode = CAN_FILTERMODE_IDMASK;  // mask mode
	CAN_FilterConfigStructure.FilterScale = CAN_FILTERSCALE_32BIT;
	CAN_FilterConfigStructure.FilterIdHigh = 0x0000;
	CAN_FilterConfigStructure.FilterIdLow = 0x0000;
	CAN_FilterConfigStructure.FilterMaskIdHigh = 0x0000;
	CAN_FilterConfigStructure.FilterMaskIdLow = 0x0000;            // set mask 0 to receive all can id
	CAN_FilterConfigStructure.FilterFIFOAssignment = CAN_RX_FIFO0; // assign to fifo0
	CAN_FilterConfigStructure.FilterActivation = ENABLE;           // enable can filter
	CAN_FilterConfigStructure.SlaveStartFilterBank = 14;           // only meaningful in dual can mode
	
	HAL_CAN_ConfigFilter(&hcan1, &CAN_FilterConfigStructure);       // init can filter
    HAL_CAN_Start(&hcan1);                                         // start can1
    HAL_CAN_ActivateNotification(&hcan1,CAN_IT_RX_FIFO0_MSG_PENDING);  // enable can1 rx interrupt

}


/**********************************************************************************************************************
  * @brief  can rx callback, get motor feedback info
  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
  *         the configuration information for the specified CAN.
  * @retval None
 *********************************************************************************************************************/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *_hcan)
{
    
    CAN_RxHeaderTypeDef   rx_header;
    uint8_t               rx_data[8];

	if(_hcan->Instance == CAN1)
	{
	    HAL_CAN_GetRxMessage(_hcan, CAN_RX_FIFO0, &rx_header, rx_data); /*recive can data*/
        can_cnt++;
	}
	switch(rx_header.StdId)
    {
        
		case CAN_3508Moto1_ID:
		case CAN_3508Moto2_ID:
		case CAN_3508Moto3_ID:
		case CAN_3508Moto4_ID:
        {
            static u8 i;
            i = rx_header.StdId - CAN_3508Moto1_ID;
            
            Chassis_Motor[i].msg_cnt++ <= 50 ? GetMotorOffset(&Chassis_Motor[i], rx_data) : GetMotorMeasure(&Chassis_Motor[i], rx_data);  /*��ȡ���̵��������Ϣ*/
            

        }break;
	    case CAN_YAW_Motor_ID: /*��Ӧ��6020�����IDΪ1*/
        {
            
            Gimbal_Motor[0].msg_cnt++ <= 50 ? GetMotorOffset(&Gimbal_Motor[0], rx_data) : GetMotorMeasure(&Gimbal_Motor[0], rx_data);  /*��ȡyaw����������Ϣ*/
            
        }break;      
        
	}
	if(can_cnt == 500)  /*����ָʾCANͨ���Ƿ�����*/
	{
		can_cnt = 0;
		HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);  
		
	}

}


/**********************************************************************************************************************
 * @brief    ����3508���ͨ��CAN����������Ϣ��2006���Ҳ���ã�����2006���û���¶�ֵ����
 * @param	 moto_measure_t *ptr����������ṹ��
 *           uint8_t can_rx_data[]��CAN�������ݻ�����
 * @retval	 None
**********************************************************************************************************************/
void GetMotorMeasure(moto_measure_t *ptr, uint8_t can_rx_data[])
{

	ptr->last_angle = ptr->angle;
	ptr->angle = (uint16_t)(can_rx_data[0]<<8 | can_rx_data[1]);
	ptr->speed_rpm  = (int16_t)(can_rx_data[2]<<8 | can_rx_data[3]);
	ptr->real_current = (can_rx_data[4]<<8 | can_rx_data[5])*5.f/16384.f;
	ptr->hall = can_rx_data[6];
    
	/*����������㴦��*/
	if(ptr->angle - ptr->last_angle > 4096)
		ptr->round_cnt --;
	else if (ptr->angle - ptr->last_angle < -4096)
		ptr->round_cnt ++;
    
    /*�õ�ת�����ܽǶȣ�����Ƕ���������ϵ�ʱ�ĽǶ�*/
	ptr->total_angle = ptr->round_cnt * 8192 + ptr->angle - ptr->offset_angle; 
    
}



/*this function should be called after system+can init */
void GetMotorOffset(moto_measure_t *ptr, uint8_t can_rx_data[])        
{
	ptr->angle = (uint16_t)(can_rx_data[0]<<8 | can_rx_data[1]) ;
	ptr->offset_angle = ptr->angle;
}



/**********************************************************************************************************************
 * @brief  send motor control message through can bus
 * @param  CAN_HandleTypeDef *hcan             CAN handle Structure
 *         s16 iq1, s16 iq2, s16 iq3, s16 iq4  �����ĸ�����ĵ���ֵ
 * @retval None
 * @others ��ʶ��0x200��Ӧǰ�ĸ�ID�ĵ������ʶ��0x1FF��Ӧ���ĸ�ID�ĵ����һ·CAN�����ԽӰ˸������
 *         �˺����Ƕ�Ӧǰ4��ID�ĵ��
**********************************************************************************************************************/
void SetChassisMotorCurrent(CAN_HandleTypeDef *hcan,s16 iq1, s16 iq2, s16 iq3, s16 iq4)
{
    
    CAN_TxHeaderTypeDef   tx_header;
    uint8_t               tx_data[8];

	tx_header.StdId = 0x200;
	tx_header.IDE = CAN_ID_STD;
	tx_header.RTR = CAN_RTR_DATA;
	tx_header.DLC = 0x08;
	tx_data[0] = (iq1 >> 8);
	tx_data[1] = iq1;
	tx_data[2] = (iq2 >> 8);
	tx_data[3] = iq2;
	tx_data[4] = (iq3 >> 8);
	tx_data[5] = iq3;
	tx_data[6] = (iq4 >> 8);
	tx_data[7] = iq4;
	
    HAL_CAN_AddTxMessage(hcan, &tx_header, tx_data,(uint32_t*)CAN_TX_MAILBOX0);

}	


/**********************************************************************************************************************
 * @brief  send motor control message through can bus
 * @param  CAN_HandleTypeDef *hcan             CAN handle Structure
 *         s16 iq1, s16 iq2, s16 iq3, s16 iq4  �����ĸ�����ĵ���ֵ
 * @retval None
 * @others ��ʶ��0x200��Ӧǰ�ĸ�ID�ĵ������ʶ��0x1FF��Ӧ���ĸ�ID�ĵ����һ·CAN�����ԽӰ˸������
 *         �˺����Ƕ�Ӧ��4��ID�ĵ��
**********************************************************************************************************************/
void SetGimbalMotorValue(CAN_HandleTypeDef *hcan,s16 iq1, s16 iq2, s16 iq3, s16 iq4)
{
    
    CAN_TxHeaderTypeDef   tx_header;
    uint8_t               tx_data[8];

	tx_header.StdId = 0x1FF;
	tx_header.IDE = CAN_ID_STD;
	tx_header.RTR = CAN_RTR_DATA;
	tx_header.DLC = 0x08;
	tx_data[0] = (iq1 >> 8);
	tx_data[1] = iq1;
	tx_data[2] = (iq2 >> 8);
	tx_data[3] = iq2;
	tx_data[4] = (iq3 >> 8);
	tx_data[5] = iq3;
	tx_data[6] = (iq4 >> 8);
	tx_data[7] = iq4;
	
    HAL_CAN_AddTxMessage(hcan, &tx_header, tx_data,(uint32_t*)CAN_TX_MAILBOX0);

}	