//
// Created by ba on 2/8/22.
//

#ifndef BANATIVECRASH_SIGNAL_EXPLAIN_H
#define BANATIVECRASH_SIGNAL_EXPLAIN_H


namespace babyte {

    extern char *GetSIGBUS(int crash_signal_code) {
        switch (static_cast<unsigned int>(crash_signal_code)) {
            case MD_EXCEPTION_FLAG_LIN_BUS_ADRALN:
                return "address cannot be aligned";
            case MD_EXCEPTION_FLAG_LIN_BUS_ADRERR:
                return "wrong physical address";
            default :
                return reinterpret_cast<char *>(crash_signal_code);
        }
    }

    extern char *GetSIGFPE(int crash_signal_code) {
        switch (static_cast<unsigned int>(crash_signal_code)) {
            case MD_EXCEPTION_FLAG_LIN_FPE_INTDIV:
                return "int / 0";
            case MD_EXCEPTION_FLAG_LIN_FPE_INTOVF:
                return "int overflow";
            case MD_EXCEPTION_FLAG_LIN_FPE_FLTDIV:
                return "float / 0";
            case MD_EXCEPTION_FLAG_LIN_FPE_FLTOVF:
                return "float overflow";
            case MD_EXCEPTION_FLAG_LIN_FPE_FLTUND:
                return "floating point underflow";
            case MD_EXCEPTION_FLAG_LIN_FPE_FLTRES:
                return "floating point numbers are imprecise";
            case MD_EXCEPTION_FLAG_LIN_FPE_FLTINV:
                return "illegal floating point operation";
            case MD_EXCEPTION_FLAG_LIN_FPE_FLTSUB:
                return "The table below is out of bounds";
            default :
                return reinterpret_cast<char *>(crash_signal_code);
        }
    }

    extern char *GetSIGSEGV(int crash_signal_code) {
        switch (static_cast<unsigned int>(crash_signal_code)) {
            case MD_EXCEPTION_FLAG_LIN_SEGV_MAPERR:
                return "Invalid address";
            case MD_EXCEPTION_FLAG_LIN_SEGV_ACCERR:
                return "address without access";
            default :
                return reinterpret_cast<char *>(crash_signal_code);
        }
    }

    extern char *GetSIGILL(int crash_signal_code) {
        switch (static_cast<unsigned int>(crash_signal_code)) {
            case MD_EXCEPTION_FLAG_LIN_ILL_ILLOPC:
                return "Illegal opcode";
            case MD_EXCEPTION_FLAG_LIN_ILL_ILLOPN:
                return "Illegal operand";
            case MD_EXCEPTION_FLAG_LIN_ILL_ILLADR:
                return "Illegal addressing mode";
            case MD_EXCEPTION_FLAG_LIN_ILL_ILLTRP:
                return "Illegal trap";
            case MD_EXCEPTION_FLAG_LIN_ILL_PRVOPC:
                return "Privileged opcode";
            case MD_EXCEPTION_FLAG_LIN_ILL_PRVREG:
                return "Privileged register";
            case MD_EXCEPTION_FLAG_LIN_ILL_COPROC:
                return "Coprocessor error";
            case MD_EXCEPTION_FLAG_LIN_ILL_BADSTK:
                return "Internal stack error";
            default :
                return reinterpret_cast<char *>(crash_signal_code);
        }
    }

    extern char *GetCrashSignalCodeString(int crash_signal_, int crash_signal_code) {
        switch (static_cast<unsigned int>(crash_signal_)) {
            case SIGBUS:
                return GetSIGBUS(crash_signal_code);
            case SIGFPE:
                return GetSIGFPE(crash_signal_code);
            case SIGSEGV:
                return GetSIGSEGV(crash_signal_code);
            case SIGILL:
                return GetSIGILL(crash_signal_code);
            default:
                return reinterpret_cast<char *>(crash_signal_code);
        }
    }
}
#endif //BANATIVECRASH_SIGNAL_EXPLAIN_H
