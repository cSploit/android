# /* Copyright (C) 2001
#  * Housemarque Oy
#  * http://www.housemarque.com
#  *
#  * Distributed under the Boost Software License, Version 1.0. (See
#  * accompanying file LICENSE_1_0.txt or copy at
#  * http://www.boost.org/LICENSE_1_0.txt)
#  */
#
# /* Revised by Paul Mensonides (2002) */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef FB_BOOST_PREPROCESSOR_CAT_HPP
# define FB_BOOST_PREPROCESSOR_CAT_HPP
#
# include <firebird/impl/boost/preprocessor/config/config.hpp>
#
# /* FB_BOOST_PP_CAT */
#
# if ~FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_MWCC()
#    define FB_BOOST_PP_CAT(a, b) FB_BOOST_PP_CAT_I(a, b)
# else
#    define FB_BOOST_PP_CAT(a, b) FB_BOOST_PP_CAT_OO((a, b))
#    define FB_BOOST_PP_CAT_OO(par) FB_BOOST_PP_CAT_I ## par
# endif
#
# if ~FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_MSVC()
#    define FB_BOOST_PP_CAT_I(a, b) a ## b
# else
#    define FB_BOOST_PP_CAT_I(a, b) FB_BOOST_PP_CAT_II(a ## b)
#    define FB_BOOST_PP_CAT_II(res) res
# endif
#
# endif
