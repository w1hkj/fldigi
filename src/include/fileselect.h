#ifndef FILESELECT_H
#define FILESELECT_H

const char* file_select(const char* title, const char* filter, const char* def = 0, int* fsel = 0);
const char* file_saveas(const char* title, const char* filter, const char* def = 0, int* fsel = 0);
const char*  dir_select(const char* title, const char* filter, const char* def = 0);

#endif // FILESELECT_H
