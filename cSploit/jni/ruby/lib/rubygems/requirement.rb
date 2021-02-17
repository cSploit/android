require "rubygems/version"

##
# A Requirement is a set of one or more version restrictions. It supports a
# few (<tt>=, !=, >, <, >=, <=, ~></tt>) different restriction operators.

class Gem::Requirement
  include Comparable

  OPS = { #:nodoc:
    "="  =>  lambda { |v, r| v == r },
    "!=" =>  lambda { |v, r| v != r },
    ">"  =>  lambda { |v, r| v > r  },
    "<"  =>  lambda { |v, r| v < r  },
    ">=" =>  lambda { |v, r| v >= r },
    "<=" =>  lambda { |v, r| v <= r },
    "~>" =>  lambda { |v, r| v = v.release; v >= r && v < r.bump }
  }

  quoted  = OPS.keys.map { |k| Regexp.quote k }.join "|"
  PATTERN = /\A\s*(#{quoted})?\s*(#{Gem::Version::VERSION_PATTERN})\s*\z/

  ##
  # Factory method to create a Gem::Requirement object.  Input may be
  # a Version, a String, or nil.  Intended to simplify client code.
  #
  # If the input is "weird", the default version requirement is
  # returned.

  def self.create input
    case input
    when Gem::Requirement then
      input
    when Gem::Version, Array then
      new input
    else
      if input.respond_to? :to_str then
        new [input.to_str]
      else
        default
      end
    end
  end

  ##
  # A default "version requirement" can surely _only_ be '>= 0'.
  #--
  # This comment once said:
  #
  # "A default "version requirement" can surely _only_ be '> 0'."

  def self.default
    new '>= 0'
  end

  ##
  # Parse +obj+, returning an <tt>[op, version]</tt> pair. +obj+ can
  # be a String or a Gem::Version.
  #
  # If +obj+ is a String, it can be either a full requirement
  # specification, like <tt>">= 1.2"</tt>, or a simple version number,
  # like <tt>"1.2"</tt>.
  #
  #     parse("> 1.0")                 # => [">", "1.0"]
  #     parse("1.0")                   # => ["=", "1.0"]
  #     parse(Gem::Version.new("1.0")) # => ["=,  "1.0"]

  def self.parse obj
    return ["=", obj] if Gem::Version === obj

    unless PATTERN =~ obj.to_s
      raise ArgumentError, "Illformed requirement [#{obj.inspect}]"
    end

    [$1 || "=", Gem::Version.new($2)]
  end

  ##
  # An array of requirement pairs. The first element of the pair is
  # the op, and the second is the Gem::Version.

  attr_reader :requirements #:nodoc:

  ##
  # Constructs a requirement from +requirements+. Requirements can be
  # Strings, Gem::Versions, or Arrays of those. +nil+ and duplicate
  # requirements are ignored. An empty set of +requirements+ is the
  # same as <tt>">= 0"</tt>.

  def initialize *requirements
    requirements = requirements.flatten
    requirements.compact!
    requirements.uniq!

    requirements << ">= 0" if requirements.empty?
    @none = (requirements == ">= 0")
    @requirements = requirements.map! { |r| self.class.parse r }
  end

  def none?
    @none ||= (to_s == ">= 0")
  end

  def as_list # :nodoc:
    requirements.map { |op, version| "#{op} #{version}" }
  end

  def hash # :nodoc:
    requirements.hash
  end

  def marshal_dump # :nodoc:
    [@requirements]
  end

  def marshal_load array # :nodoc:
    @requirements = array[0]
  end

  def prerelease?
    requirements.any? { |r| r.last.prerelease? }
  end

  def pretty_print q # :nodoc:
    q.group 1, 'Gem::Requirement.new(', ')' do
      q.pp as_list
    end
  end

  ##
  # True if +version+ satisfies this Requirement.

  def satisfied_by? version
    requirements.all? { |op, rv| OPS[op].call version, rv }
  end

  def to_s # :nodoc:
    as_list.join ", "
  end

  def <=> other # :nodoc:
    to_s <=> other.to_s
  end
end

# :stopdoc:
# Gem::Version::Requirement is used in a lot of old YAML specs. It's aliased
# here for backwards compatibility. I'd like to remove this, maybe in RubyGems
# 2.0.

::Gem::Version::Requirement = ::Gem::Requirement
# :startdoc:

