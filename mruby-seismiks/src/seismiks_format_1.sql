-- Schema for storing seismic data in a database.

BEGIN;

-- SQLite application_id and version
PRAGMA application_id = 1936542579;
PRAGMA user_version = 1;

-- This table stores chunks of seismic data.
-- Every chunk of data belongs to the channel referenced by "channel_id" and
-- covers samples from "start_offset" to "end_offset". "start offset" is the
-- offset of the first sample, so the first chunk of data in any channel has a
-- "start_offset" of 0. "end_offset" is "start_offset" plus the number of
-- samples in the chunk.
-- The format of data is determined by the channels "bit_depth" field.
-- It consists of the sample data stored as signed little-endian integers with
-- no padding. The bit depth determines the number of bytes for each integer.
-- Chunks should be about 100kB in size.
-- Every chunk is compressed with the LZ4 frame compression format.
-- See http://cyan4973.github.io/lz4/lz4_Frame_format.html
CREATE TABLE data (
  id INTEGER PRIMARY KEY,
  channel_id INTEGER,
  start_offset INTEGER,
  end_offset INTEGER,
  data BLOB);

-- This table stores channels of a recording.
-- Every channel has a name, belongs to a recording "recording_id" and is
-- produced using the clock signal "clock_id".
-- The channel contains "length" samples with the given "bit_depth".
-- Only 8, 16, 24 and 32 are allowed bit depths.
-- The comment may further describe the channnel. It might be a long name or
-- instrument description.
CREATE TABLE channels (
  id INTEGER PRIMARY KEY,
  name TEXT,
  length INTEGER,
  bit_depth INTEGER,
  clock_id INTEGER,
  recording_id INTEGER,
  gain REAL,
  comment TEXT);

-- This table stores clock signals used to create a digital signal.
-- Every channel has an associated clock, which controls the taking of samples.
-- If two channels have the same rate and are synchronized, they should use the
-- same clock.
CREATE TABLE clocks (
  id INTEGER PRIMARY KEY,
  rate REAL);

-- This table stores recordings.
-- Every recording has a name, an optional comment and is recorded by a station.
CREATE TABLE recordings (
  id INTEGER PRIMARY KEY,
  name TEXT,
  station_id INTEGER,
  comment TEXT);

-- This table stores real time clocks.
-- "name" should be a unique identifier, such as the model and serial number.
CREATE TABLE rtcs (
  id INTEGER PRIMARY KEY,
  name TEXT);

-- A clock signal can be measured and controlled by a real time clock.
-- This table stores times "time" as measured by the rtc "rtc_id" at sample
-- number "sample_number" of clock "clock_id".
-- Every clock should have at least one rtc reading for sample 0.
CREATE TABLE rtc_readings (
  id INTEGER PRIMARY KEY,
  clock_id INTEGER,
  rtc_id INTEGER,
  sample_number INTEGER,
  time TEXT);

-- This table stores the offset "offset" in seconds of the rtc "rtc_id" at time
-- "time" as measured by the rtc. Adding the offset to the measured time will
-- yield the actual time at this instant.
-- Several of this measurements can be used to correct data for clock skew.
CREATE TABLE rtc_offsets (
  id INTEGER PRIMARY KEY,
  rtc_id INTEGER,
  time TEXT,
  offset REAL);

-- This table stores stations.
CREATE TABLE stations (
  id INTEGER PRIMARY KEY,
  name TEXT,
  comment TEXT);

-- This table stores position traces of a given station "station_id" at given
-- times "time" measured by rtc "rtc_id".
-- "latitude" encodes the geographical latitude in degrees. North is positive,
-- the equator is zero.
-- "longitude" encodes the geographical longitude in degrees. East is positive,
-- the meridian passing through Greenwich is zero.
-- "height" encodes the height above sea level in meters. If the recorder is
-- submerged, this number is negative.
-- The rotations are specified in Euler z-x'-z" convention. Starting from a
-- coordinate system where x is east, y is north and z is up, "rotation1"
-- encodes a counterclockwise rotation in degrees around the z axis of this
-- coordinate system. If "rotation2" and "rotation3" are not given, the
-- resulting coordinate system is the recorders local coordinate system.
-- "rotation2" encodes a counterclockwise rotation in degrees around the x axis
-- of the coordinate system rotated by "rotation1".
-- "rotation3" encodes a counterclockwise rotation in degrees around the z axis
-- of the coordinate system rotated by "rotation1" and "rotation2".
-- If "rotation2" or "rotation3" are given, all three rotations should be given.
-- Any values, which are unknown or not needed, can be NULL.
CREATE TABLE positions (
  id INTEGER PRIMARY KEY,
  station_id INTEGER,
  time TEXT,
  rtc_id INTEGER,
  latitude REAL,
  longitude REAL,
  height REAL,
  rotation1 REAL,
  rotation2 REAL,
  rotation3 REAL,
  water_depth REAL,
  comment TEXT);

COMMIT;
