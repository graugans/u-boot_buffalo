/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Boot support
 */
//#define	BUFFALO_TEST_NO_WRITE

#include <common.h>
#include <command.h>
#include <net.h>

#if (CONFIG_COMMANDS & CFG_CMD_NET)


extern int do_bootm (cmd_tbl_t *, int, int, char *[]);

static int netboot_common (proto_t, cmd_tbl_t *, int , char *[]);

int do_bootp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common (BOOTP, cmdtp, argc, argv);
}

U_BOOT_CMD(
	bootp,	3,	1,	do_bootp,
	"bootp\t- boot image via network using BootP/TFTP protocol\n",
	"[loadAddress] [bootfilename]\n"
);

int do_tftpb (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common (TFTP, cmdtp, argc, argv);
}

U_BOOT_CMD(
	tftpboot,	3,	1,	do_tftpb,
	"tftpboot- boot image via network using TFTP protocol\n",
	"[loadAddress] [bootfilename]\n"
);

#ifdef	CONFIG_BUFFALO
int do_tftps (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common (TFTPS, cmdtp, argc, argv);
}

U_BOOT_CMD(
	TFTPS,	2,	1,	do_tftps,
	"TFTPS   - boot image via network as TFTP server\n",
	"[buf]\n"
);
#endif	//CONFIG_BUFFALO

int do_rarpb (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common (RARP, cmdtp, argc, argv);
}

U_BOOT_CMD(
	rarpboot,	3,	1,	do_rarpb,
	"rarpboot- boot image via network using RARP/TFTP protocol\n",
	"[loadAddress] [bootfilename]\n"
);

#if (CONFIG_COMMANDS & CFG_CMD_DHCP)
int do_dhcp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common(DHCP, cmdtp, argc, argv);
}

U_BOOT_CMD(
	dhcp,	3,	1,	do_dhcp,
	"dhcp\t- invoke DHCP client to obtain IP/boot params\n",
	"\n"
);
#endif	/* CFG_CMD_DHCP */

#if (CONFIG_COMMANDS & CFG_CMD_NFS)
int do_nfs (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return netboot_common(NFS, cmdtp, argc, argv);
}

U_BOOT_CMD(
	nfs,	3,	1,	do_nfs,
	"nfs\t- boot image via network using NFS protocol\n",
	"[loadAddress] [host ip addr:bootfilename]\n"
);
#endif	/* CFG_CMD_NFS */

static void netboot_update_env (void)
{
	char tmp[22];

	if (NetOurGatewayIP) {
		ip_to_string (NetOurGatewayIP, tmp);
		setenv ("gatewayip", tmp);
	}

	if (NetOurSubnetMask) {
		ip_to_string (NetOurSubnetMask, tmp);
		setenv ("netmask", tmp);
	}

	if (NetOurHostName[0])
		setenv ("hostname", NetOurHostName);

	if (NetOurRootPath[0])
		setenv ("rootpath", NetOurRootPath);

	if (NetOurIP) {
		ip_to_string (NetOurIP, tmp);
		setenv ("ipaddr", tmp);
	}

	if (NetServerIP) {
		ip_to_string (NetServerIP, tmp);
		setenv ("serverip", tmp);
	}

	if (NetOurDNSIP) {
		ip_to_string (NetOurDNSIP, tmp);
		setenv ("dnsip", tmp);
	}
#if (CONFIG_BOOTP_MASK & CONFIG_BOOTP_DNS2)
	if (NetOurDNS2IP) {
		ip_to_string (NetOurDNS2IP, tmp);
		setenv ("dnsip2", tmp);
	}
#endif
	if (NetOurNISDomain[0])
		setenv ("domain", NetOurNISDomain);

#if (CONFIG_COMMANDS & CFG_CMD_SNTP) && (CONFIG_BOOTP_MASK & CONFIG_BOOTP_TIMEOFFSET)
	if (NetTimeOffset) {
		sprintf (tmp, "%d", NetTimeOffset);
		setenv ("timeoffset", tmp);
	}
#endif
#if (CONFIG_COMMANDS & CFG_CMD_SNTP) && (CONFIG_BOOTP_MASK & CONFIG_BOOTP_NTPSERVER)
	if (NetNtpServerIP) {
		ip_to_string (NetNtpServerIP, tmp);
		setenv ("ntpserverip", tmp);
	}
#endif
}

#ifdef	CONFIG_BUFFALO

#define HEADER_STR "# Airstation FirmWare"
int header_str_check(char *buf)
{
	int i_ret = 0;
	static char * header_str = HEADER_STR;
{
	char t[64];
	memcpy(t, buf, 32);
	t[32] = 0;
}

	if (strstr(buf, header_str)){
		i_ret = 1;
	}
	return i_ret;
}

#define	HEADER_STR_PUBLIC	"# Airstation Public Fmt1"
int header_public_str_check(char *buf)
{
	int i_ret = 0;
	char	chk[32];

	memset(chk, 0, sizeof(chk));	//check 32 bytes
	strncpy(chk, HEADER_STR_PUBLIC, sizeof(chk));

	if (memcmp(buf, chk, sizeof(chk))==0) {
		i_ret	= sizeof(chk);
	}
	return i_ret;		//return header length
}

int header_check(char *buf)
{
	int i_ret = 0;
	if (i_ret){
		i_ret =1;
	}else{
		char	tmp[512];
		memcpy(tmp, buf, 512);
		i_ret = header_str_check(tmp);
		if (i_ret){
			memcpy(buf, tmp, 512);
			i_ret =1;
		}
	}
	return i_ret;
}

int header_public_check(char *buf)
{
	int	i_ret	= 0;
	char *s;

	if ((s = getenv("accept_open_rt_fmt")) != NULL) {
		if (simple_strtoul(s, NULL, 0)) {
			i_ret	= header_public_str_check(buf);
			if (i_ret) {
				printf("\nAirstation Public header\n");
			}
		}
	}

	return	i_ret;	//return header length
}
#endif	//CONFIG_BUFFALO

static int
netboot_common (proto_t proto, cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	char *s;
	int   rcode = 0;
	int   size;
#ifdef	CONFIG_BUFFALO
	int	f_erase_all	= 0;
#endif	//CONFIG_BUFFALO

	/* pre-set load_addr */
	if ((s = getenv("loadaddr")) != NULL) {
		load_addr = simple_strtoul(s, NULL, 16);
#ifdef	CONFIG_BUFFALO
	}else if((s = getenv((uchar*)("memtmp_addr"))) != NULL){
		load_addr = simple_strtoul(s, NULL, 16);
#endif	//CONFIG_BUFFALO
	}

#ifdef	CONFIG_BUFFALO
	//	erase ALL when "buffalo fw --> user fw"
	{
		ulong	addr;
		for (addr=MK_HEX(BUFFALO_FW_EADDR)+1 - (0x8000*4) ; addr < MK_HEX(BUFFALO_FW_EADDR) ; addr+=0x8000) {
			if (strncmp((char *)addr, "ProjectY",8)==0) {
				f_erase_all	= 1;
				break;
			}
		}
	}
#endif	//CONFIG_BUFFALO

	switch (argc) {
	case 1:
		break;

	case 2:
#ifdef	CONFIG_BUFFALO
	  if(strcmp(argv[0], "TFTPS") == 0){
		copy_filename (BootFile, argv[1], sizeof(BootFile));
	  }else{
#endif	//CONFIG_BUFFALO
		/* only one arg - accept two forms:
		 * just load address, or just boot file name.
		 * The latter form must be written "filename" here.
		 */
		if (argv[1][0] == '"') {	/* just boot filename */
			copy_filename (BootFile, argv[1], sizeof(BootFile));
		} else {			/* load address	*/
			load_addr = simple_strtoul(argv[1], NULL, 16);
		}
#ifdef	CONFIG_BUFFALO
	  }
#endif	//CONFIG_BUFFALO
		break;

	case 3:	load_addr = simple_strtoul(argv[1], NULL, 16);
		copy_filename (BootFile, argv[2], sizeof(BootFile));

		break;

	default: printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if ((size = NetLoop(proto)) < 0)
#ifdef	CONFIG_BUFFALO
	{
	  if (strcmp(argv[0], "TFTPS") != 0 || strcmp(argv[1], "buf") != 0) {
#endif	//CONFIG_BUFFALO
		return 1;
#ifdef	CONFIG_BUFFALO
	  }
	} else {
#endif	//CONFIG_BUFFALO


	/* NetLoop ok, update environment */
	netboot_update_env();
#ifdef	CONFIG_BUFFALO
	}
#endif	//CONFIG_BUFFALO

#ifdef	CONFIG_BUFFALO
#define HEADER_MAX 512
#define BUF_LEN_LEN 4
#define CMD_CNT_LEN 4
	{
		int		f_chkheader	= 0;
		if(strcmp(argv[0], "TFTPS") == 0 && strcmp(argv[1], "buf") == 0){
			f_chkheader	= 1;
		} else
		if (strcmp(argv[0],"tftpboot")==0 && (s=getenv("inspection"))!=NULL && strcmp(s,"1")==0) {
			//	inspection-mode
			image_header_t	*hdr	= (image_header_t *)load_addr;
			if (ntohl(hdr->ih_magic) == IH_MAGIC) {
				if (do_iminfo(NULL,0,0,NULL)==0) {
					char *local_args[2];
					local_args[0] = argv[0];
					local_args[1] = NULL;
					do_bootm(cmdtp, 0, 1, local_args);
				}
				//	BLINK DIAG
				status_led_blink_num_set(STATUS_LED_DIAG, 2);
				for (;;)	udelay(100*1000);
				//	not reach

			} else {
				f_chkheader	= 1;
			}
		}
		if (f_chkheader) {
			if(size > 0){
				char * pData = (char *)load_addr;
				char * pEnd = pData + HEADER_MAX;
				char * pNL;
				int ret = 0;

#ifdef	CONFIG_BUFFALO
				{
					char	tmp[256];
					printf("Buffalo F/W format recved.\n");
					sprintf(tmp, "erase %08lX %08lX; cp.b $fileaddr %08lX $filesize; bootm %08lX;",
								MK_HEX(BUFFALO_FW_SADDR), MK_HEX(BUFFALO_FW_EADDR), MK_HEX(BUFFALO_FW_SADDR), MK_HEX(BUFFALO_FW_SADDR));
					setenv("u_fw",tmp);
#ifndef	BUFFALO_TEST_NO_WRITE
					run_command("run u_fw", 0/*flag*/);
#else	//BUFFALO_TEST_NO_WRITE
					run_command("printenv u_fw", 0/*flag*/);
					run_command("printenv fileaddr", 0/*flag*/);
					run_command("printenv filesize", 0/*flag*/);
					printf("\n\n-- dumping --\n\n");
					setenv("uu_fw", "md.b $fileaddr");
					run_command("run uu_fw", 0/*flag*/);
					printf("skip writing...\n");
#endif	//BUFFALO_TEST_NO_WRITE
				}
#endif
				ret = header_check(pData);
				if (ret){
					{
						char	tmp[16];
						sprintf(tmp,"%lx",size-512);
						setenv("filesize",tmp);
						sprintf(tmp,"%lX",load_addr+512);
						setenv("fileaddr",tmp);
					}
					printf("Buffalo F/W format recved.\n");
					while(pData < pEnd){
						pNL = strchr(pData, '\n');
						if(pNL){
							*pNL = 0;//NULL terminated!
							if(pData[0] == 0){
								//BLANK LINE means EOF
								break;
							}else if(pData[0] != '#'){
								run_command(pData, 0/*flag*/);
							}
							pData = pNL + 1;
						}else{
							break;
						}
					}//end of while
#if	1		//	for "# AirStation Public Fmt1"
				} else if ( ret=header_public_check(pData) ) {	//return header length
					int		recv_fw_size= size - ret;
					int		max_fw_size	= (MK_HEX(BUFFALO_FW_EADDR)+1 - MK_HEX(BUFFALO_FW_SADDR));
					//	size_check
					if (recv_fw_size <= 0) {
						printf("No firmware image.\n");

						//	BLINK DIAG
						status_led_blink_num_set(STATUS_LED_DIAG, 2);
						for (;;)	udelay(100*1000);
						//	not reach
					} else if (max_fw_size < recv_fw_size) {
						printf("Firmware image size too large!\n");

						//	BLINK DIAG
						status_led_blink_num_set(STATUS_LED_DIAG, 2);
						for (;;)	udelay(100*1000);
						//	not reach
						size	= 0;
					} else {
						char	tmp[256];
						printf("Recv fw image [%d] bytes, now writing...\n\n", recv_fw_size);
						sprintf(tmp, "erase %08lX +%08lX; cp.b %08lX %08lX %lX; bootm %08lX;",
								MK_HEX(BUFFALO_FW_SADDR), f_erase_all ? (MK_HEX(BUFFALO_FW_EADDR)-MK_HEX(BUFFALO_FW_SADDR)+1): recv_fw_size,
								load_addr+ret, MK_HEX(BUFFALO_FW_SADDR), recv_fw_size,
								MK_HEX(BUFFALO_FW_SADDR));
						setenv("public_fw",tmp);
						sprintf(tmp,"%lx",size-ret);
						setenv("filesize",tmp);
						sprintf(tmp,"%lX",load_addr+ret);
						setenv("fileaddr",tmp);
#ifndef	BUFFALO_TEST_NO_WRITE
						run_command("run public_fw", 0/*flag*/);
#else	//BUFFALO_TEST_NO_WRITE
						run_command("printenv public_fw", 0/*flag*/);
						run_command("printenv fileaddr", 0/*flag*/);
						run_command("printenv filesize", 0/*flag*/);
						printf("\n\n-- dumping --\n\n");
						setenv("uu_fw", "md.b $fileaddr");
						run_command("run uu_fw", 0/*flag*/);
						printf("skip writing...\n");
#endif	//BUFFALO_TEST_NO_WRITE
						//not reach this line
					}
					return	0;
#endif		//	for "# AirStation Public Fmt1"

				}else{
				}
			}else{
				printf("no file was loaded.\n");
				if(strcmp(argv[0], "TFTPS") == 0 && strcmp(argv[1], "buf") == 0)
					return	2;
			}
		}
	}
#endif	//CONFIG_BUFFALO

	/* done if no file was loaded (no errors though) */
	if (size == 0)
		return 0;

	/* flush cache */
	flush_cache(load_addr, size);

	/* Loading ok, check if we should attempt an auto-start */
	if (((s = getenv("autostart")) != NULL) && (strcmp(s,"yes") == 0)) {
		char *local_args[2];
		local_args[0] = argv[0];
		local_args[1] = NULL;

		printf ("Automatic boot of image at addr 0x%08lX ...\n",
			load_addr);
		rcode = do_bootm (cmdtp, 0, 1, local_args);
	}

#ifdef CONFIG_AUTOSCRIPT
	if (((s = getenv("autoscript")) != NULL) && (strcmp(s,"yes") == 0)) {
		printf("Running autoscript at addr 0x%08lX ...\n", load_addr);
		rcode = autoscript (load_addr);
	}
#endif
	return rcode;
}

#if (CONFIG_COMMANDS & CFG_CMD_PING)
int do_ping (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc < 2)
		return -1;

	NetPingIP = string_to_ip(argv[1]);
	if (NetPingIP == 0) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}

	if (NetLoop(PING) < 0) {
		printf("ping failed; host %s is not alive\n", argv[1]);
		return 1;
	}

	printf("host %s is alive\n", argv[1]);

	return 0;
}

U_BOOT_CMD(
	ping,	2,	1,	do_ping,
	"ping\t- send ICMP ECHO_REQUEST to network host\n",
	"pingAddress\n"
);
#endif	/* CFG_CMD_PING */

#if (CONFIG_COMMANDS & CFG_CMD_CDP)

static void cdp_update_env(void)
{
	char tmp[16];

	if (CDPApplianceVLAN != htons(-1)) {
		printf("CDP offered appliance VLAN %d\n", ntohs(CDPApplianceVLAN));
		VLAN_to_string(CDPApplianceVLAN, tmp);
		setenv("vlan", tmp);
		NetOurVLAN = CDPApplianceVLAN;
	}

	if (CDPNativeVLAN != htons(-1)) {
		printf("CDP offered native VLAN %d\n", ntohs(CDPNativeVLAN));
		VLAN_to_string(CDPNativeVLAN, tmp);
		setenv("nvlan", tmp);
		NetOurNativeVLAN = CDPNativeVLAN;
	}

}

int do_cdp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int r;

	r = NetLoop(CDP);
	if (r < 0) {
		printf("cdp failed; perhaps not a CISCO switch?\n");
		return 1;
	}

	cdp_update_env();

	return 0;
}

U_BOOT_CMD(
	cdp,	1,	1,	do_cdp,
	"cdp\t- Perform CDP network configuration\n",
);
#endif	/* CFG_CMD_CDP */

#if (CONFIG_COMMANDS & CFG_CMD_SNTP)
int do_sntp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *toff;

	if (argc < 2) {
		NetNtpServerIP = getenv_IPaddr ("ntpserverip");
		if (NetNtpServerIP == 0) {
			printf ("ntpserverip not set\n");
			return (1);
		}
	} else {
		NetNtpServerIP = string_to_ip(argv[1]);
		if (NetNtpServerIP == 0) {
			printf ("Bad NTP server IP address\n");
			return (1);
		}
	}

	toff = getenv ("timeoffset");
	if (toff == NULL) NetTimeOffset = 0;
	else NetTimeOffset = simple_strtol (toff, NULL, 10);

	if (NetLoop(SNTP) < 0) {
		printf("SNTP failed: host %s not responding\n", argv[1]);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	sntp,	2,	1,	do_sntp,
	"sntp\t- synchronize RTC via network\n",
	"[NTP server IP]\n"
);
#endif	/* CFG_CMD_SNTP */

#endif	/* CFG_CMD_NET */
