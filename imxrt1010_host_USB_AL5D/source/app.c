/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "usb_host_hid.h"
#include "host_keyboard_mouse.h"
#include "host_keyboard.h"
#include "host_mouse.h"
#include "fsl_common.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */
#include "app.h"

#if ((!USB_HOST_CONFIG_KHCI) && (!USB_HOST_CONFIG_EHCI) && (!USB_HOST_CONFIG_OHCI) && (!USB_HOST_CONFIG_IP3516HS))
#error Please enable USB_HOST_CONFIG_KHCI, USB_HOST_CONFIG_EHCI, USB_HOST_CONFIG_OHCI, or USB_HOST_CONFIG_IP3516HS in file usb_host_config.
#endif

#include "usb_phy.h"

/* ADD_ON UART COMMUNICATION  */
#include "fsl_iomuxc.h"
#include "fsl_lpuart.h"

/* ADD_ON DMA */

#include "fsl_edma.h"
#include "fsl_flexspi_edma.h"
#include "fsl_flexspi.h"
#include "fsl_dmamux.h"
#include "fsl_cache.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define CONTROL_MODE				AUTOMATIC
#define SEQUENCE_AUTOMATIC			INIT
//#define SEQUENCE_AUTOMATIC			HISTORY
#define CONTRY_KEYBOARD				FR
#define LPUART_PC_BOARD            	LPUART1
#define LPUART_BOARD_ARM	   		LPUART4
#define DEMO_LPUART_CLK_FREQ   	 	BOARD_DebugConsoleSrcFreq()
#define DEMO_LPUART_IRQn         	LPUART1_IRQn
#define DEMO_LPUART_IRQHandler   	LPUART1_IRQHandler
#define BOARD_ARM_UART_BAUDRATE (9600U)

/*! @brief Ring buffer size (Unit: Byte). */
#define DEMO_RING_BUFFER_SIZE 	 	16
#define SIZE_BUFFER 		 	 	DEMO_RING_BUFFER_SIZE
/*! @brief Ring buffer to save received data. */

#define MAX_SIZE_ERROR_BUFFER       100
#define MAX_SIZE_INIT_BUFFER		50
#define NB_SERVO 					6
#define LIMIT_HIGH_GRIPPER			2000
#define LIMIT_LOW_GRIPPER			800
#define LIMIT_HIGH_WRIST			2000
#define LIMIT_LOW_WRIST				800
#define LIMIT_HIGH_ELBOW			2000
#define LIMIT_LOW_ELBOW				800
#define LIMIT_HIGH_SHOULDER			2000
#define LIMIT_LOW_SHOULDER			800
#define LIMIT_HIGH_BASE				2000
#define LIMIT_LOW_BASE				800
#define LIMIT_LOW_ROTATION			800
#define LIMIT_HIGH_ROTATION			2000
#define DISTANCE					50
#define DATA_STORE_SIZE				100


#define ERROR_SIG_HIGH				1u
#define ERROR_SIG_LOW				2u
#define ERROR_SIG_KEY				3u
#define NO_ERROR					4u

typedef enum {
	BASE, SHOULDER, ELBOW, WRIST, GRIPPER, ROTATION
}SERVO_ID;

typedef enum {
	FLAG_INVALIDE_DATA,
	FLAG_VALIDE_DATA,
	CONTROLE_DATA,
	NO_DATA
} flag_receive_data_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief host callback function.
 *
 * device attach/detach callback function.
 *
 * @param deviceHandle          device handle.
 * @param configurationHandle   attached device's configuration descriptor information.
 * @param eventCode             callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The application don't support the configuration.
 */
static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode);

/*!
 * @brief application initialization.
 */
static void USB_HostApplicationInit(void);

extern void USB_HostClockInit(void);
extern void USB_HostIsrEnable(void);
extern void USB_HostTaskFn(void *param);
void BOARD_InitHardware(void);

flag_receive_data_t  mapper_for_ARM(uint8_t c, uint8_t *sequence, uint8_t *sizeSequence);
void convert_pos_toString(uint16_t pos, uint8_t *posBuffer, uint8_t* bufferSize);
void initSeq_ARM(void);
void send_info_seq();

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief USB host mouse instance global variable */
extern usb_host_mouse_instance_t g_HostHidMouse;
/*! @brief USB host keyboard instance global variable */
extern usb_host_keyboard_instance_t g_HostHidKeyboard;
usb_host_handle g_HostHandle;

uint8_t sequenceBuffer[DEMO_RING_BUFFER_SIZE];
uint8_t dataStore[DATA_STORE_SIZE][DEMO_RING_BUFFER_SIZE];
uint8_t sizeSequence[DATA_STORE_SIZE];
uint16_t txIndex; /* Index of the data to send out. */
uint16_t rxIndex; /* Index of the memory to save new arrived data. */
uint16_t storeIndex;
uint16_t actual_pos[NB_SERVO];
uint16_t request_pos[NB_SERVO];
uint8_t error_signal = NO_ERROR;

/*******************************************************************************/

static uint8_t s_nor_read_buffer[256];
uint8_t demoRingBuffer[DEMO_RING_BUFFER_SIZE];

/*******************************************************************************
 * Code
 ******************************************************************************/

void USB_OTG1_IRQHandler(void)
{
    USB_HostEhciIsrFunction(g_HostHandle);
}

void USB_HostClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };

    CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
    CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 480000000U);
    USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
}

void USB_HostIsrEnable(void)
{
    uint8_t irqNumber;

    uint8_t usbHOSTEhciIrq[] = USBHS_IRQS;
    irqNumber                = usbHOSTEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];
/* USB_HOST_CONFIG_EHCI */

/* Install isr, set priority, and enable IRQ. */
#if defined(__GIC_PRIO_BITS)
    GIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#else
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#endif
    EnableIRQ((IRQn_Type)irqNumber);
}

void USB_HostTaskFn(void *param)
{
    USB_HostEhciTaskFunction(param);
}

/*!
 * @brief USB isr function.
 */

static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode)
{
    usb_status_t status1;
    usb_status_t status2;
    usb_status_t status = kStatus_USB_Success;

    switch (eventCode & 0x0000FFFFU)
    {
        case kUSB_HostEventAttach:
            status1 = USB_HostHidKeyboardEvent(deviceHandle, configurationHandle, eventCode);
            status2 = USB_HostHidMouseEvent(deviceHandle, configurationHandle, eventCode);
            if ((status1 == kStatus_USB_NotSupported) && (status2 == kStatus_USB_NotSupported))
            {
                status = kStatus_USB_NotSupported;
            }
            break;

        case kUSB_HostEventNotSupported:
            usb_echo("device not supported.\r\n");
            break;

        case kUSB_HostEventEnumerationDone:
            status1 = USB_HostHidKeyboardEvent(deviceHandle, configurationHandle, eventCode);
            status2 = USB_HostHidMouseEvent(deviceHandle, configurationHandle, eventCode);
            if ((status1 != kStatus_USB_Success) && (status2 != kStatus_USB_Success))
            {
                status = kStatus_USB_Error;
            }
            break;

        case kUSB_HostEventDetach:
            status1 = USB_HostHidKeyboardEvent(deviceHandle, configurationHandle, eventCode);
            status2 = USB_HostHidMouseEvent(deviceHandle, configurationHandle, eventCode);
            if ((status1 != kStatus_USB_Success) && (status2 != kStatus_USB_Success))
            {
                status = kStatus_USB_Error;
            }
            break;

        case kUSB_HostEventEnumerationFail:
            usb_echo("enumeration failed\r\n");
            break;

        default:
            break;
    }
    return status;
}

static void USB_HostApplicationInit(void)
{
    usb_status_t status = kStatus_USB_Success;

    USB_HostClockInit();

#if ((defined FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT))
    SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

    status = USB_HostInit(CONTROLLER_ID, &g_HostHandle, USB_HostEvent);
    if (status != kStatus_USB_Success)
    {
        usb_echo("host init error\r\n");
        return;
    }
    USB_HostIsrEnable();

    usb_echo("host init done\r\n");
}

void BOARD_InitPins_UART4(void) {
  CLOCK_EnableClock(kCLOCK_Iomuxc);

  IOMUXC_SetPinMux(
		  IOMUXC_GPIO_AD_02_LPUART4_TXD,
		  0U);
  IOMUXC_SetPinMux(
		  IOMUXC_GPIO_AD_01_LPUART4_RXD,
		  0U);

  IOMUXC_SetPinConfig(
		  IOMUXC_GPIO_AD_02_LPUART4_TXD,
		  0x10A0U);

  IOMUXC_SetPinConfig(
		  IOMUXC_GPIO_AD_01_LPUART4_RXD,
		  0x10A0U);
}

// cmd of type #1P1500\r
void initSeq_ARM(void) {
	uint8_t i;
	char temp;
	uint8_t initBuffer[SIZE_BUFFER];

	initBuffer[0] = '#';
	for(i = 0; i < NB_SERVO + 1; i++) {
		temp = i + '0';
		initBuffer[1] = temp; /**convertir en char*/
		initBuffer[2] = 'P';
		initBuffer[3] = '1';
		initBuffer[4] = '5';
		initBuffer[5] = '0';
		initBuffer[6] = '0';
		initBuffer[7] = '\r';
		actual_pos[i] = 1500;
		uint8_t k = 0;
	    rxIndex    = 0;
	    storeIndex = 0;
	    txIndex    = 0;
//	    add_on_LPUART_Write_StringNonBlocking(LPUART_BOARD_ARM, initBuffer);
//	    add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, initBuffer);
		for (k = 0; k < 8 ; k++) {
			while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_BOARD_ARM)));
			LPUART_WriteByte(LPUART_BOARD_ARM, initBuffer[k]);
			while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_PC_BOARD)));
			LPUART_WriteByte(LPUART_PC_BOARD, initBuffer[k]);
		}
	}
}

flag_receive_data_t  mapper_for_ARM(uint8_t c, uint8_t *sequence, uint8_t *sizeSequence) {
	uint8_t error_string[MAX_SIZE_ERROR_BUFFER] = "limit ";
	char id_servo;
	uint16_t position;
	uint8_t buffer_pos[16];
	flag_receive_data_t error_flag = FLAG_VALIDE_DATA;

	uint8_t id_sequence = 0;
	uint8_t test_error = 1;
	switch(c) {
#if defined(CONTRY_KEYBOARD) && (CONTRY_KEYBOARD == FR)
	case 'a':
#else
	case 'q': /*BASE LEFT/ or HIGH*/
#endif
		request_pos[BASE] = (actual_pos[BASE] - DISTANCE);
		if (request_pos[BASE] > LIMIT_LOW_BASE) {
			actual_pos[BASE] = request_pos[BASE];
			position = actual_pos[BASE];
			id_servo = BASE + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}

		break;
	case 'd':
		request_pos[BASE] = (actual_pos[BASE] + DISTANCE);
		if (request_pos[BASE] < LIMIT_HIGH_BASE) {
			actual_pos[BASE] = request_pos[BASE];
			position = actual_pos[BASE];
			id_servo = BASE + '0';
		} else {
			error_signal = ERROR_SIG_HIGH;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}
		break;
#if defined(CONTRY_KEYBOARD) && (CONTRY_KEYBOARD == FR)
	case 'w':
#else
	case 'z': /*SHOULDER LEFT/ or HIGH*/
#endif
		request_pos[SHOULDER] = (actual_pos[SHOULDER] + DISTANCE);
		if (request_pos[SHOULDER] < LIMIT_HIGH_SHOULDER) {
			actual_pos[SHOULDER] = request_pos[SHOULDER];
			position = actual_pos[SHOULDER];
			id_servo = SHOULDER + '0';
		} else {
			error_signal = ERROR_SIG_HIGH;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}
		break;
	case 's': /*SHOULDER LEFT/ or HIGH*/
		request_pos[SHOULDER] = (actual_pos[SHOULDER] - DISTANCE);
		if (request_pos[SHOULDER] > LIMIT_LOW_SHOULDER) {
			actual_pos[SHOULDER] = request_pos[SHOULDER];
			position = actual_pos[SHOULDER];
			id_servo = SHOULDER + '0';
			uint8_t test_error = 1;
		} else {
			error_signal = ERROR_SIG_LOW;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}
		break;
#if defined(CONTRY_KEYBOARD) && (CONTRY_KEYBOARD == FR)
	case 'q':
#else
	case 'a':  /*WRIST OPEN */
#endif

		request_pos[GRIPPER] = actual_pos[GRIPPER] - DISTANCE;
		if (request_pos[GRIPPER] > LIMIT_LOW_GRIPPER) {
			actual_pos[GRIPPER] = request_pos[GRIPPER];
			position = actual_pos[GRIPPER];
			id_servo = GRIPPER + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}
		break;
	case 'e':		/*WRIST CLOSE */
		request_pos[GRIPPER] = actual_pos[GRIPPER] + DISTANCE;
		if (request_pos[GRIPPER] < LIMIT_HIGH_GRIPPER) {
			actual_pos[GRIPPER]  = request_pos[GRIPPER];
			position = actual_pos[GRIPPER];
			id_servo = GRIPPER + '0';
		} else {
			error_signal = ERROR_SIG_HIGH;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}
		break;
	case 'i':
		request_pos[WRIST] = actual_pos[WRIST] + DISTANCE;
		if (request_pos[WRIST] != LIMIT_HIGH_WRIST) {
			actual_pos[WRIST] = request_pos[WRIST];
			position = actual_pos[WRIST];
			id_servo = WRIST + '0';
		} else {
			error_signal = ERROR_SIG_HIGH;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}
		break;
	case 'k':
		request_pos[WRIST] = actual_pos[WRIST] - DISTANCE;
		if (request_pos[WRIST] != LIMIT_HIGH_WRIST) {
			actual_pos[WRIST] = request_pos[WRIST];
			position = actual_pos[WRIST];
			id_servo = WRIST + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}
		break;
	case 'j':
		request_pos[ELBOW] = (actual_pos[ELBOW] - DISTANCE);
		if (request_pos[ELBOW] > LIMIT_LOW_ELBOW) {
			actual_pos[ELBOW] = request_pos[ELBOW];
			position = actual_pos[ELBOW];
			id_servo = ELBOW + '0';
		} else {
			error_signal = ERROR_SIG_LOW;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}
		break;
	case 'l':
		request_pos[ELBOW] = (actual_pos[ELBOW] + DISTANCE);
		if (request_pos[ELBOW] < LIMIT_HIGH_ELBOW) {
			actual_pos[ELBOW] = request_pos[ELBOW];
			position = actual_pos[ELBOW];
			id_servo = ELBOW + '0';
		} else {
			error_signal = ERROR_SIG_HIGH;
			error_flag = FLAG_INVALIDE_DATA;
			return error_flag;
		}
		break;
#if defined(CONTRY_KEYBOARD) && (CONTRY_KEYBOARD == FR)
	case '\n':
#else
	case '\r':  /* INIT */
#endif
		error_flag = CONTROLE_DATA;
		initSeq_ARM();
		rxIndex    = 0;
		txIndex    = 0;
		storeIndex = 0;
	break;
	case 'r':
			request_pos[ROTATION] = (actual_pos[ROTATION] - DISTANCE);
			if (request_pos[ROTATION] > LIMIT_LOW_ROTATION) {
				actual_pos[ROTATION] = request_pos[ROTATION];
				position = actual_pos[ROTATION];
				id_servo = ROTATION + '0';
			} else {
				error_signal = ERROR_SIG_LOW;
				error_flag = FLAG_INVALIDE_DATA;
				return error_flag;
			}
			break;
	case 't':
			request_pos[ROTATION] = (actual_pos[ROTATION] + DISTANCE);
			if (request_pos[ROTATION] < LIMIT_HIGH_ROTATION) {
				actual_pos[ROTATION] = request_pos[ROTATION];
				position = actual_pos[ROTATION];
				id_servo = ROTATION + '0';
			} else {
				error_signal = ERROR_SIG_HIGH;
				error_flag = FLAG_INVALIDE_DATA;
				return error_flag;
			}
			break;
	default:
		error_signal = ERROR_SIG_KEY;
		error_flag = FLAG_INVALIDE_DATA;
		return error_flag;
	}
	if (error_flag == FLAG_VALIDE_DATA) {
		sequence[id_sequence++] = '#';
		sequence[id_sequence++] = id_servo;
		sequence[id_sequence++] = 'P';
		uint8_t bufferSize;
		convert_pos_toString(position, buffer_pos, &bufferSize);
	    for(uint8_t i = 0; i < bufferSize; i++) {
	    	sequence[i + id_sequence] = buffer_pos[i];
	    }
	    id_sequence += bufferSize;
	    sequence[id_sequence] = '\r';
	    id_sequence ++;
	    *sizeSequence = id_sequence;
	}
    return error_flag;
}

/*!
 * @brief Concert decimal position (as 1800) into string buffer for send sequence to AL5D
 * uint16_t pos  				(Input) pos to convert
 * uint8_t *posBuffer			(Ouput) Buffer so store the converted position
 * uint8_t* bufferSize          (Ouput) lenght of the string usefull to send the exact sequence to AL5
 */

void convert_pos_toString(uint16_t pos, uint8_t *posBuffer, uint8_t* bufferSize) {
	uint16_t temp 	 = pos;
	char tempChar;
	uint8_t idBuffer = 0;
	uint8_t buffSize;
	uint16_t pivot = 1000;
	buffSize = ((pos >= 1000) ? 4 :
					(pos >= 100) ? 3 :
							(pos >= 10) ? 2 :
									(pos > 0) ? 1 : 0);
	*bufferSize = buffSize;
	while ((temp > 0) && (idBuffer < buffSize)) {
		if (temp >= pivot) {
			tempChar = (temp / pivot) + '0';
			posBuffer[idBuffer++] = tempChar;
			temp %= pivot;
			pivot /= 10;
			//idBuffer ++;
			if(temp == 0) {
				for(uint8_t i = idBuffer; i < buffSize; i++) {
					posBuffer[idBuffer++] = '0';
					//idBuffer++;
				}
			}
		} else { /* temp <= pivot */
			pivot /= 10;
			if (idBuffer != 0) {
				posBuffer[idBuffer++] = '0';
			}
		}
	}
}

void save_sequence_dataStore(uint8_t *data, uint8_t dataSize, uint8_t dataStore[DEMO_RING_BUFFER_SIZE][DEMO_RING_BUFFER_SIZE], uint16_t *indexToStore) {
	uint16_t tmpstoreIndex = *indexToStore;
	for(uint8_t i = 0; i < dataSize; i++) {
		dataStore[tmpstoreIndex][i] = data[i];
	}
	sizeSequence[tmpstoreIndex] = dataSize;
	*indexToStore = *indexToStore+ 1;
	//storeIndex++;
}
void send_info_seq() {
    usb_echo(" UART based C server\r\n");
    usb_echo(" please control the robot arm using your keyboard\r\n");
    usb_echo(" (a;e) =>  Gripper rotation\r\n");
    usb_echo(" (q;d) =>  Base rotation\r\n");
    usb_echo(" (z;s) =>  Shoulder rotation\r\n");
    usb_echo(" (j;l) =>  Wrist rotation\r\n");
    usb_echo(" (i;k) =>  Wrist rotation\r\n");
}
/*!
 * @brief init UART , USB communication
 */

void init_COM() {
	lpuart_config_t config;
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;
    //LPUART_Init(LPUART_PC_BOARD, &config, DEMO_LPUART_CLK_FREQ);
    config.baudRate_Bps = BOARD_ARM_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;
    LPUART_Init(LPUART_BOARD_ARM, &config , DEMO_LPUART_CLK_FREQ);
    USB_HostApplicationInit();
}
/*!
 * @brief producer function : read caractere from USB interrupt
 * g_HostHidKeyboard (Inuput) API usefull for interact with USB SKD, pointer to usb_host_keyboard_instance_t (to avoid memory copy
 * car 				 (Output) caractere read from ADD_ON field of usb_host_keyboard_instance_t
 * rxIndex			 (Output) Index of car receive since beginning of the launch
 *
 */
void producer_fn(usb_host_keyboard_instance_t *g_HostHidKeyboard, uint8_t *car, uint16_t *rxIndex, uint8_t *new_data) {
	uint8_t add_on_tmpRxIndex = g_HostHidKeyboard->add_on_rxIndex;

	if (add_on_tmpRxIndex != 0) {
		*car = *g_HostHidKeyboard->add_on_rxBuffer;
		*new_data = 1;
		demoRingBuffer[(*rxIndex % DEMO_RING_BUFFER_SIZE)] = *car;
		*rxIndex += g_HostHidKeyboard->add_on_rxIndex;
	}
}

/*!
 * @brief producer function : read caractere from USB interrupt
 * g_HostHidKeyboard (Inuput) API usefull for interact with USB SKD, pointer to usb_host_keyboard_instance_t (to avoid memory copy
 * rxIndex			 (Output) Index of car receive since beginning of the launch
 * storeIndex	     (Output) Index of car receive since beginning of the launch
 * txIndex			 (Output) Index of car transmit (LPUART) since beginning of launch
 *
 */
void error_management(usb_host_keyboard_instance_t *g_HostHidKeyboard,
					  uint16_t *rxIndex, uint16_t* storeIndex,
					  uint16_t *txIndex) {
	   if (error_signal != NO_ERROR) {
	   *storeIndex = 0;
  	   *txIndex    = 0;
  	   *rxIndex    = 0;
  	   g_HostHidKeyboard->add_on_rxIndex    = 0;
	   }
	   uint8_t *error_high_string = "limit high reached\r\n";
	   uint8_t *error_low_string  = "limit low reached\r\n";
	   uint8_t * indication_string = "(a;e), (q;d), (z;s), (i;k), \r\n";
	   uint8_t  *error_inv_string  = "invalide key please try the following combinaison: \r\n";
	   switch (error_signal) {
	   case ERROR_SIG_HIGH:
		   add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, error_high_string);
		   break;
	   case ERROR_SIG_LOW:
		   add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, error_low_string);
		   break;
	   case ERROR_SIG_KEY:
		   add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, error_inv_string);
		   add_on_LPUART_Write_StringNonBlocking(LPUART_PC_BOARD, indication_string);
		   break;
	   default: /* Last time error was detected and user did not type a correct info yet so e still wait */
		   break;
	   }

}
/*!
 * @brief consumer function : translate the command (in case of accurate one) into sequence for AL5D
 *  uint8_t cmd					 (Input)  Commande to translate into controle sequence
 *  uint8_t *storeIndex			 (Output) Index of car receive since beginning of the launch
 *  uint8_t *txIndex			 (Output) Index of car transmit (LPUART) since beginning of launch
 */
uint8_t consumer_fn(uint8_t cmd, uint16_t *rxIndex, uint16_t* storeIndex, uint16_t *txIndex) {
	uint8_t consumer_flag;
	flag_receive_data_t flag_receive_data;
	uint8_t sizeARMSequence;
	error_signal = NO_ERROR;
	uint16_t tmprxIndex    = *rxIndex;
	uint16_t tmpstoreIndex = *storeIndex;
	uint16_t tmptxIndex    = *txIndex;

    if (tmprxIndex > tmpstoreIndex) { /* Donne reçus et pas encore stockées*/
    	flag_receive_data = mapper_for_ARM(cmd, sequenceBuffer, &sizeARMSequence);                   /* Pour une donne on construit la sequence */
    	if (flag_receive_data == FLAG_VALIDE_DATA) {
    		save_sequence_dataStore(sequenceBuffer, sizeARMSequence, dataStore, storeIndex);		  /* Pour une donne reçus on stocke la sequence */
    	}
    }
    if (flag_receive_data == FLAG_VALIDE_DATA) {
        if (tmptxIndex < *storeIndex) { 													/* Donne stockés et pas encore envoyés*/
        	/* Transmit to robotic arm */
        	while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_BOARD_ARM)));
        	LPUART_WriteBlocking(LPUART_BOARD_ARM,dataStore[tmptxIndex] , sizeSequence[tmptxIndex]);/*sizeof(dataStore[tmptxIndex]) / sizeof(dataStore[tmptxIndex][0])*/
        	/* Transmit to debug session  */
        	uint8_t new_line = '\n';
        	while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_PC_BOARD)));
        	LPUART_WriteBlocking(LPUART_PC_BOARD,&new_line , 1);
        	while (!(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(LPUART_PC_BOARD)));
        	LPUART_WriteBlocking(LPUART_PC_BOARD,dataStore[tmptxIndex] , sizeSequence[tmptxIndex]);
            *txIndex = *txIndex + 1;
        }
    } else {  /* Incorrect value of data */
    		error_management(&g_HostHidKeyboard, rxIndex, storeIndex, txIndex);
       }
	return flag_receive_data;
}
/*!
 * @brief main function
 * g_HostHidKeyboard (Inuput) API usefull for interact with USB SKD, pointer to usb_host_keyboard_instance_t (to avoid memory copy
 * rxIndex			 (Output) Index of car receive since beginning of the launch
 *
 */
uint8_t control_arm_AL5D(usb_host_keyboard_instance_t* g_HostHidKeyboard, uint16_t *rxIndex , uint16_t* storeIndex, uint16_t *txIndex) {
	uint8_t cmd;
	uint8_t main_flag;
	uint8_t new_data = 0;

	producer_fn(g_HostHidKeyboard, &cmd, rxIndex, &new_data);
	if ((new_data != 0) || (*rxIndex > *storeIndex)) {					/* if new data arrived or more data has been received than store*/
		cmd = demoRingBuffer[(*storeIndex % DEMO_RING_BUFFER_SIZE)];
		main_flag = consumer_fn(cmd, rxIndex, storeIndex, txIndex);
	}
	return main_flag;
}


int main(void)
{
	lpuart_config_t config;

	BOARD_ConfigMPU();

    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();


    initSeq_ARM();

    USB_HostApplicationInit();
    BOARD_InitPins_UART4();
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;
    LPUART_Init(LPUART_PC_BOARD, &config, DEMO_LPUART_CLK_FREQ);
    config.baudRate_Bps = BOARD_ARM_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;
    LPUART_Init(LPUART_BOARD_ARM, &config , DEMO_LPUART_CLK_FREQ);
    /* ADD_ON */
    initSeq_ARM();
    send_info_seq();

    while (1)
    {
        USB_HostTaskFn(g_HostHandle);
        USB_HostHidKeyboardTask(&g_HostHidKeyboard);
        USB_HostHidMouseTask(&g_HostHidMouse);
        control_arm_AL5D(&g_HostHidKeyboard, &rxIndex , &storeIndex, &txIndex);
    }
}
