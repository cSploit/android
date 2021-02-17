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
# ifndef FB_BOOST_PREPROCESSOR_TUPLE_EAT_HPP
# define FB_BOOST_PREPROCESSOR_TUPLE_EAT_HPP
#
# include <firebird/impl/boost/preprocessor/config/config.hpp>
#
# /* FB_BOOST_PP_TUPLE_EAT */
#
# if ~FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_MWCC()
#    define FB_BOOST_PP_TUPLE_EAT(size) FB_BOOST_PP_TUPLE_EAT_I(size)
# else
#    define FB_BOOST_PP_TUPLE_EAT(size) FB_BOOST_PP_TUPLE_EAT_OO((size))
#    define FB_BOOST_PP_TUPLE_EAT_OO(par) FB_BOOST_PP_TUPLE_EAT_I ## par
# endif
#
# define FB_BOOST_PP_TUPLE_EAT_I(size) FB_BOOST_PP_TUPLE_EAT_ ## size
#
# define FB_BOOST_PP_TUPLE_EAT_0()
# define FB_BOOST_PP_TUPLE_EAT_1(a)
# define FB_BOOST_PP_TUPLE_EAT_2(a, b)
# define FB_BOOST_PP_TUPLE_EAT_3(a, b, c)
# define FB_BOOST_PP_TUPLE_EAT_4(a, b, c, d)
# define FB_BOOST_PP_TUPLE_EAT_5(a, b, c, d, e)
# define FB_BOOST_PP_TUPLE_EAT_6(a, b, c, d, e, f)
# define FB_BOOST_PP_TUPLE_EAT_7(a, b, c, d, e, f, g)
# define FB_BOOST_PP_TUPLE_EAT_8(a, b, c, d, e, f, g, h)
# define FB_BOOST_PP_TUPLE_EAT_9(a, b, c, d, e, f, g, h, i)
# define FB_BOOST_PP_TUPLE_EAT_10(a, b, c, d, e, f, g, h, i, j)
# define FB_BOOST_PP_TUPLE_EAT_11(a, b, c, d, e, f, g, h, i, j, k)
# define FB_BOOST_PP_TUPLE_EAT_12(a, b, c, d, e, f, g, h, i, j, k, l)
# define FB_BOOST_PP_TUPLE_EAT_13(a, b, c, d, e, f, g, h, i, j, k, l, m)
# define FB_BOOST_PP_TUPLE_EAT_14(a, b, c, d, e, f, g, h, i, j, k, l, m, n)
# define FB_BOOST_PP_TUPLE_EAT_15(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)
# define FB_BOOST_PP_TUPLE_EAT_16(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p)
# define FB_BOOST_PP_TUPLE_EAT_17(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q)
# define FB_BOOST_PP_TUPLE_EAT_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r)
# define FB_BOOST_PP_TUPLE_EAT_19(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s)
# define FB_BOOST_PP_TUPLE_EAT_20(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t)
# define FB_BOOST_PP_TUPLE_EAT_21(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u)
# define FB_BOOST_PP_TUPLE_EAT_22(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v)
# define FB_BOOST_PP_TUPLE_EAT_23(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w)
# define FB_BOOST_PP_TUPLE_EAT_24(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x)
# define FB_BOOST_PP_TUPLE_EAT_25(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y)
#
# endif
