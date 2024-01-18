#ifndef UDP_DEF_H
#define UDP_DEF_H

#include <QString>

const int __UDP_PORT__ = 12345;
const QString __UDP_IP__ = "192.168.2.22";
const int __UDP_CMD_LEN = 10;
const int __UDP_ARM_LEN__ = 77;
const int __UDP_FPGA_LEN__ = 7;
const int __FRAME_ARM_LEN__ = 7;

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
    RESULT_GOOD = 0x10,
    RESULT_BAD = 0x00,
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
    MODULE_CODER = 5,
    MODULE_PROCESS = 6,
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
    STAGE_PREPARING = 2,
    STAGE_READY = 3,
    STAGE_FPGA_TEST = 4,
    STAGE_COMBINE_TEST = 5,
    STAGE_PC_TEST = 6,
    STAGE_GOOD = 7,
    STAGE_BAD = 8
};

struct ARM_DATA_STRUCT{
    char mode;
    char remote;
    char working;
    char transmit;
    char codeH1;
    char codeTL1;
    char codeH3;
    char codeTL3;
    char cheap;
    char anten;
    char anten_sp;
    int csphat;
    int csphanxa;
};



#endif // UDP_DEF_H
