# qtg

This software is a time grapher for quartz watches. It uses a standard soundcard and microphone to listen to the quartz oscillation and compute its monthly drift.

what you need :
* A soundcard with a sample rate of 96000Hz minimum
* A microphone with a reasonably good with ultrasound (any electret should do)
* A very good system clock (the result cannot be better than your computer's clock), NTP is highly recommanded

![screenshot](doc/casio_a168_qtg.png)

## How does it work

As one might guess, the sound produced by a watch's quartz is very faint, you cannot detect it easily. This program use a bit of DSP magic to remove the noise and detect it.
This frequency is computed using the soundcard's clock, which is not perfect either. This program implement a soundcard frequency monitoring using the system's clock.
In summary :
1. Compare the watch's internal clock to the soundcard's clock
2. Compare the soundcard's clock to the systems clock
3. Compensate for the soundcard's clock drift.

The system's clock being very good on the long run we can use it to evaluate the drift of the soundcard's one, and then the first.

Concretely, the DSP pipeline is as follow :
watch -> microphone/soundcard -> bandpass around 32768Hz -> mixing by oscillator at 32760Hz -> low pass -> downsampling by a factor 4000 or such -> compute which frequency around 8hz has the most energy -> apply soundcard compensation to find the actual "physical" frequency

regarding the soundcard :
timestamp each new buffer and compute it's actual frequency using the system's clock using linear regression. There is a rolling buffer to only consider the last data.


## How good it is ?
I dont know… it mostly depends on the soundcard's clock stability.
It should be better than ±1sec/month with standard hardware and good condition (stable temperature and low load).

## compilation

You should have this installed
```sh
apt install libxinerama-dev \
              libxcursor-dev \
              libxrandr-dev \
              libxi-dev \
              libx11-dev \
              libglu1-mesa-dev \
              libxext-dev \
              pkg-config \
              autoconf \
              libtool \
              xorg-dev
```

I would advice to install most dependancy using `vcpkg`, it could be something like this :

```sh
git clone https://github.com/simonArchipoff/qtg.git
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
cd ../qtg
../vcpkg/vcpkg install
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
./build/qtg -l
```

## usage:
`qtg` start the program with the default soundcard
`qtg -l` give the list of the sound interfaces available on your system and their ID

`qtg -d ID` start the program with the soundcard indexed by ID.

`qtg --help` for other options, including internal parameters.

## Todo:

* Better (nicer) UI
  - be able to change unit (sec per day, sec per month, hz, ppm)
* Optimization of DSP (currently a lot of spurious computation is done)
  - switch to windowed correlation instead of fft
  - add phase drift to the computation for more precise result
* tweak it be able to use the envelope instead of bare signal frequency, then it will work with mecanical watches
