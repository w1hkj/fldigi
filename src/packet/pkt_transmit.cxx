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

void pkt::tx_octet(unsigned char c)
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
		REQ(put_rx_char_flmain, hx[i], FTextBase::CTRL);
}

void pkt::send_msg(unsigned char c)
{
	if(kiss_data_flag) return;

	// form TX pane incoming chars into a UI AX.25 packet
	// in KISS mode, we do not create a packet header.
	if (!did_pkt_head) {


		did_pkt_head = true;
	}

	write_terminal_data(c, false);
	*tx_cbuf++ = c;

}

void pkt::tx_packet_data(void)
{
	if(frame_count < 1) return;

	int size = 0;

	read_packet_data(tmp_tx_data, size, PKT_MaxPacketSize, src_flag);

	if(progdefaults.ax25_decode_enabled) {
		int count = size - 2;
		x25.decode((unsigned char *) tmp_tx_data, count, true, true);
	}

	set_prepostamble();

	if (pretone > 0)
		while (pretone-- > 0) send_symbol(0); // Mark (low) tone

	if (preamble > 0) {
		nostuff = 1;  // turn off bit stuffing here
		while (preamble-- > 0) {
			//			if (progdefaults.SHOW_PACKET_CODES) put_echo_char(PKT_Flag, FTextBase::CTRL);
			tx_octet(PKT_Flag);
		}
		nostuff = 0;  // tx_octet() is 8-bit clean
	}

	for(int i = 0; i < size; i++)
		tx_octet(tmp_tx_data[i]);

	nostuff = 1;  // turn off bit stuffing
	while (postamble-- > 0) {
		tx_octet(PKT_Flag);
	}
	nostuff = 0;

	old_src_flag = src_flag;
}

int pkt::tx_process()
{
	static int c = 0;

	c = get_tx_char();

	if(c == GET_TX_CHAR_NODATA) return 0;

	if(!kiss_data_flag) {
		if(c == GET_TX_CHAR_ETX)
			write_terminal_data(c, true);
		else
			write_terminal_data(c, false);
	}

	kiss_data_flag = 0;

	int loop_count = 0;
	while(frame_count > 0) {
		if(loop_count++ > PKT_MaxTXFrames) {
			set_pretone();
			set_prepostamble();
			loop_count = 0;
		}
		tx_packet_data();
	}

	if(c == GET_TX_CHAR_ETX)
		return -1;

	return 0;
}

void pkt::clear_frame_data(void)
{
	guard_lock data_clear_lock(&packet_data_mutex);
	guard_lock data_clear2_lock(&packet_c_data_mutex);

	kiss_data_flag = false;
	frame_head = 0;
	frame_tail = 0;
	frame_count = 0;
	memset(tmp_data, 0, sizeof(tmp_data));
	memset(tmp_tx_data, 0, sizeof(tmp_tx_data));
	tmp_data_count = 0;
	src_flag = 0;
	old_src_flag = 0;
}

void pkt::set_kiss_data_flag(bool value)
{
	kiss_data_flag = value;
}

bool pkt::write_format_packet_data(unsigned char *data, int count, int src)
{
	std::string txstr;

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

	txstr.append((const char *) data, (size_t) count);

	unsigned int fcs = x25.computeFCS((unsigned char *)txstr.c_str(), (unsigned char *)&txstr.c_str()[txstr.size()]);
	unsigned char tc = (fcs & 0xFF00) >> 8; // msB

	txstr += tc;

	if (progdefaults.SHOW_PACKET_CODES)
		show_hex(tc);

	tc = (unsigned char)(fcs & 0x00FF); // lsB

	txstr += tc;

	if (progdefaults.SHOW_PACKET_CODES)
		show_hex(tc);

	write_packet_data((unsigned char *) txstr.c_str(), txstr.size(), src);
}


// This routine should only be used for recording terminal data. Otherwise, data
// interleaving will be possible.
bool pkt::write_terminal_data(unsigned char data, int completion_flag)
{
	guard_lock data_write_lock(&packet_c_data_mutex);

	if((tmp_data_count >= MAXPAYLOADBYTES) || completion_flag) {
		write_format_packet_data(tmp_data, tmp_data_count, FROM_TERMINAL);
		tmp_data_count = 0;
		memset(tmp_data, 0, sizeof(tmp_data));
		if(completion_flag) return true;
	}

	tmp_data[tmp_data_count] = data;
	tmp_data_count++;

	return true;
}

// Fill in the number of frames it takes to handle 'count' amount of data.
// The number of storage frames are limited to 'PKT_MaxPackets'.
// Return false if the frame buffer is full or error was detected.
// Non Class definition
bool pkt::write_packet_data(unsigned char *data, int count, int src)
{
	guard_lock data_read_lock(&packet_data_mutex);

	if(!data || count < 1)  return false;

	unsigned char *dPtr = (unsigned char *)0;
	int segment_count = 0;
	int copy_size = 0;

	while(count > 0) {
		if(frame_count >= PKT_MaxPackets)
			return false;

		if(frame_head >= PKT_MaxPackets)
			frame_head = 0;

		if(count > PKT_MaxPacketSize) {
			segment_count = PKT_MaxPacketSize;
		} else {
			segment_count = count;
		}

		dPtr = pkt_data[frame_head].data;
		pkt_data[frame_head].count = segment_count;
		pkt_data[frame_head].from = src;

		memcpy(dPtr, data, segment_count);

		frame_head++;
		frame_count++;

		count -= segment_count;
	}

	return true;
}

bool pkt::read_packet_data(unsigned char *data, int &size, int limit, int &src)
{
	guard_lock data_read_lock(&packet_data_mutex);

	if(!data)  return false;

	if(frame_tail == frame_head)
		frame_count = 0;
	
	if(frame_count < 1) return false;
	
	if(frame_tail >= PKT_MaxPackets)
		frame_tail = 0;
	
	unsigned char *dPtr = pkt_data[frame_tail].data;
	
	size  = pkt_data[frame_tail].count;
	src   = pkt_data[frame_tail].from;
	
	if(size <= limit) {
		memcpy(data, dPtr, size);
	} else {
		// Don't transmit truncated frames.
		size = 0;
	}
	
	pkt_data[frame_tail].count = 0;
	
	frame_tail++;
	if(frame_count > 0) frame_count--;
	
	return true;
}


