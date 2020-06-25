#ifndef FLARQ_H
#define FLARQ_H

#include <string>

#define SOHCOUNT 10

extern void arqBEACON();
extern void arqCLOSE();
extern void arqCONNECT();
extern void cbMenuAbout();
extern void cbMenuConfig();
extern void cbSetConfig();
extern void closeConfig();
extern void sendCancel();
extern void sendOK();
extern void cb_SortByDate();
extern void cb_SortByTo();
extern void cb_SortBySubj();
extern void abortTransfer();
extern void cbAbort();
extern void cbClearText();
extern void testDirectory(std::string);
extern void cbSendTalk();
extern void cbClearTalk();
extern void help_cb();
extern void open_nbems_file_folder();

extern void sendEmailFile();
extern void sendAsciiFile();
extern void sendImageFile();
extern void sendBinaryFile();
extern void changeMyCall(const char *);
extern void changeBeaconText(const char *);

extern std::string HomeDir;
extern std::string NBEMS_dir;
extern std::string ARQ_dir;
extern std::string ARQ_files_dir;
extern std::string ARQ_recv_dir;
extern std::string ARQ_send_dir;
extern std::string ARQ_mail_dir;
extern std::string ARQ_mail_in_dir;
extern std::string ARQ_mail_out_dir;
extern std::string ARQ_mail_sent_dir;
extern std::string WRAP_dir;
extern std::string WRAP_recv_dir;
extern std::string WRAP_send_dir;
extern std::string WRAP_auto_dir;
extern std::string ICS_dir;
extern std::string ICS_msg_dir;
extern std::string ICS_tmp_dir;

extern std::string	MyCall;
extern std::string	InFolder;
extern std::string	OutFolder;
extern std::string	MailInFolder;
extern std::string	MailOutFolder;
extern std::string	MailSentFolder;
extern std::string	Logfile;
extern std::string	beacontext;
extern int		exponent;
extern int		txdelay;
extern int		iretries;
extern long		iwaittime;
extern long		itimeout;
extern int		bcnInterval;
extern int		idtimer;
extern bool		restart_beacon;

extern void		cb_idtimer();

// used by xmlrpc interface
extern int			arqstate;
extern bool			sendingfile;
extern bool			rxTextReady;
extern bool			rxARQfile;
extern std::string	txtarqload;

extern void cb_SaveComposeMail();
extern void cb_CancelComposeMail();
extern void cb_UseTemplate();
extern void cb_OpenComposeMail();
extern void ComposeMail();

extern void send_xml_text(std::string, std::string);

#endif
