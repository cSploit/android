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
# ifndef FB_BOOST_PREPROCESSOR_CONTROL_EXPR_IF_HPP
# define FB_BOOST_PREPROCESSOR_CONTROL_EXPR_IF_HPP
#
# include <firebird/impl/boost/preprocessor/config/config.hpp>
# include <firebird/impl/boost/preprocessor/control/expr_iif.hpp>
# include <firebird/impl/boost/preprocessor/logical/bool.hpp>
#
# /* FB_BOOST_PP_EXPR_IF */
#
# if ~FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_EDG()
#    define FB_BOOST_PP_EXPR_IF(cond, expr) FB_BOOST_PP_EXPR_IIF(FB_BOOST_PP_BOOL(cond), expr)
# else
#    define FB_BOOST_PP_EXPR_IF(cond, expr) FB_BOOST_PP_EXPR_IF_I(cond, expr)
#    define FB_BOOST_PP_EXPR_IF_I(cond, expr) FB_BOOST_PP_EXPR_IIF(FB_BOOST_PP_BOOL(cond), expr)
# endif
#
# endif
