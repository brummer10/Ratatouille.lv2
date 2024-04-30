# Ratatouille.lv2

![Ratatouille](https://github.com/brummer10/Ratatouille.lv2/blob/main/Ratatouille.png?raw=true)

Ratatouille is a Neural Model loader and mixer for Linux/Windows.

It can load two models, which can be [*.nam files](https://tonehunt.org/all) with the
[Neural Amp Modeler](https://github.com/sdatkinson/NeuralAmpModelerCore) module, or 
[*.json or .aidax files](https://cloud.aida-x.cc/all) with the 
[RTNeural](https://github.com/jatinchowdhury18/RTNeural) module.

You can also load just a single model file, in that case the "Blend" control will do nothing.
When you've loaded a second model, the "Blend" control will blend between the two models and
mix them to simulate your specific tone.

To round up your sound you can load two Impulse Response Files and mix them to your needs.

Ratatouille.lv2 supports resampling when needed to match the expected sample rate of the 
loaded models. Both models and the IR Files may have different expectations regarding the sample rate.

## Dependencies

- libsndfile1-dev
- libfftw3-dev
- libcairo2-dev
- libx11-dev
- lv2-dev

[![build](https://github.com/brummer10/Ratatouille.lv2/actions/workflows/build.yml/badge.svg)](https://github.com/brummer10/Ratatouille.lv2/actions/workflows/build.yml)

## Building from source code

- git clone https://github.com/brummer10//Ratatouille.lv2.git
- cd Ratatouille.lv2
- git submodule update --init --recursive
- make
- make install # will install into ~/.lv2 ... AND/OR....
- sudo make install # will install into /usr/lib/lv2
