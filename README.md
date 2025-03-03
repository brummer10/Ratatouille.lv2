# Ratatouille.lv2 [![build](https://github.com/brummer10/Ratatouille.lv2/actions/workflows/build.yml/badge.svg)](https://github.com/brummer10/Ratatouille.lv2/actions/workflows/build.yml)


<p align="center">
    <img src="https://github.com/brummer10/Ratatouille.lv2/blob/main/Ratatouille.png?raw=true" />
</p>

Ratatouille is a Neural Model loader and mixer for Linux/Windows.

It can load two models, which can be [*.nam files](https://tonehunt.org/models?tags%5B0%5D=nam) with the
[Neural Amp Modeler](https://github.com/sdatkinson/NeuralAmpModelerCore) module, or 
[*.json or .aidax files](https://tonehunt.org/models?tags%5B0%5D=aida-x) with the 
[RTNeural](https://github.com/jatinchowdhury18/RTNeural) module.

You can also load just a single model file, in that case the "Blend" control will do nothing.
When you've loaded a second model, the "Blend" control will blend between the two models and
mix them to simulate your specific tone.

Ratatouille using parallel processing for the neural models,
so, loading a second neural model wouldn't be remarkable on the dsp load.

Optional, Ratatouille could run the complete process in buffered mode. That reduce the dsp load
even more. The resulting latency will be reported to the host so that it could be compensated. 
For information the resulting latency will be shown on the GUI.

The "Delay" control could add a small delay to overcome phasing issues,
or to add some color/reverb to the sound. 
The 'ctrl' key activate fine tuning to dial in a sample accurate delay.

Optional, Ratatouille could detect the phase offset and compensate it internal.

To round up your sound you can load two Impulse Response Files and mix them to your needs.
IR-files could be normalised on load, so that they didn't influence the loudness. 

Ratatouille.lv2 supports resampling when needed to match the expected sample rate of the 
loaded models. Both models and the IR Files may have different expectations regarding the sample rate.

## Packaging Status

[![Packaging status](https://repology.org/badge/vertical-allrepos/ratatouille.lv2.svg?columns=3)](https://repology.org/project/ratatouille.lv2/versions)

## MOD Desktop

For using it on the [MOD Desktop](https://github.com/moddevices/mod-desktop) you could build Ratatouille.lv2
with a additional MOD UI included by running

```shell
make modapp
```

To build Ratatouille.lv2 with only the MOD UI included (For usage in a MOD device like a Dwarf or a Duo (X)) run
```shell
make mod
```

To build Ratatouille only as standalone JackAudioConnectionKit application run
```shell
make standalone
```

To build Ratatouille with all favours (currently as LV2 plugin with included MOD GUI and as standalone application) run
```shell
make
```

## Dependencies

- libsndfile1-dev
- libcairo2-dev
- libx11-dev
- lv2-dev

## Building LV2 plug from source code

```shell
git clone https://github.com/brummer10//Ratatouille.lv2.git
cd Ratatouille.lv2
git submodule update --init --recursive
make lv2
make install # will install into ~/.lv2 ... AND/OR....
sudo make install # will install into /usr/lib/lv2
```
