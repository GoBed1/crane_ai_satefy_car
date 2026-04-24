/*
 * bsp_quadspi_W25Q128.c
 *
 *  Created on: Apr 25, 2021
 *      Author: Administrator
 *
 *
 *
 *
 *
 */
#include <bsp_qspi_w25q128.h>

extern QSPI_HandleTypeDef hqspi;

/* ���ޱ��ļ�ʹ�õĺ��� */
static void QSPI_W25Qx_Write_Enable(QSPI_HandleTypeDef *hqspi);
static uint8_t QSPI_W25Qx_AutoPollingMemRead(uint32_t Timeout);
static void QSPI_W25Qx_Enter(QSPI_HandleTypeDef *hqspi);
static void QSPI_W25Qx_Exit(QSPI_HandleTypeDef *hqspi);

void user_Assert(char *file,uint32_t line)
{
	printf("Wrong parameters value: file %s on line %d\r\n",file, (unsigned int)line);
	return;
}

/**
  * ��������: ��ȡFlash״̬���ȴ���������
  * �������: Timeout���ȴ�ʱ��
 *
  * ����ֵ: Flash��״̬
  * ˵��:
 *
 *
 */
static uint8_t QSPI_W25Qx_AutoPollingMemRead(uint32_t Timeout)
{
	QSPI_CommandTypeDef     s_command={0};
    QSPI_AutoPollingTypeDef s_config={0};

    /* �������� */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;	  //1�߷�ʽ����ָ��
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;       //24λ��ַ
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;  //�޽����ֽ�
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;      //W25Q128FV��֧��DDRģʽ
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;  //DDRģʽ����������ӳ�
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;   //ÿ�δ��䶼��ָ��

    /* �����Զ���ѯģʽ */
    s_command.Instruction       = READ_STATUS_REG1_CMD;  //��ȡ״̬�Ĵ���
    s_command.AddressMode       = QSPI_ADDRESS_NONE;     //û�е�ַ
    s_command.DataMode          = QSPI_DATA_1_LINE;      //1������
    s_command.DummyCycles       = 0;                     //�޿�����

    /* �����Զ���ѯ�Ĵ��������ϲ�ѯ״̬�Ĵ���bit0���ȴ���Ϊ0�� */
    s_config.Match           = 0x00;				   //�ȴ���Ϊ0
    s_config.Mask            = W25Q128FV_FSR_BUSY;     //״̬�Ĵ���bit0
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;	   //�߼���
    s_config.StatusBytesSize = 1;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    /* �Զ���ѯģʽ�ȴ���̽��� */
    if(HAL_QSPI_AutoPolling(&hqspi,&s_command,&s_config,Timeout) != HAL_OK)
    {
	   user_Assert(__FILE__,__LINE__);
    }

    return FLASH_OK;

}

/**
  * ��������: ��ȡ�ⲿFLASH��ID
  * �������: ��
 *
  * ����ֵ: uint32_t
  * ˵��:
 *  1��ʹ��SPIģʽ��ָ��
 *
 */
uint32_t QSPI_W25Qx_ReadID(void)
{
	uint32_t uiID;

	QSPI_CommandTypeDef s_command = {0};
	uint8_t buf[3];

	/* �������� */
	s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;	  //1�߷�ʽ����ָ��
	s_command.AddressSize       = QSPI_ADDRESS_24_BITS;       //24λ��ַ
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;  //�޽����ֽ�
	s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;      //W25Q128FV��֧��DDRģʽ
	s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;  //DDRģʽ����������ӳ�
	s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;   //ÿ�δ��䶼��ָ��

	/* ��ȡJEDEC ID */
	s_command.Instruction       = READ_JEDEC_ID_CMD;  //��ȡID����: 0x9F
	s_command.AddressMode       = QSPI_ADDRESS_NONE;  //û�е�ַ
	s_command.DataMode          = QSPI_DATA_1_LINE;   //1������
	s_command.DummyCycles       = 0;                  //�޿�����
	s_command.NbData            = 3;                  //��ȡ��������

	/* ����ָ�� */
	if(HAL_QSPI_Command(&hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	/* �������� */
	if(HAL_QSPI_Receive(&hqspi,buf,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	uiID = (buf[0] << 16) | (buf[1] << 8) | buf[2];

	return uiID;
}

/**
  * ��������:  �ⲿFLASHдʹ��
  * �������:  hqspi  QSPI_HandleTypeDef���
 *
  * ����ֵ: ��
  * ˵��:
 *  1��ʹ��SPIģʽ��ָ��
 *
 */
static void QSPI_W25Qx_Write_Enable(QSPI_HandleTypeDef *hqspi)
{
	QSPI_CommandTypeDef s_command = {0};
	QSPI_AutoPollingTypeDef s_config={0};

	/* �������� */
	s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;	  //1�߷�ʽ����ָ��
	//s_command.AddressSize       = QSPI_ADDRESS_24_BITS;       //24λ��ַ
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;  //�޽����ֽ�
	s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;      //W25Q128FV��֧��DDRģʽ
	s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;  //DDRģʽ����������ӳ�
	s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;   //ÿ�δ��䶼��ָ��

	/* д��ʹ������ */
	s_command.Instruction       = WRITE_ENABLE_CMD;   //дʹ������
	s_command.AddressMode       = QSPI_ADDRESS_NONE;  //û�е�ַ
	s_command.DataMode          = QSPI_DATA_NONE;     //û������
	s_command.DummyCycles       = 0;                  //�޿�����
	s_command.NbData            = 0;                  //������

	/* ����ָ�� */
	if(HAL_QSPI_Command(hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	/* �����Զ���ѯģʽ�ȴ�������� */
	s_config.Match           = W25Q128FV_FSR_WREN;
	s_config.Mask            = W25Q128FV_FSR_WREN;
	s_config.MatchMode       = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval        = 0x10;
	s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

	s_command.Instruction = READ_STATUS_REG1_CMD;
	s_command.DataMode    = QSPI_DATA_1_LINE;
	s_command.NbData      = 1;

	if(HAL_QSPI_AutoPolling(hqspi,&s_command,&s_config,HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

}

/**
  * ��������:  �����ⲿFLASH����������С��4KB)
  * �������:  _uiSectorAddr: ������ַ����4KBΪ��λ�ĵ�ַ������0��4096��8192��
 *
  * ����ֵ: ��
  * ˵��:
 *  1��
 *
 */
void QSPI_W25Qx_EraseSector(uint32_t _SectorAddr)
{
	QSPI_CommandTypeDef	s_command = {0};

	/* дʹ�� */
	QSPI_W25Qx_Write_Enable(&hqspi);

	/* �������� */
	s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;	  //1�߷�ʽ����ָ��
	s_command.AddressSize       = QSPI_ADDRESS_24_BITS;       //24λ��ַ
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;  //�޽����ֽ�
	s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;      //W25Q128FV��֧��DDRģʽ
	s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;  //DDRģʽ����������ӳ�
	s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;   //ÿ�δ��䶼��ָ��

	/* ������������ */
	s_command.Instruction       = SECTOR_ERASE_CMD;     //��������ָ��
	s_command.AddressMode       = QSPI_ADDRESS_1_LINE;  //1�ߵ�ַ��ʽ
	s_command.DataMode          = QSPI_DATA_NONE;       //û������
	s_command.Address           = _SectorAddr;          //�������׵�ַ����֤��4KB������
	s_command.DummyCycles       = 0;                    //�޿�����

	/* ����ָ�� */
	if(HAL_QSPI_Command(&hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	/* �Զ���ѯģʽ�ȴ���̽��� */
	if(QSPI_W25Qx_AutoPollingMemRead(W25Q128FV_SUBSECTOR_ERASE_MAX_TIME) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

}

/**
  * ��������:  ҳ��̣�ͨ��QSPI������д���ⲿFALSH
  * �������:  _pBuf: ��Ҫ�������ݵ�ָ��
 *          _write_Addr: Ŀ�������׵�ַ����ҳ�׵�ַ������0��256��512�ȡ�
 *          _write_Size: ���ݸ��������ܳ���ҳ�Ĵ�С���������루1 ~ 256��
  * ����ֵ: ��
  * ˵��:
 *    1�������W25Q128FV����֧��SPIģʽд��
 */
uint8_t QSPI_W25Qx_Write_Buffer(uint8_t *_pBuf,uint32_t _write_Addr,uint16_t _write_Size)
{
	QSPI_CommandTypeDef	s_command = {0};

	/* ��ֹд��Ĵ�С����256�ֽ� */
	if(_write_Size > W25Q128FV_PAGE_SIZE)
	{
		/* ������ԣ���ʾ���� */
		user_Assert(__FILE__,__LINE__);
	}

	QSPI_W25Qx_Write_Enable(&hqspi);  //дʹ��

	/* �������� */
	s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;	  //1�߷�ʽ����ָ��
	s_command.AddressSize       = QSPI_ADDRESS_24_BITS;       //24λ��ַ
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;  //�޽����ֽ�
	s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;      //W25Q128FV��֧��DDRģʽ
	s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;  //DDRģʽ����������ӳ�
	s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;   //ÿ�δ��䶼��ָ��

	/*д���������� */
	s_command.Instruction       = QUAD_INPUT_PAGE_PROG_CMD;   //24λ���߿���д��ָ��
	s_command.AddressMode       = QSPI_ADDRESS_1_LINE;        //1�ߵ�ַ��ʽ
	s_command.DataMode          = QSPI_DATA_4_LINES;          //4�����ݷ�ʽ
	s_command.Address           = _write_Addr;                //д�����ݵĵ�ַ
	s_command.NbData            = _write_Size;                //д�����ݵĴ�С
	s_command.DummyCycles       = 0;                          //�޿�����

	/* ����ָ�� */
	if(HAL_QSPI_Command(&hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	/* �������� */
	if(HAL_QSPI_Transmit(&hqspi,_pBuf,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	/* �Զ���ѯģʽ�ȴ���̽��� */
	if(QSPI_W25Qx_AutoPollingMemRead(W25Q128FV_SUBSECTOR_ERASE_MAX_TIME) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	return 1;
}

/**
  * ��������:  �ⲿFLASHоƬ����QSPIģʽ
  * �������:  *hqspi: qspi���
 *
  * ����ֵ: ��
  * ˵��:
 *
 */
static void QSPI_W25Qx_Enter(QSPI_HandleTypeDef *hqspi)
{
	QSPI_CommandTypeDef	s_command = {0};
	/* ����FLASH����QPSIģʽ */
	/* �������� */
	s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;	  //1�߷�ʽ����ָ��
	s_command.AddressSize       = QSPI_ADDRESS_24_BITS;       //24λ��ַ
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;  //�޽����ֽ�
	s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;      //W25Q128FV��֧��DDRģʽ
	s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;  //DDRģʽ����������ӳ�
	s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;   //ÿ�δ��䶼��ָ��

	/* д���������� */
	s_command.Instruction       = ENTER_QPI_MODE_CMD;         //����QSPIģʽ
	s_command.AddressMode       = QSPI_ADDRESS_NONE;          //�޵�ַ��ʽ
	s_command.DataMode          = QSPI_DATA_NONE;             //�����ݷ�ʽ
	s_command.DummyCycles       = 0;                          //0����״̬����

	/* ����ָ�� */
	if(HAL_QSPI_Command(hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

}

/**
  * ��������:  �ⲿFLASHоƬ�˳�QSPIģʽ
  * �������:  *hqspi: qspi���
 *
  * ����ֵ: ��
  * ˵��:
 *
 */
static void QSPI_W25Qx_Exit(QSPI_HandleTypeDef *hqspi)
{
	QSPI_CommandTypeDef	s_command = {0};

    /* �������� */
	s_command.InstructionMode   = QSPI_INSTRUCTION_4_LINES;	  //4�߷�ʽ����ָ��
	s_command.AddressSize       = QSPI_ADDRESS_24_BITS;       //24λ��ַ
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;  //�޽����ֽ�
	s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;      //W25Q128FV��֧��DDRģʽ
	s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;  //DDRģʽ����������ӳ�
	s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;   //ÿ�δ��䶼��ָ��

	/* д���������� */
	s_command.Instruction       = EXIT_QPI_MODE_CMD;         //����QSPIģʽ
	s_command.AddressMode       = QSPI_ADDRESS_NONE;          //�޵�ַ��ʽ
	s_command.DataMode          = QSPI_DATA_NONE;             //�����ݷ�ʽ
	s_command.DummyCycles       = 0;                          //0����״̬����

	/* ����ָ�� */
	if(HAL_QSPI_Command(hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}
}

/**
  * ��������:  ������ȡ�����ֽڣ��ֽڵĸ������ܳ���оƬ����
  * �������:  _pBuf: ��ȡ���ݵĴ�ŵ�ַ
 *          _read_Addr: ��ʼ�ĵ�ַ
 *          _read_Size: ���ݸ��������Դ���W25Q128FV_PAGE_SIZE,�����ܳ���оƬ������
  * ����ֵ: ��
  * ˵��:
 *    1����SPIģʽ�л���QSPIģʽ����ȡ��Ϻ��л���SPIģʽ��������������֧��SPIģʽ����
 */
void QSPI_W25Qx_Read_Buffer(uint8_t *_pBuf,uint32_t _read_Addr,uint32_t _read_Size)
{
	QSPI_CommandTypeDef	s_command = {0};

	/* ����QSPIģʽ */
	QSPI_W25Qx_Enter(&hqspi);

	/* ��ʼ��FLASH��ȡ���� */
	/* �������� */
	s_command.InstructionMode   = QSPI_INSTRUCTION_4_LINES;	  //1�߷�ʽ����ָ��
	s_command.AddressSize       = QSPI_ADDRESS_24_BITS;       //24λ��ַ
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;  //�޽����ֽ�
	s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;      //W25Q128FV��֧��DDRģʽ
	s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;  //DDRģʽ����������ӳ�
	s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;   //ÿ�δ��䶼��ָ��

	/*��ȡ�������� */
	s_command.Instruction       = QUAD_INOUT_FAST_READ_CMD;     //24λ���߿���д��ָ��
	s_command.AddressMode       = QSPI_ADDRESS_4_LINES;       //4�ߵ�ַ��ʽ
	s_command.DataMode          = QSPI_DATA_4_LINES;          //4�����ݷ�ʽ
	s_command.Address           = _read_Addr;                 //д�����ݵĵ�ַ
	s_command.NbData            = _read_Size;                 //д�����ݵĴ�С
	s_command.DummyCycles       = 2;                          //��������״̬���ڣ�4��ʱ�����ڣ������ʱ�����

	/* ����ָ�� */
	if(HAL_QSPI_Command(&hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	/* ��ȡ���� */
	if(HAL_QSPI_Receive(&hqspi,_pBuf,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	/* �˳�QSPIģʽ */
	QSPI_W25Qx_Exit(&hqspi);

}

/**
  * ��������: ��λ�ⲿFlash
  * �������: ��
 *
  * ����ֵ: void
  * ˵��:
 *
 */
void QSPI_W25Qx_Reset_Memory()
{
	QSPI_CommandTypeDef s_command = {0};

	/* �������� */
	s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;	  //1�߷�ʽ����ָ��
	s_command.AddressSize       = QSPI_ADDRESS_24_BITS;       //24λ��ַ
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;  //�޽����ֽ�
	s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;      //W25Q128FV��֧��DDRģʽ
	s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;  //DDRģʽ����������ӳ�
	s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;   //ÿ�δ��䶼��ָ��

	/* ��λʹ��W25x */
	s_command.Instruction       = RESET_ENABLE_CMD;   //��λʹ������
	s_command.AddressMode       = QSPI_ADDRESS_NONE;  //û�е�ַ
	s_command.DataMode          = QSPI_DATA_NONE;     //û������
	s_command.DummyCycles       = 0;                  //�޿�����

	/* ���͸�λʹ������ */
	if(HAL_QSPI_Command(&hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	/* ���͸�λ���� */
	s_command.Instruction       = RESET_MEMORY_CMD;   //��λ����
	/* ���͸�λʹ������ */
	if(HAL_QSPI_Command(&hqspi,&s_command,HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

	/* �Զ���ѯģʽ�ȴ��ȴ���� */
	if(QSPI_W25Qx_AutoPollingMemRead(HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		user_Assert(__FILE__,__LINE__);
	}

}

