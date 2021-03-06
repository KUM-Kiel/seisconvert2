class Makeseed
  class Channel
    B1k = Seed::Blockette1000.new.to_s

    attr_reader :time
    def time= t
      @time = t
      @time_offset = 0
    end

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
        t = @time + @time_offset
        if @skew
          r.set_start_time t, @skew.correction(t)
        else
          r.set_start_time t
        end
        @record = r
        @next_sequence_number += 1
      end
      @record << i
      @time_offset += @sample_time
      if @record.full
        @file.write @record.to_s
        @record = nil
      end
    end

    def sample_buffer b
      return unless @time
      n = 0
      while n < b.length
        if not @record
          r = Seed::DataRecord.new
          r.channel_name = @name
          r.sequence_number = @next_sequence_number
          r.sample_rate = @sample_rate
          r.add_blockette B1k
          t = @time + @time_offset
          if @skew
            r.set_start_time t, @skew.correction(t)
          else
            r.set_start_time t
          end
          @record = r
          @next_sequence_number += 1
        end
        x = b.slice(b.length - n, n)
        i = @record.push_sample_buffer(x)
        @time_offset += i * @sample_time
        n += i
        if @record.full
          @file.write @record.to_s
          @record = nil
        end
      end
    end

    def lost n
      if @record
        @file.write @record.to_s
      end
      @record = nil
      @time_offset += n * @sample_time
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

  def duration_format d
    sec = d % 60
    d = (d - sec) / 60
    min = d % 60
    d = (d - min) / 60
    hours = d % 24
    d = (d - hours) / 24
    days = d
    if days == 0 && hours == 0 && min == 0
      '%ds' % [sec]
    elsif days == 0 && hours == 0
      '%dm %ds' % [min, sec]
    elsif days == 0
      '%dh %dm %ds' % [hours, min, sec]
    else
      '%dd %dh %dm %ds' % [days, hours, min, sec]
    end
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
        mkdir_p_unprivileged File.dirname logfilename
        @logfile = open_unprivileged(logfilename, "wb")

        [
          "Processing '#{file}'",
          "=============================================",
          "    Recorder ID: #{sd.recorder_id}",
          "         RTC ID: #{sd.rtc_id}",
          "        Comment: #{sd.comment.inspect}",
          "=============================================",
          "  Start Address: #{sd.start_address}",
          "    End Address: #{sd.end_address}",
          "============================================="
        ].each{|s| log s}
        [
          "RTC Report",
          "=============================================",
          "      Sync Time: #{time_format sd.sync_time.utc}",
          "    Sync Offset: #{'%+d µs' % [sd.sync_time_skew * 1_000_000]}",
          "      Skew Time: #{time_format sd.skew_time.utc}",
          "    Skew Offset: #{'%+d µs' % [sd.skew_time_skew * 1_000_000]}",
          "=============================================",
          "    Clock Drift: #{'%+.3f ppm' % [sd.skew.ppm]}",
          "   Current Time: #{time_format now.utc}",
          " Current Offset: #{'%+d µs' % [sd.skew.correction(now) * 1_000_000]}",
          "=============================================",
          "     Start Time: #{time_format sd.start_time}",
          "   Start Offset: #{'%+d µs' % [sd.skew.correction(sd.start_time) * 1_000_000]}",
          "       End Time: #{time_format sd.end_time}",
          "     End Offset: #{'%+d µs' % [sd.skew.correction(sd.end_time) * 1_000_000]}",
          "============================================="
        ].each{|s| log s} if sd.skew

        log "Creating #{logfilename}"

        if !options[:fast]
          edname = filename '%R/%T/engineeringdata.csv'
          mkdir_p_unprivileged File.dirname edname
          ed = open_unprivileged(edname, "wb")
          edata = {vbat: 0, humi: 0, temp: 0}
          ed.puts "Unix Timestamp;Battery Voltage [V];Humidity [%];Temperature [°C]"

          [
            "Creating #{edname}",
            "============================================="
          ].each{|s| log s}

          channels = sd.channels.map(&:to_s).map do |name|
            @channel_name = name
            fn = filename '%R/%T/%C.seed'
            mkdir_p_unprivileged File.dirname fn
            c = Channel.new name, sd.sample_rate, open_unprivileged(fn, "wb"), sd.skew
            log "Creating #{fn}"
            c
          end
          log "============================================="

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
          log "============================================="

          channels = {}
          sd.channels.each do |name|
            @channel_name = name.to_s
            fn = filename '%R/%T/%C.seed'
            mkdir_p_unprivileged File.dirname fn
            channels[name] = Channel.new name.to_s, sd.sample_rate, open_unprivileged(fn, "wb"), sd.skew
            log "Creating #{fn}"
          end
          log "============================================="

          percent = 0
          print_percent 0, false

          sd.process do |f|
            if f.is_a? SampleTimestamp
              if f.sample_number == 0
                channels.each_value do |c|
                  c.time = f.time
                end
              end
            elsif f.is_a? SampleBuffer
              channels[f.channel].sample_buffer(f)
            end
            if percent != sd.percent
              percent = sd.percent
              print_percent percent, false
            end
          end

          channels.each_value do |c|
            c.done
          end
          print_percent 100
        end
      else
        puts 'Usage: makeseed /dev/sdX'
      end
    rescue Exception => e
      puts e.inspect#.message
    end
  end
end
