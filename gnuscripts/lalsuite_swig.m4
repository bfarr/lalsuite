# SWIG configuration
# Author: Karl Wette, 2011, 2012
#
# serial 16

# enable SWIG wrapping modules
AC_DEFUN([LALSUITE_ENABLE_SWIG],[

  # option to enable/disable all languages
  AC_ARG_ENABLE(
    [swig],
    AC_HELP_STRING(
      [--enable-swig],
      [generate SWIG wrapping modules for all languages]
    ),[
      AS_CASE(["${enableval}"],
        [yes],[swig_build_all=true],
        [no],[swig_build_all=false],
        [*],[AC_MSG_ERROR([invalid value "${enableval}" for --enable-swig])]
      )
    ],[
      swig_build_all=
    ]
  )

  # options to enable/disable languages
  swig_build_any=false
  LALSUITE_ENABLE_SWIG_LANGUAGE([Octave],[false],[LALSUITE_REQUIRE_CXX])
  LALSUITE_ENABLE_SWIG_LANGUAGE([Python],[false])

  # option to use specific SWIG binary
  AC_ARG_WITH(
    [swig],
    AC_HELP_STRING(
      [--with-swig],
      [specify SWIG binary (default: search $PATH)]
    ),[
      AS_IF([test -f "${withval}"],[
        SWIG="${withval}"
      ],[
        AC_MSG_ERROR([file "${withval}" not found])
      ])
    ],[
      SWIG=
    ]
  )

])

# options to enable/disable languages
# args: $1=language, $2=default enabled?, [$3=action if enabled]
AC_DEFUN([LALSUITE_ENABLE_SWIG_LANGUAGE],[
  m4_pushdef([lowercase],translit([$1],[A-Z],[a-z]))

  # command line option to enable/disable $1
  AC_ARG_ENABLE(
    [swig-]lowercase,
    AC_HELP_STRING(
      [--enable-swig-]lowercase,
      [generate SWIG wrapping module for $1]
    ),[
      AS_CASE(["${enableval}"],
        [yes],[swig_build_]lowercase[=true],
        [no],[swig_build_]lowercase[=false],
        [*],[AC_MSG_ERROR([invalid value "${enableval}" for --enable-swig-]]lowercase[)]
      )
    ],[
      swig_build_]lowercase[=${swig_build_all:-$2}
    ]
  )

  # if $1 is enabled
  AS_IF([test "${swig_build_]lowercase[}" = true],[:
    swig_build_any=true
    $3
  ])

  m4_popdef([lowercase])
])

# configure SWIG wrapping modules
# args: $1=symbol prefixes
AC_DEFUN([LALSUITE_USE_SWIG],[

  # save and clear global compiler/linker variables
  swig_save_CPPFLAGS=${CPPFLAGS}
  swig_save_CFLAGS=${CFLAGS}
  swig_save_CXXFLAGS=${CXXFLAGS}
  swig_save_LDFLAGS=${LDFLAGS}
  swig_save_LIBS=${LIBS}
  CPPFLAGS=
  CFLAGS=
  CXXFLAGS=
  LDFLAGS=
  LIBS=

  # check for required programs
  AC_REQUIRE([AC_PROG_LN_S])
  AC_REQUIRE([AC_PROG_MKDIR_P])
  AC_REQUIRE([AC_PROG_SED])

  # if any language was configured
  AM_CONDITIONAL(SWIG_BUILD,[test "${swig_build_any}" = true])
  AM_COND_IF(SWIG_BUILD,[

    # check for SWIG binary
    AS_IF([test "x${SWIG}" = x],[
      AC_PATH_PROGS(SWIG,[swig2.0 swig],[])
      AS_IF([test "x${SWIG}" = x],[
        AC_MSG_ERROR([could not find "swig" in path])
      ])
    ])

    # check SWIG version
    swig_min_version=2.0.7
    AC_MSG_CHECKING([${SWIG} version])
    swig_regex=['s|^ *SWIG [Vv]ersion \([0-9.][0-9.]*\) *$|\1|p;d']
    swig_version=[`${SWIG} -version | ${SED} "${swig_regex}"`]
    AS_IF([test "x${swig_version}" = x],[
      AC_MSG_ERROR([could not determine SWIG version])
    ])
    AC_MSG_RESULT([${swig_version}])
    AS_VERSION_COMPARE([${swig_min_version}],[${swig_version}],[],[],[
      AC_MSG_ERROR([require SWIG version >= ${swig_min_version}])
    ])

    # symbol prefixes for this LAL library
    AC_SUBST(SWIG_SYMBOL_PREFIXES,["$1"])

    # flags for generating SWIG wrapping module sources
    AC_SUBST(SWIG_SWIGFLAGS,["-Wextra -Werror"])

    # if we are wrapping the LAL library (instead of one of the LAL* libraries)
    AS_IF([test "x${PACKAGE_NAME}" = xlal],[
      lalswig=true
    ],[
      lalswig=false
    ])

    # common SWIG interfaces (with LAL only)
    AS_IF([test ${lalswig} = true],[
      AC_SUBST([SWIG_IFACES],["swiglal_common.i"])
    ])

    # try to figure out the underlying type of int64_t
    AC_CHECK_HEADERS([stdint.h],[],[
      AC_MSG_ERROR([could not find "stdint.h"])
    ])
    AC_MSG_CHECKING([underlying type of int64_t])
    CFLAGS="-Wall -Werror"
    AC_LANG_PUSH([C])
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],[
        int64_t i64 = 0; long int *pli = &i64; *pli = 0 /*;*/
      ])
    ],[
      AC_MSG_RESULT([long int])
      swig_wordsize=-DSWIGWORDSIZE64
    ],[
      AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],[
          int64_t i64 = 0; long long int *plli = &i64; *plli = 0 /*;*/
        ])
      ],[
        AC_MSG_RESULT([long long int])
        swig_wordsize=
      ],[
        AC_MSG_FAILURE([could not determine underlying type of int64_t])
      ])
    ])
    CFLAGS=
    AC_LANG_POP([C])
    SWIG_SWIGFLAGS="${SWIG_SWIGFLAGS} ${swig_wordsize}"

    # directories SWIG should look in for interfaces and LAL headers
    SWIG_SWIGFLAGS="${SWIG_SWIGFLAGS} -I\$(abs_top_builddir)/include"

    # send language-specific SWIG output files to libtool directory
    AC_SUBST(SWIG_OUTDIR,["\$(abs_builddir)/${objdir}"])
    SWIG_SWIGFLAGS="${SWIG_SWIGFLAGS} -outdir \$(SWIG_OUTDIR)"

    # flags for generating/compiling SWIG wrapping module sources
    AC_SUBST(SWIG_CPPFLAGS,["${swig_save_CPPFLAGS}"])

    # are we (not) in debugging mode?
    AS_IF([test "x${enable_debug}" = xno],[
      SWIG_SWIGFLAGS="${SWIG_SWIGFLAGS} -DNDEBUG"
      SWIG_CPPFLAGS="${SWIG_CPPFLAGS} -DNDEBUG"
    ])

    # flags for compiling SWIG wrapping module sources
    AC_SUBST(SWIG_CFLAGS,["${swig_save_CFLAGS}"])
    AC_SUBST(SWIG_CXXFLAGS,["${swig_save_CXXFLAGS}"])

    # define C99 constant and limit macros for C++ sources
    SWIG_CXXFLAGS="${SWIG_CXXFLAGS} -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS"

    # make SWIG use C++ casts in typemaps in C++ mode
    SWIG_CXXFLAGS="${SWIG_CXXFLAGS} -DSWIG_CPLUSPLUS_CAST"

    # disable optimisation in debug mode, for faster compilation
    AS_IF([test "x${enable_debug}" != xno],[
      SWIG_CFLAGS="${SWIG_CFLAGS} -O0"
      SWIG_CXXFLAGS="${SWIG_CXXFLAGS} -O0"
    ])

    # flags for linking SWIG wrapping modules
    AC_SUBST(SWIG_LDFLAGS,["${swig_save_LDFLAGS}"])

    # libraries SWIG wrapping module should be linked against
    AC_SUBST(SWIG_LIBS,["${swig_save_LIBS}"])
    AS_IF([test ${lalswig} = true],[
      SWIG_LIBS="${SWIG_LIBS} \$(abs_top_builddir)/packages/support/src/liblalsupport.la \$(abs_top_builddir)/lib/liblal.la"
    ],[
      SWIG_LIBS="${SWIG_LIBS} \$(abs_top_builddir)/src/lib${PACKAGE_NAME}.la"
    ])

    # dynamic linker search path for pre-installed LAL libraries
    SWIG_LD_LIBRARY_PATH=[`for n in ${SWIG_LIBS}; do echo $n | ${SED} -n 's|/liblal[^.]*\.la|/'"${objdir}"'|p'; done`]
    SWIG_LD_LIBRARY_PATH=[`echo ${SWIG_LD_LIBRARY_PATH}`]   # get rid of newlines
    SWIG_LD_LIBRARY_PATH=[`echo ${SWIG_LD_LIBRARY_PATH} | ${SED} 's|(top_builddir)|(abs_top_builddir)|g;s|  *|:|g'`]
    AC_SUBST(SWIG_LD_LIBRARY_PATH)
    AS_IF([test "${build_vendor}" = apple],[
      SWIG_LD_LIBPATH_NAME=DYLD_LIBRARY_PATH
    ],[
      SWIG_LD_LIBPATH_NAME=LD_LIBRARY_PATH
    ])
    AC_SUBST(SWIG_LD_LIBPATH_NAME)

    # check for additional compiler flags:
    extra_flags=
    # - suppress warnings about uninitialized variables in SWIG-generated code
    extra_flags="${extra_flags} -Wno-uninitialized"
    for flag in ${extra_flags}; do
      AC_MSG_CHECKING([if ${flag} is supported])
      CFLAGS=${flag}
      AC_LANG_PUSH([C])
      AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],[])
      ],[
        AC_MSG_RESULT([yes])
        SWIG_CFLAGS="${SWIG_CFLAGS} ${flag}"
        SWIG_CXXFLAGS="${SWIG_CXXFLAGS} ${flag}"
      ],[
        AC_MSG_RESULT([no])
        swig_wordsize=
      ])
      CFLAGS=
      AC_LANG_POP([C])
    done

  ])

  # string to add to user environment setup scripts
  AC_SUBST(SWIG_USER_ENV,[""])

  # configure SWIG languages
  LALSUITE_USE_SWIG_OCTAVE
  LALSUITE_USE_SWIG_PYTHON

  # list of other LAL libraries SWIG wrapping module depends on
  AC_SUBST(SWIG_MODULE_DEPENDS,[""])

  # scripting-language path to search for pre-installed SWIG modules
  AC_SUBST(SWIG_PREINST_PATH,["\$(SWIG_OUTDIR)"])

  # restore global compiler/linker variables
  CPPFLAGS=${swig_save_CPPFLAGS}
  CFLAGS=${swig_save_CFLAGS}
  CXXFLAGS=${swig_save_CXXFLAGS}
  LDFLAGS=${swig_save_LDFLAGS}
  LIBS=${swig_save_LIBS}

])

# add to list of other LAL libraries SWIG wrapping module depends on
# args: $1=LAL library, $2=enable dependency?
AC_DEFUN([LALSUITE_SWIG_DEPENDS],[
  AS_IF([test "x$2" = xtrue],[
    SWIG_MODULE_DEPENDS="${SWIG_MODULE_DEPENDS} $1"

    # add to scripting-language path to search for pre-installed SWIG modules
    AS_IF([test ${LALSUITE_BUILD} = true],[
      SWIG_PREINST_PATH="${SWIG_PREINST_PATH}:\$(abs_top_builddir)/../$1/\$(subdir)/${objdir}"
    ])

  ])
])

# configure SWIG language wrapping module
# args: $1=language, $2=actions if enabled
AC_DEFUN([LALSUITE_USE_SWIG_LANGUAGE],[
  m4_pushdef([uppercase],translit([$1],[a-z],[A-Z]))
  m4_pushdef([lowercase],translit([$1],[A-Z],[a-z]))

  # check whether to configure $1
  AM_CONDITIONAL(SWIG_BUILD_[]uppercase,[test ${swig_build_]lowercase[} = true])
  AM_COND_IF(SWIG_BUILD_[]uppercase,[

    # at least one language was configured
    swig_build=true

    # set message string to indicate language will be built
    SWIG_]uppercase[_ENABLE_VAL=ENABLED

    # common and language-specific SWIG interfaces (with LAL only)
    AS_IF([test ${lalswig} = true],[
      SWIG_]uppercase[_IFACES="swiglal_]lowercase[.i"
      AC_SUBST(SWIG_]uppercase[_IFACES)
    ])

    # configure $1
    $2
    # $1 has been configured

  ],[
    SWIG_]uppercase[_ENABLE_VAL=DISABLED
  ])

  m4_popdef([uppercase])
  m4_popdef([lowercase])
])

# configure SWIG Octave wrapping module
AC_DEFUN([LALSUITE_USE_SWIG_OCTAVE],[
  LALSUITE_USE_SWIG_LANGUAGE([Octave],[

    # check for Octave binary
    AC_PATH_PROG(OCTAVE,[octave],[],[])
    AS_IF([test "x${OCTAVE}" = x],[
      AC_MSG_ERROR([could not find "octave" in path])
    ])
    octave_prefix=[`${OCTAVE} -qfH --eval "disp(octave_config_info('prefix'))" | ${SED} 's|/*$||'`]

    # check for Octave mkoctfile binary
    AC_MSG_CHECKING([for mkoctfile])
    AS_IF([test "x`${OCTAVE} -qfH --eval 'mkoctfile -p CXX' 2>/dev/null`" != x],[
      AC_MSG_RESULT([yes])
    ],[
      AC_MSG_ERROR([mkoctfile is not installed])
    ])

    # check Octave version
    octave_min_version=3.2.0
    AC_MSG_CHECKING([${OCTAVE} version])
    octave_version=[`${OCTAVE} -qfH --eval "disp(version)"`]
    AS_IF([test "x${octave_version}" = x],[
      AC_MSG_ERROR([could not determine ${OCTAVE} version])
    ])
    AC_MSG_RESULT([${octave_version}])
    AS_VERSION_COMPARE([${octave_min_version}],[${octave_version}],[],[],[
      AC_MSG_ERROR([require ${OCTAVE} version >= ${octave_min_version}])
    ])

    # check that wrappings are being compiled with the same C++ compiler used to compile Octave itself
    AC_MSG_CHECKING([C++ compiler used for building ${OCTAVE}])
    octave_CXX=`${OCTAVE} -qfH --eval "mkoctfile -p CXX"`
    AC_MSG_RESULT([${octave_CXX}])
    AS_IF([test "x${CXX}" != "x${octave_CXX}"],[
      AC_MSG_ERROR([configured C++ compiler "${CXX}" differs from ${OCTAVE} C++ compiler "${octave_CXX}"])
    ])

    # determine Octave module flags
    AC_MSG_CHECKING([for ${OCTAVE} module CPPFLAGS])
    AC_SUBST(OCTAVE_CPPFLAGS,[""])
    for n in CPPFLAGS INCFLAGS; do
      OCTAVE_CPPFLAGS="${OCTAVE_CPPFLAGS} "`${OCTAVE} -qfH --eval "mkoctfile -p $n"`
    done
    AC_MSG_RESULT([${OCTAVE_CPPFLAGS}])
    AC_MSG_CHECKING([for ${OCTAVE} module CXXFLAGS])
    AC_SUBST(OCTAVE_CXXFLAGS,[""])
    for n in ALL_CXXFLAGS; do
      OCTAVE_CXXFLAGS="${OCTAVE_CXXFLAGS} "`${OCTAVE} -qfH --eval "mkoctfile -p $n"`
    done
    AC_MSG_RESULT([${OCTAVE_CXXFLAGS}])
    AC_MSG_CHECKING([for ${OCTAVE} module LDFLAGS])
    AC_SUBST(OCTAVE_LDFLAGS,[""])
    for n in RDYNAMIC_FLAG LFLAGS RLD_FLAG OCTAVE_LIBS LIBS; do
      OCTAVE_LDFLAGS="${OCTAVE_LDFLAGS} "`${OCTAVE} -qfH --eval "mkoctfile -p $n"`
    done
    AC_MSG_RESULT([${OCTAVE_LDFLAGS}])

    # check for Octave headers
    CPPFLAGS=${OCTAVE_CPPFLAGS}
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADERS([octave/oct.h],[],[
      AC_MSG_ERROR([could not find the header "octave/oct.h"])
    ],[
      AC_INCLUDES_DEFAULT
    ])
    CPPFLAGS=
    AC_LANG_POP([C++])

    # determine where to install Octave module:
    # take site .oct file directory given by octave-config,
    # and strip off prefix; thus, if LALSuite is installed in
    # the same directory as Octave, .oct module files will be
    # found by Octave without having to add to OCTAVE_PATH
    AC_MSG_CHECKING([for ${OCTAVE} module installation directory])
    octexecdir=[`${OCTAVE} -qfH --eval "disp(octave_config_info('localoctfiledir'))" | ${SED} 's|/*$||'`]
    octexecdir=[`echo ${octexecdir} | ${SED} "s|^${octave_prefix}/||"`]
    AS_IF([test "x`echo ${octexecdir} | ${SED} -n '\|^/|p'`" != x],[
      AC_MSG_ERROR([could not build relative path from "${octexecdir}"])
    ])
    octexecdir='${prefix}'/"${octexecdir}"
    AC_MSG_RESULT([${octexecdir}])
    AC_SUBST(octexecdir)

    # string to add to user environment setup scripts
    SWIG_USER_ENV="${SWIG_USER_ENV}"'prepend OCTAVE_PATH $(octexecdir)\n'

  ])
])

# configure SWIG Python wrapping module
AC_DEFUN([LALSUITE_USE_SWIG_PYTHON],[
  LALSUITE_USE_SWIG_LANGUAGE([Python],[

    # check for Python
    python_min_version=2.5
    AM_PATH_PYTHON([${python_min_version}])

    # check for distutils
    AC_MSG_CHECKING([for distutils])
    cat <<EOD | ${PYTHON} - 2>/dev/null
import distutils
EOD
    AS_IF([test $? -ne 0],[
      AC_MSG_ERROR([could not import distutils])
    ])
    AC_MSG_RESULT([yes])

    # check for NumPy
    numpy_min_version=1.3
    AC_MSG_CHECKING([for NumPy])
    numpy_version=[`cat <<EOD | ${PYTHON} - 2>/dev/null
import numpy
print(numpy.__version__)
EOD`]
    AS_IF([test $? -ne 0],[
      AC_MSG_ERROR([could not import NumPy])
    ])
    AC_MSG_RESULT([yes])

    # check NumPy version
    AC_MSG_CHECKING([NumPy version])
    AS_VERSION_COMPARE([${numpy_min_version}],[${numpy_version}],[],[],[
      AC_MSG_ERROR([require NumPy version >= ${numpy_min_version}])
    ])
    AC_MSG_RESULT([${numpy_version}])

    # determine Python module CPPFLAGS
    AC_MSG_CHECKING([for ${PYTHON} module CPPFLAGS])
    PYTHON_CPPFLAGS=[`cat <<EOD | ${PYTHON} - 2>/dev/null
import sys
import distutils.sysconfig as cfg
import numpy.lib.utils as npyutil
sys.stdout.write( '-I' + cfg.get_python_inc())
sys.stdout.write(' -I' + cfg.get_python_inc(plat_specific=1))
sys.stdout.write(' -I' + npyutil.get_include())
EOD`]
    AS_IF([test $? -ne 0],[
      AC_MSG_ERROR([could not determine ${PYTHON} module CPPFLAGS])
    ])
    AC_SUBST(PYTHON_CPPFLAGS)
    AC_MSG_RESULT([${PYTHON_CPPFLAGS}])

    # determine Python module CFLAGS
    AC_MSG_CHECKING([for ${PYTHON} module CFLAGS])
    PYTHON_CFLAGS=[`cat <<EOD | ${PYTHON} - 2>/dev/null
import sys
import distutils.sysconfig as cfg
cflags = cfg.get_config_var('CFLAGS').split()
cflags = [f for f in cflags if f != '-DNDEBUG']
sys.stdout.write(" ".join(cflags))
EOD`]
    AS_IF([test $? -ne 0],[
      AC_MSG_ERROR([could not determine ${PYTHON} module CFLAGS])
    ])
    AC_SUBST(PYTHON_CFLAGS)
    AC_MSG_RESULT([${PYTHON_CFLAGS}])

    # determine Python module LDFLAGS
    AC_MSG_CHECKING([for ${PYTHON} module LDFLAGS])
    PYTHON_LDFLAGS=[`cat <<EOD | ${PYTHON} - 2>/dev/null
import sys, os
import distutils.sysconfig as cfg
sys.stdout.write(cfg.get_config_var('LINKFORSHARED'))
sys.stdout.write(' -L' + cfg.get_python_lib())
sys.stdout.write(' -L' + cfg.get_python_lib(plat_specific=1))
stdlibdir = cfg.get_python_lib(plat_specific=1,standard_lib=1)
sys.stdout.write(' ' + os.path.join(stdlibdir, cfg.get_config_var('LDLIBRARY')))
EOD`]
    AS_IF([test $? -ne 0],[
      AC_MSG_ERROR([could not determine ${PYTHON} module LDFLAGS])
    ])
    AC_SUBST(PYTHON_LDFLAGS)
    AC_MSG_RESULT([${PYTHON_LDFLAGS}])

    # check for Python headers
    CPPFLAGS=${PYTHON_CPPFLAGS}
    AC_LANG_PUSH([C])
    AC_CHECK_HEADERS([Python.h],[],[
      AC_MSG_ERROR([could not find the header "Python.h"])
    ],[
      AC_INCLUDES_DEFAULT
    ])
    AC_CHECK_HEADERS([numpy/arrayobject.h],[],[
      AC_MSG_ERROR([could not find the header "numpy/arrayobject.h"])
    ],[
      AC_INCLUDES_DEFAULT
      #include <Python.h>
    ])
    CPPFLAGS=
    AC_LANG_POP([C])

    # string to add to user environment setup scripts
    SWIG_USER_ENV="${SWIG_USER_ENV}"'prepend PYTHONPATH $(pyexecdir)\n'

  ])
])
