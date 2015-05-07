class Time
  # Converts the time into a BCD string.
  #
  # The year must be between 2000 and 2099 due to limitations in the format.
  #
  # See also String#as_bcd.
  def to_bcd
    t = self.utc
    raise RangeError unless 2000 <= t.year && t.year <= 2099
    [t.hour, t.min, t.sec, t.day, t.month, t.year - 2000].map{|x| "%02d" % x}.pack("H2"*6)
  end
end

class String
  # Interprets the string as BCD string and converts it to a time.
  #
  # If the string is not a valid BCD string, +nil+ is returned.
  #
  # See also Time#to_bcd.
  def as_bcd
    if self == "\0\0\0\0\0\0"
      return nil
    end
    begin
      t = self.unpack("H2"*6).map{|d| d.to_i}
      Time.utc(t[5] + 2000, t[4], t[3], t[0], t[1], t[2])
    rescue
      nil
    end
  end
end
