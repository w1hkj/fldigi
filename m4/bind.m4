dnl Look for a working std::bind or std::tr1::bind.

m4_define([_AX_CXX_COMPILE_STDCXX_11_BIND_testbody], [[
#include <functional>
void f(void) { }
int main(int argc, char **argv)
{
    std::bind(f)();
    return 0;
}
]])

m4_define([_AX_CXX_COMPILE_STDCXX_11_TR1_BIND_testbody], [[
#include <tr1/functional>
void f(void) { }
int main(int argc, char **argv)
{
    std::tr1::bind(f)();
    return 0;
}
]])

AC_DEFUN([AC_FLDIGI_BIND], [
   AC_LANG_PUSH([C++])dnl

   AC_CACHE_CHECK(for std::bind in <functional>,
      ax_cv_std_bind,
      [AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_BIND_testbody])],
      [ac_cv_std_bind=yes],
      [ac_cv_std_bind=no])])

   ac_success=no
  if test "x$ac_cv_std_bind" = "xyes"; then
      ac_success=yes
      AC_DEFINE(HAVE_STD_BIND, 1, [Defined 1 if we have std::bind in <functional>])
  else
      AC_DEFINE(HAVE_STD_BIND, 0, [Defined 0 if we do not have std::bind in <functional>])
  fi

   if test x$ac_success = xno; then
      AC_CACHE_CHECK(for std::tr1::bind in <tr1/functional>,
         ax_cv_std_tr1_bind,
         [AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_TR1_BIND_testbody])],
         [ac_cv_std_tr1_bind=yes],
         [ac_cv_std_tr1_bind=no])])

      if test "x$ac_cv_std_tr1_bind" = "xyes"; then
         ac_success=yes
         AC_DEFINE(HAVE_STD_TR1_BIND, 1, [Defined 1 if we have std::tr1::bind in <tr1/functional>])
      else
         AC_DEFINE(HAVE_STD_TR1_BIND, 0, [Defined 0 if we do not have std::tr1::bind in <tr1/functional>])
      fi
  fi

   if test x$ac_success = xno; then
      for switch in -std=gnu++11 -std=gnu++0x -std=c++11 -std=c++0x; do
         cachevar=AS_TR_SH([ax_cv_std_bind_$switch])
         AC_CACHE_CHECK(for $CXX supports std::bind() feature with $switch,
         $cachevar,
         [ac_save_CXXFLAGS="$CXXFLAGS"
         CXXFLAGS="$CXXFLAGS $switch"
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_BIND_testbody])],
         [eval $cachevar=yes],
         [eval $cachevar=no])
         CXXFLAGS="$ac_save_CXXFLAGS"])

         if eval test x\$$cachevar = xyes; then
            CXXFLAGS="$CXXFLAGS $switch"
            ac_cv_std_bind=yes
            ac_success=yes
            break
         fi
      done
   fi

   AC_LANG_POP([C++])

   if test "x$ac_success" = "xno"; then
      AC_MSG_ERROR([Could not find std::bind or std::tr1::bind])
  fi
])
