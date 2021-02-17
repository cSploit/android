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
# ifndef FB_BOOST_PREPROCESSOR_CONTROL_IIF_HPP
# define FB_BOOST_PREPROCESSOR_CONTROL_IIF_HPP
#
# include <firebird/impl/boost/preprocessor/config/config.hpp>
#
# if ~FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_MWCC()
#    define FB_BOOST_PP_IIF(bit, t, f) FB_BOOST_PP_IIF_I(bit, t, f)
# else
#    define FB_BOOST_PP_IIF(bit, t, f) FB_BOOST_PP_IIF_OO((bit, t, f))
#    define FB_BOOST_PP_IIF_OO(par) FB_BOOST_PP_IIF_I ## par
# endif
#
# if ~FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_MSVC()
#    define FB_BOOST_PP_IIF_I(bit, t, f) FB_BOOST_PP_IIF_ ## bit(t, f)
# else
#    define FB_BOOST_PP_IIF_I(bit, t, f) FB_BOOST_PP_IIF_II(FB_BOOST_PP_IIF_ ## bit(t, f))
#    define FB_BOOST_PP_IIF_II(id) id
# endif
#
# define FB_BOOST_PP_IIF_0(t, f) f
# define FB_BOOST_PP_IIF_1(t, f) t
#
# endif
