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
# ifndef FB_BOOST_PREPROCESSOR_SEQ_FOR_EACH_I_HPP
# define FB_BOOST_PREPROCESSOR_SEQ_FOR_EACH_I_HPP
#
# include <firebird/impl/boost/preprocessor/arithmetic/dec.hpp>
# include <firebird/impl/boost/preprocessor/arithmetic/inc.hpp>
# include <firebird/impl/boost/preprocessor/config/config.hpp>
# include <firebird/impl/boost/preprocessor/repetition/for.hpp>
# include <firebird/impl/boost/preprocessor/seq/seq.hpp>
# include <firebird/impl/boost/preprocessor/seq/size.hpp>
# include <firebird/impl/boost/preprocessor/tuple/elem.hpp>
# include <firebird/impl/boost/preprocessor/tuple/rem.hpp>
#
# /* FB_BOOST_PP_SEQ_FOR_EACH_I */
#
# if ~FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_EDG()
#    define FB_BOOST_PP_SEQ_FOR_EACH_I(macro, data, seq) FB_BOOST_PP_FOR((macro, data, seq (nil), 0), FB_BOOST_PP_SEQ_FOR_EACH_I_P, FB_BOOST_PP_SEQ_FOR_EACH_I_O, FB_BOOST_PP_SEQ_FOR_EACH_I_M)
# else
#    define FB_BOOST_PP_SEQ_FOR_EACH_I(macro, data, seq) FB_BOOST_PP_SEQ_FOR_EACH_I_I(macro, data, seq)
#    define FB_BOOST_PP_SEQ_FOR_EACH_I_I(macro, data, seq) FB_BOOST_PP_FOR((macro, data, seq (nil), 0), FB_BOOST_PP_SEQ_FOR_EACH_I_P, FB_BOOST_PP_SEQ_FOR_EACH_I_O, FB_BOOST_PP_SEQ_FOR_EACH_I_M)
# endif
#
# define FB_BOOST_PP_SEQ_FOR_EACH_I_P(r, x) FB_BOOST_PP_DEC(FB_BOOST_PP_SEQ_SIZE(FB_BOOST_PP_TUPLE_ELEM(4, 2, x)))
#
# if FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_STRICT()
#    define FB_BOOST_PP_SEQ_FOR_EACH_I_O(r, x) FB_BOOST_PP_SEQ_FOR_EACH_I_O_I x
# else
#    define FB_BOOST_PP_SEQ_FOR_EACH_I_O(r, x) FB_BOOST_PP_SEQ_FOR_EACH_I_O_I(FB_BOOST_PP_TUPLE_ELEM(4, 0, x), FB_BOOST_PP_TUPLE_ELEM(4, 1, x), FB_BOOST_PP_TUPLE_ELEM(4, 2, x), FB_BOOST_PP_TUPLE_ELEM(4, 3, x))
# endif
#
# define FB_BOOST_PP_SEQ_FOR_EACH_I_O_I(macro, data, seq, i) (macro, data, FB_BOOST_PP_SEQ_TAIL(seq), FB_BOOST_PP_INC(i))
#
# if FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_STRICT()
#    define FB_BOOST_PP_SEQ_FOR_EACH_I_M(r, x) FB_BOOST_PP_SEQ_FOR_EACH_I_M_IM(r, FB_BOOST_PP_TUPLE_REM_4 x)
#    define FB_BOOST_PP_SEQ_FOR_EACH_I_M_IM(r, im) FB_BOOST_PP_SEQ_FOR_EACH_I_M_I(r, im)
# else
#    define FB_BOOST_PP_SEQ_FOR_EACH_I_M(r, x) FB_BOOST_PP_SEQ_FOR_EACH_I_M_I(r, FB_BOOST_PP_TUPLE_ELEM(4, 0, x), FB_BOOST_PP_TUPLE_ELEM(4, 1, x), FB_BOOST_PP_TUPLE_ELEM(4, 2, x), FB_BOOST_PP_TUPLE_ELEM(4, 3, x))
# endif
#
# define FB_BOOST_PP_SEQ_FOR_EACH_I_M_I(r, macro, data, seq, i) macro(r, data, i, FB_BOOST_PP_SEQ_HEAD(seq))
#
# /* FB_BOOST_PP_SEQ_FOR_EACH_I_R */
#
# if ~FB_BOOST_PP_CONFIG_FLAGS() & FB_BOOST_PP_CONFIG_EDG()
#    define FB_BOOST_PP_SEQ_FOR_EACH_I_R(r, macro, data, seq) FB_BOOST_PP_FOR_ ## r((macro, data, seq (nil), 0), FB_BOOST_PP_SEQ_FOR_EACH_I_P, FB_BOOST_PP_SEQ_FOR_EACH_I_O, FB_BOOST_PP_SEQ_FOR_EACH_I_M)
# else
#    define FB_BOOST_PP_SEQ_FOR_EACH_I_R(r, macro, data, seq) FB_BOOST_PP_SEQ_FOR_EACH_I_R_I(r, macro, data, seq)
#    define FB_BOOST_PP_SEQ_FOR_EACH_I_R_I(r, macro, data, seq) FB_BOOST_PP_FOR_ ## r((macro, data, seq (nil), 0), FB_BOOST_PP_SEQ_FOR_EACH_I_P, FB_BOOST_PP_SEQ_FOR_EACH_I_O, FB_BOOST_PP_SEQ_FOR_EACH_I_M)
# endif
#
# endif
