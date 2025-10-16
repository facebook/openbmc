#include "libcxl.hpp"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <errno.h>

const char *DEVICE_ERRORS[23] = {
	"Success: The command completed successfully.",
	"Background Command Started: The background command started successfully. Refer to the Background Command Status register to retrieve the command result.",
	"Invalid Input: A command input was invalid.",
	"Unsupported: The command is not supported.",
	"Internal Error: The command was not completed due to an internal device error.",
	"Retry Required: The command was not completed due to a temporary error. An optional single retry may resolve the issue.",
	"Busy: The device is currently busy processing a background operation. Wait until background command completes and then retry the command.",
	"Media Disabled: The command could not be completed because it requires media access and media is disabled.",
	"FW Transfer in Progress: Only one FW package can be transferred at a time. Complete the current FW package transfer before starting a new one.",
	"FW Transfer Out of Order: The FW package transfer was aborted because the FW package content was transferred out of order.",
	"FW Authentication Failed: The FW package was not saved to the device because the FW package authentication failed.",
	"Invalid Slot: The FW slot specified is not supported or not valid for the requested operation.",
	"Activation Failed, FW Rolled Back: The new FW failed to activate and rolled back to the previous active FW.",
	"Activation Failed, Cold Reset Required: The new FW failed to activate. A cold reset is required.",
	"Invalid Handle: One or more Event Record Handles were invalid.",
	"Invalid Physical Address: The physical address specified is invalid.",
	"Inject Poison Limit Reached: The devices limit on allowed poison injection has been reached. Clear injected poison requests before attempting to inject more.",
	"Permanent Media Failure: The device could not clear poison due to a permanent issue with the media.",
	"Aborted: The background command was aborted by the device.",
	"Invalid Security State: The command is not valid in the current security state.",
	"Incorrect Passphrase: The passphrase does not match the currently set passphrase.",
	"Unsupported Mailbox: The command is not supported on the mailbox it was issued on. Used to indicate an unsupported command issued on the secondary mailbox.",
	"Invalid Payload Length: The payload length specified in the Command Register is not valid. The device is required to perform this check prior to processing any command defined in this specification.",
};

int send_cci_command(uint8_t eid, uint16_t opcode, const void* payload, size_t payload_len, 
                     std::vector<uint8_t>& response)
{
    DEBUG_PRINT("Starting CCI command: EID=%d, opcode=0x%x, payload_len=%zu", 
                eid, opcode, payload_len);
    
    struct sockaddr_mctp_ext addr = {};
    size_t msg_size = sizeof(mctp_cci_hdr) + payload_len;
    
    std::vector<uint8_t> txbuf(msg_size);
    mctp_cci_hdr* hdr = reinterpret_cast<mctp_cci_hdr*>(txbuf.data());
    
    auto sd = socket(AF_MCTP, SOCK_DGRAM, 0);
    if (sd < 0) {
        DEBUG_PRINT("Failed to create socket: %s", strerror(errno));
        std::cerr << "Failed to create socket for MCTP" << std::endl;
        return -1;
    }
    
    DEBUG_PRINT("Socket created: %d", sd);
    
    // Set socket timeout
    struct timeval timeout = {10, 0};
    if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        DEBUG_PRINT("Failed to set receive timeout: %s", strerror(errno));
    }
    
    if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        DEBUG_PRINT("Failed to set send timeout: %s", strerror(errno));
    }

    socklen_t addrlen = sizeof(struct sockaddr_mctp);
    addr.smctp_base.smctp_family = AF_MCTP;
    addr.smctp_base.smctp_network = DEFAULT_NET;
    addr.smctp_base.smctp_addr.s_addr = eid;
    addr.smctp_base.smctp_type = MCTP_MEG_TYPE_CCI;
    
    static uint8_t tag_counter = 0;
    uint8_t unique_tag = MCTP_TAG_OWNER | ((getpid() & 0x3) << 1) | (tag_counter++ & 0x1);
    addr.smctp_base.smctp_tag = unique_tag;

    DEBUG_PRINT("Using MCTP tag: 0x%x", unique_tag);

    hdr->cci_msg_req_resp = 0;
    hdr->msg_tag = unique_tag;
    hdr->op = opcode;
    hdr->pl_len = payload_len;
    hdr->cci_rsv = 0;
    hdr->rsv = 0;
    hdr->BO = 0;
    hdr->ret = 0;
    hdr->stat = 0;
    
    if (payload && payload_len > 0) {
        memcpy(txbuf.data() + sizeof(mctp_cci_hdr), payload, payload_len);
        DEBUG_PRINT("Copied %zu bytes of payload", payload_len);
    }

    if (g_debug_mode) {
        printf("[DEBUG] Request packet (%zu bytes):\n", msg_size);
        for (size_t i = 0; i < msg_size && i < 64; i++) {
            printf("%02x ", txbuf[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        if (msg_size % 16 != 0) printf("\n");
        fflush(stdout);
    }

    DEBUG_PRINT("Sending command to EID %d", eid);
    auto ret = sendto(sd, txbuf.data(), msg_size, 0, (struct sockaddr*)&addr, addrlen);
    if (ret < 0) {
        DEBUG_PRINT("Failed to send: %s (errno=%d)", strerror(errno), errno);
        std::cerr << "Failed to send CCI command: " << strerror(errno) << std::endl;
        close(sd);
        return -1;
    }
    
    DEBUG_PRINT("Sent %zd bytes, waiting for response", ret);

    ret = recvfrom(sd, nullptr, 0, MSG_PEEK | MSG_TRUNC, nullptr, 0);
    if (ret < 0) {
        DEBUG_PRINT("Peek failed: %s (errno=%d)", strerror(errno), errno);
        std::cerr << "Failed to receive response: " << strerror(errno) << std::endl;
        close(sd);
        return -1;
    }
    
    DEBUG_PRINT("Peek successful, response size: %zd bytes", ret);
    response.resize(ret);
    addrlen = sizeof(addr);
    ret = recvfrom(sd, response.data(), response.size(), 0, 
                   (struct sockaddr*)&addr, &addrlen);
    if (ret < 0) {
        DEBUG_PRINT("Failed to receive after peek: %s (errno=%d)", strerror(errno), errno);
        std::cerr << "Failed to receive response: " << strerror(errno) << std::endl;
        close(sd);
        return -1;
    }
    
    DEBUG_PRINT("Received %zd bytes", ret);
    
    if (!response.empty() && response.size() >= sizeof(mctp_cci_hdr)) {
        mctp_cci_hdr* resp_hdr = (mctp_cci_hdr*)response.data();
        
        if (resp_hdr->ret != 0 || resp_hdr->stat != 0) {
            DEBUG_PRINT("CCI command returned error: ret=0x%x, stat=0x%x", resp_hdr->ret, resp_hdr->stat);
            
            if (resp_hdr->ret < 23) {
                std::cerr << "CCI command failed with return code: 0x" << std::hex << resp_hdr->ret 
                          << " (" << DEVICE_ERRORS[resp_hdr->ret] << "), status: 0x" << resp_hdr->stat << std::dec << std::endl;
            } else {
                std::cerr << "CCI command failed with return code: 0x" << std::hex << resp_hdr->ret 
                          << ", status: 0x" << resp_hdr->stat << std::dec << std::endl;
            }
            close(sd);
            return -1;
        }
    }
    
    if (g_debug_mode && !response.empty()) {
        printf("[DEBUG] Response packet (%zu bytes):\n", response.size());
        for (size_t i = 0; i < response.size() && i < 64; i++) {
            printf("%02x ", response[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        if (response.size() % 16 != 0) printf("\n");
        fflush(stdout);
        
        if (response.size() >= sizeof(mctp_cci_hdr)) {
            mctp_cci_hdr* resp_hdr = (mctp_cci_hdr*)response.data();
            DEBUG_PRINT("Response CCI header: msg_tag=0x%x, op=0x%x, pl_len=%d, ret=0x%x, stat=0x%x", 
                       resp_hdr->msg_tag, resp_hdr->op, resp_hdr->pl_len, resp_hdr->ret, resp_hdr->stat);
        }
    }

    close(sd);
    DEBUG_PRINT("Socket closed, command completed successfully");
    return 0;
}

std::string find_argument(const std::vector<std::string>& params, const std::string& flag)
{
    DEBUG_PRINT("Looking for argument: %s", flag.c_str());
    for (size_t i = 0; i < params.size(); ++i) {
        if (params[i] == flag && i + 1 < params.size()) {
            DEBUG_PRINT("Found argument %s = %s", flag.c_str(), params[i + 1].c_str());
            return params[i + 1];
        }
    }
    DEBUG_PRINT("Argument %s not found", flag.c_str());
    return "";
}

// Helper functions
void int_to_string(uint8_t *string, uint8_t *integer, uint8_t sizeInByte) {
    uint8_t index;

    for (index = 0; index < sizeInByte; index++) {
        *(string + index * 2) = (*(integer + index) >> 4) & 0x0F;
        *(string + index * 2 + 1) = *(integer + index) & 0x0F;
    }
    for (index = 0; index < (sizeInByte * 2); index++) {
        if (*(string + index) >= 0x0A) {
            *(string + index) += 0x37;
        } else {
            *(string + index) += 0x30;
        }
    }
    *(string + sizeInByte * 2) = 0x0;
}

const char* decode_ddr4_module_type(uint8_t *bytes) {
    switch (bytes[3]) {
    case 0x01: return "RDIMM (Registered DIMM)";
    case 0x02: return "UDIMM (Unbuffered DIMM)";
    case 0x03: return "SODIMM (Small Outline Unbuffered DIMM)";
    case 0x04: return "LRDIMM (Load-Reduced DIMM)";
    case 0x05: return "Mini-RDIMM (Mini Registered DIMM)";
    case 0x06: return "Mini-UDIMM (Mini Unbuffered DIMM)";
    case 0x08: return "72b-SO-RDIMM (Small Outline Registered DIMM, 72-bit data bus)";
    case 0x09: return "72b-SO-UDIMM (Small Outline Unbuffered DIMM, 72-bit data bus)";
    case 0x0c: return "16b-SO-UDIMM (Small Outline Unbuffered DIMM, 16-bit data bus)";
    case 0x0d: return "32b-SO-UDIMM (Small Outline Unbuffered DIMM, 32-bit data bus)";
    default: return "Unknown";
    }
}

float ddr4_mtb_ftb_calc(unsigned char b1, signed char b2) {
    float mtb = 0.125;
    float ftb = 0.001;
    return b1 * mtb + b2 * ftb;
}

int decode_ddr4_module_speed(uint8_t *bytes) {
    float ctime;
    float ddrclk;

    ctime = ddr4_mtb_ftb_calc(bytes[18], bytes[125]);
    ddrclk = 2 * (1000 / ctime);

    return (int)ddrclk;
}

int decode_ddr4_module_size(uint8_t *bytes) {
    double size;
    int sdrcap = 256 << (bytes[4] & 15);
    int buswidth = 8 << (bytes[13] & 7);
    int sdrwidth = 4 << (bytes[12] & 7);
    int signal_loading = bytes[6] & 3;
    int lranks_per_dimm = ((bytes[12] >> 3) & 7) + 1;

    if (signal_loading == 2) lranks_per_dimm *= ((bytes[6] >> 4) & 7) + 1;
    size = sdrcap / 8 * buswidth / sdrwidth * lranks_per_dimm;
    return (int) size/1024;
}

const char *vendors[VENDORS_BANKS][VENDORS_ITEMS] =
{
{"AMD", "AMI", "Fairchild", "Fujitsu",
 "GTE", "Harris", "Hitachi", "Inmos",
 "Intel", "I.T.T.", "Intersil", "Monolithic Memories",
 "Mostek", "Freescale (former Motorola)", "National", "NEC",
 "RCA", "Raytheon", "Conexant (Rockwell)", "Seeq",
 "NXP (former Signetics, Philips Semi.)", "Synertek", "Texas Instruments", "Toshiba",
 "Xicor", "Zilog", "Eurotechnique", "Mitsubishi",
 "Lucent (AT&T)", "Exel", "Atmel", "SGS/Thomson",
 "Lattice Semi.", "NCR", "Wafer Scale Integration", "IBM",
 "Tristar", "Visic", "Intl. CMOS Technology", "SSSI",
 "MicrochipTechnology", "Ricoh Ltd.", "VLSI", "Micron Technology",
 "SK Hynix (former Hyundai Electronics)", "OKI Semiconductor", "ACTEL", "Sharp",
 "Catalyst", "Panasonic", "IDT", "Cypress",
 "DEC", "LSI Logic", "Zarlink (former Plessey)", "UTMC",
 "Thinking Machine", "Thomson CSF", "Integrated CMOS (Vertex)", "Honeywell",
 "Tektronix", "Oracle Corporation (former Sun Microsystems)", "Silicon Storage Technology", "ProMos/Mosel Vitelic",
 "Infineon (former Siemens)", "Macronix", "Xerox", "Plus Logic",
 "SunDisk", "Elan Circuit Tech.", "European Silicon Str.", "Apple Computer",
 "Xilinx", "Compaq", "Protocol Engines", "SCI",
 "Seiko Instruments", "Samsung", "I3 Design System", "Klic",
 "Crosspoint Solutions", "Alliance Semiconductor", "Tandem", "Hewlett-Packard",
 "Integrated Silicon Solutions", "Brooktree", "New Media", "MHS Electronic",
 "Performance Semi.", "Winbond Electronic", "Kawasaki Steel", "Bright Micro",
 "TECMAR", "Exar", "PCMCIA", "LG Semi (former Goldstar)",
 "Northern Telecom", "Sanyo", "Array Microsystems", "Crystal Semiconductor",
 "Analog Devices", "PMC-Sierra", "Asparix", "Convex Computer",
 "Quality Semiconductor", "Nimbus Technology", "Transwitch", "Micronas (ITT Intermetall)",
 "Cannon", "Altera", "NEXCOM", "QUALCOMM",
 "Sony", "Cray Research", "AMS(Austria Micro)", "Vitesse",
 "Aster Electronics", "Bay Networks (Synoptic)", "Zentrum or ZMD", "TRW",
 "Thesys", "Solbourne Computer", "Allied-Signal", "Dialog",
 "Media Vision", "Numonyx Corporation (former Level One Communication)"},
{"Cirrus Logic", "National Instruments", "ILC Data Device", "Alcatel Mietec",
 "Micro Linear", "Univ. of NC", "JTAG Technologies", "BAE Systems",
 "Nchip", "Galileo Tech", "Bestlink Systems", "Graychip",
 "GENNUM", "VideoLogic", "Robert Bosch", "Chip Express",
 "DATARAM", "United Microelec Corp.", "TCSI", "Smart Modular",
 "Hughes Aircraft", "Lanstar Semiconductor", "Qlogic", "Kingston",
 "Music Semi", "Ericsson Components", "SpaSE", "Eon Silicon Devices",
 "Programmable Micro Corp", "DoD", "Integ. Memories Tech.", "Corollary Inc.",
 "Dallas Semiconductor", "Omnivision", "EIV(Switzerland)", "Novatel Wireless",
 "Zarlink (former Mitel)", "Clearpoint", "Cabletron", "STEC (former Silicon Technology)",
 "Vanguard", "Hagiwara Sys-Com", "Vantis", "Celestica",
 "Century", "Hal Computers", "Rohm Company Ltd.", "Juniper Networks",
 "Libit Signal Processing", "Mushkin Enhanced Memory", "Tundra Semiconductor", "Adaptec Inc.",
 "LightSpeed Semi.", "ZSP Corp.", "AMIC Technology", "Adobe Systems",
 "Dynachip", "PNY Electronics", "Newport Digital", "MMC Networks",
 "T Square", "Seiko Epson", "Broadcom", "Viking Components",
 "V3 Semiconductor", "Flextronics (former Orbit)", "Suwa Electronics", "Transmeta",
 "Micron CMS", "American Computer & Digital Components Inc", "Enhance 3000 Inc", "Tower Semiconductor",
 "CPU Design", "Price Point", "Maxim Integrated Product", "Tellabs",
 "Centaur Technology", "Unigen Corporation", "Transcend Information", "Memory Card Technology",
 "CKD Corporation Ltd.", "Capital Instruments, Inc.", "Aica Kogyo, Ltd.", "Linvex Technology",
 "MSC Vertriebs GmbH", "AKM Company, Ltd.", "Dynamem, Inc.", "NERA ASA",
 "GSI Technology", "Dane-Elec (C Memory)", "Acorn Computers", "Lara Technology",
 "Oak Technology, Inc.", "Itec Memory", "Tanisys Technology", "Truevision",
 "Wintec Industries", "Super PC Memory", "MGV Memory", "Galvantech",
 "Gadzoox Nteworks", "Multi Dimensional Cons.", "GateField", "Integrated Memory System",
 "Triscend", "XaQti", "Goldenram", "Clear Logic",
 "Cimaron Communications", "Nippon Steel Semi. Corp.", "Advantage Memory", "AMCC",
 "LeCroy", "Yamaha Corporation", "Digital Microwave", "NetLogic Microsystems",
 "MIMOS Semiconductor", "Advanced Fibre", "BF Goodrich Data.", "Epigram",
 "Acbel Polytech Inc.", "Apacer Technology", "Admor Memory", "FOXCONN",
 "Quadratics Superconductor", "3COM"},
{"Camintonn Corporation", "ISOA Incorporated", "Agate Semiconductor", "ADMtek Incorporated",
 "HYPERTEC", "Adhoc Technologies", "MOSAID Technologies", "Ardent Technologies",
 "Switchcore", "Cisco Systems, Inc.", "Allayer Technologies", "WorkX AG (Wichman)",
 "Oasis Semiconductor", "Novanet Semiconductor", "E-M Solutions", "Power General",
 "Advanced Hardware Arch.", "Inova Semiconductors GmbH", "Telocity", "Delkin Devices",
 "Symagery Microsystems", "C-Port Corporation", "SiberCore Technologies", "Southland Microsystems",
 "Malleable Technologies", "Kendin Communications", "Great Technology Microcomputer", "Sanmina Corporation",
 "HADCO Corporation", "Corsair", "Actrans System Inc.", "ALPHA Technologies",
 "Silicon Laboratories, Inc. (Cygnal)", "Artesyn Technologies", "Align Manufacturing", "Peregrine Semiconductor",
 "Chameleon Systems", "Aplus Flash Technology", "MIPS Technologies", "Chrysalis ITS",
 "ADTEC Corporation", "Kentron Technologies", "Win Technologies", "Tachyon Semiconductor (former ASIC Designs Inc.)",
 "Extreme Packet Devices", "RF Micro Devices", "Siemens AG", "Sarnoff Corporation",
 "Itautec SA (former Itautec Philco SA)", "Radiata Inc.", "Benchmark Elect. (AVEX)", "Legend",
 "SpecTek Incorporated", "Hi/fn", "Enikia Incorporated", "SwitchOn Networks",
 "AANetcom Incorporated", "Micro Memory Bank", "ESS Technology", "Virata Corporation",
 "Excess Bandwidth", "West Bay Semiconductor", "DSP Group", "Newport Communications",
 "Chip2Chip Incorporated", "Phobos Corporation", "Intellitech Corporation", "Nordic VLSI ASA",
 "Ishoni Networks", "Silicon Spice", "Alchemy Semiconductor", "Agilent Technologies",
 "Centillium Communications", "W.L. Gore", "HanBit Electronics", "GlobeSpan",
 "Element 14", "Pycon", "Saifun Semiconductors", "Sibyte, Incorporated",
 "MetaLink Technologies", "Feiya Technology", "I & C Technology", "Shikatronics",
 "Elektrobit", "Megic", "Com-Tier", "Malaysia Micro Solutions",
 "Hyperchip", "Gemstone Communications", "Anadigm (former Anadyne)", "3ParData",
 "Mellanox Technologies", "Tenx Technologies", "Helix AG", "Domosys",
 "Skyup Technology", "HiNT Corporation", "Chiaro", "MDT Technologies GmbH (former MCI Computer GMBH)",
 "Exbit Technology A/S", "Integrated Technology Express", "AVED Memory", "Legerity",
 "Jasmine Networks", "Caspian Networks", "nCUBE", "Silicon Access Networks",
 "FDK Corporation", "High Bandwidth Access", "MultiLink Technology", "BRECIS",
 "World Wide Packets", "APW", "Chicory Systems", "Xstream Logic",
 "Fast-Chip", "Zucotto Wireless", "Realchip", "Galaxy Power",
 "eSilicon", "Morphics Technology", "Accelerant Networks", "Silicon Wave",
 "SandCraft", "Elpida"},
{"Solectron", "Optosys Technologies", "Buffalo (former Melco)", "TriMedia Technologies",
 "Cyan Technologies", "Global Locate", "Optillion", "Terago Communications",
 "Ikanos Communications", "Princeton Technology", "Nanya Technology", "Elite Flash Storage",
 "Mysticom", "LightSand Communications", "ATI Technologies", "Agere Systems",
 "NeoMagic", "AuroraNetics", "Golden Empire", "Mushkin",
 "Tioga Technologies", "Netlist", "TeraLogic", "Cicada Semiconductor",
 "Centon Electronics", "Tyco Electronics", "Magis Works", "Zettacom",
 "Cogency Semiconductor", "Chipcon AS", "Aspex Technology", "F5 Networks",
 "Programmable Silicon Solutions", "ChipWrights", "Acorn Networks", "Quicklogic",
 "Kingmax Semiconductor", "BOPS", "Flasys", "BitBlitz Communications",
 "eMemory Technology", "Procket Networks", "Purple Ray", "Trebia Networks",
 "Delta Electronics", "Onex Communications", "Ample Communications", "Memory Experts Intl",
 "Astute Networks", "Azanda Network Devices", "Dibcom", "Tekmos",
 "API NetWorks", "Bay Microsystems", "Firecron Ltd", "Resonext Communications",
 "Tachys Technologies", "Equator Technology", "Concept Computer", "SILCOM",
 "3Dlabs", "c't Magazine", "Sanera Systems", "Silicon Packets",
 "Viasystems Group", "Simtek", "Semicon Devices Singapore", "Satron Handelsges",
 "Improv Systems", "INDUSYS GmbH", "Corrent", "Infrant Technologies",
 "Ritek Corp", "empowerTel Networks", "Hypertec", "Cavium Networks",
 "PLX Technology", "Massana Design", "Intrinsity", "Valence Semiconductor",
 "Terawave Communications", "IceFyre Semiconductor", "Primarion", "Picochip Designs Ltd",
 "Silverback Systems", "Jade Star Technologies", "Pijnenburg Securealink",
 "takeMS - Ultron AG (former Memorysolution GmbH)", "Cambridge Silicon Radio",
 "Swissbit", "Nazomi Communications", "eWave System",
 "Rockwell Collins", "Picocel Co., Ltd.", "Alphamosaic Ltd", "Sandburst",
 "SiCon Video", "NanoAmp Solutions", "Ericsson Technology", "PrairieComm",
 "Mitac International", "Layer N Networks", "MtekVision", "Allegro Networks",
 "Marvell Semiconductors", "Netergy Microelectronic", "NVIDIA", "Internet Machines",
 "Peak Electronics", "Litchfield Communication", "Accton Technology", "Teradiant Networks",
 "Scaleo Chip (former Europe Technologies)", "Cortina Systems", "RAM Components", "Raqia Networks",
 "ClearSpeed", "Matsushita Battery", "Xelerated", "SimpleTech",
 "Utron Technology", "Astec International", "AVM gmbH", "Redux Communications",
 "Dot Hill Systems", "TeraChip"},
{"T-RAM Incorporated", "Innovics Wireless", "Teknovus", "KeyEye Communications",
 "Runcom Technologies", "RedSwitch", "Dotcast", "Silicon Mountain Memory",
 "Signia Technologies", "Pixim", "Galazar Networks", "White Electronic Designs",
 "Patriot Scientific", "Neoaxiom Corporation", "3Y Power Technology", "Scaleo Chip (former Europe Technologies)",
 "Potentia Power Systems", "C-guys Incorporated", "Digital Communications Technology Incorporated", "Silicon-Based Technology",
 "Fulcrum Microsystems", "Positivo Informatica Ltd", "XIOtech Corporation", "PortalPlayer",
 "Zhiying Software", "Parker Vision, Inc. (former Direct2Data)", "Phonex Broadband", "Skyworks Solutions",
 "Entropic Communications", "Pacific Force Technology", "Zensys A/S", "Legend Silicon Corp.",
 "sci-worx GmbH", "SMSC (former Oasis Silicon Systems)", "Renesas Electronics (former Renesas Technology)", "Raza Microelectronics",
 "Phyworks", "MediaTek", "Non-cents Productions", "US Modular",
 "Wintegra Ltd", "Mathstar", "StarCore", "Oplus Technologies",
 "Mindspeed", "Just Young Computer", "Radia Communications", "OCZ",
 "Emuzed", "LOGIC Devices", "Inphi Corporation", "Quake Technologies",
 "Vixel", "SolusTek", "Kongsberg Maritime", "Faraday Technology",
 "Altium Ltd.", "Insyte", "ARM Ltd.", "DigiVision",
 "Vativ Technologies", "Endicott Interconnect Technologies", "Pericom", "Bandspeed",
 "LeWiz Communications", "CPU Technology", "Ramaxel Technology", "DSP Group",
 "Axis Communications", "Legacy Electronics", "Chrontel", "Powerchip Semiconductor",
 "MobilEye Technologies", "Excel Semiconductor", "A-DATA Technology", "VirtualDigm",
 "G.Skill Intl", "Quanta Computer", "Yield Microelectronics", "Afa Technologies",
 "KINGBOX Technology Co. Ltd.", "Ceva", "iStor Networks", "Advance Modules",
 "Microsoft", "Open-Silicon", "Goal Semiconductor", "ARC International",
 "Simmtec", "Metanoia", "Key Stream", "Lowrance Electronics",
 "Adimos", "SiGe Semiconductor", "Fodus Communications", "Credence Systems Corp.",
 "Genesis Microchip Inc.", "Vihana, Inc.", "WIS Technologies", "GateChange Technologies",
 "High Density Devices AS", "Synopsys", "Gigaram", "Enigma Semiconductor Inc.",
 "Century Micro Inc.", "Icera Semiconductor", "Mediaworks Integrated Systems", "O'Neil Product Development",
 "Supreme Top Technology Ltd.", "MicroDisplay Corporation", "Team Group Inc.", "Sinett Corporation",
 "Toshiba Corporation", "Tensilica", "SiRF Technology", "Bacoc Inc.",
 "SMaL Camera Technologies", "Thomson SC", "Airgo Networks", "Wisair Ltd.",
 "SigmaTel", "Arkados", "Compete IT gmbH Co. KG", "Eudar Technology Inc.",
 "Focus Enhancements", "Xyratex"},
{"Specular Networks", "Patriot Memory", "U-Chip Technology Corp.", "Silicon Optix",
 "Greenfield Networks", "CompuRAM GmbH", "Stargen, Inc.", "NetCell Corporation",
 "Excalibrus Technologies Ltd", "SCM Microsystems", "Xsigo Systems, Inc.", "CHIPS & Systems Inc",
 "Tier 1 Multichip Solutions", "CWRL Labs", "Teradici", "Gigaram, Inc.",
 "g2 Microsystems", "PowerFlash Semiconductor", "P.A. Semi, Inc.", "NovaTech Solutions, S.A.",
 "c2 Microsystems, Inc.", "Level5 Networks", "COS Memory AG", "Innovasic Semiconductor",
 "02IC Co. Ltd", "Tabula, Inc.", "Crucial Technology", "Chelsio Communications",
 "Solarflare Communications", "Xambala Inc.", "EADS Astrium", "Terra Semiconductor Inc. (former ATO Semicon Co. Ltd.)",
 "Imaging Works, Inc.", "Astute Networks, Inc.", "Tzero", "Emulex",
 "Power-One", "Pulse~LINK Inc.", "Hon Hai Precision Industry", "White Rock Networks Inc.",
 "Telegent Systems USA, Inc.", "Atrua Technologies, Inc.", "Acbel Polytech Inc.",
 "eRide Inc.","ULi Electronics Inc.", "Magnum Semiconductor Inc.", "neoOne Technology, Inc.",
 "Connex Technology, Inc.", "Stream Processors, Inc.", "Focus Enhancements", "Telecis Wireless, Inc.",
 "uNav Microelectronics", "Tarari, Inc.", "Ambric, Inc.", "Newport Media, Inc.", "VMTS",
 "Enuclia Semiconductor, Inc.", "Virtium Technology Inc.", "Solid State System Co., Ltd.", "Kian Tech LLC",
 "Artimi", "Power Quotient International", "Avago Technologies", "ADTechnology", "Sigma Designs",
 "SiCortex, Inc.", "Ventura Technology Group", "eASIC", "M.H.S. SAS", "Micro Star International",
 "Rapport Inc.", "Makway International", "Broad Reach Engineering Co.",
 "Semiconductor Mfg Intl Corp", "SiConnect", "FCI USA Inc.", "Validity Sensors",
 "Coney Technology Co. Ltd.", "Spans Logic", "Neterion Inc.", "Qimonda",
 "New Japan Radio Co. Ltd.", "Velogix", "Montalvo Systems", "iVivity Inc.", "Walton Chaintech",
 "AENEON", "Lorom Industrial Co. Ltd.", "Radiospire Networks", "Sensio Technologies, Inc.",
 "Nethra Imaging", "Hexon Technology Pte Ltd", "CompuStocx (CSX)", "Methode Electronics, Inc.",
 "Connect One Ltd.", "Opulan Technologies", "Septentrio NV", "Goldenmars Technology Inc.",
 "Kreton Corporation", "Cochlear Ltd.", "Altair Semiconductor", "NetEffect, Inc.",
 "Spansion, Inc.", "Taiwan Semiconductor Mfg", "Emphany Systems Inc.",
 "ApaceWave Technologies", "Mobilygen Corporation", "Tego", "Cswitch Corporation",
 "Haier (Beijing) IC Design Co.", "MetaRAM", "Axel Electronics Co. Ltd.", "Tilera Corporation",
 "Aquantia", "Vivace Semiconductor", "Redpine Signals", "Octalica", "InterDigital Communications",
 "Avant Technology", "Asrock, Inc.", "Availink", "Quartics, Inc.", "Element CXI",
 "Innovaciones Microelectronicas", "VeriSilicon Microelectronics", "W5 Networks"},
{"MOVEKING", "Mavrix Technology, Inc.", "CellGuide Ltd.", "Faraday Technology",
 "Diablo Technologies, Inc.", "Jennic", "Octasic", "Molex Incorporated", "3Leaf Networks",
 "Bright Micron Technology", "Netxen", "NextWave Broadband Inc.", "DisplayLink", "ZMOS Technology",
 "Tec-Hill", "Multigig, Inc.", "Amimon", "Euphonic Technologies, Inc.", "BRN Phoenix",
 "InSilica", "Ember Corporation", "Avexir Technologies Corporation", "Echelon Corporation",
 "Edgewater Computer Systems", "XMOS Semiconductor Ltd.", "GENUSION, Inc.", "Memory Corp NV",
 "SiliconBlue Technologies", "Rambus Inc.", "Andes Technology Corporation", "Coronis Systems",
 "Achronix Semiconductor", "Siano Mobile Silicon Ltd.", "Semtech Corporation", "Pixelworks Inc.",
 "Gaisler Research AB", "Teranetics", "Toppan Printing Co. Ltd.", "Kingxcon",
 "Silicon Integrated Systems", "I-O Data Device, Inc.", "NDS Americas Inc.", "Solomon Systech Limited",
 "On Demand Microelectronics", "Amicus Wireless Inc.", "SMARDTV SNC", "Comsys Communication Ltd.",
 "Movidia Ltd.", "Javad GNSS, Inc.", "Montage Technology Group", "Trident Microsystems", "Super Talent",
 "Optichron, Inc.", "Future Waves UK Ltd.", "SiBEAM, Inc.", "Inicore, Inc.", "Virident Systems",
 "M2000, Inc.", "ZeroG Wireless, Inc.", "Gingle Technology Co. Ltd.", "Space Micro Inc.", "Wilocity",
 "Novafora, Inc.", "iKoa Corporation", "ASint Technology", "Ramtron", "Plato Networks Inc.",
 "IPtronics AS", "Infinite-Memories", "Parade Technologies Inc.", "Dune Networks",
 "GigaDevice Semiconductor", "Modu Ltd.", "CEITEC", "Northrop Grumman", "XRONET Corporation",
 "Sicon Semiconductor AB", "Atla Electronics Co. Ltd.", "TOPRAM Technology", "Silego Technology Inc.",
 "Kinglife", "Ability Industries Ltd.", "Silicon Power Computer & Communications",
 "Augusta Technology, Inc.", "Nantronics Semiconductors", "Hilscher Gesellschaft", "Quixant Ltd.",
 "Percello Ltd.", "NextIO Inc.", "Scanimetrics Inc.", "FS-Semi Company Ltd.", "Infinera Corporation",
 "SandForce Inc.", "Lexar Media", "Teradyne Inc.", "Memory Exchange Corp.", "Suzhou Smartek Electronics",
 "Avantium Corporation", "ATP Electronics Inc.", "Valens Semiconductor Ltd", "Agate Logic, Inc.",
 "Netronome", "Zenverge, Inc.", "N-trig Ltd", "SanMax Technologies Inc.", "Contour Semiconductor Inc.",
 "TwinMOS", "Silicon Systems, Inc.", "V-Color Technology Inc.", "Certicom Corporation", "JSC ICC Milandr",
 "PhotoFast Global Inc.", "InnoDisk Corporation", "Muscle Power", "Energy Micro", "Innofidei",
 "CopperGate Communications", "Holtek Semiconductor Inc.", "Myson Century, Inc.", "FIDELIX",
 "Red Digital Cinema", "Densbits Technology", "Zempro", "MoSys", "Provigent", "Triad Semiconductor, Inc."},
{"Siklu Communication Ltd.", "A Force Manufacturing Ltd.", "Strontium", "Abilis Systems", "Siglead, Inc.",
 "Ubicom, Inc.", "Unifosa Corporation", "Stretch, Inc.", "Lantiq Deutschland GmbH", "Visipro",
 "EKMemory", "Microelectronics Institute ZTE", "Cognovo Ltd.", "Carry Technology Co. Ltd.", "Nokia",
 "King Tiger Technology", "Sierra Wireless", "HT Micron", "Albatron Technology Co. Ltd.",
 "Leica Geosystems AG", "BroadLight", "AEXEA", "ClariPhy Communications, Inc.", "Green Plug",
 "Design Art Networks", "Mach Xtreme Technology Ltd.", "ATO Solutions Co. Ltd.", "Ramsta",
 "Greenliant Systems, Ltd.", "Teikon", "Antec Hadron", "NavCom Technology, Inc.",
 "Shanghai Fudan Microelectronics", "Calxeda, Inc.", "JSC EDC Electronics", "Kandit Technology Co. Ltd.",
 "Ramos Technology", "Goldenmars Technology", "XeL Technology Inc.", "Newzone Corporation",
 "ShenZhen MercyPower Tech", "Nanjing Yihuo Technology", "Nethra Imaging Inc.", "SiTel Semiconductor BV",
 "SolidGear Corporation", "Topower Computer Ind Co Ltd.", "Wilocity", "Profichip GmbH",
 "Gerad Technologies", "Ritek Corporation", "Gomos Technology Limited", "Memoright Corporation",
 "D-Broad, Inc.", "HiSilicon Technologies", "Syndiant Inc.", "Enverv Inc.", "Cognex",
 "Xinnova Technology Inc.", "Ultron AG", "Concord Idea Corporation", "AIM Corporation",
 "Lifetime Memory Products", "Ramsway", "Recore Systems BV", "Haotian Jinshibo Science Tech",
 "Being Advanced Memory", "Adesto Technologies", "Giantec Semiconductor, Inc.", "HMD Electronics AG",
 "Gloway International (HK)", "Kingcore", "Anucell Technology Holding",
 "Accord Software & Systems Pvt. Ltd.", "Active-Semi Inc.", "Denso Corporation", "TLSI Inc.",
 "Shenzhen Daling Electronic Co. Ltd.", "Mustang", "Orca Systems", "Passif Semiconductor",
 "GigaDevice Semiconductor (Beijing) Inc.", "Memphis Electronic", "Beckhoff Automation GmbH",
 "Harmony Semiconductor Corp (former ProPlus Design Solutions)", "Air Computers SRL", "TMT Memory",
 "Eorex Corporation", "Xingtera", "Netsol", "Bestdon Technology Co. Ltd.", "Baysand Inc.",
 "Uroad Technology Co. Ltd. (former Triple Grow Industrial Ltd.)", "Wilk Elektronik S.A.",
 "AAI", "Harman", "Berg Microelectronics Inc.", "ASSIA, Inc.", "Visiontek Products LLC",
 "OCMEMORY", "Welink Solution Inc.", "Shark Gaming", "Avalanche Technology",
 "R&D Center ELVEES OJSC", "KingboMars Technology Co. Ltd.",
 "High Bridge Solutions Industria Eletronica", "Transcend Technology Co. Ltd.",
 "Everspin Technologies", "Hon-Hai Precision", "Smart Storage Systems", "Toumaz Group",
 "Zentel Electronics Corporation", "Panram International Corporation",
 "Silicon Space Technology"}
};

const char* decode_ddr4_manufacturer(uint8_t *bytes) {
    if (!bytes) {
        return "Unknown";
    }
    
    uint8_t count = bytes[320];
    uint8_t code = bytes[321];

    if (code == 0x00 || code == 0xFF) {
        return "Unknown";
    }

    uint8_t bank = count & 0x7f;
    uint8_t index = code & 0x7f;
    if (bank >= VENDORS_BANKS || index == 0) {
        return "Unknown";
    }
    if (index > VENDORS_ITEMS) {
        return "Unknown";
    }
    
    return vendors[bank][index-1];
}

int decode_ram_type(uint8_t *bytes) {
    if (bytes[0] < 4) {
        switch (bytes[2]) {
        case 1: return DIRECT_RAMBUS;
        case 17: return RAMBUS;
        }
    } else {
        switch (bytes[2]) {
        case 1: return FPM_DRAM;
        case 2: return EDO;
        case 3: return PIPELINED_NIBBLE;
        case 4: return SDR_SDRAM;
        case 5: return MULTIPLEXED_ROM;
        case 6: return DDR_SGRAM;
        case 7: return DDR_SDRAM;
        case 8: return DDR2_SDRAM;
        case 11: return DDR3_SDRAM;
        case 12: return DDR4_SDRAM;
        }
    }

    return UNKNOWN;
}

// CCI functions
int get_fw_info(uint8_t eid)
{
    DEBUG_PRINT("get_fw_info called for EID: %d", eid);
    std::vector<uint8_t> response;
    
    if (send_cci_command(eid, CCI_GET_FW_INFO, nullptr, 0, response) < 0) {
        return -1;
    }

    if (response.size() != sizeof(cci_fw_info_resp)) {
        std::cerr << "Invalid response length for Get FW Info: " << response.size() << std::endl;
        return -1;
    }

    cci_fw_info_resp* fw_info = (cci_fw_info_resp*)response.data();
    
    std::cout << "================================= get fw info ==================================" << std::endl;
    std::cout << "FW Slots Supported: " << +fw_info->fw_slot_supported << std::endl;
    std::cout << "Active FW Slot: " << +fw_info->fw_slot_info.fields.ACTIVE_FW_SLOT << std::endl;
    if (fw_info->fw_slot_info.fields.STAGED_FW_SLOT) {
        std::cout << "Staged FW Slot: " << +fw_info->fw_slot_info.fields.STAGED_FW_SLOT << std::endl;
    }
    std::cout << "FW Activation Capabilities: " << +fw_info->fw_active_capability << std::endl;
    
    std::cout << "Slot 1 FW Revision: " << std::string(fw_info->slot1_fw_revision, 16) << std::endl;
    std::cout << "Slot 2 FW Revision: " << std::string(fw_info->slot2_fw_revision, 16) << std::endl;
    std::cout << "Slot 3 FW Revision: " << std::string(fw_info->slot3_fw_revision, 16) << std::endl;
    std::cout << "Slot 4 FW Revision: " << std::string(fw_info->slot4_fw_revision, 16) << std::endl;

    return 0;
}

int get_event_records(uint8_t eid, uint8_t event_log_type)
{
    DEBUG_PRINT("get_event_records called for EID: %d, log_type: %d", eid, event_log_type);
    std::vector<uint8_t> response;
    
    if (send_cci_command(eid, CCI_GET_EVENT_RECORDS, &event_log_type, 1, response) < 0) {
        return -1;
    }

    if (response.size() < sizeof(mctp_cci_hdr) + sizeof(cxl_get_event_record_info)) {
        std::cerr << "Invalid response length for Get Event Records: " << response.size() << std::endl;
        return -1;
    }

    uint8_t* payload_start = response.data() + sizeof(mctp_cci_hdr);
    uint32_t payload_size = response.size() - sizeof(mctp_cci_hdr);
    
    cxl_get_event_record_info* event_info = reinterpret_cast<cxl_get_event_record_info*>(payload_start);
    
    std::cout << "cxl_dram_event_record size: 0x" << std::hex << sizeof(struct cxl_dram_event_record) << std::dec << std::endl;
    std::cout << "cxl_memory_module_record size: 0x" << std::hex << sizeof(struct cxl_memory_module_record) << std::dec << std::endl;
    std::cout << "cxl_event_record size: 0x" << std::hex << sizeof(struct cxl_event_record) << std::dec << std::endl;
    std::cout << "cxl_get_event_record_info size: 0x" << std::hex << sizeof(struct cxl_get_event_record_info) << std::dec << std::endl;
    std::cout << "========= Get Event Records Info =========" << std::endl;
    std::cout << "  out size: 0x" << std::hex << payload_size << std::dec << std::endl;
    std::cout << "  flags: 0x" << std::hex << +event_info->flags << std::dec << std::endl;
    std::cout << "  overflow_err_cnt: 0x" << std::hex << le16_to_cpu(event_info->overflow_err_cnt) << std::dec << std::endl;
    std::cout << "  first_overflow_evt_ts: 0x" << std::hex << le64_to_cpu(event_info->first_overflow_evt_ts) << std::dec << std::endl;
    std::cout << "  last_overflow_evt_ts: 0x" << std::hex << le64_to_cpu(event_info->last_overflow_evt_ts) << std::dec << std::endl;
    std::cout << "  event_record_count: 0x" << std::hex << le16_to_cpu(event_info->event_record_count) << std::dec << std::endl;

    uint16_t event_record_count = le16_to_cpu(event_info->event_record_count);
    if (event_record_count == 0) {
        return 0;
    }

    int max_records = std::min(CXL_MAX_RECORDS_TO_DUMP, (int)event_record_count);
    
    struct cxl_event_record* event_records = reinterpret_cast<struct cxl_event_record*>(
        payload_start + sizeof(cxl_get_event_record_info)
    );
    
    size_t available_space = payload_size - sizeof(cxl_get_event_record_info);
    size_t available_records = available_space / sizeof(struct cxl_event_record);
    max_records = std::min(max_records, (int)available_records);
    
    for (int rec = 0; rec < max_records; ++rec) {
        char uuid_str[37];
        struct cxl_event_record* event_record = &event_records[rec];

        uuid_unparse(event_record->uuid, uuid_str);

        if (strcmp(uuid_str, CXL_DRAM_EVENT_GUID) == 0) {
            std::cout << "  Event Record: " << rec << " (DRAM guid: " << uuid_str << ")" << std::endl;
        } else if (strcmp(uuid_str, CXL_MEM_MODULE_EVENT_GUID) == 0) {
            std::cout << "  Event Record: " << rec << " (Memory Module Event guid: " << uuid_str << ")" << std::endl;
        } else {
            std::cout << "  Event Record: " << rec << " (uuid: " << uuid_str << ")" << std::endl;
        }

        std::cout << "    event_record_length: 0x" << std::hex << +event_record->event_record_length << std::dec << std::endl;
        std::cout << "    event_record_flags: 0x" << std::hex 
                  << std::setfill('0') << std::setw(2) << +event_record->event_record_flags[0]
                  << std::setw(2) << +event_record->event_record_flags[1]
                  << std::setw(2) << +event_record->event_record_flags[2] << std::dec << std::endl;
        std::cout << "    event_record_handle: 0x" << std::hex << le16_to_cpu(event_record->event_record_handle) << std::dec << std::endl;
        std::cout << "    related_event_record_handle: 0x" << std::hex << le16_to_cpu(event_record->related_event_record_handle) << std::dec << std::endl;
        std::cout << "    event_record_ts: 0x" << std::hex << le64_to_cpu(event_record->event_record_ts) << std::dec << std::endl;

        if (strcmp(uuid_str, CXL_DRAM_EVENT_GUID) == 0) {
            struct cxl_dram_event_record* dram_event = &event_record->event_record.dram_event_record;
            std::cout << "    physical_addr: 0x" << std::hex << le64_to_cpu(dram_event->physical_addr) << std::dec << std::endl;
            std::cout << "    memory_event_descriptor: 0x" << std::hex << +dram_event->memory_event_descriptor << std::dec << std::endl;
            std::cout << "    memory_event_type: 0x" << std::hex << +dram_event->memory_event_type << std::dec << std::endl;
            std::cout << "    transaction_type: 0x" << std::hex << +dram_event->transaction_type << std::dec << std::endl;
            std::cout << "    validity_flags: 0x" << std::hex << le16_to_cpu(dram_event->validity_flags) << std::dec << std::endl;
            std::cout << "    channel: 0x" << std::hex << +dram_event->channel << std::dec << std::endl;
            std::cout << "    rank: 0x" << std::hex << +dram_event->rank << std::dec << std::endl;
            std::cout << "    nibble_mask: 0x" << std::hex 
                      << std::setfill('0') << std::setw(2) << +dram_event->nibble_mask[0]
                      << std::setw(2) << +dram_event->nibble_mask[1]
                      << std::setw(2) << +dram_event->nibble_mask[2] << std::dec << std::endl;
            std::cout << "    bank_group: 0x" << std::hex << +dram_event->bank_group << std::dec << std::endl;
            std::cout << "    bank: 0x" << std::hex << +dram_event->bank << std::dec << std::endl;
            std::cout << "    row: 0x" << std::hex 
                      << std::setfill('0') << std::setw(2) << +dram_event->row[0]
                      << std::setw(2) << +dram_event->row[1]
                      << std::setw(2) << +dram_event->row[2] << std::dec << std::endl;
            std::cout << "    column: 0x" << std::hex << le16_to_cpu(dram_event->column) << std::dec << std::endl;
            
            for (int i = 0; i < 4; i++) {
                std::cout << "    correction mask[" << i << "]: 0x";
                for (int j = 0; j < 8; j++) {
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << +dram_event->correction_mask[i*8+j];
                }
                std::cout << std::dec << std::endl;
            }
            std::cout << "    component identifier: 0x" << std::hex 
                      << std::setfill('0') << std::setw(2) << +dram_event->component_identifier[0]
                      << std::setw(2) << +dram_event->component_identifier[1]
                      << std::setw(2) << +dram_event->component_identifier[2] << std::dec << std::endl;
        }
    }

    return 0;
}

int get_supported_logs(uint8_t eid)
{
    DEBUG_PRINT("get_supported_logs called for EID: %d", eid);
    std::vector<uint8_t> response;
    
    if (send_cci_command(eid, CCI_GET_SUPPORTED_LOGS, nullptr, 0, response) < 0) {
        return -1;
    }

    if (response.size() < sizeof(mctp_cci_hdr) + 8) {
        std::cerr << "Invalid response length for Get Supported Logs" << std::endl;
        return -1;
    }

    uint8_t* payload_start = response.data() + sizeof(mctp_cci_hdr);
    
    uint16_t entries = le16_to_cpu(*(uint16_t*)payload_start);
    uint8_t* entry_start = payload_start + sizeof(uint16_t) + 6;
    
    std::cout << "payload info" << std::endl;
    std::cout << "    out size: 0x" << std::hex << (response.size() - sizeof(mctp_cci_hdr)) << std::dec << std::endl;
    std::cout << "    entries: " << entries << std::endl;
    
    for (int e = 0; e < entries; ++e) {
        uint8_t* current_entry = entry_start + e * (16 + 4);
        
        uuid_t uuid;
        memcpy(uuid, current_entry, 16);
        uint32_t size = le32_to_cpu(*(uint32_t*)(current_entry + 16));
        
        char uuid_str[37];
        uuid_unparse(uuid, uuid_str);
        
        DEBUG_PRINT("Raw UUID bytes: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                   uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
                   uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
        
        std::cout << "        entries[" << e << "] uuid: " << uuid_str << ", size: " << size << std::endl;
        
        if (strcmp(uuid_str, CEL_UUID) == 0) {
            std::cout << "          -> This is CEL (Command Effects Log)" << std::endl;
        } else if (strcmp(uuid_str, VENDOR_LOG_UUID) == 0) {
            std::cout << "          -> This is Vendor Debug Log" << std::endl;
        }
    }

    return 0;
}

int get_log(uint8_t eid, const std::string& uuid_str, uint32_t data_size)
{
    DEBUG_PRINT("get_log called for EID: %d, UUID: %s, size: %u", eid, uuid_str.c_str(), data_size);
    std::vector<uint8_t> response;
    cxl_mbox_get_log get_log_input = {};
    uint32_t remaining_bytes = data_size;
    uint32_t bytes_read = 0;
    const uint32_t payload_max = 64;
    int retry_count = 0;
    const int max_retries = 3;
    
    int total_entries_displayed = 0;
    bool is_first_chunk = true;

    if (uuid_parse(uuid_str.c_str(), get_log_input.uuid) != 0) {
        std::cerr << "Invalid UUID format: " << uuid_str << std::endl;
        return -1;
    }

    DEBUG_PRINT("UUID parsed successfully using standard uuid_parse");
    
    if (g_debug_mode) {
        printf("[DEBUG] Parsed UUID bytes: ");
        for (int i = 0; i < 16; i++) {
            printf("%02x ", get_log_input.uuid[i]);
        }
        printf("\n");
        fflush(stdout);
    }

    do {
        uint32_t chunk_size = std::min(remaining_bytes, payload_max);
        
        get_log_input.offset = bytes_read;
        get_log_input.length = chunk_size;
        
        DEBUG_PRINT("Reading log chunk: offset=%u, length=%u (remaining=%u)", 
                   get_log_input.offset, get_log_input.length, remaining_bytes);
        
        cxl_mbox_get_log send_input = get_log_input;
        send_input.offset = cpu_to_le32(get_log_input.offset);
        send_input.length = cpu_to_le32(get_log_input.length);
        
        if (g_debug_mode) {
            printf("[DEBUG] Sending get_log request:\n");
            printf("[DEBUG]   UUID: %s\n", uuid_str.c_str());
            printf("[DEBUG]   Offset: %u (0x%x)\n", get_log_input.offset, get_log_input.offset);
            printf("[DEBUG]   Length: %u (0x%x)\n", get_log_input.length, get_log_input.length);
            fflush(stdout);
        }
        
        if (send_cci_command(eid, CCI_GET_LOG, &send_input, sizeof(send_input), response) < 0) {
            DEBUG_PRINT("send_cci_command failed for get_log");
            return -1;
        }

        if (response.size() < sizeof(mctp_cci_hdr)) {
            std::cerr << "Invalid response length for Get Log" << std::endl;
            return -1;
        }

        mctp_cci_hdr* resp_hdr = (mctp_cci_hdr*)response.data();
        if (resp_hdr->ret != 0 || resp_hdr->stat != 0) {
            DEBUG_PRINT("CCI get_log returned error: ret=0x%x, stat=0x%x", resp_hdr->ret, resp_hdr->stat);
            std::cerr << "Get Log failed with CCI error: ret=0x" << std::hex << resp_hdr->ret 
                      << ", stat=0x" << resp_hdr->stat << std::dec << std::endl;
            return -1;
        }

        uint32_t payload_size = response.size() - sizeof(mctp_cci_hdr);
        uint8_t* payload = response.data() + sizeof(mctp_cci_hdr);

        DEBUG_PRINT("Received log chunk: payload_size=%u", payload_size);

        if (payload_size == 0) {
            retry_count++;
            if (retry_count >= max_retries) {
                DEBUG_PRINT("Got empty response %d times, stopping", retry_count);
                std::cout << "Warning: Received empty log data after " << retry_count << " attempts" << std::endl;
                break;
            }
            DEBUG_PRINT("Got empty response, retry %d/%d", retry_count, max_retries);
            usleep(100000);
            continue;
        }

        retry_count = 0;

        if (is_first_chunk) {
            std::cout << "payload info" << std::endl;
            std::cout << "    out size: 0x" << std::hex << data_size << std::dec << std::endl;
            
            if (uuid_str == CEL_UUID) {
                int total_cel_entries = data_size / sizeof(cel_entry);
                std::cout << "    no_cel_entries size: " << total_cel_entries << std::endl;
            } else if (uuid_str == VENDOR_LOG_UUID) {
                std::cout << " number of received bytes = " << data_size << std::endl;
            }
            is_first_chunk = false;
        }

        if (uuid_str == CEL_UUID) {
            cel_entry* cel_entries = (cel_entry*)payload;
            int no_cel_entries = payload_size / sizeof(cel_entry);
            for (int e = 0; e < no_cel_entries; ++e) {
                std::cout << "    cel_entry[" << total_entries_displayed << "] opcode: 0x" 
                          << std::hex << le16_to_cpu(cel_entries[e].opcode)
                          << ", effect: 0x" << le16_to_cpu(cel_entries[e].effect) 
                          << std::dec << std::endl;
                total_entries_displayed++;
            }
        } else if (uuid_str == VENDOR_LOG_UUID) {
            if (payload_size > 0) {
                std::string vendor_log((char*)payload, payload_size);
                std::cout << vendor_log;
                std::cout.flush();
            }
        } else {
            DEBUG_PRINT("Showing hex dump for unknown UUID");
            if (payload_size > 0) {
                if (is_first_chunk) {
                    std::cout << "Log data (" << data_size << " bytes):" << std::endl;
                }
                for (uint32_t i = 0; i < payload_size; i++) {
                    printf("%02x ", payload[i]);
                    if ((i + 1) % 16 == 0) printf("\n");
                }
                fflush(stdout);
            }
        }

        bytes_read += payload_size;
        if (remaining_bytes >= payload_size) {
            remaining_bytes -= payload_size;
        } else {
            remaining_bytes = 0;
        }
        
        DEBUG_PRINT("Progress: bytes_read=%u, remaining=%u", bytes_read, remaining_bytes);
        
        if (payload_size == 0 && remaining_bytes > 0) {
            DEBUG_PRINT("Breaking due to zero payload size with remaining bytes");
            break;
        }
        
    } while (remaining_bytes > 0 && bytes_read < data_size);

    DEBUG_PRINT("get_log completed: total_bytes_read=%u, requested=%u", bytes_read, data_size);
    return 0;
}

static const char *ram_types[] = {"Unknown", "Direct Rambus", "Rambus", "FPM DRAM",
                                  "EDO", "Pipelined Nibble", "SDR SDRAM", "Multiplexed ROM",
                                  "DDR SGRAM", "DDR SDRAM", "DDR2", "DDR3", "DDR4"};

int dimm_spd_read(uint8_t eid, uint32_t spd_id, uint32_t offset, uint32_t num_bytes)
{
    DEBUG_PRINT("dimm_spd_read called for EID: %d, spd_id: %u, offset: %u, num_bytes: %u", 
                eid, spd_id, offset, num_bytes);
    
    std::vector<uint8_t> all_data;
    uint32_t remaining_bytes = num_bytes;
    uint32_t bytes_read = 0;
    uint32_t current_offset = offset;
    const uint32_t payload_max = 64;
    int retry_count = 0;
    const int max_retries = 3;
    
    // Reserve space for all data
    all_data.reserve(num_bytes);
    
    std::cout << "=========================== DIMM SPD READ Data ============================" << std::endl;
    std::cout << "Reading " << num_bytes << " bytes from SPD ID " << spd_id 
              << " starting at offset 0x" << std::hex << offset << std::dec << std::endl;
    
    do {
        uint32_t chunk_size = std::min(remaining_bytes, payload_max);
        
        DEBUG_PRINT("Reading SPD chunk: spd_id=%u, offset=%u, chunk_size=%u (remaining=%u)", 
                   spd_id, current_offset, chunk_size, remaining_bytes);
        
        cxl_mbox_dimm_spd_read_in spd_input = {};
        spd_input.spd_id = cpu_to_le32(spd_id);
        spd_input.offset = cpu_to_le32(current_offset);
        spd_input.num_bytes = cpu_to_le32(chunk_size);
        
        if (g_debug_mode) {
            printf("[DEBUG] Sending dimm_spd_read request:\n");
            printf("[DEBUG]   SPD ID: %u\n", spd_id);
            printf("[DEBUG]   Offset: %u (0x%x)\n", current_offset, current_offset);
            printf("[DEBUG]   Chunk Size: %u (0x%x)\n", chunk_size, chunk_size);
            fflush(stdout);
        }
        
        std::vector<uint8_t> response;
        if (send_cci_command(eid, CCI_DIMM_SPD_READ, &spd_input, sizeof(spd_input), response) < 0) {
            DEBUG_PRINT("send_cci_command failed for dimm_spd_read");
            return -1;
        }

        if (response.size() < sizeof(mctp_cci_hdr)) {
            std::cerr << "Invalid response length for DIMM SPD Read" << std::endl;
            return -1;
        }

        mctp_cci_hdr* resp_hdr = (mctp_cci_hdr*)response.data();
        if (resp_hdr->ret != 0 || resp_hdr->stat != 0) {
            DEBUG_PRINT("CCI dimm_spd_read returned error: ret=0x%x, stat=0x%x", resp_hdr->ret, resp_hdr->stat);
            std::cerr << "DIMM SPD Read failed with CCI error: ret=0x" << std::hex << resp_hdr->ret 
                      << ", stat=0x" << resp_hdr->stat << std::dec << std::endl;
            return -1;
        }

        uint32_t payload_size = response.size() - sizeof(mctp_cci_hdr);
        uint8_t* payload = response.data() + sizeof(mctp_cci_hdr);

        DEBUG_PRINT("Received SPD chunk: payload_size=%u", payload_size);

        if (payload_size == 0) {
            retry_count++;
            if (retry_count >= max_retries) {
                DEBUG_PRINT("Got empty response %d times, stopping", retry_count);
                std::cout << "Warning: Received empty SPD data after " << retry_count << " attempts" << std::endl;
                break;
            }
            DEBUG_PRINT("Got empty response, retry %d/%d", retry_count, max_retries);
            usleep(100000);
            continue;
        }

        retry_count = 0;

        // Append this chunk to all_data
        all_data.insert(all_data.end(), payload, payload + payload_size);

        bytes_read += payload_size;
        current_offset += payload_size;
        if (remaining_bytes >= payload_size) {
            remaining_bytes -= payload_size;
        } else {
            remaining_bytes = 0;
        }
        
        DEBUG_PRINT("Progress: bytes_read=%u, remaining=%u, current_offset=%u", 
                   bytes_read, remaining_bytes, current_offset);
        
        if (payload_size == 0 && remaining_bytes > 0) {
            DEBUG_PRINT("Breaking due to zero payload size with remaining bytes");
            break;
        }
        
    } while (remaining_bytes > 0 && bytes_read < num_bytes);

    DEBUG_PRINT("dimm_spd_read completed: total_bytes_read=%u, requested=%u", bytes_read, num_bytes);

    // Display the complete data
    if (!all_data.empty()) {
        std::cout << "Output Payload:";
        for (uint32_t i = 0; i < all_data.size(); i++) {
            if (i % 16 == 0) {
                printf("\n%04x  %02x ", i + offset, all_data[i]);
            } else {
                printf("%02x ", all_data[i]);
            }
        }
        std::cout << "\n" << std::endl;

        // Decode SPD data for DDR4 SDRAM only if we have enough data
        RamType ram_type = (RamType)decode_ram_type(all_data.data());
        
        if (ram_type == DDR4_SDRAM && all_data.size() >= 512) {
            int buswidth = 8 << (all_data[13] & 7);
            uint8_t serial[9];

            std::cout << "\n====== DIMM SPD DECODE ============" << std::endl;
            std::cout << "Total Width: TBD" << std::endl;
            std::cout << "Data Width: " << buswidth << " bits" << std::endl;
            std::cout << "Size: " << decode_ddr4_module_size(all_data.data()) << " GB" << std::endl;
            std::cout << "Form Factor: TBD" << std::endl;
            std::cout << "Set: TBD" << std::endl;
            std::cout << "Locator: DIMM_X" << std::endl;
            std::cout << "Bank Locator: _Node1_ChannelX_DimmX" << std::endl;
            std::cout << "Type: " << ram_types[ram_type] << std::endl;
            std::cout << "Type Detail: " << decode_ddr4_module_type(all_data.data()) << std::endl;
            std::cout << "Speed: " << decode_ddr4_module_speed(all_data.data()) << " MT/s" << std::endl;
            std::cout << "Manufacturer: " << decode_ddr4_manufacturer(all_data.data()) << std::endl;
            
            if (all_data.size() >= 329) {
                int_to_string(serial, &all_data[325], SPD_MODULE_SERIAL_NUMBER_LEN);
                std::cout << "Serial Number: " << (char*)serial << std::endl;
            }
            std::cout << "Asset Tag: TBD" << std::endl;
        } else if (ram_type == DDR4_SDRAM && all_data.size() < 512) {
            std::cout << "\nNote: DDR4 SDRAM detected but insufficient data for full decode (need 512 bytes, got " 
                      << all_data.size() << " bytes)" << std::endl;
        }
    } else {
        std::cout << "No data received" << std::endl;
    }

    return 0;
}

int dimm_slot_info(uint8_t eid)
{
    DEBUG_PRINT("dimm_slot_info called for EID: %d", eid);
    std::vector<uint8_t> response;
    
    if (send_cci_command(eid, CCI_DIMM_SLOT_INFO, nullptr, 0, response) < 0) {
        return -1;
    }

    if (response.size() != sizeof(mctp_cci_hdr) + sizeof(cxl_dimm_slot_info_out)) {
        std::cerr << "Invalid response length for DIMM Slot Info: " << response.size() << std::endl;
        return -1;
    }

    uint8_t* payload = response.data() + sizeof(mctp_cci_hdr);
    uint32_t payload_size = response.size() - sizeof(mctp_cci_hdr);
    cxl_dimm_slot_info_out* slot_info = (cxl_dimm_slot_info_out*)payload;

    std::cout << "=========================== DIMM SLOT INFO ============================" << std::endl;
    std::cout << "Output Payload:" << std::endl;
    for (uint32_t i = 0; i < payload_size; i++) {
        if (i % 16 == 0) {
            printf("\n%04x  %02x ", i, payload[i]);
        } else {
            printf("%02x ", payload[i]);
        }
    }
    std::cout << "\n" << std::endl;

    std::cout << "\n====== DIMM SLOTS INFO DECODE ============" << std::endl;
    std::cout << "Number of DIMM Slots: " << +slot_info->num_dimm_slots << std::endl;
    
    std::cout << "DIMM SPD Index: 0" << std::endl;
    std::cout << "    DIMM Present: 0x" << std::hex << +slot_info->slot0_dimm_present << std::dec << std::endl;
    std::cout << "    DIMM Silk Screen: " << (char)slot_info->slot0_dimm_silk_screen << std::endl;
    std::cout << "    Channel ID: 0x" << std::hex << +slot_info->slot0_channel_id << std::dec << std::endl;
    std::cout << "    I2C Address: 0x" << std::hex << +slot_info->slot0_spd_i2c_addr << std::dec << std::endl;
    
    std::cout << "DIMM SPD Index: 1" << std::endl;
    std::cout << "    DIMM Present: 0x" << std::hex << +slot_info->slot1_dimm_present << std::dec << std::endl;
    std::cout << "    DIMM Silk Screen: " << (char)slot_info->slot1_dimm_silk_screen << std::endl;
    std::cout << "    Channel ID: 0x" << std::hex << +slot_info->slot1_channel_id << std::dec << std::endl;
    std::cout << "    I2C Address: 0x" << std::hex << +slot_info->slot1_spd_i2c_addr << std::dec << std::endl;
    
    std::cout << "DIMM SPD Index: 2" << std::endl;
    std::cout << "    DIMM Present: 0x" << std::hex << +slot_info->slot2_dimm_present << std::dec << std::endl;
    std::cout << "    DIMM Silk Screen: " << (char)slot_info->slot2_dimm_silk_screen << std::endl;
    std::cout << "    Channel ID: 0x" << std::hex << +slot_info->slot2_channel_id << std::dec << std::endl;
    std::cout << "    I2C Address: 0x" << std::hex << +slot_info->slot2_spd_i2c_addr << std::dec << std::endl;
    
    std::cout << "DIMM SPD Index: 3" << std::endl;
    std::cout << "    DIMM Present: 0x" << std::hex << +slot_info->slot3_dimm_present << std::dec << std::endl;
    std::cout << "    DIMM Silk Screen: " << (char)slot_info->slot3_dimm_silk_screen << std::endl;
    std::cout << "    Channel ID: 0x" << std::hex << +slot_info->slot3_channel_id << std::dec << std::endl;
    std::cout << "    I2C Address: 0x" << std::hex << +slot_info->slot3_spd_i2c_addr << std::dec << std::endl;

    std::cout << "\n" << std::endl;
    return 0;
}

int get_health_counters(uint8_t eid)
{
    DEBUG_PRINT("get_health_counters called for EID: %d", eid);
    std::vector<uint8_t> response;
    
    if (send_cci_command(eid, CCI_GET_HEALTH_COUNTERS, nullptr, 0, response) < 0) {
        return -1;
    }

    if (response.size() != sizeof(mctp_cci_hdr) + sizeof(cxl_mbox_health_counters_get_out)) {
        std::cerr << "Invalid response length for Get Health Counters: " << response.size() << std::endl;
        return -1;
    }

    cxl_mbox_health_counters_get_out* health_counters = (cxl_mbox_health_counters_get_out*)(response.data() + sizeof(mctp_cci_hdr));
    
    std::cout << "============================= get health counters ==============================" << std::endl;
    std::cout << "0: CRITICAL_OVER_TEMPERATURE_EXCEEDED = " << le32_to_cpu(health_counters->critical_over_temperature_exceeded) << std::endl;
    std::cout << "1: OVER_TEMPERATURE_WARNING_LEVEL_EXCEEDED = " << le32_to_cpu(health_counters->over_temperature_warning_level_exceeded) << std::endl;
    std::cout << "2: CRITICAL_UNDER_TEMPERATURE_EXCEEDED = " << le32_to_cpu(health_counters->critical_under_temperature_exceeded) << std::endl;
    std::cout << "3: UNDER_TEMPERATURE_WARNING_LEVEL_EXCEEDED = " << le32_to_cpu(health_counters->under_temperature_warning_level_exceeded) << std::endl;
    std::cout << "4: POWER_ON_EVENTS = " << le32_to_cpu(health_counters->power_on_events) << std::endl;
    std::cout << "5: POWER_ON_HOURS = " << le32_to_cpu(health_counters->power_on_hours) << std::endl;
    std::cout << "6: CXL_MEM_LINK_CRC_ERRORS = " << le32_to_cpu(health_counters->cxl_mem_link_crc_errors) << std::endl;
    std::cout << "7: CXL_IO_LINK_LCRC_ERRORS = " << le32_to_cpu(health_counters->cxl_io_link_lcrc_errors) << std::endl;
    std::cout << "8: CXL_IO_LINK_ECRC_ERRORS = " << le32_to_cpu(health_counters->cxl_io_link_ecrc_errors) << std::endl;
    std::cout << "9: NUM_DDR_COR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_correctable_ecc_errors) << std::endl;
    std::cout << "10: NUM_DDR_UNCOR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_uncorrectable_ecc_errors) << std::endl;
    std::cout << "11: LINK_RECOVERY_EVENTS = " << le32_to_cpu(health_counters->link_recovery_events) << std::endl;
    std::cout << "12: TIME_IN_THROTTLED = " << le32_to_cpu(health_counters->time_in_throttled) << std::endl;
    std::cout << "13: RX_RETRY_REQUEST = " << le32_to_cpu(health_counters->rx_retry_request) << std::endl;
    std::cout << "14: RCMD_QS0_HI_THRESHOLD_DETECT = " << le32_to_cpu(health_counters->rcmd_qs0_hi_threshold_detect) << std::endl;
    std::cout << "15: RCMD_QS1_HI_THRESHOLD_DETECT = " << le32_to_cpu(health_counters->rcmd_qs1_hi_threshold_detect) << std::endl;
    std::cout << "16: NUM_PSCAN_COR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_pscan_correctable_ecc_errors) << std::endl;
    std::cout << "17: NUM_PSCAN_UNCOR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_pscan_uncorrectable_ecc_errors) << std::endl;
    std::cout << "18: NUM_DDR_DIMM0_COR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_dimm0_correctable_ecc_errors) << std::endl;
    std::cout << "19: NUM_DDR_DIMM0_UNCOR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_dimm0_uncorrectable_ecc_errors) << std::endl;
    std::cout << "20: NUM_DDR_DIMM1_COR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_dimm1_correctable_ecc_errors) << std::endl;
    std::cout << "21: NUM_DDR_DIMM1_UNCOR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_dimm1_uncorrectable_ecc_errors) << std::endl;
    std::cout << "22: NUM_DDR_DIMM2_COR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_dimm2_correctable_ecc_errors) << std::endl;
    std::cout << "23: NUM_DDR_DIMM2_UNCOR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_dimm2_uncorrectable_ecc_errors) << std::endl;
    std::cout << "24: NUM_DDR_DIMM3_COR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_dimm3_correctable_ecc_errors) << std::endl;
    std::cout << "25: NUM_DDR_DIMM3_UNCOR_ECC_ERRORS = " << le32_to_cpu(health_counters->num_ddr_dimm3_uncorrectable_ecc_errors) << std::endl;

    return 0;
}

int get_cxl_membridge_stats(uint8_t eid)
{
    DEBUG_PRINT("get_cxl_membridge_stats called for EID: %d", eid);
    std::vector<uint8_t> response;
    
    if (send_cci_command(eid, CCI_GET_CXL_MEMBRIDGE_STATS, nullptr, 0, response) < 0) {
        return -1;
    }

    if (response.size() != sizeof(mctp_cci_hdr) + sizeof(cxl_cmd_membridge_stats_out)) {
        std::cerr << "Invalid response length for Get CXL Membridge Stats: " << response.size() << std::endl;
        return -1;
    }

    cxl_cmd_membridge_stats_out* stats = (cxl_cmd_membridge_stats_out*)(response.data() + sizeof(mctp_cci_hdr));
    
    std::cout << "m2s_req_count:              " << stats->m2s_req_count << std::endl;
    std::cout << "m2s_rwd_count:              " << stats->m2s_rwd_count << std::endl;
    std::cout << "s2m_drs_count:              " << stats->s2m_drs_count << std::endl;
    std::cout << "s2m_ndr_count:              " << stats->s2m_ndr_count << std::endl;
    std::cout << "rwd_first_poison_hpa:       0x" << std::hex << stats->rwd_first_poison_hpa_log << std::dec << std::endl;
    std::cout << "rwd_latest_poison_hpa:      0x" << std::hex << stats->rwd_latest_poison_hpa_log << std::dec << std::endl;
    std::cout << "req_first_hpa_log:          0x" << std::hex << stats->req_first_hpa_log << std::dec << std::endl;
    std::cout << "rwd_first_hpa_log:          0x" << std::hex << stats->rwd_first_hpa_log << std::dec << std::endl;
    std::cout << "m2s_req_corr_err_count:     " << stats->mst_m2s_req_corr_err_count << std::endl;
    std::cout << "m2s_rwd_corr_err_count:     " << stats->mst_m2s_rwd_corr_err_count << std::endl;
    std::cout << "fifo_full_status:           0x" << std::hex << stats->fifo_full_status << std::dec << std::endl;
    std::cout << "fifo_empty_status:          0x" << std::hex << stats->fifo_empty_status << std::dec << std::endl;
    std::cout << "m2s_rwd_credit_count:       " << +stats->m2s_rwd_credit_count << std::endl;
    std::cout << "m2s_req_credit_count:       " << +stats->m2s_req_credit_count << std::endl;
    std::cout << "s2m_ndr_credit_count:       " << +stats->s2m_ndr_credit_count << std::endl;
    std::cout << "s2m_drc_credit_count:       " << +stats->s2m_drc_credit_count << std::endl;
    std::cout << "rx_status_rx_deinit:        0x" << std::hex << +stats->rx_fsm_status_rx_deinit << std::dec << std::endl;
    std::cout << "rx_status_m2s_req:          0x" << std::hex << +stats->rx_fsm_status_m2s_req << std::dec << std::endl;
    std::cout << "rx_status_m2s_rwd:          0x" << std::hex << +stats->rx_fsm_status_m2s_rwd << std::dec << std::endl;
    std::cout << "rx_status_ddr0_ar_req:      0x" << std::hex << +stats->rx_fsm_status_ddr0_ar_req << std::dec << std::endl;
    std::cout << "rx_status_ddr0_aw_req:      0x" << std::hex << +stats->rx_fsm_status_ddr0_aw_req << std::dec << std::endl;
    std::cout << "rx_status_ddr0_w_req:       0x" << std::hex << +stats->rx_fsm_status_ddr0_w_req << std::dec << std::endl;
    std::cout << "rx_status_ddr1_ar_req:      0x" << std::hex << +stats->rx_fsm_status_ddr1_ar_req << std::dec << std::endl;
    std::cout << "rx_status_ddr1_aw_req:      0x" << std::hex << +stats->rx_fsm_status_ddr1_aw_req << std::dec << std::endl;
    std::cout << "rx_status_ddr1_w_req:       0x" << std::hex << +stats->rx_fsm_status_ddr1_w_req << std::dec << std::endl;
    std::cout << "tx_status_tx_deinit:        0x" << std::hex << +stats->tx_fsm_status_tx_deinit << std::dec << std::endl;
    std::cout << "tx_status_s2m_ndr:          0x" << std::hex << +stats->tx_fsm_status_s2m_ndr << std::dec << std::endl;
    std::cout << "tx_status_s2m_drc:          0x" << std::hex << +stats->tx_fsm_status_s2m_drc << std::dec << std::endl;
    std::cout << "qos_tel_dev_load_read:      " << +stats->stat_qos_tel_dev_load_read << std::endl;
    std::cout << "qos_tel_dev_load_type2_read:" << +stats->stat_qos_tel_dev_load_type2_read << std::endl;
    std::cout << "qos_tel_dev_load_write:     " << +stats->stat_qos_tel_dev_load_write << std::endl;

    return 0;
}

// Wrapper functions
int get_fw_info_wrapper(uint8_t eid, const std::vector<std::string>& params)
{
    DEBUG_PRINT("get_fw_info_wrapper called");
    (void)params;
    return get_fw_info(eid);
}

int get_event_records_wrapper(uint8_t eid, const std::vector<std::string>& params)
{
    DEBUG_PRINT("get_event_records_wrapper called with %zu params", params.size());
    
    std::string event_log_type_str = find_argument(params, "--log_type");
    if (event_log_type_str.empty()) {
        event_log_type_str = find_argument(params, "-t");
    }
    
    uint8_t event_log_type = 0;
    if (!event_log_type_str.empty()) {
        event_log_type = static_cast<uint8_t>(std::stoul(event_log_type_str, nullptr, 0));
        if (event_log_type > 3) {
            std::cerr << "Error: Invalid event log type. Valid values: 0-3" << std::endl;
            return -1;
        }
    }
    
    DEBUG_PRINT("Using event log type: %d", event_log_type);
    return get_event_records(eid, event_log_type);
}

int get_supported_logs_wrapper(uint8_t eid, const std::vector<std::string>& params)
{
    DEBUG_PRINT("get_supported_logs_wrapper called");
    (void)params;
    return get_supported_logs(eid);
}

int get_log_wrapper(uint8_t eid, const std::vector<std::string>& params)
{
    DEBUG_PRINT("get_log_wrapper called with %zu params", params.size());
    
    std::string uuid = find_argument(params, "--log-uuid");
    if (uuid.empty()) {
        uuid = find_argument(params, "-l");
    }
    
    std::string size_str = find_argument(params, "--log-size");
    if (size_str.empty()) {
        size_str = find_argument(params, "-s");
    }
    
    if (uuid.empty() || size_str.empty()) {
        std::cerr << "Error: get-log command requires --log-uuid and --log-size parameters" << std::endl;
        std::cerr << "Usage: get-log --log-uuid <uuid> --log-size <size>" << std::endl;
        return -1;
    }
    
    uint32_t size = static_cast<uint32_t>(std::stoul(size_str, nullptr, 0));
    DEBUG_PRINT("Using UUID: %s, size: %u", uuid.c_str(), size);
    return get_log(eid, uuid, size);
}

int dimm_spd_read_wrapper(uint8_t eid, const std::vector<std::string>& params)
{
    DEBUG_PRINT("dimm_spd_read_wrapper called with %zu params", params.size());
    
    std::string spd_id_str = find_argument(params, "--spd-id");
    if (spd_id_str.empty()) {
        spd_id_str = find_argument(params, "-s");
    }
    
    std::string offset_str = find_argument(params, "--offset");
    if (offset_str.empty()) {
        offset_str = find_argument(params, "-o");
    }
    
    std::string num_bytes_str = find_argument(params, "--num-bytes");
    if (num_bytes_str.empty()) {
        num_bytes_str = find_argument(params, "-n");
    }
    
    if (spd_id_str.empty() || offset_str.empty() || num_bytes_str.empty()) {
        std::cerr << "Error: dimm-spd-read command requires --spd-id, --offset, and --num-bytes parameters" << std::endl;
        std::cerr << "Usage: dimm-spd-read --spd-id <id> --offset <offset> --num-bytes <bytes>" << std::endl;
        return -1;
    }
    
    uint32_t spd_id = static_cast<uint32_t>(std::stoul(spd_id_str, nullptr, 0));
    uint32_t offset = static_cast<uint32_t>(std::stoul(offset_str, nullptr, 0));
    uint32_t num_bytes = static_cast<uint32_t>(std::stoul(num_bytes_str, nullptr, 0));
    
    DEBUG_PRINT("Using spd_id: %u, offset: %u, num_bytes: %u", spd_id, offset, num_bytes);
    return dimm_spd_read(eid, spd_id, offset, num_bytes);
}

int dimm_slot_info_wrapper(uint8_t eid, const std::vector<std::string>& params)
{
    DEBUG_PRINT("dimm_slot_info_wrapper called");
    (void)params;
    return dimm_slot_info(eid);
}

int get_health_counters_wrapper(uint8_t eid, const std::vector<std::string>& params)
{
    DEBUG_PRINT("get_health_counters_wrapper called");
    (void)params;
    return get_health_counters(eid);
}

int get_cxl_membridge_stats_wrapper(uint8_t eid, const std::vector<std::string>& params)
{
    DEBUG_PRINT("get_cxl_membridge_stats_wrapper called");
    (void)params;
    return get_cxl_membridge_stats(eid);
}
