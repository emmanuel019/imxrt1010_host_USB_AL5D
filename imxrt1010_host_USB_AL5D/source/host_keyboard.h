/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016, 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HOST_KEYBOARD_H_
#define _HOST_KEYBOARD_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief USB host keyboard instance structure */
typedef struct _usb_host_keyboard_instance
{
    usb_host_configuration_handle configHandle; /*!< the keybaord's configuration handle */
    usb_device_handle deviceHandle;             /*!< the keybaord's device handle */
    usb_host_class_handle classHandle;          /*!< the keybaord's class handle */
    usb_host_interface_handle interfaceHandle;  /*!< the keybaord's interface handle */
    uint8_t lastPressData[6];                   /*!< keyboard last keyboard data */
    uint16_t maxPacketSize;                     /*!< interrupt in max packet size */
    uint8_t deviceState;                        /*!< device attach/detach status */
    uint8_t prevState;                          /*!< device attach/detach previous status */
    uint8_t runState;                           /*!< keyboard application run status */
    uint8_t runWaitState; /*!< keyboard application wait status, go to next run status when the wait status success */
    uint8_t shiftPressed; /*!< set 1 when shift key is pressed */
    uint8_t *keyboardBuffer; /*!< use to receive report descriptor and data */
    uint8_t *add_on_rxBuffer;
    uint16_t add_on_rxIndex;
} usb_host_keyboard_instance_t;

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief host keyboard task function.
 *
 * This function implements the host keyboard action, it is used to create task.
 *
 * @param param   the host keyboard instance pointer.
 */
extern void USB_HostHidKeyboardTask(void *param);

/*!
 * @brief host keyboard callback function.
 *
 * This function should be called in the host callback function.
 *
 * @param deviceHandle         device handle.
 * @param configurationHandle  attached device's configuration descriptor information.
 * @param eventCode            callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The configuration don't contain hid keyboard interface.
 */
extern usb_status_t USB_HostHidKeyboardEvent(usb_device_handle deviceHandle,
                                             usb_host_configuration_handle configurationHandle,
                                             uint32_t eventCode);

#endif /* _HOST_KEYBOARD_H_ */
