/*
 *	Copyright 1994, 1995, 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000, 2001 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include "tftp.h"
#include "bootp.h"

#ifdef	CONFIG_BUFFALO
#define	CONFIG_FIRMWARE_IMAGE_COMBINED
#define	CONFIG_FIRM_HEADER_DECRYPT
#define	SPECIFICATION_MULTI
#define	CONFIG_HW_VERSION_HEADER
#endif	//CONFIG_BUFFALO

#undef	ET_DEBUG

#ifdef	CONFIG_BUFFALO
#define TRACE() printf("%s(%d):PASS\n",__FUNCTION__,__LINE__);
#endif	//CONFIG_BUFFALO


#if (CONFIG_COMMANDS & CFG_CMD_NET)

#define WELL_KNOWN_PORT	69		/* Well known TFTP port #		*/
#define TIMEOUT		5		/* Seconds to timeout for a lost pkt	*/
#ifndef	CONFIG_NET_RETRY_COUNT
# define TIMEOUT_COUNT	10		/* # of timeouts before giving up  */
#else
# define TIMEOUT_COUNT  (CONFIG_NET_RETRY_COUNT * 2)
#endif
					/* (for checking the image size)	*/
#define HASHES_PER_LINE	65		/* Number of "loading" hashes per line	*/

/*
 *	TFTP operations.
 */
#define TFTP_RRQ	1
#define TFTP_WRQ	2
#define TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5
#define TFTP_OACK	6


static int	TftpServerPort;		/* The UDP port at their end		*/
static int	TftpOurPort;		/* The UDP port at our end		*/
static int	TftpTimeoutCount;
static ulong	TftpBlock;		/* packet sequence number		*/
static ulong	TftpLastBlock;		/* last packet sequence number received */
static ulong	TftpBlockWrap;		/* count of sequence number wraparounds */
static ulong	TftpBlockWrapOffset;	/* memory offset due to wrapping	*/
static int	TftpState;

#define STATE_RRQ	1
#define STATE_DATA	2
#define STATE_TOO_LARGE	3
#define STATE_BAD_MAGIC	4
#define STATE_OACK	5
#ifdef	CONFIG_BUFFALO
#define	STATE_BUFFALO_DISK_FULL			11
#define	STATE_BUFFALO_INVALID_MODEL		12
#define	STATE_BUFFALO_INVALID_HWREV		13
#define	STATE_BUFFALO_INVALID_REGION	14
#endif	//CONFIG_BUFFALO

#define	debug(FMT, ARGs...)	do {} while(0)

#define TFTP_BLOCK_SIZE		512		    /* default TFTP block size	*/
#define TFTP_SEQUENCE_SIZE	((ulong)(1<<16))    /* sequence number is 16 bit */

#define DEFAULT_NAME_LEN	(8 + 4 + 1)
static char default_filename[DEFAULT_NAME_LEN];
static char *tftp_filename;

#ifdef	CONFIG_BUFFALO
static void	TftpTimeout_BUFFALO (void);
extern	ulong	NetTimeoutBuffalo;
static	ulong	tmp_bottom	= 0;
#endif	//CONFIG_BUFFALO

#ifdef CFG_DIRECT_FLASH_TFTP
extern flash_info_t flash_info[];
#endif

static __inline__ void
store_block (unsigned block, uchar * src, unsigned len)
{
	ulong offset = block * TFTP_BLOCK_SIZE + TftpBlockWrapOffset;
	ulong newsize = offset + len;
#ifdef	CONFIG_BUFFALO
#undef	CFG_DIRECT_FLASH_TFTP
	if (0
	|| (block>=65534)	//max tftp block-no is 65535
	|| (tmp_bottom > 0 && (load_addr + newsize) > tmp_bottom)
	) {
		TftpState	= STATE_TOO_LARGE;
		NetState = NETLOOP_FAIL;
		NetBootFileXferSize	= 0;
		return;
	}
#endif	//CONFIG_BUFFALO
#ifdef CFG_DIRECT_FLASH_TFTP
	int i, rc = 0;

	for (i=0; i<CFG_MAX_FLASH_BANKS; i++) {
		/* start address in flash? */
		if (load_addr + offset >= flash_info[i].start[0]) {
			rc = 1;
			break;
		}
	}

	if (rc) { /* Flash is destination for this packet */
		rc = flash_write ((char *)src, (ulong)(load_addr+offset), len);
		if (rc) {
			flash_perror (rc);
			NetState = NETLOOP_FAIL;
			return;
		}
	}
	else
#endif /* CFG_DIRECT_FLASH_TFTP */
	{
		(void)memcpy((void *)(load_addr + offset), src, len);
	}

	if (NetBootFileXferSize < newsize)
		NetBootFileXferSize = newsize;
}

static void TftpSend (void);
static void TftpTimeout (void);

/**********************************************************************/

static void
TftpSend (void)
{
	volatile uchar *	pkt;
	volatile uchar *	xp;
	int			len = 0;
	volatile ushort *s;

	/*
	 *	We will always be sending some sort of packet, so
	 *	cobble together the packet headers now.
	 */
	pkt = NetTxPacket + NetEthHdrSize() + IP_HDR_SIZE;

	switch (TftpState) {

	case STATE_RRQ:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_RRQ);
		pkt = (uchar *)s;
		strcpy ((char *)pkt, tftp_filename);
		pkt += strlen(tftp_filename) + 1;
		strcpy ((char *)pkt, "octet");
		pkt += 5 /*strlen("octet")*/ + 1;
		strcpy ((char *)pkt, "timeout");
		pkt += 7 /*strlen("timeout")*/ + 1;
		sprintf((char *)pkt, "%d", TIMEOUT);
#ifdef ET_DEBUG
		printf("send option \"timeout %s\"\n", (char *)pkt);
#endif
		pkt += strlen((char *)pkt) + 1;
		len = pkt - xp;
		break;

	case STATE_DATA:
	case STATE_OACK:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ACK);
		*s++ = htons(TftpBlock);
		pkt = (uchar *)s;
		len = pkt - xp;
		break;

	case STATE_TOO_LARGE:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(3);
		pkt = (uchar *)s;
		strcpy ((char *)pkt, "File too large");
		pkt += 14 /*strlen("File too large")*/ + 1;
		len = pkt - xp;
		break;

	case STATE_BAD_MAGIC:
		xp = pkt;
		s = (ushort *)pkt;
		*s++ = htons(TFTP_ERROR);
		*s++ = htons(2);
		pkt = (uchar *)s;
		strcpy ((char *)pkt, "File has bad magic");
		pkt += 18 /*strlen("File has bad magic")*/ + 1;
		len = pkt - xp;
		break;

#ifdef	CONFIG_BUFFALO
	case	STATE_BUFFALO_DISK_FULL:
	case	STATE_BUFFALO_INVALID_MODEL:
	case	STATE_BUFFALO_INVALID_HWREV:
	case	STATE_BUFFALO_INVALID_REGION:
		{
			uchar	*msgtbl[STATE_BUFFALO_INVALID_REGION-STATE_BUFFALO_DISK_FULL+1]	= {
						"Disk Full",		//STATE_BUFFALO_DISK_FULL
						"Unsupport MODEL",	//STATE_BUFFALO_INVALID_MODEL
						"Unsupport HW",		//STATE_BUFFALO_INVALID_HWREV
						"Unsupport REGION"	//STATE_BUFFALO_INVALID_REGION
			};
			ushort	errtbl[STATE_BUFFALO_INVALID_REGION-STATE_BUFFALO_DISK_FULL+1]	= {
						3,		//STATE_BUFFALO_DISK_FULL
						0,		//STATE_BUFFALO_INVALID_MODEL
						0,		//STATE_BUFFALO_INVALID_HWREV
						0		//STATE_BUFFALO_INVALID_REGION
			};
			xp = pkt;
			s = (ushort *)pkt;
			*s++ = htons(TFTP_ERROR);
			*s++ = htons(errtbl[TftpState-STATE_BUFFALO_DISK_FULL]);
			pkt = (uchar *)s;
			strcpy ((char *)pkt, msgtbl[TftpState-STATE_BUFFALO_DISK_FULL]);
			pkt += strlen(msgtbl[TftpState-STATE_BUFFALO_DISK_FULL]) + 1;
			len = pkt - xp;
		}
		break;
#endif	//CONFIG_BUFFALO
	}

	NetSendUDPPacket(NetServerEther, NetServerIP, TftpServerPort, TftpOurPort, len);

#ifdef	CONFIG_BUFFALO
	if (TftpState >= STATE_BUFFALO_DISK_FULL) {
		//	BLINK DIAG
		status_led_blink_num_set(STATUS_LED_DIAG, 2);
		for (;;)	udelay(100*1000);
		//	not reach
	}
#endif	//CONFIG_BUFFALO
}


static void
TftpHandler (uchar * pkt, unsigned dest, unsigned src, unsigned len)
{
	ushort proto;
	ushort *s;

#ifdef	CONFIG_BUFFALO
//	printf("@@ TftpHandler(%08lX, %08lX, %08lX, %08lX), STATE=%d\n", pkt, dest, src, len, TftpState);
	//	2008.08.08	Modified for the issue of delay to send RRQ by 'tftpboot command'.
	if (TftpState == STATE_RRQ) {
		if (pkt==0 && dest==0 && src==0 && len==0) {	//ARP get event
			TftpSend ();	//	start TFTP sequence
			return;
		}
	}
#endif	//CONFIG_BUFFALO
	if (dest != TftpOurPort) {
		return;
	}
	if (TftpState != STATE_RRQ && src != TftpServerPort) {
		return;
	}

	if (len < 2) {
		return;
	}
	len -= 2;
	/* warning: don't use increment (++) in ntohs() macros!! */
	s = (ushort *)pkt;
	proto = *s++;
	pkt = (uchar *)s;
	switch (ntohs(proto)) {

	case TFTP_RRQ:
	case TFTP_WRQ:
	case TFTP_ACK:
		break;
	default:
		break;

	case TFTP_OACK:
#ifdef ET_DEBUG
		printf("Got OACK: %s %s\n", pkt, pkt+strlen(pkt)+1);
#endif
		TftpState = STATE_OACK;
		TftpServerPort = src;
		TftpSend (); /* Send ACK */
		break;
	case TFTP_DATA:
		if (len < 2)
			return;
		len -= 2;
		TftpBlock = ntohs(*(ushort *)pkt);

		/*
		 * RFC1350 specifies that the first data packet will
		 * have sequence number 1. If we receive a sequence
		 * number of 0 this means that there was a wrap
		 * around of the (16 bit) counter.
		 */
		if (TftpBlock == 0) {
			TftpBlockWrap++;
			TftpBlockWrapOffset += TFTP_BLOCK_SIZE * TFTP_SEQUENCE_SIZE;
			printf ("\n\t %lu MB reveived\n\t ", TftpBlockWrapOffset>>20);
		} else {
			if (((TftpBlock - 1) % 10) == 0) {
				putc ('#');
			} else if ((TftpBlock % (10 * HASHES_PER_LINE)) == 0) {
				puts ("\n\t ");
			}
		}

#ifdef ET_DEBUG
		if (TftpState == STATE_RRQ) {
			puts ("Server did not acknowledge timeout option!\n");
		}
#endif

		if (TftpState == STATE_RRQ || TftpState == STATE_OACK) {
			/* first block received */
			TftpState = STATE_DATA;
			TftpServerPort = src;
			TftpLastBlock = 0;
			TftpBlockWrap = 0;
			TftpBlockWrapOffset = 0;

			if (TftpBlock != 1) {	/* Assertion */
				printf ("\nTFTP error: "
					"First block is not block 1 (%ld)\n"
					"Starting again\n\n",
					TftpBlock);
				NetStartAgain ();
				break;
			}
		}

		if (TftpBlock == TftpLastBlock) {
			/*
			 *	Same block again; ignore it.
			 */
			break;
		}

		TftpLastBlock = TftpBlock;
#ifdef	CONFIG_BUFFALO
		if (NetTimeoutBuffalo)
			NetSetTimeout (1 * CFG_HZ, TftpTimeout_BUFFALO);
		else
#endif
		NetSetTimeout (TIMEOUT * CFG_HZ, TftpTimeout);

		store_block (TftpBlock - 1, pkt + 2, len);

		/*
		 *	Acknoledge the block just received, which will prompt
		 *	the server for the next one.
		 */
		TftpSend ();

		if (len < TFTP_BLOCK_SIZE) {
			/*
			 *	We received the whole thing.  Try to
			 *	run it.
			 */
			puts ("\ndone\n");
			NetState = NETLOOP_SUCCESS;
		}
		break;

	case TFTP_ERROR:
		printf ("\nTFTP error: '%s' (%d)\n",
					pkt + 2, ntohs(*(ushort *)pkt));
		puts ("Starting again\n\n");
		NetStartAgain ();
		break;
	}
}

#ifdef	CONFIG_BUFFALO
static void
TftpTimeout_BUFFALO (void)
{
	if (++TftpTimeoutCount > 4) {
		puts ("\nRetry count exceeded; starting again\n");
		NetStartAgain ();
	} else {
		puts ("T ");
		NetSetTimeout (1 * CFG_HZ, TftpTimeout_BUFFALO);
		TftpSend ();
	}
}
#endif	//CONFIG_BUFFALO

static void
TftpTimeout (void)
{
	if (++TftpTimeoutCount > TIMEOUT_COUNT) {
		puts ("\nRetry count exceeded; starting again\n");
		NetStartAgain ();
	} else {
		puts ("T ");
		NetSetTimeout (TIMEOUT * CFG_HZ, TftpTimeout);
		TftpSend ();
	}
}


void
TftpStart (void)
{
#ifdef CONFIG_TFTP_PORT
	char *ep;             /* Environment pointer */
#endif
#ifdef	CONFIG_BUFFALO
	tmp_bottom	= simple_strtoul(getenv("tmp_bottom") ? : "0", NULL, 16);
#endif	//CONFIG_BUFFALO

	if (BootFile[0] == '\0') {
		sprintf(default_filename, "%02lX%02lX%02lX%02lX.img",
			NetOurIP & 0xFF,
			(NetOurIP >>  8) & 0xFF,
			(NetOurIP >> 16) & 0xFF,
			(NetOurIP >> 24) & 0xFF	);
		tftp_filename = default_filename;

		printf ("*** Warning: no boot file name; using '%s'\n",
			tftp_filename);
	} else {
		tftp_filename = BootFile;
	}

#if defined(CONFIG_NET_MULTI)
	printf ("Using %s device\n", eth_get_name());
#endif
	puts ("TFTP from server ");	print_IPaddr (NetServerIP);
	puts ("; our IP address is ");	print_IPaddr (NetOurIP);

	/* Check if we need to send across this subnet */
	if (NetOurGatewayIP && NetOurSubnetMask) {
	    IPaddr_t OurNet 	= NetOurIP    & NetOurSubnetMask;
	    IPaddr_t ServerNet 	= NetServerIP & NetOurSubnetMask;

	    if (OurNet != ServerNet) {
		puts ("; sending through gateway ");
		print_IPaddr (NetOurGatewayIP) ;
	    }
	}
	putc ('\n');

	printf ("Filename '%s'.", tftp_filename);

	if (NetBootFileSize) {
		printf (" Size is 0x%x Bytes = ", NetBootFileSize<<9);
		print_size (NetBootFileSize<<9, "");
	}

	putc ('\n');

	printf ("Load address: 0x%lx\n", load_addr);

	puts ("Loading: *\b");

#ifdef	CONFIG_BUFFALO
	if (NetTimeoutBuffalo)
		NetSetTimeout (1 * CFG_HZ, TftpTimeout_BUFFALO);
	else
#endif	//CONFIG_BUFFALO
	NetSetTimeout (TIMEOUT * CFG_HZ, TftpTimeout);
	NetSetHandler (TftpHandler);

	TftpServerPort = WELL_KNOWN_PORT;
	TftpTimeoutCount = 0;
	TftpState = STATE_RRQ;
	/* Use a pseudo-random port unless a specific port is set */
	TftpOurPort = 1024 + (get_timer(0) % 3072);
#ifdef CONFIG_TFTP_PORT
	if ((ep = getenv("tftpdstp")) != NULL) {
		TftpServerPort = simple_strtol(ep, NULL, 10);
	}
	if ((ep = getenv("tftpsrcp")) != NULL) {
		TftpOurPort= simple_strtol(ep, NULL, 10);
	}
#endif
	TftpBlock = 0;

	/* zero out server ether in case the server ip has changed */
	memset(NetServerEther, 0, 6);

	TftpSend ();
}

#ifdef	CONFIG_BUFFALO
static int	TftpServer_StartTimeout = 5;

static void TftpServer_StartTimeoutHdl(void)
{
	puts ("\nTftpServer Timeout;\n");
	NetStartAgain ();
	NetState = NETLOOP_FAIL;
	eth_halt();
}


static void TftpServer_TimeoutHdl(void)
{
	if (++TftpTimeoutCount > TIMEOUT_COUNT) {
		puts ("\nRetry count exceeded;\n");
		NetState = NETLOOP_FAIL;
		NetStartAgain ();
	} else {
		puts ("O ");
		NetSetTimeout (TIMEOUT * CFG_HZ, TftpServer_TimeoutHdl);
		TftpSend ();
	}
}

#define STATE_WRQ	11

static int	TftpClientPort;/* The UDP port at client end*/

static void TftpServer_Handler (
uchar * pkt, unsigned dest, unsigned src, unsigned len)
{
	ushort proto;
	volatile IP_t *ip = (volatile IP_t *)(pkt - IP_HDR_SIZE);


	if ((dest != WELL_KNOWN_PORT) && (dest != TftpOurPort)) {
		return;
	}
	if (TftpState != STATE_WRQ && src != TftpClientPort) {
		return;
	}

	if (len < 2) {
		return;
	}
	len -= 2;
	/* warning: don't use increment (++) in ntohs() macros!! */
	proto = *((ushort *)pkt)++;
	switch (ntohs(proto)) {
	case TFTP_WRQ:
		TftpClientPort = src;
		TftpOurPort = 1024 + (get_timer(0) % 3072);
		TftpServerPort = src;
		NetServerIP = NetReadIP((void *)&ip->ip_src);

		TftpState = STATE_OACK;
		TftpBlock = 0;
		TftpLastBlock = 0xffffffff;
		NetSetTimeout (TIMEOUT * CFG_HZ, TftpServer_TimeoutHdl);
		TftpSend (); /* Send ACK */
		break;
	case TFTP_OACK:
#ifdef ET_DEBUG
		printf("Got OACK: %s %s\n", pkt, pkt+strlen(pkt)+1);
#endif
		TftpState = STATE_OACK;
		TftpSend (); /* Send ACK */
		break;

	case TFTP_DATA:
		if (len < 2)
			return;
		len -= 2;
		TftpBlock = ntohs(*(ushort *)pkt);

		{
			if (((TftpBlock - 1) % 10) == 0) {
				putc ('$');
			} else if ((TftpBlock % (10 * HASHES_PER_LINE)) == 0) {
				puts ("\n\t ");
			}
		}

		if (TftpBlock == TftpLastBlock) {
			/*
			 *	Same block again; ignore it.
			 */
			break;
		}

		TftpLastBlock = TftpBlock;
		if (len == TFTP_BLOCK_SIZE) {
			NetSetTimeout (TIMEOUT * CFG_HZ,
				TftpServer_TimeoutHdl);
		}
#ifdef	CONFIG_BUFFALO
		if (TftpState < STATE_BUFFALO_DISK_FULL)
#endif	//CONFIG_BUFFALO
		store_block (TftpBlock - 1, pkt + 2, len);

		TftpSend ();/* Send ACK */

		if (len < TFTP_BLOCK_SIZE) {
			/*
			 *	We received the whole thing.  Try to
			 *	run it.
			 */
			puts ("\ntftp server done\n");
			NetState = NETLOOP_SUCCESS;
		}
		break;

	case TFTP_ERROR:
		printf ("\nTFTPserver error: '%s' (%d)\n",
					pkt + 2, ntohs(*(ushort *)pkt));
		puts ("Starting again\n\n");
		NetStartAgain ();
		break;

	default:
		break;
	}
}

void TftpServer(void)
{
	// This server receive one file to pointer 'load_addr',

	char *timeout = getenv("tftp_wait");
	int t = (int)simple_strtoul(timeout, NULL, 10);


	tmp_bottom	= simple_strtoul(getenv("tmp_bottom") ? : "0", NULL, 16);


	if(t){
		TftpServer_StartTimeout = t;
	}

	printf("\ntftp server(receive) go, waiting:%d[sec]\n",
		TftpServer_StartTimeout);
	memset((void *)load_addr, 0, TFTP_BLOCK_SIZE);
	NetStartAgain ();
	//ArpRequest ();
	printf ("Load address: 0x%lx\n", load_addr);
	//read WRQ with timeout
	TftpState = STATE_WRQ;
	NetSetTimeout (TftpServer_StartTimeout * CFG_HZ,
			TftpServer_StartTimeoutHdl);
	NetSetHandler (TftpServer_Handler);
}
#endif	//CONFIG_BUFFALO

#endif /* CFG_CMD_NET */
