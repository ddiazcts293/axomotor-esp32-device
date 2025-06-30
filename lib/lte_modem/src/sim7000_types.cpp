#include "sim7000_types.hpp"
#include <esp_err.h>

namespace axomotor::lte_modem {

static const at_cmd_def_t AT_COMMANDS_TABLE[] =
{
    { at_cmd_t::AT, at_cmd_type_t::BASIC, "", 0 },
    { at_cmd_t::A_SLASH, at_cmd_type_t::BASIC, "A/", 0 },
    { at_cmd_t::D, at_cmd_type_t::BASIC, "D", 0 },
    { at_cmd_t::E, at_cmd_type_t::BASIC, "E", 0 },
    { at_cmd_t::H, at_cmd_type_t::BASIC, "H", 20 },
    { at_cmd_t::I, at_cmd_type_t::BASIC, "I", 0 },
    { at_cmd_t::L, at_cmd_type_t::BASIC, "L", 0 },
    { at_cmd_t::M, at_cmd_type_t::BASIC, "M", 0 },
    { at_cmd_t::PLUS_TO_3RD, at_cmd_type_t::BASIC, "+++", 0 },
    { at_cmd_t::O, at_cmd_type_t::BASIC, "O", 0 },
    { at_cmd_t::Q, at_cmd_type_t::BASIC, "Q", 0 },
    { at_cmd_t::S0, at_cmd_type_t::S_PARAM, "S0", 0 },
    { at_cmd_t::S3, at_cmd_type_t::S_PARAM, "S3", 0 },
    { at_cmd_t::S4, at_cmd_type_t::S_PARAM, "S4", 0 },
    { at_cmd_t::S5, at_cmd_type_t::S_PARAM, "S5", 0 },
    { at_cmd_t::S6, at_cmd_type_t::S_PARAM, "S6", 0 },
    { at_cmd_t::S7, at_cmd_type_t::S_PARAM, "S7", 0 },
    { at_cmd_t::S8, at_cmd_type_t::S_PARAM, "S8", 0 },
    { at_cmd_t::S10, at_cmd_type_t::S_PARAM, "S10", 0 },
    { at_cmd_t::V, at_cmd_type_t::BASIC, "V", 0 },
    { at_cmd_t::X, at_cmd_type_t::BASIC, "X", 0 },
    { at_cmd_t::AND_C, at_cmd_type_t::BASIC, "&C", 0 },
    { at_cmd_t::AND_D, at_cmd_type_t::BASIC, "&D", 0 },
    { at_cmd_t::AND_E, at_cmd_type_t::BASIC, "&E", 0 },
    { at_cmd_t::GCAP, at_cmd_type_t::EXTENDED, "GCAP", 0 },
    { at_cmd_t::GMI, at_cmd_type_t::EXTENDED, "GMI", 0 },
    { at_cmd_t::GMM, at_cmd_type_t::EXTENDED, "GMM", 0 },
    { at_cmd_t::GMR, at_cmd_type_t::EXTENDED, "GMR", 0 },
    { at_cmd_t::GOI, at_cmd_type_t::EXTENDED, "GOI", 0 },
    { at_cmd_t::GSN, at_cmd_type_t::EXTENDED, "GSN", 0 },
    { at_cmd_t::ICF, at_cmd_type_t::EXTENDED, "ICF", 0 },
    { at_cmd_t::IFC, at_cmd_type_t::EXTENDED, "IFC", 0 },
    { at_cmd_t::IPR, at_cmd_type_t::EXTENDED, "IPR", 0 },

    { at_cmd_t::CGMI, at_cmd_type_t::EXTENDED, "CGMI", },
    { at_cmd_t::CGMM, at_cmd_type_t::EXTENDED, "CGMM", 0 },
    { at_cmd_t::CGMR, at_cmd_type_t::EXTENDED, "CGMR", 0 },
    { at_cmd_t::CGSN, at_cmd_type_t::EXTENDED, "CGSN", 0 },
    { at_cmd_t::CSCS, at_cmd_type_t::EXTENDED, "CSCS", 0 },
    { at_cmd_t::CIMI, at_cmd_type_t::EXTENDED, "CIMI", 20 },
    { at_cmd_t::CLCK, at_cmd_type_t::EXTENDED, "CLCK", 15 },
    { at_cmd_t::CMEE, at_cmd_type_t::EXTENDED, "CMEE", 0 },
    { at_cmd_t::COPS, at_cmd_type_t::EXTENDED, "COPS", 120 },
    { at_cmd_t::CPAS, at_cmd_type_t::EXTENDED, "CPAS", 0 },
    { at_cmd_t::CPIN, at_cmd_type_t::EXTENDED, "CPIN", 5 },
    { at_cmd_t::CPWD, at_cmd_type_t::EXTENDED, "CPWD", 15 },
    { at_cmd_t::CRC, at_cmd_type_t::EXTENDED, "CRC", 0 },
    { at_cmd_t::CREG, at_cmd_type_t::EXTENDED, "CREG", 0 },
    { at_cmd_t::CRSM, at_cmd_type_t::EXTENDED, "CRSM", 0 },
    { at_cmd_t::CSQ, at_cmd_type_t::EXTENDED, "CSQ", 0 },
    { at_cmd_t::CPOL, at_cmd_type_t::EXTENDED, "CPOL", 0 },
    { at_cmd_t::COPN, at_cmd_type_t::EXTENDED, "COPN", 0 },
    { at_cmd_t::CFUN, at_cmd_type_t::EXTENDED, "CFUN", 10 },
    { at_cmd_t::CCLK, at_cmd_type_t::EXTENDED, "CCLK", 0 },
    { at_cmd_t::CSIM, at_cmd_type_t::EXTENDED, "CSIM", 0 },
    { at_cmd_t::CBC, at_cmd_type_t::EXTENDED, "CBC", 0 },
    { at_cmd_t::CUSD, at_cmd_type_t::EXTENDED, "CUSD", 0 },
    { at_cmd_t::CNUM, at_cmd_type_t::EXTENDED, "CNUM", 0 },

    { at_cmd_t::CMGD, at_cmd_type_t::EXTENDED, "CMGD", 25 },
    { at_cmd_t::CMGF, at_cmd_type_t::EXTENDED, "CMGF", 0 },
    { at_cmd_t::CMGL, at_cmd_type_t::EXTENDED, "CMGL", 20 },
    { at_cmd_t::CMGR, at_cmd_type_t::EXTENDED, "CMGR", 5 },
    { at_cmd_t::CMGS, at_cmd_type_t::EXTENDED, "CMGS", 60 },
    { at_cmd_t::CMGW, at_cmd_type_t::EXTENDED, "CMGW", 5 },
    { at_cmd_t::CMSS, at_cmd_type_t::EXTENDED, "CMSS", 0 },
    { at_cmd_t::CNMI, at_cmd_type_t::EXTENDED, "CNMI", 0 },
    { at_cmd_t::CPMS, at_cmd_type_t::EXTENDED, "CPMS", 0 },
    { at_cmd_t::CRES, at_cmd_type_t::EXTENDED, "CRES", 5 },
    { at_cmd_t::CSAS, at_cmd_type_t::EXTENDED, "CSAS", 5 },
    { at_cmd_t::CSCA, at_cmd_type_t::EXTENDED, "CSCA", 5 },
    { at_cmd_t::CSDH, at_cmd_type_t::EXTENDED, "CSDH", 0 },
    { at_cmd_t::CSMP, at_cmd_type_t::EXTENDED, "CSMP", 0 },
    { at_cmd_t::CSMS, at_cmd_type_t::EXTENDED, "CSMS", 0 },

    { at_cmd_t::CPOWD, at_cmd_type_t::EXTENDED, "CPOWD", 0 },
    { at_cmd_t::CADC, at_cmd_type_t::EXTENDED, "CADC", 2 },
    { at_cmd_t::CFGRI, at_cmd_type_t::EXTENDED, "CFGRI", 0 },
    { at_cmd_t::CLTS, at_cmd_type_t::EXTENDED, "CLTS", 0 },
    { at_cmd_t::CBAND, at_cmd_type_t::EXTENDED, "CBAND", 0 },
    { at_cmd_t::CNSMOD, at_cmd_type_t::EXTENDED, "CNSMOD", 0 },
    { at_cmd_t::CSCLK, at_cmd_type_t::EXTENDED, "CSCLK", 0 },
    { at_cmd_t::CCID, at_cmd_type_t::EXTENDED, "CCID", 2 },
    { at_cmd_t::CDEVICE, at_cmd_type_t::EXTENDED, "CDEVICE", 0 },
    { at_cmd_t::GSV, at_cmd_type_t::EXTENDED, "GSV", 0 },
    { at_cmd_t::SGPIO, at_cmd_type_t::EXTENDED, "SGPIO", 0 },
    { at_cmd_t::SLEDS, at_cmd_type_t::EXTENDED, "SLEDS", 0 },
    { at_cmd_t::CNETLIGHT, at_cmd_type_t::EXTENDED, "CNETLIGHT", 0 },
    { at_cmd_t::CSGS, at_cmd_type_t::EXTENDED, "CSGS", 0 },
    { at_cmd_t::CGPIO, at_cmd_type_t::EXTENDED, "CGPIO", 0 },
    { at_cmd_t::CBATCHK, at_cmd_type_t::EXTENDED, "CBATCHK", 0 },
    { at_cmd_t::CNMP, at_cmd_type_t::EXTENDED, "CNMP", 0 },
    { at_cmd_t::CMNB, at_cmd_type_t::EXTENDED, "CMNB", 0 },
    { at_cmd_t::CPSMS, at_cmd_type_t::EXTENDED, "CPSMS", 0 },
    { at_cmd_t::CEDRXS, at_cmd_type_t::EXTENDED, "CEDRXS", 0 },
    { at_cmd_t::CPSI, at_cmd_type_t::EXTENDED, "CPSI", 0 },
    { at_cmd_t::CGNAPN, at_cmd_type_t::EXTENDED, "CGNAPN", 0 },
    { at_cmd_t::CSDP, at_cmd_type_t::EXTENDED, "CSDP", 0 },
    { at_cmd_t::MCELLLOCK, at_cmd_type_t::EXTENDED, "MCELLLOCK", 0 },
    { at_cmd_t::NCELLLOCK, at_cmd_type_t::EXTENDED, "NCELLLOCK", 0 },
    { at_cmd_t::NBSC, at_cmd_type_t::EXTENDED, "NBSC", 0 },
    { at_cmd_t::CAPNMODE, at_cmd_type_t::EXTENDED, "CAPNMODE", 0 },
    { at_cmd_t::CRRCSTATE, at_cmd_type_t::EXTENDED, "CRRCSTATE", 0 },
    { at_cmd_t::CBANDCFG, at_cmd_type_t::EXTENDED, "CBANDCFG", 0 },
    { at_cmd_t::CNACT, at_cmd_type_t::EXTENDED, "CNACT", 0 },
    { at_cmd_t::CNCFG, at_cmd_type_t::EXTENDED, "CNCFG", 0 },
    { at_cmd_t::CEDUMP, at_cmd_type_t::EXTENDED, "CEDUMP", 0 },
    { at_cmd_t::CNBS, at_cmd_type_t::EXTENDED, "CNBS", 0 },
    { at_cmd_t::CNDS, at_cmd_type_t::EXTENDED, "CNDS", 0 },
    { at_cmd_t::CENG, at_cmd_type_t::EXTENDED, "CENG", 0 },
    { at_cmd_t::CNACTCFG, at_cmd_type_t::EXTENDED, "CNACTCFG", 0 },
    { at_cmd_t::CTLIIC, at_cmd_type_t::EXTENDED, "CTLIIC", 0 },
    { at_cmd_t::CWIIC, at_cmd_type_t::EXTENDED, "CWIIC", 0 },
    { at_cmd_t::CRIIC, at_cmd_type_t::EXTENDED, "CRIIC", 0 },
    { at_cmd_t::CMCFG, at_cmd_type_t::EXTENDED, "CMCFG", 0 },
    { at_cmd_t::CSIMLOCK, at_cmd_type_t::EXTENDED, "CSIMLOCK", 0 },
    { at_cmd_t::CRATSRCH, at_cmd_type_t::EXTENDED, "CRATSRCH", 0 },
    { at_cmd_t::SPWM, at_cmd_type_t::EXTENDED, "SPWM", 0 },
    { at_cmd_t::CASRIP, at_cmd_type_t::EXTENDED, "CASRIP", 0 },
    { at_cmd_t::CEDRX, at_cmd_type_t::EXTENDED, "CEDRX", 0 },
    { at_cmd_t::CPSMRDP, at_cmd_type_t::EXTENDED, "CPSMRDP", 0 },
    { at_cmd_t::CPSMCFG, at_cmd_type_t::EXTENDED, "CPSMCFG", 0 },
    { at_cmd_t::CPSMCFGEXT, at_cmd_type_t::EXTENDED, "CPSMCFGEXT", 0 },
    { at_cmd_t::CPSMSTATUS, at_cmd_type_t::EXTENDED, "CPSMSTATUS", 0 },
    { at_cmd_t::CEDRXRDP, at_cmd_type_t::EXTENDED, "CEDRXRDP", 0 },
    { at_cmd_t::CRAI, at_cmd_type_t::EXTENDED, "CRAI", 0 },

    { at_cmd_t::CGATT, at_cmd_type_t::EXTENDED, "CGATT", 75 },
    { at_cmd_t::CGDCONT, at_cmd_type_t::EXTENDED, "CGDCONT", 0 },
    { at_cmd_t::CGACT, at_cmd_type_t::EXTENDED, "CGACT", 150 },
    { at_cmd_t::CGPADDR, at_cmd_type_t::EXTENDED, "CGPADDR", 0 },
    { at_cmd_t::CGREG, at_cmd_type_t::EXTENDED, "CGREG", 0 },
    { at_cmd_t::CGSMS, at_cmd_type_t::EXTENDED, "CGSMS", 0 },
    { at_cmd_t::CEREG, at_cmd_type_t::EXTENDED, "CEREG", 0 },

    { at_cmd_t::SAPBR, at_cmd_type_t::EXTENDED, "SAPBR", 85 },

    { at_cmd_t::CIPMUX, at_cmd_type_t::EXTENDED, "CIPMUX", 0 },
    { at_cmd_t::CIPSTART, at_cmd_type_t::EXTENDED, "CIPSTART", 160 },
    { at_cmd_t::CIPSEND, at_cmd_type_t::EXTENDED, "CIPSEND", 645 },
    { at_cmd_t::CIPQSEND, at_cmd_type_t::EXTENDED, "CIPQSEND", 0 },
    { at_cmd_t::CIPACK, at_cmd_type_t::EXTENDED, "CIPACK", 0 },
    { at_cmd_t::CIPCLOSE, at_cmd_type_t::EXTENDED, "CIPCLOSE", 0 },
    { at_cmd_t::CIPSHUT, at_cmd_type_t::EXTENDED, "CIPSHUT", 65 },
    { at_cmd_t::CLPORT, at_cmd_type_t::EXTENDED, "CLPORT", 0 },
    { at_cmd_t::CSTT, at_cmd_type_t::EXTENDED, "CSTT", 0 },
    { at_cmd_t::CIICR, at_cmd_type_t::EXTENDED, "CIICR", 85 },
    { at_cmd_t::CIFSR, at_cmd_type_t::EXTENDED, "CIFSR", 0 },
    { at_cmd_t::CIFSREX, at_cmd_type_t::EXTENDED, "CIFSREX", 0 },
    { at_cmd_t::CIPSTATUS, at_cmd_type_t::EXTENDED, "CIPSTATUS", 0 },
    { at_cmd_t::CDNSCFG, at_cmd_type_t::EXTENDED, "CDNSCFG", 0 },
    { at_cmd_t::CDNSGIP, at_cmd_type_t::EXTENDED, "CDNSGIP", 0 },
    { at_cmd_t::CIPHEAD, at_cmd_type_t::EXTENDED, "CIPHEAD", 0 },
    { at_cmd_t::CIPATS, at_cmd_type_t::EXTENDED, "CIPATS", 0 },
    { at_cmd_t::CIPSPRT, at_cmd_type_t::EXTENDED, "CIPSPRT", 0 },
    { at_cmd_t::CIPSERVER, at_cmd_type_t::EXTENDED, "CIPSERVER", 0 },
    { at_cmd_t::CIPCSGP, at_cmd_type_t::EXTENDED, "CIPCSGP", 0 },
    { at_cmd_t::CIPSRIP, at_cmd_type_t::EXTENDED, "CIPSRIP", 0 },
    { at_cmd_t::CIPDPDP, at_cmd_type_t::EXTENDED, "CIPDPDP", 0 },
    { at_cmd_t::CIPMODE, at_cmd_type_t::EXTENDED, "CIPMODE", 0 },
    { at_cmd_t::CIPCCFG, at_cmd_type_t::EXTENDED, "CIPCCFG", 0 },
    { at_cmd_t::CIPSHOWTP, at_cmd_type_t::EXTENDED, "CIPSHOWTP", 0 },
    { at_cmd_t::CIPUDPMODE, at_cmd_type_t::EXTENDED, "CIPUDPMODE", 0 },
    { at_cmd_t::CIPRXGET, at_cmd_type_t::EXTENDED, "CIPRXGET", 0 },
    { at_cmd_t::CIPRDTIMER, at_cmd_type_t::EXTENDED, "CIPRDTIMER", 0 },
    { at_cmd_t::CIPSGTXT, at_cmd_type_t::EXTENDED, "CIPSGTXT", 0 },
    { at_cmd_t::CIPSENDHEX, at_cmd_type_t::EXTENDED, "CIPSENDHEX", 0 },
    { at_cmd_t::CIPHEXS, at_cmd_type_t::EXTENDED, "CIPHEXS", 0 },
    { at_cmd_t::CIPTKA, at_cmd_type_t::EXTENDED, "CIPTKA", 0 },
    { at_cmd_t::CIPOPTION, at_cmd_type_t::EXTENDED, "CIPOPTION", 0 },

    { at_cmd_t::SHSSL, at_cmd_type_t::EXTENDED, "SHSSL", 0 },
    { at_cmd_t::SHCONF, at_cmd_type_t::EXTENDED, "SHCONF", 0 },
    { at_cmd_t::SHCONN, at_cmd_type_t::EXTENDED, "SHCONN", 0 },
    { at_cmd_t::SHBOD, at_cmd_type_t::EXTENDED, "SHBOD", 0 },
    { at_cmd_t::SHBODEXT, at_cmd_type_t::EXTENDED, "SHBODEXT", 0 },
    { at_cmd_t::SHAHEAD, at_cmd_type_t::EXTENDED, "SHAHEAD", 0 },
    { at_cmd_t::SHCHEAD, at_cmd_type_t::EXTENDED, "SHCHEAD", 0 },
    { at_cmd_t::SHPARA, at_cmd_type_t::EXTENDED, "SHPARA", 0 },
    { at_cmd_t::SHCPARA, at_cmd_type_t::EXTENDED, "SHCPARA", 0 },
    { at_cmd_t::SHSTATE, at_cmd_type_t::EXTENDED, "SHSTATE", 0 },
    { at_cmd_t::SHREQ, at_cmd_type_t::EXTENDED, "SHREQ", 0 },
    { at_cmd_t::SHREAD, at_cmd_type_t::EXTENDED, "SHREAD", 0 },
    { at_cmd_t::SHDISC, at_cmd_type_t::EXTENDED, "SHDISC", 0 },
    { at_cmd_t::HTTPTOFS, at_cmd_type_t::EXTENDED, "HTTPTOFS", 0 },
    { at_cmd_t::HTTPTOFSRL, at_cmd_type_t::EXTENDED, "HTTPTOFSRL", 0 },

    { at_cmd_t::CNTPCID, at_cmd_type_t::EXTENDED, "CNTPCID", 0 },
    { at_cmd_t::CNTP, at_cmd_type_t::EXTENDED, "CNTP", 0 },

    { at_cmd_t::CGNSPWR, at_cmd_type_t::EXTENDED, "CGNSPWR", 0 },
    { at_cmd_t::CGNSINF, at_cmd_type_t::EXTENDED, "CGNSINF", 0 },
    { at_cmd_t::CGNSURC, at_cmd_type_t::EXTENDED, "CGNSURC", 0 },
    { at_cmd_t::CGNSPORT, at_cmd_type_t::EXTENDED, "CGNSPORT", 0 },
    { at_cmd_t::CGNSCOLD, at_cmd_type_t::EXTENDED, "CGNSCOLD", 0 },
    { at_cmd_t::CGNSWARM, at_cmd_type_t::EXTENDED, "CGNSWARM", 0 },
    { at_cmd_t::CGNSHOT, at_cmd_type_t::EXTENDED, "CGNSHOT", 0 },
    { at_cmd_t::CGNSMOD, at_cmd_type_t::EXTENDED, "CGNSMOD", 0 },
    { at_cmd_t::CGNSCFG, at_cmd_type_t::EXTENDED, "CGNSCFG", 0 },
    { at_cmd_t::CGNSTST, at_cmd_type_t::EXTENDED, "CGNSTST", 0 },
    { at_cmd_t::CGNSXTRA, at_cmd_type_t::EXTENDED, "CGNSXTRA", 0 },
    { at_cmd_t::CGNSCPY, at_cmd_type_t::EXTENDED, "CGNSCPY", 0 },
    { at_cmd_t::CGNSRTMS, at_cmd_type_t::EXTENDED, "CGNSRTMS", 0 },
    { at_cmd_t::CGNSHOR, at_cmd_type_t::EXTENDED, "CGNSHOR", 0 },
    { at_cmd_t::CGNSUTIPR, at_cmd_type_t::EXTENDED, "CGNSUTIPR", 0 },
    { at_cmd_t::CGNSNMEA, at_cmd_type_t::EXTENDED, "CGNSNMEA", 0 },
    { at_cmd_t::CGTP, at_cmd_type_t::EXTENDED, "CGTP", 0 },
    { at_cmd_t::CGNSSUPLCFG, at_cmd_type_t::EXTENDED, "CGNSSUPLCFG", 0 },
    { at_cmd_t::CGNSSUPL, at_cmd_type_t::EXTENDED, "CGNSSUPL", 0 }
};

static const result_code_def_t RESULT_CODE_TABLE[] = 
{
    { result_code_t::Ok, "OK" },
    { result_code_t::Error, "ERROR" },
    { result_code_t::CME_Error, "+CME ERROR:" },
    { result_code_t::CMS_Error, "+CMS ERROR:" }
};

const at_cmd_def_t *get_command_def(at_cmd_t command)
{
    const int table_size = sizeof(AT_COMMANDS_TABLE) / sizeof(at_cmd_def_t);
    
    for (size_t i = 0; i < table_size; i++) {
        if (AT_COMMANDS_TABLE[i].command == command) {
            return &AT_COMMANDS_TABLE[i];
        }
    }
    
    return nullptr;
}

const result_code_def_t *get_result_code_def(result_code_t code)
{
    const int table_size = sizeof(RESULT_CODE_TABLE) / sizeof(result_code_def_t);
    
    for (size_t i = 0; i < table_size; i++) {
        if (RESULT_CODE_TABLE[i].code == code) {
            return &RESULT_CODE_TABLE[i];
        }
    }
    
    return nullptr;
}

} // namespace axomotor::lte_modem
