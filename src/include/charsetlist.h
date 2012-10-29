#ifndef CHARSETLIST_H
#define CHARSETLIST_H

struct charset_info
{
   const char *name;
   int tiniconv_id;
};

extern const struct charset_info charset_list[];
extern const unsigned int number_of_charsets;

#endif
