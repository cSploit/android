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
# ifndef FB_BOOST_PREPROCESSOR_SEQ_SEQ_HPP
# define FB_BOOST_PREPROCESSOR_SEQ_SEQ_HPP
#
# include <firebird/impl/boost/preprocessor/config/config.hpp>
# include <firebird/impl/boost/preprocessor/seq/elem.hpp>
#
# /* FB_BOOST_PP_SEQ_HEAD */
#
# define FB_BOOST_PP_SEQ_HEAD(seq) FB_BOOST_PP_SEQ_ELEM(0, seq)
#
# /* FB_BOOST_PP_SEQ_TAIL */
#
# if FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_MWCC()
#    define FB_BOOST_PP_SEQ_TAIL(seq) FB_BOOST_PP_SEQ_TAIL_1((seq))
#    define FB_BOOST_PP_SEQ_TAIL_1(par) FB_BOOST_PP_SEQ_TAIL_2 ## par
#    define FB_BOOST_PP_SEQ_TAIL_2(seq) FB_BOOST_PP_SEQ_TAIL_I ## seq
# elif FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_MSVC()
#    define FB_BOOST_PP_SEQ_TAIL(seq) FB_BOOST_PP_SEQ_TAIL_ID(FB_BOOST_PP_SEQ_TAIL_I seq)
#    define FB_BOOST_PP_SEQ_TAIL_ID(id) id
# elif FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_EDG()
#    define FB_BOOST_PP_SEQ_TAIL(seq) FB_BOOST_PP_SEQ_TAIL_D(seq)
#    define FB_BOOST_PP_SEQ_TAIL_D(seq) FB_BOOST_PP_SEQ_TAIL_I seq
# else
#    define FB_BOOST_PP_SEQ_TAIL(seq) FB_BOOST_PP_SEQ_TAIL_I seq
# endif
#
# define FB_BOOST_PP_SEQ_TAIL_I(x)
#
# /* FB_BOOST_PP_SEQ_NIL */
#
# define FB_BOOST_PP_SEQ_NIL(x) (x)
#
# endif
