#ifndef SOUNDCONF_H
#define SOUNDCONF_H

enum { SND_IDX_UNKNOWN = -1, SND_IDX_OSS, SND_IDX_PORT,
       SND_IDX_PULSE, SND_IDX_NULL, SND_IDX_END
};

enum {
	FLDIGI_SRC_BEST,
	FLDIGI_SRC_MEDIUM,
	FLDIGI_SRC_FASTEST,
#if !(defined(__ppc__) || defined(__powerpc__) || defined(__PPC__))
	FLDIGI_SRC_LINEAR,
#endif
	FLDIGI_NUM_SRC
};
extern int sample_rate_converters[FLDIGI_NUM_SRC];

void sound_init(void);
void sound_close(void);
void sound_update(unsigned idx);

#endif // SOUNDCONF_H
