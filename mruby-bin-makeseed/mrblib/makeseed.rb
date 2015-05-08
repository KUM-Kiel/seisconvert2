class Makeseed
  class Channel
    B1k = Seed::Blockette1000.new.to_s

    attr_accessor :time

    def initialize name, rate, file, skew = nil
      @name = name
      @sample_rate = rate
      @sample_time = 1.0 / rate
      @file = file
      @skew = skew
      @time = nil
      @record = nil
      @next_sequence_number = 1
    end

    def sample i
      return unless @time
      if not @record
        r = Seed::DataRecord.new
        r.channel_name = @name
        r.sequence_number = @next_sequence_number
        r.sample_rate = @sample_rate
        r.add_blockette B1k
        if @skew
          r.set_start_time @time, @skew.correction(@time)
        else
          r.set_start_time @time
        end
        @record = r
        @next_sequence_number += 1
      end
      @record << i
      @time += @sample_time
      if @record.full
        @file.write @record.to_s
        @record = nil
      end
    end

    def lost n
      if @record
        @file.write @record.to_s
      end
      @record = nil
      @time += n * @sample_time
    end

    def done
      if @record
        @file.write @record.to_s
      end
    end
  end

  def print_percent p, n = true
    a = (p / 5.0).round
    print "[#{'#' * a}#{' ' * (20 - a)}] #{'%3d' % p} %#{n ? "\n" : "\r"}"
  end

  def filename template
    template.gsub(/%./) do |x|
      if x == '%T'
        t = @sd.start_time
        '%04d%02d%02dT%02d%02d%02dZ' % [t.year, t.month, t.day, t.hour, t.min, t.sec]
      elsif x == '%R'
        @sd.recorder_id
      elsif x == '%C'
        @channel_name
      elsif x == '%%'
        '%'
      else
        raise "Invalid replacement #{x}"
      end
    end
  end

  def time_format t
    '%04d-%02d-%02d %02d:%02d:%02d UTC' % [t.year, t.month, t.day, t.hour, t.min, t.sec]
  end

  def time_format_usec t
    '%04d-%02d-%02d %02d:%02d:%02d.%06d UTC' % [t.year, t.month, t.day, t.hour, t.min, t.sec, t.usec]
  end

  def log s
    puts(s)
    @logfile.puts(s) if @logfile
  end

  def run file, options = {}
    begin
      if file
        sd = KumSd::File.new file
        @sd = sd
        now = Time.now

        logfilename = filename '%R/%T/events.txt'
        mkdir_p File.dirname logfilename
        @logfile = File.open(logfilename, "wb")

        [
          "Processing '#{file}'",
          "=============================================",
          "    Recorder ID: #{sd.recorder_id}",
          "         RTC ID: #{sd.rtc_id}",
          "        Comment: '#{sd.comment}'",
          "=============================================",
          "  Start Address: #{sd.start_address}",
          "    End Address: #{sd.end_address}",
          "============================================="
        ].each{|s| log s}
        [
          "RTC Report",
          "=============================================",
          "      Sync Time: #{sd.sync_time.utc}",
          "    Sync Offset: #{'%+d µs' % [sd.sync_time_skew * 1_000_000]}",
          "      Skew Time: #{sd.skew_time.utc}",
          "    Skew Offset: #{'%+d µs' % [sd.skew_time_skew * 1_000_000]}",
          "=============================================",
          "    Clock Drift: #{'%+.3f ppm' % [sd.skew.ppm]}",
          "   Current Time: #{now.utc}",
          " Current Offset: #{'%+d µs' % [sd.skew.correction(now) * 1_000_000]}",
          "=============================================",
          "     Start Time: #{sd.start_time}",
          "   Start Offset: #{'%+d µs' % [sd.skew.correction(sd.start_time) * 1_000_000]}",
          "       End Time: #{sd.end_time}",
          "     End Offset: #{'%+d µs' % [sd.skew.correction(sd.end_time) * 1_000_000]}",
          "============================================="
        ].each{|s| log s} if sd.skew

        channels = sd.channels.map do |name|
          @channel_name = name
          fn = filename '%R/%T/%C.seed'
          mkdir_p File.dirname fn
          c = Channel.new name, sd.sample_rate, File.open(fn, "wb"), sd.skew
          log "Creating #{fn}"
          c
        end
        log "============================================="

        edname = filename '%R/%T/engeneeringdata.csv'
        mkdir_p File.dirname edname
        ed = File.open(edname, "wb")
        edata = {vbat: 0, humi: 0, temp: 0}
        ed.puts "Unix Timestamp;Battery Voltage [V];Humidity [%];Temperature [°C]"

        [
          "Creating #{edname}",
          "============================================="
        ].each{|s| log s}

        percent = 0
        print_percent 0, false
        sd.each_frame do |f|
          if f.is_a? KumSd::ControlFrame
            if f.is_a? KumSd::Timestamp
              channels.each do |c|
                c.time = f.time
              end
            elsif f.is_a? KumSd::LostFrames
              @logfile.puts "#{time_format_usec(channels[0].time)}: #{f.lost} lost frames."
              channels.each do |c|
                c.lost f.lost
              end
            elsif f.is_a? KumSd::VoltageHumidity
              edata[:vbat] = f.voltage
              edata[:humi] = f.humidity
            elsif f.is_a? KumSd::Temperature
              edata[:temp] = f.temperature
              ed.puts "#{channels[0].time.to_i};#{edata[:vbat]};#{edata[:humi]};#{edata[:temp]}"
            end
          else
            (0...f.length).each do |i|
              channels[i].sample f[i]
            end
          end
          if percent != sd.percent
            percent = sd.percent
            print_percent percent, false
          end
        end
        channels.each do |c|
          c.done
        end
        print_percent 100
      else
        puts 'Usage: makeseed /dev/sdX'
      end
    rescue Exception => e
      puts e.inspect#.message
    end
  end
end
