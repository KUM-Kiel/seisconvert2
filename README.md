# This repository is deprecated!

Seisconvert 2 has been replaced by [6D6-Compat](https://github.com/KUM-Kiel/6d6-compat).
Please use the newest version there!

# Seisconvert 2

This is *Seisconvert 2*, a collection of tools for reading and converting seismic formats.

Seisconvert 2 provides tools for converting the KUM-Y and Muk1 formats, which are produced by KUM’s OBSs, into standard seismic formats like MiniSEED and SEG-Y.

Seisconvert 2 is based on [Mruby](https://github.com/mruby/mruby).

## Install

This assumes you are using Ubuntu or LinuxMint.
For other systems the dependencies must be installed by other means.

```text
$ sudo apt-get update
$ sudo apt-get install git ruby bison build-essential
$ git clone --recursive https://github.com/KUM-Kiel/seisconvert2.git
$ cd seisconvert2
$ make
$ sudo make install
```

This will install the tools `makeseed` and `makezft` to `/usr/local/bin` with the appropriate permissions to read SD cards.
If `/usr/local/bin` is not in your `$PATH`, you will have to add it.

## License

This Software is licensed under the GNU GPL.
See [LICENSE](https://github.com/KUM-Kiel/seisconvert2/blob/master/LICENSE) for more Info.

Mruby itself and some of the gems are licensed under an MIT license.
