class MakeZft
  def print_percent p, n = true
    a = (p / 5.0).round
    print "[#{'#' * a}#{' ' * (20 - a)}] #{'%3d' % p} %#{n ? "\n" : "\r"}"
  end

  def filename template
    template.gsub(/%./) do |x|
      if x == '%T'
        t = @sd.start_time
        '%04d%02d%02dT%02d%02d%02dZ' % [t.year, t.month, t.day, t.hour, t.min, t.sec]
      elsif x == '%J'
        t = @sd.start_time
        '%04d.%03d.%02d.%02d.%02d' % [t.year, t.yday, t.hour, t.min, t.sec]
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

        logfilename = filename '%R.%J.events.txt'
        mkdir_p File.dirname logfilename
        @logfile = File.open(logfilename, "wb")

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

        t = nil
        while !t
          f = sd.read_frame
          if f.is_a? KumSd::Timestamp
            t = f.time
          end
        end

        start_time = t

        fn = filename '%R.%J.zft'
        mkdir_p File.dirname fn
        zft = Zft.new fn
        log "Creating #{fn}"
        log "============================================="

        edname = filename '%R.%J.engineeringdata.csv'
        mkdir_p File.dirname edname
        ed = File.open(edname, "wb")
        edata = {vbat: 0, humi: 0, temp: 0}
        ed.puts "Unix Timestamp;Battery Voltage [V];Humidity [%];Temperature [°C]"

        [
          "Creating #{logfilename}",
          "Creating #{edname}",
          "============================================="
        ].each{|s| log s}

        sd.channels.each_with_index do |c, i|
          if sd.skew
            zft.add_channel c,
              rate: sd.sample_rate,
              start_time: start_time,
              end_time: sd.end_time,
              sync_time: sd.sync_time,
              skew_time: sd.skew_time,
              skew: sd.skew_time_skew,
              channel_number: i + 1,
              channel_count: sd.channels.length,
              frame_count: sd.frame_count,
              comment: sd.comment
          else
            zft.add_channel c,
              rate: sd.sample_rate,
              start_time: start_time,
              end_time: sd.end_time,
              channel_number: i + 1,
              channel_count: sd.channels.length,
              frame_count: sd.frame_count,
              comment: sd.comment
          end
        end

        percent = 0
        print_percent 0, false
        sd.each_frame do |f|
          if f.is_a? KumSd::ControlFrame
            if f.is_a? KumSd::Timestamp
              t = f.time
              # Zft has no timestamps
            elsif f.is_a? KumSd::LostFrames
              @logfile.puts "#{time_format_usec(t)}: #{f.lost} lost frames."
              f.lost.times do
                zft.push_frame([2147483647] * sd.channels.length)
              end
            elsif f.is_a? KumSd::VoltageHumidity
              edata[:vbat] = f.voltage
              edata[:humi] = f.humidity
            elsif f.is_a? KumSd::Temperature
              edata[:temp] = f.temperature
              ed.puts "#{t.to_i};#{edata[:vbat]};#{edata[:humi]};#{edata[:temp]}"
            end
          else
            zft.push_frame f
          end
          if percent != sd.percent
            percent = sd.percent
            print_percent percent, false
          end
        end
        zft.close
        print_percent 100
      end
    rescue Exception => e
      puts e.inspect#.message
    end
  end
end
