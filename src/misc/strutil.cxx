#include <config.h>

#include <vector>
#include <string>
#include <cstring>
#include <climits>

#include "re.h"
#include "strutil.h"

using namespace std;

vector<string> split(const char* re_str, const char* str, unsigned max_split)
{
	vector<string> v;
	size_t n = strlen(re_str);
	string s; s.reserve(n + 2); s.append(1, '(').append(re_str, n).append(1, ')');
	fre_t re(s.c_str(), REG_EXTENDED);

	bool ignore_trailing_empty = false;
	if (max_split == 0) {
		max_split = UINT_MAX;
		ignore_trailing_empty = true;
	}

	s = str;
	const vector<regmatch_t>& sub = re.suboff();
	while (re.match(s.c_str())) {
		if (unlikely(sub.empty() || ((max_split != UINT_MAX) && --max_split == 0)))
			break;
		else {
			s[sub[0].rm_so] = '\0';
			v.push_back(s.c_str());
			s.erase(0, sub[0].rm_eo);
		}
	}

	if (!(ignore_trailing_empty && s.empty()))
		v.push_back(s);
	return v;
}
