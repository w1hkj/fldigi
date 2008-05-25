#ifndef SOUNDCONF_H
#define SOUNDCONF_H

enum { SND_IDX_UNKNOWN = -1, SND_IDX_OSS, SND_IDX_PORT,
       SND_IDX_PULSE, SND_IDX_NULL, SND_IDX_END
};

void sound_init(void);
void sound_close(void);
void sound_update(unsigned idx);

#endif // SOUNDCONF_H
