#ifndef UDP_DEF_H
#define UDP_DEF_H

#include <QString>

enum ARM_CMD_ENUM {
    ARM_CMD_NONE = 0x00,
    ARM_CMD_MODE = 0x01,
    ARM_CMD_WORKING = 0x03,
    ARM_CMD_TRANSMIT = 0x04,
    ARM_CMD_CODEH1 = 0x05,
    ARM_CMD_CODETL1 = 0x06,
    ARM_CMD_CODEH3 = 0x07,
    ARM_CMD_CODETL3 = 0x08,
    ARM_CMD_ANTEN_EN = 0x09,
    ARM_CMD_ANTEN_SYNC = 0x0A,
    ARM_CMD_ANTEN_SPEED = 0x0B,
};

enum FPGA_CMD_ENUM {
    FPGA_CMD_IDLE = 0x00,
    FPGA_CMD_PREPARE = 0x01,
    FPGA_CMD_FPGA_TEST = 0x02,
    FPGA_CMD_COMBINE_TEST = 0x03,
    FPGA_CMD_RESULT = 0x04,
    FPGA_CMD_NONE = 0xFF
};

enum FPGA_STA_ENUM {
    FPGA_STA_IDLE = 0,
    FPGA_STA_READY = 1,
    FPGA_STA_FPGA_TEST = 2,
    FPGA_STA_COMBINE_TEST = 3,
    FPGA_STA_MODULE_WRONG = 252,
    FPGA_STA_TIMEOUT = 253,
    FPGA_STA_UNKNOWN = 254,
    FPGA_STA_NONE = 255
};

enum RESULT_ENUM {
    RESULT_GOOD = 0x00, //Thay đổi theo giao thức mới (2025)
    RESULT_BAD = 0x01,  //Mã lỗi mới (2025)
    RESULT_TIMEOUT = 253,
    RESULT_UNKNOWN = 254,
    RESULT_NONE = 255
};

enum MODULE_ENUM {
    MODULE_NONE = 0,
    MODULE_TRANSMITTER = 1,
    MODULE_RECEIVER = 2,
    MODULE_TEST = 3,
    MODULE_INTERCONNECT = 4,
    MODULE_PROCESS = 5,
    MODULE_CODER = 6,
    MODULE_ANTEN1 = 7,
    MODULE_ANTEN2 = 8
};

enum PC_CMD_ENUM {
    PC_CMD_PREPARE = 1,
    PC_CMD_TEST = 2
};

enum STAGE_ENUM {
    STAGE_IDLING = 0,
    STAGE_IDLE = 1,
    STAGE_START = 2,
    STAGE_ARM_PREPARE = 3,
    STAGE_FPGA_PREPARE = 4,
    STAGE_FPGA_READY = 5,
    STAGE_FPGA_TEST = 6,
    STAGE_COMBINE_TEST = 7,
    STAGE_PC_TEST = 8,
    STAGE_GOOD = 9,
    STAGE_BAD = 10,
    STAGE_FINISHED = 11,
    STAGE_LEN = 12
};

const QString stages[12] = {"IDLING",
                           "IDLE",
                           "START",
                           "ARM_PREPARE",
                           "FPGA_PREPARE",
                           "FPGA_READY",
                           "FPGA_TEST",
                           "COMBINE_TEST",
                           "PC_TEST",
                           "GOOD",
                           "BAD",
                           "FINISHED"};

struct ARM_DATA_STRUCT{
    int mode;
    int remote;
    int working;
    int transmit;
    int codeH1;
    int codeTL1;
    int codeH3;
    int codeTL3;
    int cheap;
    int anten;
    int anten_sp;
    int csphat;
    int csphanxa;
};

struct  FPGA_DATA_STRUCT
{
    FPGA_DATA_STRUCT() {}
    int cmd0;
    int cmd1;
    int status;
    int module;
    int step;
    int result;
};

//Constants
const int __HOST_PORT__ = 1000;
const QString __HOST_IP__ = "192.168.1.12";
const int __FPGA_PORT__ = 100;
const QString __FPGA_IP__ = "192.168.1.14";
const int __PROCESS_PORT__ = 12789;
const QString __PROCESS_IP__ = "192.168.1.240";
const int __INTERCONNECT_PORT__ = 12345;
const QString __INTERCONNECT_IP__ = "192.168.1.245";
const int __UDP_CMD_LEN = 10;
const int __UDP_ARM_LEN__ = 77;
const int __UDP_FPGA_LEN__ = 7;
const int __FRAME_ARM_LEN__ = 7;

//Global Variables
extern ARM_DATA_STRUCT g_arm_cmd;
extern ARM_DATA_STRUCT g_arm_sta;
extern FPGA_DATA_STRUCT g_fpga_cmd;
extern FPGA_DATA_STRUCT g_fpga_sta;

extern int g_cs_offset;

#endif // UDP_DEF_H
