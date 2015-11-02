//=====================================================================
// pkt receive
//=====================================================================

void pkt::update_syncscope()
{
	int j, len = pkt_syncdisplen;

	for (int i = 0; i < len; i++) {
	j = pipeptr - i;
	if (j < 0) j += len;
	dsppipe[i] = pipe[j];
	}
	set_scope(dsppipe, len, false);
}

void pkt::clear_syncscope()
{
	set_scope(0, 0, false);
}

void pkt::rx(bool bit)
{
	static unsigned char c = 0, bcounter = 0;

	c >>= 1;
	++bcounter;

	if (bit == false) {
	c &= ~(1 << (pkt_nbits-1)); // bits are sent lsb first

	if (seq_ones == 6) { // flag byte found
		bcounter = 0;

		if (cbuf >= &x25.rxbuf[MINOCTETS]) { // flag at end of frame

		*cbuf = PKT_Flag; // == 0x7e

		if (debug::level == debug::DEBUG_LEVEL)
			fprintf(stderr,"7e\n");

		//   monitor Metric and Squelch Level
		if ( !progStatus.sqlonoff ||
			 metric >= progStatus.sldrSquelchValue ) { // lazy eval
			/*
			// check FCS at end of frame
			// if FCS is OK - packet is in rxbuffer,
			//		  put_rx_char() for each byte in rxbuffer
			*/
			if (x25.checkFCS((cbuf - 2)) == true) {
//			if (progdefaults.PKT_RXTimestamp) {
//				unsigned char ts[16], *tc = &ts[0];
//				time_t t = time(NULL);
//				struct tm stm;
//				(void)gmtime_r(&t, &stm);
//				snprintf((char *)ts, sizeof(ts),
//					 "[%02d:%02d:%02d] ",
//					 stm.tm_hour, stm.tm_min, stm.tm_sec);
//				while (*tc)  put_rx_char(*tc++);
//			}
			x25.mode = FTextBase::RECV;
			x25.do_put_rx_char((cbuf - 2));
			}
		}

		cbuf = &x25.rxbuf[0]; // reset after first end frame flag
		x25.clear_rxbuf();
		}
		else {
		// packet too short if cbuf < &rxbuf[MINOCTETS]

		// put only one beginning flag into buffer
		x25.rxbuf[0] = PKT_Flag;
		cbuf = &x25.rxbuf[1];

		if (debug::level == debug::DEBUG_LEVEL)
			fprintf(stderr,"7e ");
		}
	}
	else if (seq_ones == 5) { // need bit un-stuffing
		c <<= 1;  // shift c back to skip stuffed bit
		--bcounter;
	}

	seq_ones = 0;
	}
	else {
	c |= (1 << (pkt_nbits-1)); // bits are sent lsb first
	++seq_ones;

	//if (seq_ones > 6) { // something is wrong
	//}
	}

	if (bcounter == pkt_nbits) {
	bcounter = 0;
	if (cbuf < &x25.rxbuf[MAXOCTETS]) {
		*cbuf++ = c;

		if (debug::level == debug::DEBUG_LEVEL)
		fprintf(stderr,"%02x ",c);
	}
	else
		// else complain: cbuf is at MAXOCTETS
		LOG_WARN("Long input packet, %d octets!",(int)(cbuf - &x25.rxbuf[0]));
	}

	/*
	//   when flag found: keep collecting flags (0x7e) until !(0x7e) found
	//   (first non-0x7e begins DATA)

	//   keep collecting bits in DATA, 8 at-a-time, until 0x7e found
	//   while collecting bits, perform bit-unstuffing so we place in
	//   databuffer exactly 8 bits at-a-time
	//   (at times more than 8 bits in to 8 bits out)
	//   first trailing 0x7e ends DATA and begins STOP
	*/
}

char pkt::msg[20];

void pkt::Metric()
{
	double snr = 0;
	double pr;
	if (signal_power == 0 || (signal_power < 0.001 * noise_power)) {
		pr = 0;
		power_ratio = 1e-8;
	} else {
		pr = signal_power / noise_power;
		power_ratio = decayavg(power_ratio, pr, pr-power_ratio > 0 ? 2 : 8);
	}
	snr = (10*log10( power_ratio ) - 53) / 2;

	snprintf(msg, sizeof(msg), "s/n %3.0f dB", snr);
	put_Status2(msg);

	metric = CLAMP(snr + 30, 0.0, 100.0);
	display_metric(metric);

//if (signal_power > 1e-4)
//std::cout << signal_power << "," << noise_power << "," << snr << "\n";

}

void pkt::idle_signal_power(cmplx sample)
{
	// average of signal energy over PKT_IdleLen duration
	double pwr = norm(sample);
	idle_signal_pwr += pwr;
//	idle_signal_pwr -= idle_signal_buf[idle_signal_buf_ptr];
//	idle_signal_buf[idle_signal_buf_ptr] = pwr;

//	++idle_signal_buf_ptr %= pkt_idlelen; // circular buffer
}

#ifndef NDEBUG
double pkt::corr_power(cmplx z)
#else
inline double pkt::corr_power(cmplx z)
#endif
{
	// scaled magnitude
	double power = norm(z);
	power /= 2 * symbollen;

	return power;
}

void pkt::correlate(cmplx yl, cmplx yh, cmplx yt)
{
	lo_signal_energy += yl;
	lo_signal_energy -= lo_signal_buf[correlate_buf_ptr];
	lo_signal_buf[correlate_buf_ptr] = yl;
	lo_signal_corr = lo_signal_gain * corr_power(lo_signal_energy);

	hi_signal_energy += yh;
	hi_signal_energy -= hi_signal_buf[correlate_buf_ptr];
	hi_signal_buf[correlate_buf_ptr] = yh;
	hi_signal_corr = hi_signal_gain * corr_power(hi_signal_energy);

	mid_signal_energy += yt;
	mid_signal_energy -= mid_signal_buf[correlate_buf_ptr];
	mid_signal_buf[correlate_buf_ptr] = yt;

	++correlate_buf_ptr %= symbollen; // SymbolLen correlation window

	yt = mid_signal_energy;

	if (abs(lo_signal_corr - hi_signal_corr) < 0.1) {
		pipe[pipeptr] = 0.0;
		QI[QIptr] = cmplx(yt.imag(), yt.real());
	}
	else if (lo_signal_corr > hi_signal_corr) {
		pipe[pipeptr] = 0.5;
		QI[QIptr] = cmplx (0.125 * yt.imag(), yt.real());
	}
	else {
		pipe[pipeptr] = -0.5;
		QI[QIptr] = cmplx( yt.real(), 0.125 * yt.imag());
	}
	++pipeptr %= pkt_syncdisplen;

	yt_avg = decayavg(yt_avg, abs(yt), symbollen/2);

	if (yt_avg > 0)
		QI[QIptr] = QI[QIptr] / yt_avg;

	++QIptr %= MAX_ZLEN;
}

void pkt::detect_signal()
{
	double sig_corr = lo_signal_corr + hi_signal_corr;// - mid_signal_corr;

	signal_pwr += sig_corr;
	signal_pwr -= signal_buf[signal_buf_ptr];
	signal_buf[signal_buf_ptr] = sig_corr;

	++signal_buf_ptr %= pkt_detectlen; // circular buffer

	signal_power = signal_pwr / pkt_detectlen * signal_gain;
	noise_power = idle_signal_pwr / pkt_idlelen;

	// allow lazy eval
	if (signal_power > PKT_MinSignalPwr && signal_power > noise_power) {
	// lo+hi freq signals are present
		detect_drop = pkt_detectlen; // reset signal drop counter

		if (rxstate == PKT_RX_STATE_DROP) {
			rxstate = PKT_RX_STATE_START;

		// init STATE_START
			signal_gain = 1.0; // 5.0
			scounter = pkt_detectlen; //(was) 9 * symbollen;//11 instead of 9?
			lo_sync = 100.0;
			hi_sync = -100.0;
		}
	}
	else if (--detect_drop < 1 && rxstate == PKT_RX_STATE_DATA) { // lazy eval
	// give up on signals - gone,gone.
		rxstate = PKT_RX_STATE_STOP;
		scounter = 0; // (scounter is signed int)

		if (debug::level == debug::DEBUG_LEVEL)
			fprintf(stderr,"\n");
	}
}

void pkt::do_sync()
{
	double power_delta = lo_signal_corr - hi_signal_corr;

	lo_sync_ptr--; hi_sync_ptr--;

	if (lo_sync > power_delta && power_delta < 0) {
	lo_sync_ptr = 0;
	lo_sync = power_delta;
	}

	if (hi_sync < power_delta && power_delta > 0) {
	hi_sync_ptr = 0;
	hi_sync = power_delta;
	}

	if (--scounter < 1) {
	int offset;

	if (fabs(hi_sync) > fabs(lo_sync))
		offset = -hi_sync_ptr;
	else
		offset = -lo_sync_ptr;

	offset %= symbollen; // clamp offset to +/- one symbol

	mid_symbol = symbollen - offset;
	prev_symbol = pll_symbol = hi_signal_corr < lo_signal_corr;

	rxstate = PKT_RX_STATE_DATA;
	seq_ones = 0;	 // reset once on transition to STATE_DATA
	cbuf = &x25.rxbuf[0]; // and reset packet buffer ptr
	x25.clear_rxbuf();
	}
}

void pkt::rx_data()
{
	bool tbit = hi_signal_corr < lo_signal_corr; // detect current symbol

	mid_symbol -= 1;
	int imid_symbol = (int) floor(mid_symbol+0.5);

	// hard-decision symbol decoding.  done at mid-symbol (imid_symbol < 1).
	//
	// might do instead a soft-decision by cumulative voting
	// of more than one mid-symbol sample?  symbollen-2 samples max?
	//
	if (imid_symbol < 1) { // counting down to middle of next symbol
		bool bit = prev_symbol == tbit; // detect symbol change
		prev_symbol = pll_symbol = tbit;
		mid_symbol += symbollen; // advance to middle of next symbol
	// remember to check value of reverse here to determine symbol
		rx(reverse ? !bit : bit);
	}
	else if (tbit != pll_symbol) { // update PLL
		if (mid_symbol < ((double)symbollen/2))
			mid_symbol += PKT_PLLAdjVal;
		else
			mid_symbol -= PKT_PLLAdjVal;
		pll_symbol = tbit;
	}
}

cmplx pkt::mixer(double &phase, double f, cmplx in)
{
	cmplx z = cmplx( cos(phase), sin(phase)) * in;

	phase -= TWOPI * f / samplerate;
	if (phase < -TWOPI) phase += TWOPI;

	return z;
}

void pkt::hard_decode(double smpl)
{
	cmplx z, zlo, zhi, zmid, *zplo, *zphi, *zpmid;
	int num = 0;

	z = cmplx(smpl, smpl);

	zlo = mixer(lo_phase, frequency - pkt_shift/2.0, z);
	lo_filt->run(zlo, &zplo);

	zhi = mixer(hi_phase, frequency + pkt_shift/2.0, z);
	hi_filt->run(zhi, &zphi);

	zmid = mixer(mid_phase, frequency, z);
	num = mid_filt->run(zmid, &zpmid);

	for (int snbr = 0; snbr < num; snbr++) {
		switch (rxstate) {

		case PKT_RX_STATE_STOP:
		default:
			rxstate = PKT_RX_STATE_IDLE;

		case PKT_RX_STATE_IDLE:
			idle_signal_power(zpmid[snbr]);

			if (--scounter < 1) {
			rxstate = PKT_RX_STATE_DROP;

			scounter = pkt_idlelen;
			signal_gain = 1.0;

			signal_pwr = signal_buf_ptr = 0;

			for(int i = 0; i < pkt_detectlen; i++)
				signal_buf[i] = 0;

			lo_signal_energy = hi_signal_energy =
				mid_signal_energy = cmplx(0, 0);

			yt_avg = correlate_buf_ptr = 0;

			for(int i = 0; i < symbollen; i++)
				lo_signal_buf[i] = hi_signal_buf[i] =
				mid_signal_buf[i] = cmplx(0, 0);
			}
			break;

		case PKT_RX_STATE_DROP:
			idle_signal_power(zpmid[snbr]);
			correlate(zplo[snbr], zphi[snbr], zpmid[snbr]);
			detect_signal();
			break;

		case PKT_RX_STATE_START:
			correlate(zplo[snbr], zphi[snbr], zpmid[snbr]);
			do_sync();
			break;

		case PKT_RX_STATE_DATA:
			correlate(zplo[snbr], zphi[snbr], zpmid[snbr]);
			rx_data();
			detect_signal();
			break;
		}
	}
}

int pkt::rx_process(const double *buf, int len)
{

	if (select_val != progdefaults.PKT_BAUD_SELECT ||
		(pkt_ctrfreq != frequency && progdefaults.PKT_MANUAL_TUNE)) {
	// need to re-init modem
		restart();
	}

	Metric();

	while (len-- > 0) hard_decode(*buf++);

	if (metric < progStatus.sldrSquelchValue && progStatus.sqlonoff) {
		if (clear_zdata) {
			for (int i = 0; i < MAX_ZLEN; i++)
				QI[i] = cmplx(0,0);
			QIptr = 0;
			set_zdata(QI, MAX_ZLEN);
			clear_zdata = false;
			clear_syncscope();
		}
	} else {
		clear_zdata = true;
		update_syncscope();
		set_zdata(QI, MAX_ZLEN);
	}

	return 0;
}
