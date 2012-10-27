/*====================================================================*
 *   
 *   Copyright (c) 2011 by Qualcomm Atheros.
 *   
 *   Permission to use, copy, modify, and/or distribute this software 
 *   for any purpose with or without fee is hereby granted, provided 
 *   that the above copyright notice and this permission notice appear 
 *   in all copies.
 *   
 *   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
 *   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL  
 *   THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
 *   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 *   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 *   NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 *   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *   
 *--------------------------------------------------------------------*/

/*====================================================================*
 *
 *   edsu.c - Qualcomm Atheros Ethernet II Data Send Utility
 *
 *   send one or more files over Ethernet using IEEE 802.2 Ethernet 
 *   Frames;
 *
 *   this program can be used as a data source when testing AR6405
 *   UART applications; use this program to send files and program
 *   edru to read and display or save them at the other end;
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *
 *   Contributor(s):
 *      Charles Maier <cmaier@qualcomm.com>
 *
 *--------------------------------------------------------------------*/

#define _GETOPT_H

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/

#include <stdlib.h>
#include <limits.h>
#include <memory.h>
#include <signal.h>
#include <unistd.h>

/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

#include "../tools/getoptv.h"
#include "../tools/putoptv.h"
#include "../tools/memory.h"
#include "../tools/number.h"
#include "../tools/error.h"
#include "../tools/files.h"
#include "../tools/flags.h"
#include "../ether/ether.h"
#include "../ether/channel.h"

/*====================================================================*
 *   custom source files;
 *--------------------------------------------------------------------*/

#ifndef MAKEFILE
#include "../tools/getoptv.c"
#include "../tools/putoptv.c"
#include "../tools/version.c"
#include "../tools/efreopen.c"
#include "../tools/basespec.c"
#include "../tools/uintspec.c"
#include "../tools/todigit.c"
#include "../tools/hexdump.c"
#include "../tools/hexencode.c"
#include "../tools/error.c"
#endif

#ifndef MAKEFILE
#include "../ether/channel.c"
#include "../ether/openchannel.c"
#include "../ether/closechannel.c"
#include "../ether/readpacket.c"
#include "../ether/sendpacket.c"
#endif

/*====================================================================*
 *   program constants;
 *--------------------------------------------------------------------*/

#define EDSU_INTERFACE "PLC"
#define EDSU_ETHERTYPE ETH_P_802_2
#define EDSU_PAUSE 0

/*====================================================================*
 *
 *   signed function (struct channel * channel, unsigned pause, signed fd);
 *
 *   read a file and transmit it over network as a stream of Ethernet 
 *   frames; pause between frames to prevent over-loading the remote 
 *   host;                              
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *
 *--------------------------------------------------------------------*/

signed function (struct channel * channel, unsigned pause, signed fd) 

{
	struct ether_frame frame;
	signed length = sizeof (frame.frame_data);
	memcpy (frame.frame_dhost, channel->peer, sizeof (frame.frame_dhost));
	memcpy (frame.frame_shost, channel->host, sizeof (frame.frame_shost));
	while ((length = read (fd, frame.frame_data, sizeof (frame.frame_data))) > 0) 
	{
		frame.frame_type = htons (length);
		if (length < ETHERMIN) 
		{
			length = ETHERMIN;
		}
		length += ETHER_HDR_LEN;
		if (sendpacket (channel, &frame, length) < 0) 
		{
			error (1, errno, CHANNEL_CANTSEND);
		}
		sleep (pause);
	}
	return (0);
}


/*====================================================================*
 *
 *   int main (int argc, char const * argv []);
 *
 *
 *.  Qualcomm Atheros HomePlug AV Powerline Toolkit
 *:  Published 2009-2011 by Qualcomm Atheros. ALL RIGHTS RESERVED
 *;  For demonstration and evaluation only. Not for production use
 *
 *--------------------------------------------------------------------*/

int main (int argc, char const * argv []) 

{
	extern struct channel channel;
	static char const * optv [] = 
	{
		"e:d:i:p:qv",
		"file [file] [...]",
		"Qualcomm Atheros Ethernet II Data Send Utility",
		"e x\tethertype is (x) [" LITERAL (EDSU_ETHERTYPE) "]",
		"d x\tdestination address is (x) [00:B0:52:00:00:01]",

#if defined (WINPCAP) || defined (LIBPCAP)

		"i n\thost interface is (n) [" LITERAL (CHANNEL_ETHNUMBER) "]",

#else

		"i s\thost interface is (s) [" LITERAL (CHANNEL_ETHDEVICE) "]",

#endif

		"p n\tpause (n) seconds between frames [" LITERAL (EDSU_PAUSE) "]",
		"q\tquiet mode",
		"v\tverbose mode",
		(char const *)(0)
	};
	unsigned pause = EDSU_PAUSE;
	signed c;
	if (getenv (EDSU_INTERFACE)) 
	{

#if defined (WINPCAP) || defined (LIBPCAP)

		channel.ifindex = atoi (getenv (EDSU_INTERFACE));

#else

		channel.ifname = strdup (getenv (EDSU_INTERFACE));

#endif

	}
	channel.type = EDSU_ETHERTYPE;
	while ((c = getoptv (argc, argv, optv)) != -1) 
	{
		switch (c) 
		{
		case 'e':
			channel.type = (uint16_t)(basespec (optarg, 16, sizeof (channel.type)));
			break;
		case 'd':
			if (!hexencode (channel.peer, sizeof (channel.peer), optarg)) 
			{
				error (1, errno, "%s", optarg);
			}
			break;
		case 'i':

#if defined (WINPCAP) || defined (LIBPCAP)

			channel.ifindex = atoi (optarg);

#else

			channel.ifname = optarg;

#endif

			break;
		case 'p':
			pause = (unsigned)(uintspec (optarg, 0, UCHAR_MAX));
			break;
		case 'q':
			_setbits (channel.flags, CHANNEL_SILENCE);
			break;
		case 'v':
			_setbits (channel.flags, CHANNEL_VERBOSE);
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;
	openchannel (&channel);
	if (!argc) 
	{
		function (&channel, pause, STDIN_FILENO);
	}
	while ((argc) && (* argv)) 
	{
		if (efreopen (* argv, "rb", stdin)) 
		{
			function (&channel, pause, fileno (stdin));
		}
		argc--;
		argv++;
	}
	closechannel (&channel);
	exit (0);
}
