//======================================================================
//	script_parsing.h  (FLDIGI)
//
//  Author(s):
//
//	Robert Stiles, KK5VD, Copyright (C) 2014
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// =====================================================================

#ifndef __script_parsing__
#define __script_parsing__

#include <string>
#include <stdio.h>

#ifdef __WIN32__
#define PATH_SEPERATOR "\\"
#define PATH_CHAR_SEPERATOR '\\'
#include <direct.h>
#define get_current_dir _getcwd
#else
#define PATH_SEPERATOR "/"
#define PATH_CHAR_SEPERATOR '/'
#include <unistd.h>
#define get_current_dir getcwd
#endif

#define MAX_CMD_PARAMETERS 5
#define MAX_COMMAND_LENGTH 128
#define MAX_PARAMETER_LENGTH FILENAME_MAX
#define MAX_READLINE_LENGTH (FILENAME_MAX+FILENAME_MAX)
#define MAX_SUB_SCRIPTS 5

#define SCRIPT_FILE_TAG ((char *)"FLDIGI_CONFIG")
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

#define RESET_ALL     0x01
#define RESET_PARTIAL 0X02

#undef __LOCALIZED_SCRIPT_COMMANDS
#ifdef __LOCALIZED_SCRIPT_COMMANDS
#pragma message ( \
"\n" \
"************************************************************************\n" \
"* Defining __LOCALIZED_SCRIPT_COMMANDS breaks script compatibility. In *\n" \
"* doing so, FLDIGI development team does not assume responsibility for *\n" \
"* maintaining the code sections that are altered/effected as a result. *\n" \
"************************************************************************\n" )
#define F_LOC(a) _(a)
#else
#define F_LOC(a) a
#endif

#define CMD_OPERATOR                F_LOC("OPERATOR")
#define CMD_CALLSIGN                   F_LOC("CALLSIGN")
#define CMD_NAME                       F_LOC("NAME")
#define CMD_QTH                        F_LOC("QTH")
#define CMD_LOCATOR                    F_LOC("LOC")
#define CMD_ANTENNA                    F_LOC("ANT")

#define CMD_AUDIO_DEVICE            F_LOC("AUDIO DEVICE")
#define CMD_OSS_AUDIO                  F_LOC("OSS")
#define CMD_OSS_AUDIO_DEV_PATH         F_LOC("OSS DEV")
#define CMD_PORT_AUDIO                 F_LOC("PA")
#define CMD_PORTA_CAP                  F_LOC("PA CAPTURE")
#define CMD_PORTA_PLAY                 F_LOC("PA PLAYBACK")
#define CMD_PULSEA                     F_LOC("PUA")
#define CMD_PULSEA_SERVER              F_LOC("PUA SERVER")

#define CMD_AUDIO_SETTINGS          F_LOC("AUDIO SETTINGS")
#define CMD_CAPTURE_SAMPLE_RATE        F_LOC("CAPTURE")
#define CMD_PLAYBACK_SAMPLE_RATE       F_LOC("PLAYBACK")
#define CMD_AUDIO_CONVERTER            F_LOC("CONVERTER")
#define CMD_RX_PPM                     F_LOC("RX PPM")
#define CMD_TX_PPM                     F_LOC("TX PPM")
#define CMD_TX_OFFSET                  F_LOC("TX OFFSET")

#define CMD_AUDIO_RT_CHANNEL        F_LOC("AUDIO RT CHANNEL")
#define CMD_MONO_AUDIO                 F_LOC("MONO AUDIO")
#define CMD_AUDIO_L_R                  F_LOC("MODEM LR")
#define CMD_AUDIO_REV_L_R              F_LOC("REV LR")
#define CMD_PTT_RIGHT_CHAN             F_LOC("PTT RT CHAN")
#define CMD_CW_QSK_RT_CHAN             F_LOC("QSK RT CHAN")
#define CMD_PSEUDO_FSK_RT_CHAN         F_LOC("FSK RT CHAN")

#define CMD_AUDIO_WAVE              F_LOC("AUDIO WAVE")
#define CMD_WAVE_SR                    F_LOC("SRATE")

#define CMD_HRDWR_PTT               F_LOC("RIG HRDWR PTT")
#define CMD_HPPT_PTT_RT                F_LOC("PTT RT CHAN")
#define CMD_HPTT_SP2                   F_LOC("SERIAL PORT")
#define CMD_HPTT_SP2_PATH              F_LOC("DEVICE")
#define CMD_HPTT_SP2_RTS               F_LOC("RTS")
#define CMD_HPTT_SP2_DTR               F_LOC("DTR")
#define CMD_HPTT_SP2_RTS_V             F_LOC("RTSV")
#define CMD_HPTT_SP2_DTR_V             F_LOC("DTRV")
#define CMD_HPTT_SP2_START_DELAY       F_LOC("START PTT DELAY")
#define CMD_HPTT_SP2_END_DELAY         F_LOC("END PTT DELAY")
#define CMD_HPTT_UHROUTER              F_LOC("UHROUTER")
#define CMD_HPTT_PARALLEL              F_LOC("PARALLEL")
#define CMD_HPTT_SP2_INITIALIZE        F_LOC("INIT")

#define CMD_RIGCAT                  F_LOC("RIGCAT")
#define CMD_RIGCAT_STATE                F_LOC("STATE")
#define CMD_RIGCAT_DESC_FILE            F_LOC("DESC FILE")
#define CMD_RIGCAT_DEV_PATH             F_LOC("DEV PATH")
#define CMD_RIGCAT_RETRIES              F_LOC("RETRIES")
#define CMD_RIGCAT_RETRY_INTERVAL       F_LOC("RETRY INT")
#define CMD_RIGCAT_WRITE_DELAY          F_LOC("WDELAY")
#define CMD_RIGCAT_INTIAL_DELAY         F_LOC("IDELAY")
#define CMD_RIGCAT_BAUD_RATE            F_LOC("BRATE")
#define CMD_RIGCAT_STOP_BITS            F_LOC("SBITS")
#define CMD_RIGCAT_ECHO                 F_LOC("ECHO")
#define CMD_RIGCAT_TOGGLE_RTS_PTT       F_LOC("TOGGLE RTS PTT")
#define CMD_RIGCAT_TOGGLE_DTR_PTT       F_LOC("TOGGLE DTR PTT")
#define CMD_RIGCAT_RESTORE              F_LOC("RESTORE")
#define CMD_RIGCAT_PTT_COMMAND          F_LOC("PTT COMMAND")
#define CMD_RIGCAT_RTS_12V              F_LOC("RTS 12V")
#define CMD_RIGCAT_DTR_12V              F_LOC("DTR 12V")
#define CMD_RIGCAT_HRDWR_FLOW           F_LOC("HRDWR FC")
#define CMD_RIGCAT_VSP                  F_LOC("VSP")
#define CMD_RIGCAT_INITIALIZE           F_LOC("INIT")

#define CMD_HAMLIB                  F_LOC("HAMLIB")
#define CMD_HAMLIB_STATE               F_LOC("STATE")
#define CMD_HAMLIB_RIG                 F_LOC("RIG")
#define CMD_HAMLIB_DEV_PATH            F_LOC("DEV PATH")
#define CMD_HAMLIB_RETRIES             F_LOC("RETRIES")
#define CMD_HAMLIB_RETRY_INTERVAL      F_LOC("RETRY INT")
#define CMD_HAMLIB_WRITE_DELAY         F_LOC("WDELAY")
#define CMD_HAMLIB_POST_WRITE_DELAY    F_LOC("PWDELAY")
#define CMD_HAMLIB_BAUD_RATE           F_LOC("BRATE")
#define CMD_HAMLIB_STOP_BITS           F_LOC("SBITS")
#define CMD_HAMLIB_POLL_RATE           F_LOC("POLL_RATE")
#define CMD_HAMLIB_SIDE_BAND           F_LOC("SBAND")
#define CMD_HAMLIB_PTT_COMMAND         F_LOC("PTT COMMAND")
#define CMD_HAMLIB_DTR_12V             F_LOC("DTR 12V")
#define CMD_HAMLIB_RTS_12V             F_LOC("RTS 12V")
#define CMD_HAMLIB_HRDWR_FLOW          F_LOC("HRDWR FC")
#define CMD_HAMLIB_SFTWR_FLOW          F_LOC("SFTWR FC")
#define CMD_HAMLIB_ADV_CONFIG          F_LOC("ADV CONF")
#define CMD_HAMLIB_INITIALIZE          F_LOC("INIT")

#define CMD_RC_XMLRPC               F_LOC("XMLRPC RC")
#define CMD_RC_XMLRPC_STATE            F_LOC("STATE")
#define CMD_RC_XMLRPC_BW_DELAY         F_LOC("BWDELAY")
#define CMD_RC_XMLRPC_INITIALIZE       F_LOC("INIT")

#define CMD_FLDIGI                  F_LOC("FLDIGI")
#define CMD_FLDIGI_FREQ                F_LOC("FREQ")
#define CMD_FLDIGI_MODE                F_LOC("MODE")
#define CMD_FLDIGI_WFHZ                F_LOC("WFHZ")
#define CMD_FLDIGI_RXID                F_LOC("RXID")
#define CMD_FLDIGI_TXID                F_LOC("TXID")
#define CMD_FLDIGI_SPOT                F_LOC("SPOT")
#define CMD_FLDIGI_REV                 F_LOC("REV")
#define CMD_FLDIGI_LOCK                F_LOC("LOCK")
#define CMD_FLDIGI_AFC                 F_LOC("AFC")
#define CMD_FLDIGI_SQL                 F_LOC("SQL")
#define CMD_FLDIGI_KPSQL               F_LOC("KPSQL")
#define CMD_FLDIGI_KPSM                F_LOC("KPSM")
#define CMD_FLDIGI_MODEM               F_LOC("MODEM")

#define CMD_IO                      F_LOC("IO")
#define CMD_IO_LOCK                   F_LOC("LOCK")
#define CMD_IO_ACT_PORT               F_LOC("PORT")
#define CMD_IO_AX25_DECODE            F_LOC("AX25D")
#define CMD_IO_CSMA                   F_LOC("CSMA")
#define CMD_IO_KISS                   F_LOC("KISS")
#define CMD_IO_KISS_IPA                  F_LOC("IPA")
#define CMD_IO_KISS_IOPN                 F_LOC("IOPN")
#define CMD_IO_KISS_OPN                  F_LOC("OPN")
#define CMD_IO_KISS_DP                   F_LOC("DP")
#define CMD_IO_KISS_BUSY                 F_LOC("BUSY")
#define CMD_IO_KISS_CONT                 F_LOC("CONT")
#define CMD_IO_KISS_ATTEN                F_LOC("ATTEN")
#define CMD_IO_ARQ                    F_LOC("ARQ")
#define CMD_IO_ARQ_IPA                   F_LOC("IPA")
#define CMD_IO_ARQ_IOPN                  F_LOC("IOPN")
#define CMD_IO_XMLRPC                 F_LOC("XMLRPC")
#define CMD_IO_XMLRPC_IPA                F_LOC("IPA")
#define CMD_IO_XMLRPC_IOPN               F_LOC("IOPN")

#define CMD_NBEMS                  F_LOC("MISC NBEMS")
#define CMD_NBEMS_STATE               F_LOC("STATE")
#define CMD_NBEMS_MSG                 F_LOC("OPEN MSG")
#define CMD_NBEMS_FLMSG               F_LOC("OPEN FLMSG")
#define CMD_NBEMS_FLMSG_PATH          F_LOC("PATH")
#define CMD_NBEMS_BRWSR               F_LOC("OPEN BRWSR")
#define CMD_NBEMS_TIMEOUT             F_LOC("TIMEOUT")

#define CMD_ID                      F_LOC("ID")
#define CMD_ID_RSID                   F_LOC("RSID")
#define CMD_ID_RSID_NOTIFY               F_LOC("NOTIFY")
#define CMD_ID_RSID_SRCH_BP              F_LOC("SRCH BP")
#define CMD_ID_RSID_MARK_PREV            F_LOC("MARK PREV")
#define CMD_ID_RSID_DETECTOR             F_LOC("DETECTOR")
#define CMD_ID_RSID_ALRT_DIALOG          F_LOC("ALRT DIALOG")
#define CMD_ID_RSID_TX_FREQ_LOCK         F_LOC("TX FREQ LOCK")
#define CMD_ID_RSID_FREQ_CHANGE          F_LOC("FREQ CHANGE")
#define CMD_ID_RSID_ALLOW_ERRORS         F_LOC("ALLOW ERRORS")
#define CMD_ID_RSID_SQL_OPEN             F_LOC("SQL OPEN")
#define CMD_ID_RSID_PRETONE              F_LOC("PRETONE")
#define CMD_ID_RSID_END_XMT_ID           F_LOC("END XMT ID")
#define CMD_ID_VIDEO                  F_LOC("VIDEO")
#define CMD_ID_VIDEO_TX_ID_MODE          F_LOC("ID MODE")
#define CMD_ID_VIDEO_TX_VIDEO_TXT        F_LOC("VIDEO TXT")
#define CMD_ID_VIDEO_TX_TXT_INP          F_LOC("TEXT INPUT")
#define CMD_ID_VIDEO_SMALL_FONT          F_LOC("SMALL FONT")
#define CMD_ID_VIDEO_500_HZ              F_LOC("500HZ")
#define CMD_ID_VIDEO_WIDTH_LIMIT         F_LOC("WIDTH LIMIT")
#define CMD_ID_VIDEO_CHAR_ROW            F_LOC("CHAR ROW")
#define CMD_ID_CW                     F_LOC("CW")
#define CMD_ID_CW_TX_CALLSIGN            F_LOC("TX CALL")
#define CMD_ID_CW_SPEED                  F_LOC("SPEED")


#define CMD_LOAD_MACRO              F_LOC("MACRO")
#define CMD_END_CMD                 F_LOC("END")

// Parameters localized - no restrictions.

#define PARM_ENABLE                 _("ENABLE")
#define PARM_YES                    _("YES")
#define PARM_DISABLE                _("DISABLE")
#define PARM_NO                     _("NO")
#define PARM_KISS                   _("KISS")
#define PARM_ARQ                    _("ARQ")

#define SCRIPT_COMMAND           0x0001
#define SCRIPT_DOT_NOTATION_ONLY 0x0002
#define SCRIPT_STRUCTURED_ONLY   0x0004

#define RECURSIVE_LIMIT 3
#define MAX_DELETE_LIST_COUNT 100

//! @enum script_codes
//! Error codes.

//! @typedef SCRIPT_CODES
//! @see script_codes

typedef enum script_codes {
	script_recursive_limit_reached = -100, //!< Reached command recursive limit.
	script_command_not_found = -100,       //!< Script command not found.
	script_unexpected_eof,                 //!< Unexspected end of file reached.
	script_file_read_error,                //!< File read error
	script_errors_reported,                //!< Script Errors found during parsing of file.
	script_file_not_found,                 //!< Script file not found.
	script_path_not_found,                 //!< Script directory path not found.
	script_device_path_not_found,          //!< Script device path not found.
	script_non_script_file,                //!< Opened file not a Script file.
	script_max_sub_script_reached,         //!< Maximum open subscripts reached.
	script_subscript_exec_fail,            //!< Subscript execution fail (internal).
	script_incorrectly_assigned_value,     //!< Incorrect parameter type.
	script_invalid_parameter,              //!< Parameter is not valid.
	script_parameter_error,                //!< Script parameter invalid.
	script_function_parameter_error,       //!< Function parameter error (internal, non script error).
	script_mismatched_quotes,              //!< Opening or closing quotes missing prior to reaching end if line.
	script_command_seperator_missing,      //!< Command missing ':'
	script_args_eol,                       //!< Reached end of args list sooner then expected
	script_param_check_eol,                //!< Reached end of parameter check list sooner then expected
	script_paramter_exceeds_length,        //!< Data length exceeds expectations
	script_memory_allocation_error,        //!< Memory Allocation Error (Non-Script internal Error).
	script_general_error = -1,             //!< General purpose error (undefined erros).
	script_no_errors,                      //!< No Errors
	script_char_match_not_found,           //!< Search char not found (Warning)
	script_end_of_line_reached,            //!< End of line reached (Warning)
	script_end_of_list_reached,            //!< End of list reached (Info)
	script_list,                           //!< List command (Info)
	script_command_handled,                //!< Command was procedded and handled (Info)
} SCRIPT_CODES;


//! @enum paramter_types
//! Parameter type flags. Used to validate the informarion.

//! @typedef PARAM_TYPES
//! @see paramter_types

typedef enum paramter_types {
	p_null = 0,
	p_void,
	p_bool,
	p_char,
	p_int,
	p_unsigned_int,
	p_long,
	p_unsigned_long,
	p_float,
	p_double,
	p_string,
	p_path,
	p_filename,
	p_dev_path,
	p_dev_name,
	p_list,
	p_list_end
} PARAM_TYPES;

class ScriptParsing;

//! @struct script_cmds
//! Vector table of script command string and executing function member.

//! @typedef SCRIPT_COMMANDS
//! @see script_cmds
typedef struct script_cmds {
	char command[MAX_COMMAND_LENGTH];                      //!< Command matching string.
	int  flags;                                            //!< General purpose flags
	size_t command_length;                                 //!< Number of chacters in the command string.
	int  argc;                                             //!< Number of required prarmeters for the command
	char *args[MAX_CMD_PARAMETERS+1];                      //!< String vector table of parameters
	enum paramter_types param_check[MAX_CMD_PARAMETERS+1]; //!< Flags to determine type of parameter.
	const char **valid_values;                             //!< A list of valid paramters.
	int valid_value_count;                                 //!< The number of valid paramters.
	SCRIPT_CODES (ScriptParsing::*func)(struct script_cmds *); //!< The function (member) to execute on positive match
	int (*cb)(ScriptParsing *sp, struct script_cmds *sd);      //!< Call back function for script command
	struct script_cmds * sub_commands;                     //!< List of sub commands
	size_t sub_command_count;                              //!< Number of table entires in the sub table.
} SCRIPT_COMMANDS;

//! @class script_parsing_class
class ScriptParsing {

public:

private:
	pthread_mutex_t ac;            //<! Thread safe data access
	int recursive_count;
	bool _script_error_detected;
	bool _restart_flag;

	FILE *fd;
	int line_number;
	// Storage for each parameter

	// Special processing data storage.
	std::string _macro_command;

	// Internal Class Variables.

	std::string _path;
	int _file_type;

	SCRIPT_COMMANDS *_script_commands;    //!< Table of commands and vector functions @see script_cmds
	size_t _script_command_count;      	  //!< Number of assigned positions in the table.

	ScriptParsing *_parent;       //!< Calling ScriptParsing pointer. Primarly used for the reset command on the local 'this' pointer.

	// Keep track of what needs to be deleted on exit (deallocation).
	SCRIPT_COMMANDS * memory_delete_list[MAX_DELETE_LIST_COUNT];
	int delete_list_count; // Number of entries in the table.


	char line_buffer[MAX_READLINE_LENGTH + 1]; //!< Line buffer for script read operations.
	char error_cmd_buffer[MAX_COMMAND_LENGTH];
	char error_string[MAX_COMMAND_LENGTH];
	char error_buffer[MAX_COMMAND_LENGTH + MAX_COMMAND_LENGTH + 1];

	SCRIPT_COMMANDS *top_cache;

public:

	// Aceess methods for special processing data
	std::string macro_command(void) { return _macro_command; }
	void macro_command(std::string str) { _macro_command.assign(str); }

	bool restart_flag(void) { return _restart_flag; }
	void restart_flag(bool value) {  _restart_flag = value; }

	bool script_errors(void) { return _script_error_detected; }
	// Parsing/varification routine for each of the commands.

	SCRIPT_CODES check_parameters(struct script_cmds *);
	SCRIPT_CODES check_filename(char *filename);
	SCRIPT_CODES check_path(const char *);
	SCRIPT_CODES check_numbers(char *, paramter_types p);
	SCRIPT_CODES check_dev_path(const char *path);
	SCRIPT_CODES check_bool(const char *value, bool &flag);

	void to_uppercase(char *str, int limit);
	void to_uppercase(std::string &str);
	void trim(char *buffer, size_t size);
	std::string &trim(std::string &s);

	// Script handling code.

	inline SCRIPT_CODES test_on_off_state(bool &state, char *string, char *true_state);

	SCRIPT_COMMANDS * script_commands(void) { return _script_commands;	}
	void script_commands(SCRIPT_COMMANDS *ptr) { if(ptr) _script_commands = ptr; }

	size_t script_command_count(void) { return _script_command_count;	}
	void script_command_count(size_t value) { _script_command_count = value; }


	// Internal script fuctions

	SCRIPT_CODES parse_commands(char *file_name_path); //!< The calling function to parse a script file.
	SCRIPT_CODES read_file(FILE *fd, SCRIPT_COMMANDS *cur_list, size_t limit);

	ScriptParsing *parent(void) { return _parent; }
	void parent(ScriptParsing *value) { _parent = value; }


	int assign_callback(const char *scriptCommand, int (*cb)(ScriptParsing *sp, SCRIPT_COMMANDS *sc));
	int assign_valid_parameters(const char *command, const char **array, const int array_count);

	void defaults(bool all);
	ScriptParsing();
	~ScriptParsing();

private:

	// Special processing member.
	SCRIPT_CODES sc_macros(struct script_cmds *cmd);

	// Internal class routines

	void add_to_delete_list(SCRIPT_COMMANDS *);

	SCRIPT_CODES sc_dummy(struct script_cmds *);
	SCRIPT_CODES remove_crlf_comments(char *data, char *limit, size_t &count);
	SCRIPT_CODES parse_single_command(FILE *fd, SCRIPT_COMMANDS *cur_list, size_t limit, char *data, size_t buffer_size);
	SCRIPT_CODES parse_hierarchy_command(char *data, size_t buffer_size);

	int copy_tables(struct script_cmds * subcmds, size_t count);

	std::string path(void) { return _path; }
	int file_type(void) { return _file_type; }
	void file_type(int value) { _file_type = value; }

	SCRIPT_COMMANDS * search_command(SCRIPT_COMMANDS * table, size_t limit, const char *command);
	SCRIPT_CODES copy_string_uppercase(char *buffer, char *cPtr, char *ePtr, size_t limit);
	SCRIPT_CODES parse_parameters(char *s, char *d, SCRIPT_COMMANDS *matching_command);

	int call_callback(SCRIPT_COMMANDS *cb_data);
	int check_parameters_from_list(SCRIPT_COMMANDS *sc);
	int CopyScriptParsingEnv(ScriptParsing *src);
	void clear_script_parameters(bool all);

	SCRIPT_CODES assign_member_func(char *cmd, SCRIPT_CODES (ScriptParsing::*func)(struct script_cmds *));

	void initialize_function_members(void);

	int str_cnt(char * str, int count_limit);

	char * script_error_string(SCRIPT_CODES error_no, int ln, char *cmd);
	char * skip_alpha_numbers(char * data, char *limit, SCRIPT_CODES &error);
	char * skip_characters(char * data, char *limit, SCRIPT_CODES &error);
	char * skip_numbers(char * data, char *limit, SCRIPT_CODES &error);
	char * skip_spaces(char * data, char *limit, SCRIPT_CODES &error);
	char * skip_to_character(char c, char * data, char *limit, SCRIPT_CODES &error);
	char * skip_white_spaces(char * data, char *limit, SCRIPT_CODES &error);
};

int callback_dummy(ScriptParsing *sp, struct script_cmds *sc);

#endif /* defined(__script_parsing__) */
