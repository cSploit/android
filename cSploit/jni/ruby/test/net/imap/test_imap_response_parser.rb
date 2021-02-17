require "net/imap"
require "test/unit"

class IMAPResponseParserTest < Test::Unit::TestCase
  def setup
    @do_not_reverse_lookup = Socket.do_not_reverse_lookup
    Socket.do_not_reverse_lookup = true
    if Net::IMAP.respond_to?(:max_flag_count)
      @max_flag_count = Net::IMAP.max_flag_count
      Net::IMAP.max_flag_count = 3
    end
  end

  def teardown
    Socket.do_not_reverse_lookup = @do_not_reverse_lookup
    if Net::IMAP.respond_to?(:max_flag_count)
      Net::IMAP.max_flag_count = @max_flag_count
    end
  end

  def test_flag_list_safe
    parser = Net::IMAP::ResponseParser.new
    response = lambda {
      $SAFE = 1
      parser.parse(<<EOF.gsub(/\n/, "\r\n").taint)
* LIST (\\HasChildren) "." "INBOX"
EOF
    }.call
    assert_equal [:Haschildren], response.data.attr
  end

  def test_flag_list_too_many_flags
    parser = Net::IMAP::ResponseParser.new
    assert_nothing_raised do
      3.times do |i|
      parser.parse(<<EOF.gsub(/\n/, "\r\n").taint)
* LIST (\\Foo#{i}) "." "INBOX"
EOF
      end
    end
    assert_raise(Net::IMAP::FlagCountError) do
      parser.parse(<<EOF.gsub(/\n/, "\r\n").taint)
* LIST (\\Foo3) "." "INBOX"
EOF
    end
  end

  def test_flag_list_many_same_flags
    parser = Net::IMAP::ResponseParser.new
    assert_nothing_raised do
      100.times do
      parser.parse(<<EOF.gsub(/\n/, "\r\n").taint)
* LIST (\\Foo) "." "INBOX"
EOF
      end
    end
  end

  def test_resp_text_code
    parser = Net::IMAP::ResponseParser.new
    response = parser.parse(<<EOF.gsub(/\n/, "\r\n").taint)
* OK [CLOSED] Previous mailbox closed.
EOF
    assert_equal "CLOSED", response.data.code.name
  end
end
