module Makeseed
  class Channel
    B1k = Seed::Blockette1000.new.to_s

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

    def time t
      @time = t
    end

    def done
      if @record
        @file.write @record.to_s
      end
    end
  end

  def self.print_percent p, n = true
    a = (p / 5.0).round
    print "[#{'#' * a}#{' ' * (20 - a)}] #{'%3d' % p} %#{n ? "\n" : "\r"}"
  end

  def self.run file
    begin
      if file
        sd = KumSd::File.new file
        now = Time.now

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
        ].each{|s| puts s}
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
        ].each{|s| puts s} if sd.skew

        #p sd
        t = sd.start_time
        folder = "%s/%04d.%02d.%02d.%02d.%02d.%02d" % [sd.recorder_id, t.year, t.month, t.day, t.hour, t.min, t.sec]
        mkdir_p folder
        channels = sd.channels.map do |name|
          c = Channel.new name, sd.sample_rate, File.open("#{folder}/#{name}.seed", "wb"), sd.skew
          puts "Creating #{folder}/#{name}.seed"
          c
        end
        puts "============================================="
        percent = 0
        print_percent 0, false
        sd.each_frame do |f|
          if f.is_a? KumSd::ControlFrame
            if f.is_a? KumSd::Timestamp
              channels.each do |c|
                c.time f.time
              end
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
