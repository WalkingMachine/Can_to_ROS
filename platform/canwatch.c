/*
 * Original work copyright 2013 Fabio Baltieri <fabio.baltieri@gmail.com>
 * Modified work copyright 2015 Dan Kouba
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

//#define USE_CURSE
#define CANUSE 0

#if defined( USE_OLD_CODE )
	#include <unistd.h>
	#include <string.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	//#include <linux/can.h>
	#include <linux/can/raw.h>
	#include <net/if.h>
	#include <sys/ioctl.h>

	#include <endian.h>
#endif
#if defined( USE_CURSE )
	#include <curses.h>
#endif
#include <linux/can.h>
#include "CAN/native_can.h"
#include "can_Interface.h"

#define __packed __attribute__((packed))

#if defined( USE_OLD_CODE )
	// SocketCAN flags
	// Upper 3 bits of ID integer in CAN frame are used for these flags


	static int sk;
	static int frame_cnt = 0;

#else
	can_driver * m_can_driver;
#endif

#if defined( USE_CURSE )
	#define CAN_EFF_FLAG 0x80000000U
	#define CAN_RTR_FLAG 0x40000000U
	#define CAN_ERR_FLAG 0x20000000U
	static int valid_frame_cnt = 0;
	union dataframe {

		struct __packed {
	        uint8_t b7;
	        uint8_t b6;
	        uint8_t b5;
	        uint8_t b4;
	        uint8_t b3;
	        uint8_t b2;
	        uint8_t b1;
	        uint8_t b0;
	    } generic;

	};
#endif

uint32_t arb_id=0;
void process_one(struct can_frame *frm);

static void _event_cb(can_t *dev, can_event_t event, uint8_t *message, uint16_t lenght)
{

 printf("receive event from can%i\n", *dev);
 can_frame *frameptr=(can_frame *)message;
 switch(event)
 {
	case CAN_EVENT_RX_COMPLETE:
		if ( (frameptr->can_id & 0x1FFFFFFF) == arb_id)
			process_one((can_frame *)message);
		break;
 }
}


  void process_one(struct can_frame *frm)
	{
		#if defined( USE_CURSE )
			valid_frame_cnt++;

		union dataframe *dat;

		dat = (union dataframe *)frm->data;

	    bool eff_flag = frm->can_id & CAN_EFF_FLAG;
	    bool rtr_flag = frm->can_id & CAN_RTR_FLAG;
	    bool err_flag = frm->can_id & CAN_ERR_FLAG;

	    // ID

	    move(0, 0);
	    clrtoeol();
	    move(1, 0);
	    clrtoeol();
	    move(2, 0);
	    clrtoeol();
	    move(3, 0);
	    clrtoeol();
	    // Should update total framecount when any frame is receved, not just when the filter is matched.
	    mvprintw(0, 0, "Arbitration ID: %#08X, Frame Number: %d/%d", frm->can_id & 0x1FFFFFFF, valid_frame_cnt, m_can_driver->get_frame_count(CANUSE));
	    mvprintw(1, 0, "EFF Frame: %s", eff_flag ? "True" : "False");
	    mvprintw(2, 0, "RTR Frame: %s", rtr_flag ? "True" : "False");
	    mvprintw(3, 0, "ERR Frame: %s", err_flag ? "True" : "False");

	    // Hex data
	    move(5, 0);
	    clrtoeol();
	    mvprintw(5, 0, " %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X",
	            dat->generic.b7,
	            dat->generic.b6,
	            dat->generic.b5,
	            dat->generic.b4,
	            dat->generic.b3,
	            dat->generic.b2,
	            dat->generic.b1,
	            dat->generic.b0
	            );

	    // Decimal Data
	    move(6, 0);
	    clrtoeol();
			mvprintw(6, 0, "%03d %03d %03d %03d %03d %03d %03d %03d",
	            dat->generic.b7,
	            dat->generic.b6,
	            dat->generic.b5,
	            dat->generic.b4,
	            dat->generic.b3,
	            dat->generic.b2,
	            dat->generic.b1,
	            dat->generic.b0
	            );

	    // Char Data
	    move(7, 0);
	    clrtoeol();
			mvprintw(7, 0, "  %c   %c   %c   %c   %c   %c   %c   %c",
	            (dat->generic.b7 > 32 && dat->generic.b7 < 127) ? dat->generic.b7 : '.',
	            (dat->generic.b6 > 32 && dat->generic.b6 < 127) ? dat->generic.b6 : '.',
	            (dat->generic.b5 > 32 && dat->generic.b5 < 127) ? dat->generic.b5 : '.',
	            (dat->generic.b4 > 32 && dat->generic.b4 < 127) ? dat->generic.b4 : '.',
	            (dat->generic.b3 > 32 && dat->generic.b3 < 127) ? dat->generic.b3 : '.',
	            (dat->generic.b2 > 32 && dat->generic.b2 < 127) ? dat->generic.b2 : '.',
	            (dat->generic.b1 > 32 && dat->generic.b1 < 127) ? dat->generic.b1 : '.',
	            (dat->generic.b0 > 32 && dat->generic.b0 < 127) ? dat->generic.b0 : '.'
	            );


	    refresh();
		#else
			printf("lenght: %i\n",frm->can_dlc );
			for(uint8_t i=0; i< frm->can_dlc; i++)
			{
				printf("[ %02X ] ",frm->data[i]);
			}
			printf("\nrecus\n" );//printf("%s\n", );

		#endif
	}
#if defined( USE_OLD_CODE )
	static int net_init(char *ifname)
	{
		int recv_own_msgs;
		struct sockaddr_can addr;
		struct ifreq ifr;

		sk = socket(PF_CAN, SOCK_RAW, CAN_RAW);
		if (sk < 0) {
			perror("socket");
			exit(1);
		}

		memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		if (ioctl(sk, SIOCGIFINDEX, &ifr) < 0) {
			perror("SIOCGIFINDEX");
			exit(1);
		}

		memset(&addr, 0, sizeof(addr));
		addr.can_family = AF_CAN;
		addr.can_ifindex = ifr.ifr_ifindex;
		if (bind(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			perror("bind");
			return 1;
		}

		recv_own_msgs = 0; // 0 = disabled (default), 1 = enabled //
		setsockopt(sk, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,
				&recv_own_msgs, sizeof(recv_own_msgs));

		return 0;
	}

	static void receive_one(uint32_t arb_id)
	{
		struct can_frame frm;
		struct sockaddr_can addr;
		int ret;
		socklen_t len;

		ret = recvfrom(sk, &frm, sizeof(struct can_frame), 0,
				(struct sockaddr *)&addr, &len);
		if (ret < 0) {
			perror("can raw socket read");
			exit(1);
		}

	    frame_cnt++;

	    if ( (frm.can_id & 0x1FFFFFFF) == arb_id) {
		    process_one(&frm);
	    }
	}
#endif



int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Syntax: %s <Arbitration ID>\n", argv[0]);
		exit(1);
	}

  arb_id = (uint32_t)strtol(argv[1], NULL, 16) & 0x7FFFFFFF;

	#if defined( USE_CURSE )
  	initscr();
	#else
		//creation du driver can

		m_can_driver = new can_driver(&_event_cb);
		m_can_driver->init(false);
	#endif

	#if defined( USE_OLD_CODE )
		net_init(argv[1]);

		for (;;)
			receive_one(arb_id);
	#else
		for (;;)
			m_can_driver->run();
	#endif

	#if defined( USE_CURSE )
		endwin();
	#endif

	return 0;
}
