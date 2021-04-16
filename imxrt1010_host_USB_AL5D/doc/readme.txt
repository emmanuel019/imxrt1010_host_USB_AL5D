This project is a C server that controls the AL5D robot arm, via UART communication between the board and the arm. 
As well as via USB communication between the board (host) and a keyboard (peripheral).

The USB communication is based on NXP's SKD, evkmimxrt1010_host_hid_mouse_keyboard_bm which enables USB communication between a keyboard and the board. 



Getting started: 

	  (1)   Clone the project 
	  (2)   Importe the project using File -> Open Project from file system
	  (3)	Compile the project 
	  
	  (4)   HW Connection:
	  		 - Connect a keyboard to the board on its OTG port (left), via a USB (female) to micro USB (wrong) adapter
			 - connect the Tx pin (UART4 =>> J56 [16] to the Rx pin of the AL5D arm
			 - Connect the board to a PC (for power supply and to flash the code)
			 
	  (6)	Flash the project (Debug session or Run session)
	  (7) 	Open a serial console terminal (115200 bps)
	  (8)   Use the keyboard to control the robot arm (using the help displayed on the console)
	
Brief presentation of the project :

The main application can be found in the source/app.c file :
the application decomposes into several major parts 

1.  the processing of commands received in USB, via a mapper, 
    the calculation of control sequences.

2.  The transmission in UART to the robot arm 
    (in blocking mode in order to have the most instantaneous control possible), 
     as well as the transmission in UART to the PC (for debugging, therefore this action can be asynchronous)
3.   Reception of data from the keyboard. On the USB side, the reception is done via Interrupt, an API allows us to access the USB drive, 
     USB_HostHidKeyboardTask (called in the main function), 
     and thus access the received character via an ADD_ON that I added in the source/host_keyboard.c and host_keyboard.h files.

