class SampleTimestamp
  attr_accessor :sample_number, :time

  def initialize sample_number, time
    self.sample_number = sample_number
    self.time = time
  end
end
