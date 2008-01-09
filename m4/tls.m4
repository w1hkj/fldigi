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
                                    [ac_cv_have_tls=yes], [ac_cv_have_tls=no] )
                     CXXFLAGS="$chktls_save_CXXFLAGS"
                 fi],
                [ac_cv_have_tls=no],
                [AC_LINK_IFELSE([__thread int a; int b; int main() { return a = b; }],
		                [ac_cv_have_tls=yes], [ac_cv_have_tls=no])]
  )

  AC_LANG_POP(C++)
  AC_MSG_RESULT([$ac_cv_have_tls])
])
