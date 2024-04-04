# Ratatouille.lv2

![Ratatouille](https://github.com/brummer10/Ratatouille.lv2/blob/main/Ratatouille.png?raw=true)

Ratatouille is a Neural Model loader/mixer for Linux/Windows.
It could load 2 models, could be [*.nam files](https://tonehunt.org/all) with the [Neural Amp Modeler](https://github.com/sdatkinson/NeuralAmpModelerCore) module,
or [*.json or .aidax files](https://cloud.aida-x.cc/all) with the [RTNeural](https://github.com/jatinchowdhury18/RTNeural) module.

You could load just a single model file, in that case the "Blend" control will do nothing.
When you've loaded a 2. model, the "Blend" control will blend between the two models.
and mix them to simulate your specific tone.

Ratatouille.lv2 support resampling when needed to match the expected Sample Rate of the loaded models.
Both models could have different expectations. 

## Dependencys

- libcairo2-dev
- libx11-dev
- lv2-dev

## Building from source code

- git clone https://github.com/brummer10//Ratatouille.lv2.git
- cd Ratatouille.lv2
- git submodule update --init --recursive
- make
- make install # will install into ~/.lv2 ... AND/OR....
- sudo make install # will install into /usr/lib/lv2
