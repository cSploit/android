require_relative 'helper'

module Psych
  class TestJSONTree < TestCase
    def test_string
      assert_match(/(['"])foo\1/, Psych.to_json("foo"))
    end

    def test_symbol
      assert_match(/(['"])foo\1/, Psych.to_json(:foo))
    end

    def test_nil
      assert_match(/^null/, Psych.to_json(nil))
    end

    def test_int
      assert_match(/^10/, Psych.to_json(10))
    end

    def test_float
      assert_match(/^1.2/, Psych.to_json(1.2))
    end

    def test_hash
      hash = { 'one' => 'two' }
      json = Psych.to_json(hash)
      assert_match(/}$/, json)
      assert_match(/^\{/, json)
      assert_match(/['"]one['"]/, json)
      assert_match(/['"]two['"]/, json)
    end

    def test_list_to_json
      list = %w{ one two }
      json = Psych.to_json(list)
      assert_match(/]$/, json)
      assert_match(/^\[/, json)
      assert_match(/['"]one['"]/, json)
      assert_match(/['"]two['"]/, json)
    end
  end
end
