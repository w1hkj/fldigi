dnl Check whether TLS is supported. C test code from gcc's config/tls.m4.
AC_DEFUN([CHECK_TLS], [
  AC_MSG_CHECKING([for TLS support])
  AC_LANG_PUSH(C++)

  AC_RUN_IFELSE([__thread int a; int b; int main() { return a = b; }],
                [chktls_save_CXXFLAGS="$CXXFLAGS"
                 thread_CXXFLAGS=failed
                 for flag in "" "-pthread" "-lpthread"; do
                     CXXFLAGS="$flag $chktls_save_CXXFLAGS"
                     AC_LINK_IFELSE( [AC_LANG_PROGRAM( [[#include <pthread.h>
                                                         void *g(void *d) { return NULL; }]],
                                                       [[pthread_t t; pthread_create(&t, NULL, g, NULL);]] )
                                     ],
                                     [thread_CXXFLAGS="$flag"] )
                     if test "x$thread_CXXFLAGS" != "xfailed"; then
                         break
                     fi
                 done
                 CXXFLAGS="$chktls_save_CXXFLAGS"

                 if test "x$thread_CXXFLAGS" != "xfailed"; then
                     CXXFLAGS="$thread_CXXFLAGS $chktls_save_CXXFLAGS"
                     AC_RUN_IFELSE( [AC_LANG_PROGRAM( [[#include <pthread.h>
                                                        __thread int a;
                                                        static int *a_in_other_thread;
                                                        static void *thread_func(void *arg)
                                                        {
                                                            a_in_other_thread = &a;
                                                            return (void *)0;
                                                        }]],
                                                      [[pthread_t thread;
                                                        void *thread_retval;
                                                        int *a_in_main_thread;
                                                        if (pthread_create(&thread, (pthread_attr_t *)0,
                                                                           thread_func, (void *)0))
                                                            return 0;
                                                        a_in_main_thread = &a;
                                                        if (pthread_join (thread, &thread_retval))
                                                            return 0;
                                                        return (a_in_other_thread == a_in_main_thread);]] )
                                    ],
                                    [ac_cv_tls=yes], [ac_cv_tls=no] )
                     CXXFLAGS="$chktls_save_CXXFLAGS"
                 fi],
                [ac_cv_tls=no],
                [AC_LINK_IFELSE([__thread int a; int b; int main() { return a = b; }],
		                [ac_cv_tls=yes], [ac_cv_tls=no])]
  )

  AC_LANG_POP(C++)
  AC_MSG_RESULT([$ac_cv_tls])
])


AC_DEFUN([AC_FLDIGI_TLS], [
  AC_ARG_ENABLE([tls],
                AC_HELP_STRING([--enable-tls], [enable use of TLS @<:@autodetect@:>@]),
                [case "${enableval}" in
                  yes|no) ac_cv_want_tls="${enableval}" ;;
                  *)      AC_MSG_ERROR([bad value "${enableval}" for --enable-tls]) ;;
                 esac],
                 [ac_cv_want_tls=check])

  if test "x$target_mingw32" = "xyes"; then
      ac_cv_want_tls=no
  fi

  if test "x$ac_cv_want_tls" = "xno"; then
      AC_DEFINE(USE_TLS, 0, [Defined if we are using TLS])
      ac_cv_tls=no
      AC_MSG_CHECKING([for TLS support])
      AC_MSG_RESULT([disabled])
  else
      CHECK_TLS()
      if test "x$ac_cv_want_tls" = "xcheck"; then
          if test "x$ac_cv_tls" = "xyes"; then
              AC_DEFINE(USE_TLS, 1, [Defined if we are using TLS])
          else
              AC_DEFINE(USE_TLS, 0, [Defined if we are using TLS])
          fi
      else # $ac_cv_want_tls is yes
          if test "x$ac_cv_tls" = "xno"; then
              AC_MSG_FAILURE([--enable-tls was given, but TLS is not supported])
          else
              AC_DEFINE(USE_TLS, 1, [Defined if we are using TLS])
          fi
      fi
  fi
])
