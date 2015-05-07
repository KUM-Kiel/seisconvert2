class Time
  def to_seed_btime
    t = self.utc
    [t.year, t.yday, t.hour, t.min, t.sec, 0, (t.subsec * 10000).round].pack 'S>S>CCCCS>'
  end

  def to_seed_time
    t = self.utc
    "%04d,%03d,%02d:%02d:%02d.%04d~" % [t.year, t.yday, t.hour, t.min, t.sec, t.usec / 100]
    #utc.strftime "%Y,%j,%H:%M:%S.%4N~"
  end
end

class String
  def as_seed_btime
    t = self.unpack 'S>S>CCCCS>'
    Time.utc(t[0], 1, 1, t[2], t[3], t[4], t[6] * 100) + (t[1] - 1) * 86400
  end
end

module Seed
  # Field Volume Identifier Blockette
  class Blockette5
    def initialize start_time = Time.now
      @start_time = start_time
    end

    def to_s
      s = '02.412' <<
        @start_time.to_seed_time
      "%03d%04d%s" % [5, 7 + s.length, s]
    end
  end

  # Volume Identifier Blockette
  class Blockette10
    def initialize start_time, end_time, organization, label
      @start_time = start_time
      @end_time = end_time
      @organization = organization
      @label = label
    end

    def to_s
      s = '02.412' <<
        @start_time.to_seed_time <<
        @end_time.to_seed_time <<
        Time.now.to_seed_time <<
        (@organization + '~') <<
        (@label + '~')
      "%03d%04d%s" % [10, 7 + s.length, s]
    end
  end

  # Volume Station Header Index Blockette
  class Blockette11
    def initialize stations
      @stations = stations
    end

    def to_s
      s = ("%03d" % @stations.length)
      @stations.each do |station|
        s << station.code << ("%06d" % station.sequence_number)
      end
      "%03d%04d%s" % [11, 7 + s.length, s]
    end
  end

  # Data Format Dictionary Blockette
  class Blockette30
    def initialize name, identifier, family, keys
      @name = name
      @identifier = identifier
      @family = family
      @keys = keys
    end

    def to_s
      s = (@name + "~") <<
        ("%04d%03d" % [@identifier, @family]) <<
        ("%02d" % @keys.length)
      @keys.each do |k|
        s << k << "~"
      end
      "%03d%04d%s" % [30, 7 + s.length, s]
    end
  end

  # Generic Abbreviation Blockette
  class Blockette33
    def initialize code, content
      @code = code
      @content = content
    end

    def to_s
      s = ("%03d" % @code) << @content << "~"
      "%03d%04d%s" % [33, 7 + s.length, s]
    end
  end

  # Units Abbreviations Blockette
  class Blockette34
    def initialize code, name, description
      @code = code
      @name = name
      @description = description
    end

    def to_s
      s = ("%03d" % @code) <<
        @name << "~" <<
        @description << "~"
      "%03d%04d%s" % [34, 7 + s.length, s]
    end
  end

  # FIR Dictionary Blockette
  class Blockette41
    def initialize key, name, h, unit
      @key = key
      @name = name
      @h = h
      @unit = unit
    end

    def to_s
      s = ("%04d" % @key) <<
        @name << "~" <<
        "A" <<
        ("%03d%03d" % [@unit, @unit]) <<
        ("%04d" % @h.length) <<
        @h.map{|h| "%+.7E" % h}.join("")
      "%03d%04d%s" % [41, 7 + s.length, s]
    end
  end

  # Station Identifier Blockette
  class Blockette50
    def initialize
    end

    def to_s
      s = ''
      "%03d%04d%s" % [50, 7 + s.length, s]
    end
  end

  # Data Only SEED Blockette
  class Blockette1000
    def to_s
      "\003\350\000\000\003\001\014\000" # Exponent is always 12
    end
  end
end
