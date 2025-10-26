# Guild Wars 2 - Scrolling Combat Text (GW2SCT)

![GitHub Release](https://img.shields.io/github/v/release/jake-greygoose/GW2-SCT?include_prereleases)

This addon overlays local combat data in highly configurable scroll areas.
It can be used as a replacement for, or in addition to, the in game combat text.
Inspired by the WoW addon Mik's Scrolling Battle Text.

### This is an opinionated fork

- Configuration files from previous versions are not supported and will be overwritten
- Some features like messages teleporting down on queue are removed
- I've not made any attempt to profile performance
- A spiritual continuation in absence of the original author

My primary motivation for resurrecting this addon was seeing how useful it could be in WvW, but being unable to use it with my AMD graphics card as it crashed immediately.
I also didn't like that the optimal setup for WvW required editing the json config to blocklist thousands of id's. Even though these configs were quite widely shared it was still more gatekept than I would like.  

An example of this configuration can be seen here: https://youtu.be/2O03Fe7lbnk?t=136

**So what's fixed?**

- AMD crash / Deferred texture creation
- Profile switching
- Config file getting overwritten

**What improvements have been made?**

- Filters can be configured as Allow or Block lists for each message receiver
- Global and Scroll Area scoped thresholds reduce noise
- Toggle [X Hits] text off per scroll area
- Number shortening & Skill Name abbreviation
- Upward scrolling and additional animations, Static & Angled
- Auto check for updates

Check the Trello to see exactly what is being worked on and what changes have been completed https://trello.com/b/pgoWhgwq/gw2-sct


**Example of Angled + Upward Scroll animation**

<img src="https://s14.gifyu.com/images/bT1Np.gif" alt="sct" border="0" />


### Installing

Go to the the [release page](https://github.com/jake-greygoose/GW2-SCT/releases) and download the newest release. Copy the file "gw2-sct.dll" into the "addons/"" subfolder of your game directory.

If you want to add your own fonts copy them into the "addons/sct/fonts/" directory. 
Similarly you may add additional icons in the "addons/sct/icons/" directory. 

You can configure the output in the arcdps options panel (opened by default with Alt+Shift+T).

### Translations

All additions use language keys so translations could be made if someone was motivated to do it.

## Contributing

Feel free to contact me on discord @__unreal

if anyone has researched suppressing the games combat text (one of Arcdps few remaining extras) I'd be interested in hearing from you.

## Known Bugs

* When having problems accessing the GW2 render API no skill icons can be downloaded or it takes a very long time. Download and extract the images in icons.zip file from [here](https://github.com/Artenuvielle/GW2-SCT/issues/11#issuecomment-606794158) and into the "addons/sct/icons" subfolder of your game directory.


## Authors

* **René Martin** - *Initial work* - [Artenuvielle](https://github.com/Artenuvielle)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.
