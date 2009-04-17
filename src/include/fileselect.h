#ifndef FILESELECT_H
#define FILESELECT_H

#ifdef __WOE32__
#  define FSEL_THREAD 1
#endif

class Fl_Native_File_Chooser;

class FSEL
{
public:
	static void create(void);
	static void destroy(void);
	static const char* select(const char* title, const char* filter, const char* def = 0, int* fsel = 0);
	static const char* saveas(const char* title, const char* filter, const char* def = 0, int* fsel = 0);
	static const char* dir_select(const char* title, const char* filter, const char* def = 0);
	~FSEL();
private:
	FSEL();
	FSEL(const FSEL&);
	FSEL& operator=(const FSEL&);

	const char* get_file(void);
#if FSEL_THREAD
	static void* thread_func(void* arg);
#endif
private:
	static FSEL* inst;
	Fl_Native_File_Chooser* chooser;
	int result;
};

#endif // FILESELECT_H
