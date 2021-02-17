require 'bigdecimal'

#
#--
# Contents:
#   sqrt(x, prec)
#   sin (x, prec)
#   cos (x, prec)
#   atan(x, prec)  Note: |x|<1, x=0.9999 may not converge.
#   exp (x, prec)
#   log (x, prec)
#   PI  (prec)
#   E   (prec) == exp(1.0,prec)
#
# where:
#   x    ... BigDecimal number to be computed.
#            |x| must be small enough to get convergence.
#   prec ... Number of digits to be obtained.
#++
#
# Provides mathematical functions.
#
# Example:
#
#   require "bigdecimal"
#   require "bigdecimal/math"
#
#   include BigMath
#
#   a = BigDecimal((PI(100)/2).to_s)
#   puts sin(a,100) # -> 0.10000000000000000000......E1
#
module BigMath
  module_function

  # Computes the square root of x to the specified number of digits of
  # precision.
  #
  # BigDecimal.new('2').sqrt(16).to_s
  #  -> "0.14142135623730950488016887242096975E1"
  #
  def sqrt(x,prec)
    x.sqrt(prec)
  end

  # Computes the sine of x to the specified number of digits of precision.
  #
  # If x is infinite or NaN, returns NaN.
  def sin(x, prec)
    raise ArgumentError, "Zero or negative precision for sin" if prec <= 0
    return BigDecimal("NaN") if x.infinite? || x.nan?
    n    = prec + BigDecimal.double_fig
    one  = BigDecimal("1")
    two  = BigDecimal("2")
    x = -x if neg = x < 0
    if x > (twopi = two * BigMath.PI(prec))
      if x > 30
        x %= twopi
      else
        x -= twopi while x > twopi
      end
    end
    x1   = x
    x2   = x.mult(x,n)
    sign = 1
    y    = x
    d    = y
    i    = one
    z    = one
    while d.nonzero? && ((m = n - (y.exponent - d.exponent).abs) > 0)
      m = BigDecimal.double_fig if m < BigDecimal.double_fig
      sign = -sign
      x1  = x2.mult(x1,n)
      i  += two
      z  *= (i-one) * i
      d   = sign * x1.div(z,m)
      y  += d
    end
    neg ? -y : y
  end

  # Computes the cosine of x to the specified number of digits of precision.
  #
  # If x is infinite or NaN, returns NaN.
  def cos(x, prec)
    raise ArgumentError, "Zero or negative precision for cos" if prec <= 0
    return BigDecimal("NaN") if x.infinite? || x.nan?
    n    = prec + BigDecimal.double_fig
    one  = BigDecimal("1")
    two  = BigDecimal("2")
    x = -x if x < 0
    if x > (twopi = two * BigMath.PI(prec))
      if x > 30
        x %= twopi
      else
        x -= twopi while x > twopi
      end
    end
    x1 = one
    x2 = x.mult(x,n)
    sign = 1
    y = one
    d = y
    i = BigDecimal("0")
    z = one
    while d.nonzero? && ((m = n - (y.exponent - d.exponent).abs) > 0)
      m = BigDecimal.double_fig if m < BigDecimal.double_fig
      sign = -sign
      x1  = x2.mult(x1,n)
      i  += two
      z  *= (i-one) * i
      d   = sign * x1.div(z,m)
      y  += d
    end
    y
  end

  # Computes the arctangent of x to the specified number of digits of precision.
  #
  # If x is NaN, returns NaN.
  def atan(x, prec)
    raise ArgumentError, "Zero or negative precision for atan" if prec <= 0
    return BigDecimal("NaN") if x.nan?
    pi = PI(prec)
    x = -x if neg = x < 0
    return pi.div(neg ? -2 : 2, prec) if x.infinite?
    return pi / (neg ? -4 : 4) if x.round(prec) == 1
    x = BigDecimal("1").div(x, prec) if inv = x > 1
    x = (-1 + sqrt(1 + x**2, prec))/x if dbl = x > 0.5
    n    = prec + BigDecimal.double_fig
    y = x
    d = y
    t = x
    r = BigDecimal("3")
    x2 = x.mult(x,n)
    while d.nonzero? && ((m = n - (y.exponent - d.exponent).abs) > 0)
      m = BigDecimal.double_fig if m < BigDecimal.double_fig
      t = -t.mult(x2,n)
      d = t.div(r,m)
      y += d
      r += 2
    end
    y *= 2 if dbl
    y = pi / 2 - y if inv
    y = -y if neg
    y
  end

  # Computes the value of e (the base of natural logarithms) raised to the
  # power of x, to the specified number of digits of precision.
  #
  # If x is infinite or NaN, returns NaN.
  #
  # BigMath::exp(BigDecimal.new('1'), 10).to_s
  # -> "0.271828182845904523536028752390026306410273E1"
  def exp(x, prec)
    raise ArgumentError, "Zero or negative precision for exp" if prec <= 0
    if x.infinite?
      if x < 0
        return BigDecimal("0", prec)
      else
        return BigDecimal("+Infinity", prec)
      end
    elsif x.nan?
      return BigDecimal("NaN", prec)
    end
    n    = prec + BigDecimal.double_fig
    one  = BigDecimal("1")
    x = -x if neg = x < 0
    x1 = one
    y  = one
    d  = y
    z  = one
    i  = 0
    while d.nonzero? && ((m = n - (y.exponent - d.exponent).abs) > 0)
      m = BigDecimal.double_fig if m < BigDecimal.double_fig
      x1  = x1.mult(x,n)
      i += 1
      z *= i
      d  = x1.div(z,m)
      y += d
    end
    if neg
      one.div(y, prec)
    else
      y.round(prec - y.exponent)
    end
  end

  # Computes the natural logarithm of x to the specified number of digits
  # of precision.
  #
  # Returns x if x is infinite or NaN.
  #
  def log(x, prec)
    raise ArgumentError, "Zero or negative argument for log" if x <= 0 || prec <= 0
    return x if x.infinite? || x.nan?
    one = BigDecimal("1")
    two = BigDecimal("2")
    n  = prec + BigDecimal.double_fig
    if (expo = x.exponent) < 0 || expo >= 3
      x = x.mult(BigDecimal("1E#{-expo}"), n)
    else
      expo = nil
    end
    x  = (x - one).div(x + one,n)
    x2 = x.mult(x,n)
    y  = x
    d  = y
    i = one
    while d.nonzero? && ((m = n - (y.exponent - d.exponent).abs) > 0)
      m = BigDecimal.double_fig if m < BigDecimal.double_fig
      x  = x2.mult(x,n)
      i += two
      d  = x.div(i,m)
      y += d
    end
    y *= two
    if expo
      y += log(BigDecimal("10"),prec) * BigDecimal(expo.to_s)
    end
    y
  end

  # Computes the value of pi to the specified number of digits of precision.
  def PI(prec)
    raise ArgumentError, "Zero or negative argument for PI" if prec <= 0
    n      = prec + BigDecimal.double_fig
    zero   = BigDecimal("0")
    one    = BigDecimal("1")
    two    = BigDecimal("2")

    m25    = BigDecimal("-0.04")
    m57121 = BigDecimal("-57121")

    pi     = zero

    d = one
    k = one
    w = one
    t = BigDecimal("-80")
    while d.nonzero? && ((m = n - (pi.exponent - d.exponent).abs) > 0)
      m = BigDecimal.double_fig if m < BigDecimal.double_fig
      t   = t*m25
      d   = t.div(k,m)
      k   = k+two
      pi  = pi + d
    end

    d = one
    k = one
    w = one
    t = BigDecimal("956")
    while d.nonzero? && ((m = n - (pi.exponent - d.exponent).abs) > 0)
      m = BigDecimal.double_fig if m < BigDecimal.double_fig
      t   = t.div(m57121,n)
      d   = t.div(k,m)
      pi  = pi + d
      k   = k+two
    end
    pi
  end

  # Computes e (the base of natural logarithms) to the specified number of
  # digits of precision.
  def E(prec)
    raise ArgumentError, "Zero or negative precision for E" if prec <= 0
    n    = prec + BigDecimal.double_fig
    one  = BigDecimal("1")
    y  = one
    d  = y
    z  = one
    i  = 0
    while d.nonzero? && ((m = n - (y.exponent - d.exponent).abs) > 0)
      m = BigDecimal.double_fig if m < BigDecimal.double_fig
      i += 1
      z *= i
      d  = one.div(z,m)
      y += d
    end
    y
  end
end
