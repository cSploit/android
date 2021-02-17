#++
# Copyright (C) 2004 Mauricio Julio Fern�ndez Pradier
# See LICENSE.txt for additional licensing information.
#--

##
# Class for reading entries out of a tar file

class Gem::Package::TarReader::Entry

  ##
  # Header for this tar entry

  attr_reader :header

  ##
  # Creates a new tar entry for +header+ that will be read from +io+

  def initialize(header, io)
    @closed = false
    @header = header
    @io = io
    @orig_pos = @io.pos
    @read = 0
  end

  def check_closed # :nodoc:
    raise IOError, "closed #{self.class}" if closed?
  end

  ##
  # Number of bytes read out of the tar entry

  def bytes_read
    @read
  end

  ##
  # Closes the tar entry

  def close
    @closed = true
  end

  ##
  # Is the tar entry closed?

  def closed?
    @closed
  end

  ##
  # Are we at the end of the tar entry?

  def eof?
    check_closed

    @read >= @header.size
  end

  ##
  # Full name of the tar entry

  def full_name
    if @header.prefix != "" then
      File.join @header.prefix, @header.name
    else
      @header.name
    end
  end

  ##
  # Read one byte from the tar entry

  def getc
    check_closed

    return nil if @read >= @header.size

    ret = @io.getc
    @read += 1 if ret

    ret
  end

  ##
  # Is this tar entry a directory?

  def directory?
    @header.typeflag == "5"
  end

  ##
  # Is this tar entry a file?

  def file?
    @header.typeflag == "0"
  end

  ##
  # The position in the tar entry

  def pos
    check_closed

    bytes_read
  end

  ##
  # Reads +len+ bytes from the tar file entry, or the rest of the entry if
  # nil

  def read(len = nil)
    check_closed

    return nil if @read >= @header.size

    len ||= @header.size - @read
    max_read = [len, @header.size - @read].min

    ret = @io.read max_read
    @read += ret.size

    ret
  end

  ##
  # Rewinds to the beginning of the tar file entry

  def rewind
    check_closed

    raise Gem::Package::NonSeekableIO unless @io.respond_to? :pos=

    @io.pos = @orig_pos
    @read = 0
  end

end

