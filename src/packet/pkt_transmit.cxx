//=====================================================================
// pkt transmit
//=====================================================================

void pkt::send_symbol(bool bit)
{
	if (reverse)  bit = !bit;

	for (int i = 0; i < symbollen; i++) {
	if (bit)
		outbuf[i] = hi_tone->sample() * hi_txgain; // modem::outbuf[]
	else
		outbuf[i] = lo_tone->sample() * lo_txgain; // modem::outbuf[]
	}

	// adjust nco phase accumulator to pick up where the other nco left off
	if (bit)
	lo_tone->setphaseacc(hi_tone->getphaseacc());
	else
	hi_tone->setphaseacc(lo_tone->getphaseacc());

	ModulateXmtr(outbuf, symbollen); // ignore retv?
}

/*
void pkt::send_stop()
{
	double freq;
	bool invert = reverse;

	if (invert)
		freq = get_txfreq_woffset() - pkt_shift / 2.0;
	else
		freq = get_txfreq_woffset() + pkt_shift / 2.0;

	//	stoplen = 0;
	// stoplen ::= (int) ((1.0 * 12000 / 1200) + 0.5) = 10

	for (int i = 0; i < stoplen; i++) {
		outbuf[i] = nco(freq);
		if (invert)
			FSKbuf[i] = 0.0 * FSKnco();
		else
			FSKbuf[i] = FSKnco();
	}
	if (progdefaults.PseudoFSK)
		ModulateStereo(outbuf, FSKbuf, stoplen);
	else
		ModulateXmtr(outbuf, stoplen);
}
*/

void pkt::send_octet(unsigned char c)
{
	// if nostuff == 1 then { do _no_ bit stuffing }
	// else { keep track of no-transition (one bits) and stuff as needed }

	for (int i = 0; i < pkt_nbits; i++) {

		if ((c & 0x01) == 0) {
			currbit = !currbit; // transition on zero bits
			send_symbol(currbit);
			nr_ones = 0;
		} else {
			send_symbol(currbit);
			if (++nr_ones == 5 && !nostuff) {
				currbit = !currbit; // stuff zero bit
				send_symbol(currbit);
				nr_ones = 0;
			}
		}
		c >>= 1; // lsb to msb bit order
	}
}

string upcase(string s)
{
	// this function is also defined in arq::upcase
	// how to make a fldigi::upcase and use it here and in flarq?
	// maybe add it to misc/strutil.cxx ?

	for (size_t i = 0; i < s.length(); i++) s[i] = toupper(s[i]);
	return s;
}

string hexstr(int c)
{
	static char hx[5];
	snprintf(hx, sizeof(hx), "<%02X>", c & 0xFF);
	return hx;
}

void show_hex(int c)
{
	string hx = hexstr(c);
	for (int i = 0; i < 4; i++)
		put_echo_char(hx[i], FTextBase::CTRL);
}

void pkt::send_msg(unsigned char c)
{
	// form TX pane incoming chars into a UI AX.25 packet
	// in KISS mode, we do not create a packet header.
	if (!did_pkt_head) {

		string txstr;

		txstr.clear();
		int bitnbr = -1;

		string dest = progdefaults.pkt_dest_call;
		if (dest.empty())
			dest = inpCall->value();
		while (dest.length() < 6) dest += ' ';
		for (int i = 0; i < 6; i++)
			txstr += (toupper(dest[i]) << 1);
		bitnbr += 6;

		txstr += (0xE0 | (progdefaults.pkt_dest_id << 1));
		bitnbr++;

		string own_call;
		if (progdefaults.pkt_own_call.empty())
			own_call = progdefaults.myCall;
		else
			own_call = progdefaults.pkt_own_call;
		while (own_call.length() < 6) own_call += ' ';
		for (int i = 0; i < 6; i++)
			txstr += (toupper(own_call[i]) << 1);
		bitnbr += 6;

		txstr += (0x61 | (progdefaults.pkt_own_id << 1));
		bitnbr++;

		string digi1 = progdefaults.pkt_digi_1;
		string digi2 = progdefaults.pkt_digi_2;

		if ((!digi1.empty() || !digi2.empty()) && pkt_baud >= 1200) {
			txstr[bitnbr] &= 0xFE;  // clear the 1 bit
			if (!digi1.empty()) {
				if (!digi1.empty())
					while (digi1.length() < 6) digi1 += ' ';
				for (int i = 0; i < 6; i++)
					txstr += (toupper(digi1[i]) << 1);
				bitnbr += 6;
				txstr += (0x60 | (progdefaults.pkt_digi_1_id << 1));
				bitnbr++;
			}

			if (digi2.empty())
				txstr[bitnbr] |= 1; // set SSID bit 0
			else {
				if (!digi2.empty())
					while (digi2.length() < 6) digi2 += ' ';
				for (int i = 0; i < 6; i++)
					txstr += (toupper(digi2[i]) << 1);
				bitnbr += 6;
				txstr += (0x61 | (progdefaults.pkt_digi_2_id << 1));
				bitnbr++;
			}
		}
		txstr += 0x03; // add CTRL value for U_FRAME type UI
		txstr += 0xF0; // add PID of NONE
		bitnbr += 2;

		if (progdefaults.SHOW_PACKET_CODES) {
			for (size_t i = 0; i < txstr.length(); i++)
				show_hex(txstr[i]);
		}

		for (int i = 0; i <= bitnbr; i++)
			send_octet(txstr[i]);

		tx_char_count -= bitnbr;

		bitnbr = txstr.length();
		memset(txbuf, 0, MAXOCTETS + 4);
		memcpy(txbuf, txstr.c_str(), bitnbr);
		tx_cbuf = &txbuf[bitnbr];

		did_pkt_head = true;
	}

	send_octet(c);
	*tx_cbuf++ = c;

}


int pkt::tx_process()
{
	if (pretone > 0)
		while (pretone-- > 0) send_symbol(0); // Mark (low) tone

	if (preamble > 0) {
		nostuff = 1;  // turn off bit stuffing here
		while (preamble-- > 0) {
//			if (progdefaults.SHOW_PACKET_CODES) put_echo_char(PKT_Flag, FTextBase::CTRL);
			send_octet(PKT_Flag);
		}
		nostuff = 0;  // send_octet() is 8-bit clean
		tx_char_count--; // count only last flag char
	}

	static int c = 0;
	if (tx_char_count >  0) c = get_tx_char();

// TX buffer empty

	if (c == GET_TX_CHAR_ETX || stopflag || tx_char_count == 0) {
		if (!stopflag) {
// compute FCS and add it to the frame via *tx_cbuf

			unsigned int fcs = x25.computeFCS(&txbuf[0], tx_cbuf);
			unsigned char tc = (fcs & 0xFF00) >> 8; // msB

			*tx_cbuf++ = tc;
			if (progdefaults.SHOW_PACKET_CODES) show_hex(tc);
			send_octet(tc);

			tc = (unsigned char)(fcs & 0x00FF); // lsB

			*tx_cbuf++ = tc;
			if (progdefaults.SHOW_PACKET_CODES) {
				show_hex(tc);
			}
			send_octet(tc);

			nostuff = 1;  // turn off bit stuffing
			while (postamble-- > 0) {
				send_octet(PKT_Flag);
			}
			nostuff = 0;

			stopflag = true;
			return 0;
		}
		x25.decode(txbuf, tx_cbuf - txbuf - 2, true, true);
		stopflag = false;
		tx_char_count = MAXOCTETS - 3; // leave room for FCS and end-flag
		tx_cbuf = &txbuf[0];
		currbit = 0;
		did_pkt_head = false;

		cwid();
		return -1;
	}

	if (tx_char_count-- > 0) {
		send_msg((unsigned char)c);
		if (progdefaults.SHOW_PACKET_CODES)
			put_echo_char(c);
	}
	else {
		LOG_WARN("Long output packet, %d octets!",(int)(tx_cbuf - &txbuf[0]));
		return -1;
	}

	return 0;
}

