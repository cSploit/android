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
# ifndef FB_BOOST_PREPROCESSOR_DEBUG_ERROR_HPP
# define FB_BOOST_PREPROCESSOR_DEBUG_ERROR_HPP
#
# include <firebird/impl/boost/preprocessor/cat.hpp>
# include <firebird/impl/boost/preprocessor/config/config.hpp>
#
# /* FB_BOOST_PP_ERROR */
#
# if FB_BOOST_PP_CONFIG_ERRORS
#    define FB_BOOST_PP_ERROR(code) FB_BOOST_PP_CAT(FB_BOOST_PP_ERROR_, code)
# endif
#
# define FB_BOOST_PP_ERROR_0x0000 FB_BOOST_PP_ERROR(0x0000, FB_BOOST_PP_INDEX_OUT_OF_BOUNDS)
# define FB_BOOST_PP_ERROR_0x0001 FB_BOOST_PP_ERROR(0x0001, FB_BOOST_PP_WHILE_OVERFLOW)
# define FB_BOOST_PP_ERROR_0x0002 FB_BOOST_PP_ERROR(0x0002, FB_BOOST_PP_FOR_OVERFLOW)
# define FB_BOOST_PP_ERROR_0x0003 FB_BOOST_PP_ERROR(0x0003, FB_BOOST_PP_REPEAT_OVERFLOW)
# define FB_BOOST_PP_ERROR_0x0004 FB_BOOST_PP_ERROR(0x0004, FB_BOOST_PP_LIST_FOLD_OVERFLOW)
# define FB_BOOST_PP_ERROR_0x0005 FB_BOOST_PP_ERROR(0x0005, FB_BOOST_PP_SEQ_FOLD_OVERFLOW)
# define FB_BOOST_PP_ERROR_0x0006 FB_BOOST_PP_ERROR(0x0006, FB_BOOST_PP_ARITHMETIC_OVERFLOW)
# define FB_BOOST_PP_ERROR_0x0007 FB_BOOST_PP_ERROR(0x0007, FB_BOOST_PP_DIVISION_BY_ZERO)
#
# endif
