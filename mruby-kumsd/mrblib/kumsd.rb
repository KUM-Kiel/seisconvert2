module KumSd
  # An open KUM SD card.
  class File
    attr_reader :start_address, :end_address, :start_time, :end_time
    attr_reader :sync_time, :sync_time_skew, :skew_time, :skew_time_skew, :skew
    attr_reader :channels, :sample_rate, :recorder_id, :rtc_id, :comment

    # Open the device file named +device+.
    # If it is just a name like +sdb+, +/dev/device+ will also be checked,
    # if the name does not exist.
    def initialize device
      if device.include? '/'
        @file = KumSdFile.new device
      else
        begin
          @file = KumSdFile.new device
        rescue Exception => e
          begin
            @file = KumSdFile.new "/dev/#{device}"
          rescue Exception => e2
            raise e
          end
        end
      end

      @start_address = 0
      @end_address = 0
      next_file
    end

    # Loads the next set of headers from the card and moves to the beginning of
    # the data.
    def next_file
      goto_address @end_address

      begin
        start_header = read_header
        end_header = read_header
      rescue
        raise 'This does not look like a KUM SD file.'
      end

      #p start_header
      #p end_header

      @start_address = start_header[:address]
      @end_address = end_header[:address]
      @start_time = start_header[:time]
      @end_time = end_header[:time]

      @channels = start_header[:channels]
      @sample_rate = start_header[:sample_rate]

      @file.channel_count = @channels.length
      @file.base_time = @start_time
      @file.last_address = @end_address

      @sync_time = start_header[:sync_time]
      @sync_time_skew = start_header[:skew] * 1e-6

      @skew_time = end_header[:sync_time]
      @skew_time_skew = end_header[:skew] * 1e-6

      if @sync_time && @skew_time
        @skew = ClockSkew.new(@sync_time, @sync_time_skew, @skew_time, @skew_time_skew)
      end

      @recorder_id = start_header[:recorder_id]
      @rtc_id = start_header[:rtc_id]
      @comment = start_header[:comment]

      goto_address @start_address
    end

    # Reads a 512 byte header beginning at the current file position.
    def read_header
      d = @file.read 512
      KumSd::parse_header d
    end

    # Reads and parses a frame at the current file position.
    def read_frame
      @file.read_frame
    end

    # Seeks to the given address in the file.
    # The address is the number of a 512 byte block.
    def goto_address address
      @file.goto_addr address
    end

    # Iterates over each frame of the current file.
    def each_frame
      goto_address @start_address
      @file.last_address = @end_address
      f = @file.read_frame
      while !f.is_a?(EndFrame)
        yield f
        f = @file.read_frame
      end
      self
    end

    def process &block
      @file.process(&block)
    end

    def percent
      @file.percent
    end

    def close
      @file.close
      @file = nil
    end
  end

  # Base class for control frames.
  class ControlFrame
  end

  # Stores a timestamp.
  class Timestamp < ControlFrame
    attr_accessor :time, :correction

    def initialize time, correction = nil
      @time = time
      @correction = correction
      @corrected = false
    end

    def correct!
      if @correction
        @time += @correction
        @corrected = (@corrected || 0) + @correction
        @correction = 0
      end
    end

    def corrected?
      @corrected
    end

    def real_time
      @time + (@corrected || 0) + (@correction || 0)
    end

    def inspect
      c = @correction ? (" (%+d µs)" % [((@corrected || 0) + @correction) * 1_000_000]) : ''
      t = time.utc
      s = nil
      if t.usec != 0
        s = "%04d-%02d-%02d %02d:%02d:%02d.%06d UTC" % [t.year, t.month, t.day, t.hour, t.min, t.sec, t.usec]
      else
        s = "%04d-%02d-%02d %02d:%02d:%02d UTC" % [t.year, t.month, t.day, t.hour, t.min, t.sec]
      end
      "[#{s}#{c}]"
    end
  end

  # Stores voltage in volts and humidity in percent.
  class VoltageHumidity < ControlFrame
    attr_accessor :voltage, :humidity

    # Initialize with voltage and humidity.
    def initialize voltage, humidity
      @voltage = voltage
      @humidity = humidity
    end
  end

  # Stores the temperature.
  class Temperature < ControlFrame
    attr_accessor :temperature

    def initialize temperature
      @temperature = temperature
    end
  end

  class LostFrames < ControlFrame
    attr_accessor :lost

    def initialize lost
      @lost = lost
    end
  end

  class Check < ControlFrame
    attr_accessor :token

    def initialize token
      @token = token
    end
  end

  class Reboot < ControlFrame
  end

  class EndFrame < ControlFrame
  end

  class FrameNumber < ControlFrame
    attr_accessor :number

    def initialize number
      @number = number
    end
  end
end

=begin
f = KumSd::File.new("sdb")

p [f.start_time, f.end_time]
p [f.start_address, f.end_address]
p [f.sync_time, f.sync_time_skew, f.skew_time, f.skew_time_skew]
puts "ClockSkew.new(Time.at(#{f.sync_time.to_i}), #{f.sync_time_skew.to_f}, Time.at(#{f.skew_time.to_i}), #{f.skew_time_skew.to_f})"

=begin
f.each_frame do |f|
  case f
  when KumSd::ControlFrame
    f.correct! if f.is_a? KumSd::Timestamp
    p f
  end
  if f.is_a? KumSd::EndFrame
    raise StopIteration
  end
end
=end
