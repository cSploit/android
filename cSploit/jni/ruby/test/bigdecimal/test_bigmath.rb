require_relative "testbase"
require "bigdecimal/math"

class TestBigMath < Test::Unit::TestCase
  include TestBigDecimalBase
  include BigMath
  N = 20
  PINF = BigDecimal("+Infinity")
  MINF = BigDecimal("-Infinity")
  NAN = BigDecimal("NaN")

  def test_const
    assert_in_delta(Math::PI, PI(N))
    assert_in_delta(Math::E, E(N))
  end

  def test_sqrt
    assert_in_delta(2**0.5, sqrt(BigDecimal("2"), N))
    assert_equal(10, sqrt(BigDecimal("100"), N))
    assert_equal(0.0, sqrt(BigDecimal("0"), N))
    assert_equal(0.0, sqrt(BigDecimal("-0"), N))
    assert_raise(FloatDomainError) {sqrt(BigDecimal("-1.0"), N)}
    assert_raise(FloatDomainError) {sqrt(NAN, N)}
    assert_raise(FloatDomainError) {sqrt(PINF, N)}
  end

  def test_sin
    assert_in_delta(0.0, sin(BigDecimal("0.0"), N))
    assert_in_delta(Math.sqrt(2.0) / 2, sin(PI(N) / 4, N))
    assert_in_delta(1.0, sin(PI(N) / 2, N))
    assert_in_delta(0.0, sin(PI(N) * 2, N))
    assert_in_delta(0.0, sin(PI(N), N))
    assert_in_delta(-1.0, sin(PI(N) / -2, N))
    assert_in_delta(0.0, sin(PI(N) * -2, N))
    assert_in_delta(0.0, sin(-PI(N), N))
    assert_in_delta(0.0, sin(PI(N) * 21, N))
    assert_in_delta(0.0, sin(PI(N) * 30, N))
    assert_in_delta(-1.0, sin(PI(N) * BigDecimal("301.5"), N))
  end

  def test_cos
    assert_in_delta(1.0, cos(BigDecimal("0.0"), N))
    assert_in_delta(Math.sqrt(2.0) / 2, cos(PI(N) / 4, N))
    assert_in_delta(0.0, cos(PI(N) / 2, N))
    assert_in_delta(1.0, cos(PI(N) * 2, N))
    assert_in_delta(-1.0, cos(PI(N), N))
    assert_in_delta(0.0, cos(PI(N) / -2, N))
    assert_in_delta(1.0, cos(PI(N) * -2, N))
    assert_in_delta(-1.0, cos(-PI(N), N))
    assert_in_delta(-1.0, cos(PI(N) * 21, N))
    assert_in_delta(1.0, cos(PI(N) * 30, N))
    assert_in_delta(0.0, cos(PI(N) * BigDecimal("301.5"), N))
  end

  def test_atan
    assert_equal(0.0, atan(BigDecimal("0.0"), N))
    assert_in_delta(Math::PI/4, atan(BigDecimal("1.0"), N))
    assert_in_delta(Math::PI/6, atan(sqrt(BigDecimal("3.0"), N) / 3, N))
    assert_in_delta(Math::PI/2, atan(PINF, N))
    assert_equal(BigDecimal("0.823840753418636291769355073102514088959345624027952954058347023122539489"),
                 atan(BigDecimal("1.08"), 72).round(72), '[ruby-dev:41257]')
  end

  def test_exp
    assert_in_epsilon(Math::E, exp(BigDecimal("1"), N))
    assert_in_epsilon(Math.exp(N), exp(BigDecimal("20"), N))
    assert_in_epsilon(Math.exp(40), exp(BigDecimal("40"), N))
    assert_in_epsilon(Math.exp(-N), exp(BigDecimal("-20"), N))
    assert_in_epsilon(Math.exp(-40), exp(BigDecimal("-40"), N))
    begin
      old_mode = BigDecimal.mode(BigDecimal::EXCEPTION_INFINITY)
      BigDecimal.mode(BigDecimal::EXCEPTION_INFINITY, false)
      assert(exp(BigDecimal::INFINITY, N).infinite?, "exp(INFINITY) is not an infinity")
    ensure
      #BigDecimal.mode(BigDecimal::EXCEPTION_INFINITY, old_mode)
    end
    assert_equal(0.0, exp(-BigDecimal::INFINITY, N))
  end

  def test_log
    assert_in_delta(0.0, log(BigDecimal("1"), N))
    assert_in_delta(1.0, log(E(N), N))
    assert_in_delta(Math.log(2.0), log(BigDecimal("2"), N))
    assert_in_delta(2.0, log(E(N)*E(N), N))
    assert_in_delta(Math.log(42.0), log(BigDecimal("42"), N))
    assert_in_delta(Math.log(1e-42), log(BigDecimal("1e-42"), N))
  end
end
