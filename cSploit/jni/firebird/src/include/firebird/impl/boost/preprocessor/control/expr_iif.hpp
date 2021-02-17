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
# ifndef FB_BOOST_PREPROCESSOR_CONTROL_EXPR_IIF_HPP
# define FB_BOOST_PREPROCESSOR_CONTROL_EXPR_IIF_HPP
#
# include <firebird/impl/boost/preprocessor/config/config.hpp>
#
# /* FB_BOOST_PP_EXPR_IIF */
#
# if ~FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_MWCC()
#    define FB_BOOST_PP_EXPR_IIF(bit, expr) FB_BOOST_PP_EXPR_IIF_I(bit, expr)
# else
#    define FB_BOOST_PP_EXPR_IIF(bit, expr) FB_BOOST_PP_EXPR_IIF_OO((bit, expr))
#    define FB_BOOST_PP_EXPR_IIF_OO(par) FB_BOOST_PP_EXPR_IIF_I ## par
# endif
#
# define FB_BOOST_PP_EXPR_IIF_I(bit, expr) FB_BOOST_PP_EXPR_IIF_ ## bit(expr)
#
# define FB_BOOST_PP_EXPR_IIF_0(expr)
# define FB_BOOST_PP_EXPR_IIF_1(expr) expr
#
# endif
