# qtg

This software is a time grapher for quartz watches. It uses a standard soundcard and microphone to listen to the quartz oscillation and compute its monthly drift.

what you need :
* A soundcard with a sample rate of 96000Hz minimum
* A microphone reasonably good with ultrasound (any COTS electret should do)
* A very good system clock (the result cannot be better than your computer's clock), NTP is highly recommanded

![screenshot](doc/casio_a168_qtg.png)

## How does it work

As one might guess, the sound produced by a watch's quartz is very faint, you cannot detect it easily. This program use a bit of DSP magic to remove the noise and detect it.
This frequency is computed using the soundcard's clock, which is not perfect either. This program implement a soundcard frequency monitoring using the system's clock.
In summary :
1. Compare the watch's internal clock to the soundcard's clock
2. Compare the soundcard's clock to the systems clock
3. Compensate for the soundcard's clock drift.

An other way to see it, is to consider 3 clocks :
1. the watch, we assume good stability, and unknown drift, to be determined
2. the soundcard, we assume good stability, and an unknown drift, to be determined
3. The system's clock, with very poor stability, but virtually no drift on the long run.

The last clock being very good on the long run we can use it to evaluate the drift of the second one, and then the first.


Concretely, the DSP pipeline is as follow :
watch -> microphone/soundcard -> bandpass around 32768 -> mixing by oscillator at 32760Hz -> low pass at 16Hz -> downsampling by a factor 1000 or such -> compute which frequency around 8hz has the most energy -> apply soundcard compensation to find the actual "physical" frequency

regarding the soundcard :
timestamp each new buffer and compute it's actual frequency using the system's clock. Actually take a bunch of measures to estimate the precision.


## How good it is ?
I dont know… it mostly depends on the soundcard's clock stability.
It should be better than ±1sec/month with standard hardware and good condition (stable temperature and low load).
+1sec/month for a quartz watch is just 0.012Hz too fast, it's one extra period every 83 seconds.

## compilation

In the system you should have opengl, alsa and such installed, I'll do the exact list later.
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
`qtg -l` give the list of the sound interfaces available on your system and their ID

`qtg -d ID` start the program with the soundcard indexed by ID.
Tip : you can snap your finger in front of the microphone while looking at the temporal plot to make sure you selected the proper microphone

`qtg --help` for other options

## Todo:

* Better (nicer) UI
* Smarter drift computation for soundcard. Here the software assume that it is constant.
* Optimization of DSP (currently a lot of spurious computation is done)
* Make all parameters configurable
* better software engeneering (this is mostly a proof of concept, if this were to become a real thing more work should go there)
* Tweak it to timegraph some tuning fork watches as well
