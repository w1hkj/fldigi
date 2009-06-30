#ifndef STRUTIL_H_
#define STRUTIL_H_

#include <ostream>
#include <iterator>
#include <sstream>
#include <algorithm>
#include <string>

namespace join_ {
	template <typename T> struct empty {
		bool operator()(const T& v) const { return false; };
	};
	template <> struct empty<const char*> {
		bool operator()(const char* v) const { return !v || *v == '\0'; };
	};
	template <> struct empty<char*> {
		bool operator()(char* v) const { return !v || *v == '\0'; };
	};
	template <> struct empty<const wchar_t*> {
		bool operator()(const wchar_t* v) const { return !v || *v == L'\0'; };
	};
	template <> struct empty<wchar_t*> {
		bool operator()(wchar_t* v) const { return !v || *v == L'\0'; };
	};
	template <typename C> struct empty<std::basic_string<C> > {
		bool operator()(const std::basic_string<C>& v) const { return v.empty(); };
	};

	template <typename T, typename CharT = char, typename TraitsT = std::char_traits<CharT> >
	class ostream_iterator
		: public std::iterator<std::output_iterator_tag, void, void, void, void>
	{
	    public:
		typedef std::basic_ostream<CharT, TraitsT> ostream_type;

		ostream_iterator(ostream_type& s, const CharT* sep = 0, bool ie = false)
			: stream(&s), join_string(sep), print_sep(false), ignore_empty(ie) { }

		ostream_iterator& operator=(const T& value)
		{
			if (!ignore_empty || !is_empty(value)) {
				if (print_sep)
					*stream << join_string;
				*stream << value;
				print_sep = true;
			}

			return *this;
		}

		ostream_iterator& operator*(void) { return *this; }
		ostream_iterator& operator++(void) { return *this; }
		ostream_iterator& operator++(int) { return *this; }

	    private:
		ostream_type* stream;
		const CharT* join_string;
		bool print_sep, ignore_empty;
		empty<T> is_empty;
	};
};

template <typename T, typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>&
join(std::basic_ostream<CharT, TraitsT>& stream,
     const T* begin, const T* end, const char* sep, bool ignore_empty = false)
{
	std::copy(begin, end, join_::ostream_iterator<T, CharT, TraitsT>(stream, sep, ignore_empty));
	return stream;
}
template <typename T, typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>&
join(std::basic_ostream<CharT, TraitsT>& stream,
     const T* ptr, size_t len, const char* sep, bool ignore_empty = false)
{
	join<T, CharT, TraitsT>(stream, ptr, ptr + len, sep, ignore_empty);
	return stream;
}

template <typename T>
std::string join(const T* begin, const T* end, const char* sep, bool ignore_empty = false)
{
	std::ostringstream stream;
	join<T>(stream, begin, end, sep, ignore_empty);
	return stream.str();
}
template <typename T>
std::string join(const T* ptr, size_t len, const char* sep, bool ignore_empty = false)
{
	return join<T>(ptr, ptr + len, sep, ignore_empty);
}

template <typename CharT>
std::basic_string<CharT> join(const std::basic_string<CharT>* begin, const std::basic_string<CharT>* end,
			      const char* sep, bool ignore_empty = false)
{
	std::basic_ostringstream<CharT, std::char_traits<CharT> > stream;
	join<std::basic_string<CharT> >(stream, begin, end, sep, ignore_empty);
	return stream.str();
}
template <typename CharT>
std::basic_string<CharT>  join(const std::basic_string<CharT>* begin, size_t len,
		 const char* sep, bool ignore_empty = false)
{
	return join<CharT>(begin, begin + len, sep, ignore_empty);
}

#include <vector>
#include <climits>

std::vector<std::string> split(const char* re_str, const char* str, unsigned max_split = UINT_MAX);

#endif // STRUTIL_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
