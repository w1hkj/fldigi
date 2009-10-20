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

extern void sendEmailFile();
extern void sendAsciiFile();
extern void sendImageFile();
extern void sendBinaryFile();
extern void changeMyCall(const char *);
extern void changeBeaconText(const char *);

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

extern void cb_SaveComposeMail();
extern void cb_CancelComposeMail();
extern void cb_UseTemplate();
extern void cb_OpenComposeMail();
extern void ComposeMail();

#endif
