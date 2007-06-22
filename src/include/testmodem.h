#include "trx.h"

class psktest : public trx {
public:
	psktest(char *dev) : trx(dev) {};
	~psktest(){};	
protected:
	virtual void txinit (){};
	virtual	void rxinit (){};
	virtual int txprocess (){};
	virtual int rxprocess (unsigned char *, int len){};
};
