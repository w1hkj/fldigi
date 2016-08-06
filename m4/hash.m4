dnl Look for a working std::hash or std::tr1::hash.
dnl test for gcc ge 4.1

m4_define([_AX_CXX_COMPILE_STDCXX_11_HASH_testbody], [[
#include <unordered_map>
void f(void) { }
int main(int argc, char **argv)
{
    std::hash<int>()(1);
    return 0;
}
]])

m4_define([_AX_CXX_COMPILE_STDCXX_11_TR1_HASH_testbody], [[
#include <tr1/unordered_map>
void f(void) { }
int main(int argc, char **argv)
{
    std::tr1::hash<int>()(1);
    return 0;
}
]])

AC_DEFUN([AC_FLDIGI_HASH], [
   AC_LANG_PUSH([C++])dnl

   AC_CACHE_CHECK(for std::hash in <unordered_map>,
      ax_cv_std_hash,
      [AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_HASH_testbody])],
      [ac_cv_std_hash=yes],
      [ac_cv_std_hash=no])])

   ac_success=no
   if test "x$ac_cv_std_hash" = "xyes"; then
      ac_success=yes
      AC_DEFINE(HAVE_STD_HASH, 1, [Defined 1 if we have std::hash in <unordered_map>])
   else
      AC_DEFINE(HAVE_STD_HASH, 0, [Defined 0 if we do not have std::bind in <unordered_map>])
   fi

   if test x$ac_success = xno; then
      AC_CACHE_CHECK(for std::tr1::hash in <tr1/unordered_map>,
         ax_cv_std_tr1_hash,
         [AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_TR1_HASH_testbody])],
         [ac_cv_std_tr1_hash=yes],
         [ac_cv_std_tr1_hash=no])])

      if test "x$ac_cv_std_tr1_hash" = "xyes"; then
         ac_success=yes
         AC_DEFINE(HAVE_STD_TR1_HASH, 1, [Defined 1 if we have std::tr1::hash in <tr1/unordered_map>])
      else
         AC_DEFINE(HAVE_STD_TR1_HASH, 0, [Defined 0 if we do not have std::tr1::hash in <tr1/unordered_map>])
      fi
   fi

   if test x$ac_success = xno; then
      for switch in -std=gnu++11 -std=gnu++0x -std=c++11 -std=c++0x; do
         cachevar=AS_TR_SH([ax_cv_std_hash_$switch])
         AC_CACHE_CHECK(for $CXX supports std::hash() feature with $switch,
         $cachevar,
         [ac_save_CXXFLAGS="$CXXFLAGS"
         CXXFLAGS="$CXXFLAGS $switch"
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_HASH_testbody])],
         [eval $cachevar=yes],
         [eval $cachevar=no])
         CXXFLAGS="$ac_save_CXXFLAGS"])

         if eval test x\$$cachevar = xyes; then
            CXXFLAGS="$CXXFLAGS $switch"
            ac_cv_std_hash=yes
            ac_success=yes
            break
         fi
      done
   fi

   if test "x$ac_cv_std_hash" = "xyes"; then
      ac_success=yes
      AC_DEFINE(HAVE_STD_HASH, 1, [Defined 1 if we have std::hash in <unordered_map>])
   else
      AC_DEFINE(HAVE_STD_HASH, 0, [Defined 0 if we do not have std::bind in <unordered_map>])
   fi


   gcc_dver=$($CXX -dumpversion)
   gcc_minver=4.1.0

   AC_MSG_CHECKING([for $CXX version >= $gcc_minver])
   AX_COMPARE_VERSION([$gcc_dver], [ge], [$gcc_minver],
                      [AC_MSG_RESULT([yes ($gcc_dver)])]; GCCVER="ok",
                      [AC_MSG_RESULT([no ($gcc_dver)])]; GCCVER="bad")

   if test "y$GCCVER" = "yok"; then
      AC_DEFINE(GCC_VER_OK, 1, [Define to 1 if gcc >= 4.1.0])
   else
      AC_DEFINE(GCC_VER_OK, 0, [Define to 0 if gcc < 4.1.0])
   fi

   AC_LANG_POP([C++])

   if test "x$ac_success" = "xno"; then
      AC_MSG_ERROR([Could not find std::hash or std::tr1::hash])
   fi
])
