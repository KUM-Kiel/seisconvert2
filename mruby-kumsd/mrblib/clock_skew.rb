class ClockSkew
  def initialize t1, skew1, t2, skew2
    @m = 1.respond_to?(:to_r) ? :to_r : :to_f
    raise(TypeError, 't1 must be a Time') unless t1.is_a? Time
    raise(TypeError, 'skew1 must be Numeric and real') unless (skew1.is_a?(Numeric))
    raise(TypeError, 't2 must be a Time') unless t2.is_a? Time
    raise(TypeError, 'skew2 must be Numeric and real') unless (skew2.is_a?(Numeric))
    raise(ArgumentError, 't1 and t2 must be distinct') unless t1.utc.send(@m) != t2.utc.send(@m)

    @t1 = t1.utc.send(@m)
    @skew1 = skew1.send(@m)
    @t2 = t2.utc.send(@m)
    @skew2 = skew2.send(@m)
  end

  def correction t
    raise(TypeError, 't must be a Time') unless t.is_a? Time

    (@skew1 + (@skew2 - @skew1) * (t.utc.send(@m) - @t1) / (@t2 - @t1)).to_f
  end

  def ppm
    (1_000_000 * (@skew2 - @skew1) / (@t2 - @t1)).to_f
  end
end
