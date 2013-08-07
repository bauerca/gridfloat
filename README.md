# gridfloat

Slice and dice GridFloat files from the command line.

## Installation

The following will produce the executable, `gridfloat` in
the repo. This program depends on libpng, readily available
from your friendly Linux distro package manager (You'll need
the development version. Currently, on Ubuntu 12.04, that is
"libpng12-dev").

```
> git clone https://github.com/bauerca/gridfloat.git
> cd gridfloat
> make
```

## Get some data

Grab some elevation data from
[The National Map Viewer](http://viewer.nationalmap.gov/viewer/) by
clicking on the light-blue down-arrow icon and following the
instructions, making sure to check the "Elevation" box.

When it comes time to select your specific elevation product, go
for the GridFloat format.

Once downloaded, unzip and check out the usage instructions
below. 


## Usage

The following can be obtained on the command line from
`./gridfloat -h`:

### Summary

Extract subgrids of data from a GridFloat file. GridFloat
is a simple format commonly used by the USGS for packaging
elevation data. A header file with geolocation metadata is
required for making queries based on latitude/longitude.

### Usage

```
gridfloat [options] FLOATFILE HEADERFILE
```

```
gridfloat [options] FLOAT_HEADER_PREFIX
```

where, for the first case, FLOATFILE should have
extension .flt and HEADERFILE .hdr. If both HEADERFILE
and FLOATFILE have the same filename prefix, then you
can use the second case.

### Basic options

```
  -h:  Print this help message.
  -i:  Print helpful info derived from GridFloat header file.
  -R:  Resolution of extraction. If a single integer is
       supplied, then the resolution is the same in both
       directions. Otherwise, the format is (by example):
       '-R 128x256'.
  -l:  Left bound for extraction.
  -r:  Right bound for extraction.
  -b:  Bottom bound for extraction.
  -t:  Top bound for extraction.
  -B:  Specify extraction bounds using a comma-separated
       list of the form LEFT,RIGHT,BOTTOM,TOP. E.g.
       '-B -123.112,-123.110,42.100,42.101'.
  -n:  Longitude midpoint of extraction. Must use with '-s'
       option. Example: '-n 42'.
  -w:  Latitude midpoint of extraction. Must use with '-s'
       option. Example: '-w 123' (notice there is no minus
       sign here).
  -p:  Lat/Lng midpoint of extraction. Must use with '-s'
       option. Example: '-p -123,42'.
  -s:  Size of box (in degrees) around midpoint. Used with '-p'
       and '-w','-n' options. Examples: '-s 1.0x1.0' or just '-s 1'.
  -T:  Transpose and invert along y before printing the array
       (so that a[i, j] gives longitude increasing with i and
       latitude increasing with j).
```

### PNG output options

```
  -o:  Output subgrid data to a file. Must match *.png.
  -A:  Azimuthal angle (in degrees) of view toward sun (for
       relief shading). 0 means sun is East, 90 North, etc.
       Default: 45 (NE).
  -P:  Polar angle (in degrees) of view toward sun (for
       relief shading). 0 means sun is on horizon, 90 when
       directly overhead. Default: 30.
```

### Examples

All of the following are equivalent and simply print data
to stdout.

```
  gridfloat -l -123 -r -122 -b 42 -t 43 file.{hdr,flt}
  gridfloat -B -123,-122,42,43 file.{hdr,flt}
  gridfloat -p -122.5,42.5 -s 1 file.{hdr,flt}
  gridfloat -w 122.5 -n 42.5 -s 1x1 file.{hdr,flt}
```
