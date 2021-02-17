# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef FB_BOOST_PREPROCESSOR_CONFIG_CONFIG_HPP
# define FB_BOOST_PREPROCESSOR_CONFIG_CONFIG_HPP
#
# /* FB_BOOST_PP_CONFIG_FLAGS */
#
# define FB_BOOST_PP_CONFIG_STRICT() 0x0001
# define FB_BOOST_PP_CONFIG_IDEAL() 0x0002
#
# define FB_BOOST_PP_CONFIG_MSVC() 0x0004
# define FB_BOOST_PP_CONFIG_MWCC() 0x0008
# define FB_BOOST_PP_CONFIG_BCC() 0x0010
# define FB_BOOST_PP_CONFIG_EDG() 0x0020
# define FB_BOOST_PP_CONFIG_DMC() 0x0040
#
# ifndef FB_BOOST_PP_CONFIG_FLAGS
#    if defined(__GCCXML__)
#        define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_STRICT())
#    elif defined(__WAVE__)
#        define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_STRICT())
#    elif defined(__MWERKS__) && __MWERKS__ >= 0x3200
#        define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_STRICT())
#    elif defined(__EDG__) || defined(__EDG_VERSION__)
#        if defined(_MSC_VER) && __EDG_VERSION__ >= 308
#            define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_MSVC())
#        else
#            define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_EDG() | FB_BOOST_PP_CONFIG_STRICT())
#        endif
#    elif defined(__MWERKS__)
#        define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_MWCC())
#    elif defined(__DMC__)
#        define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_DMC())
#    elif defined(__BORLANDC__) && __BORLANDC__ >= 0x581
#        define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_STRICT())
#    elif defined(__BORLANDC__) || defined(__IBMC__) || defined(__IBMCPP__) || defined(__SUNPRO_CC)
#        define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_BCC())
#    elif defined(_MSC_VER)
#        define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_MSVC())
#    else
#        define FB_BOOST_PP_CONFIG_FLAGS() (FB_BOOST_PP_CONFIG_STRICT())
#    endif
# endif
#
# /* FB_BOOST_PP_CONFIG_EXTENDED_LINE_INFO */
#
# ifndef FB_BOOST_PP_CONFIG_EXTENDED_LINE_INFO
#    define FB_BOOST_PP_CONFIG_EXTENDED_LINE_INFO 0
# endif
#
# /* FB_BOOST_PP_CONFIG_ERRORS */
#
# ifndef FB_BOOST_PP_CONFIG_ERRORS
#    ifdef NDEBUG
#        define FB_BOOST_PP_CONFIG_ERRORS 0
#    else
#        define FB_BOOST_PP_CONFIG_ERRORS 1
#    endif
# endif
#
# endif
